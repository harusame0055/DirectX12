//Windows.hからゲームでは使わないヘッダを排除するマクロ
#define	WIN32_LEAN_AND_MEAN
#define	NOMINMAX
#define	NODRAWTEXT
#define	NOGDI
#define	NOBITMAP
#define	NOMCX
#define	NOSERVICE
#define	NOHELP

//Windows APIを使うためのヘッダファイル
#include<Windows.h>
#include<wrl.h>

//DEBUGビルド時のメモリリークチェック
#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include<crtdbg.h>
#endif

/*----------------------------------------------*/
/*  C++　標準ライブラリ                               */
/*----------------------------------------------*/
#include<string>

/*----------------------------------------------*/
/*  DirectX関連ヘッダ                                     */
/*----------------------------------------------*/
#include<d3d12.h>
#include<dxgi1_6.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

/*----------------------------------------------*/
/*  DirectX関連のライブラリ                          */
/*----------------------------------------------*/
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")

//無名名前空間
namespace {

	//ウィンドウクラスネームの文字配列
	constexpr wchar_t kWindowClassName[]{ L"DirecrtX12WindowClass" };

	//タイトルバーの表示される文字列
	constexpr wchar_t kWindowTitle[]{ L"DirectX12 Application" };

	//デフォルトのクライアント領域
	constexpr int kClienWidth = 800;
	constexpr int kClienHeight = 600;

	//ComPtrのnamespaceが長いのでエイリアス(別名)を設定
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	/*---------------------------------------------------------------------*/
	/*  D３Dオブジェクト関連                                                                 */
	/*---------------------------------------------------------------------*/
	/*D3D12デバイス                                                                               */
	ComPtr<ID3D12Device> d3d12_device = nullptr;
	//WARPアダプタ仕様フラグ(デバッグビルドのみ有効)
	constexpr bool kUseWarpAdapter = true;
	//バックバッファの最大値
	constexpr int kMaxBackBufferSize = 3;


	//コマンドリスト,GPUに対して処理手順をリストとして持つ
	ComPtr<ID3D12GraphicsCommandList> graphics_commandlist = nullptr;
	//コマンドアロケータ, コマンドを作成するためのメモリを確保するオブジェクト
	ComPtr<ID3D12CommandAllocator> command_allocators[kMaxBackBufferSize];
	//コマンドキュー，1つ以上のコマンドリストをキューに積んでGPUに送る
	ComPtr<ID3D12CommandQueue> command_queue = nullptr;


	//レンダーターゲットの設定(Descriptor)をVRAMに保存するため必要
	ComPtr<ID3D12DescriptorHeap> rtv_descriptr_heap = nullptr;
	//上のレンダーターゲットデスクリプタの1個当たりのサイズを覚える
	UINT rtv_descriptr_size = 0;

	//GPU処理が終わっているかを確認するために必要なフェンスオブジェクト
	ComPtr<ID3D12Fence> d3d12_fence = nullptr;
	//フェンスに書き込む値,バックバッファと同じ数を用意しておく
	UINT64 fence_values[kMaxBackBufferSize]{};
	//CPU. GPUの同期処理を楽にとるために使う
	Microsoft::WRL::Wrappers::Event fence_event;

	/*---------------------------------------------------------------------*/
	/*  バックバッファ関連                                                                      */
	/*---------------------------------------------------------------------*/
	//スワップチェインオブジェクト. 描画結果を画面へ出力してくれる
	ComPtr<IDXGISwapChain4> swap_chain = nullptr;
	//バックバッファとして使うレンダーターゲット配列
	ComPtr<ID3D12Resource> render_targets[kMaxBackBufferSize]{};
	//バックバッファテクスチャのフォーマット. 1pxをRGBA各8bitずつの32bit
	DXGI_FORMAT backbuffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//作成するバックバッファ数
	int backbuffer_size = 2;
	//現在のバックバッファのインデックス
	int backbuffer_index = 0;



	/// @brief  ウィンドウプロシージャ
	/// @param hwnd  ウィンドウハンドル
	/// @param message メッセージの種類
	/// @param wParam メッセージのパラメータ
	/// @param lParam 　メッセージのパラメータ
	/// @return メッセージの戻り値
	LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CLOSE:	//プログラム終了シグナル
			DestroyWindow(hwnd);
			break;

		case WM_DESTROY:	//ウィンドウ破棄シグナル
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
			break;
		}
		return 0;
	}

	/// @brief D3Dオブジェクトの初期化
	/// @param hwnd	 アプリケーションのウィンドウハンドル
	/// @return  成功 true / 失敗 false
	bool InitializeD3D(HWND hwnd)
	{
		HRESULT hr = S_OK;

		//ファクトリ作成フラグ
		UINT dxgi_factory_flags = 0;

		//_DEBUGビルド時はD3D12のデバッグ機能の有効化する
#if _DEBUG
		ComPtr<ID3D12Debug> debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
		{
			debug->EnableDebugLayer();
		}

		//DXGIのデバッグ機能設定
		ComPtr<IDXGIInfoQueue> info_queue;
		if (SUCCEEDED(DXGIGetDebugInterface1(
			0, IID_PPV_ARGS(info_queue.GetAddressOf()))))
		{
			dxgi_factory_flags = DXGI_CREATE_FACTORY_DEBUG;

			//DXGIに問題があった時にプログラムがブレークするようにする
			info_queue->SetBreakOnSeverity(
				DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			info_queue->SetBreakOnSeverity(
				DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
#endif

		//アダプタを取得するためのファクトリ作成
		ComPtr<IDXGIFactory5> dxgi_factory;
		hr = CreateDXGIFactory2(dxgi_factory_flags,
			IID_PPV_ARGS(&dxgi_factory));

		if (FAILED(hr))
		{
			return 1;
		}

		//D3D12が使えるアダプタの探索処理
		ComPtr<IDXGIAdapter1> adapter;	//アダプタ(ビデオカード)オブジェクト
		{
			UINT adapter_index = 0;

			//dxgi_factory->EnumAdapters1によりアダプタの性能をチェックしていく
			while (DXGI_ERROR_NOT_FOUND
				!= dxgi_factory->EnumAdapters1(adapter_index, &adapter))
			{
				DXGI_ADAPTER_DESC1 desc{};
				adapter->GetDesc1(&desc);
				++adapter_index;

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;
				}

				//D3D12は使用可能か
				hr = D3D12CreateDevice(adapter.Get(),
					D3D_FEATURE_LEVEL_11_0,
					__uuidof(ID3D12Device), nullptr);

				if (SUCCEEDED(hr))
				{
					break;	//使えるアダプタがあった場合探索を中断して次の処理へ
				}
			}
		}
#if _DEBUG
		//適切なハードウェアアダプタがなくて、kUseWarpAdapterがtrueなら
		//WARPアダプタを取得(WARP = Windows Advanced Rasterization + Platform)
		if (adapter == nullptr && kUseWarpAdapter)
		{
			//WARPはGPU機能の一部をCPU側で実行してくれるアダプタ
			//純粋なソフトウェアより随分早いが本番のゲームで使うのは無理
			hr = dxgi_factory->EnumWarpAdapter(
				IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()));
			if (FAILED(hr))
			{
				return false;
			}
		}
#endif

		if (!adapter)
		{
			return false;
		}

		//ゲームで使うデバイス作成
		hr = D3D12CreateDevice(adapter.Get(),	//デバイス生成に使うアダプタ
			D3D_FEATURE_LEVEL_11_0,					//D3Dの機能レベル
			IID_PPV_ARGS(&d3d12_device));			//受け取る変数

		if (FAILED(hr))
		{
			return false;
		}
		d3d12_device->SetName(L"d3d12_device");

		//コマンドキューの作成
		{
			//キューの設定用構造体
			D3D12_COMMAND_QUEUE_DESC desc{};
			desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

			//キューの作成
			hr = d3d12_device->CreateCommandQueue(
				&desc,
				IID_PPV_ARGS(command_queue.ReleaseAndGetAddressOf()));

			if (FAILED(hr))
			{
				return false;
			}
			command_queue->SetName(L"command_queue");
		}

		return true;
	}

	/// @brief  D3Dオブジェクト解放処理
	void FinalizeD3D()
	{

	}
}// namespace

/// @brief Windows アプリのエントリーポイント
/// @param hInstance インスタンスハンドル,OSから見たアプリの管理番号みたいなもの
/// @param hPrevInstance	 不使用(過去のAPIとの整合性維持用)
/// @param lpCmdLine コマンドライン引数が渡される
/// @param nCmdShow ウィンドウ表示設定が渡される
/// @return 終了コード,通常は0
int WINAPI wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{

#if _DEBUG
	//メモリリークの検出
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	//COMオブジェクトを使うときには呼んでおく必要がある
	if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
	{
		return 1;
	}

	//ウィンドウクラス
	WNDCLASSEX wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIconW(hInstance, L"IDI_ICON");
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = kWindowClassName;

	//ウィンドウクラス登録
	if (!RegisterClassExW(&wc))
	{
		return 1;
	}

	//ウィンドウの表示スタイルフラグ,OVERLAPPEWINDOWは一般的なスタイルになる
	DWORD window_style = WS_OVERLAPPEDWINDOW;

	//描画領域としてのサイズを指定
	RECT window_rect{
		0,
		0,
		kClienWidth,
		kClienHeight };

	//ウィンドウスタイルを含めたウィンドウサイズを計算する
	AdjustWindowRect(&window_rect, window_style, FALSE);

	//ウィンドウの実サイズを計算
	auto window_width = window_rect.right - window_rect.left;
	auto window_height = window_rect.bottom - window_rect.top;

	//ウィンドウ作成
	HWND hwnd = CreateWindowExW(
		0, kWindowClassName,
		kWindowTitle,
		window_style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		window_width,
		window_height,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	if (!hwnd)
	{
		return 1;
	}

	//ウィンドウを表示
	ShowWindow(hwnd, nCmdShow);

	if (InitializeD3D(hwnd) == false)
	{
		return 1;
	}

	//メインループ
	//WM_QUITがきたらプログラムの終了
	for (MSG msg{}; WM_QUIT != msg.message;)
	{
		//MSG構造体を使ってメッセージを受け取る
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			//メッセージがあれば下の２行でメッセージプロシージャ
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//ここにゲームの処理を書く
		}
	}

	FinalizeD3D();

	return 0;
}