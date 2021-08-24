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

//DEBUGビルド時のメモリリークチェック
#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include<crtdbg.h>
#endif

//ComPtr
#include<wrl/client.h>
#include<wrl.h>

/*----------------------------------------------*/
/*  C++ 標準ライブラリ                             */
/*----------------------------------------------*/
#include<string>
#include<filesystem>
#include<fstream>
#include<vector>

/*----------------------------------------------*/
/*  DirectX関連ヘッダ                                 */
/*----------------------------------------------*/
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<dxcapi.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

/*----------------------------------------------*/
/*  DirectX関連のライブラリ                         */
/*----------------------------------------------*/
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")


/*----------------------------------------------------*/
/* 自作したヘッダーファイル                               */
/*----------------------------------------------------*/
#include"fps_timer.h"
#include"keyboard.h"
#include"device_context.h"
#include"sprite.h"
#include"game.h"


//無名名前空間
namespace {

	//ウィンドウクラスネームの文字配列
	constexpr wchar_t windowClassName[]{ L"DirecrtX12WindowClass" };

	//タイトルバーの表示される文字列
	constexpr wchar_t windowTitle[]{ L"DirectX12 Application" };

	//デフォルトのクライアント領域
	constexpr int clientWidth = 800;
	constexpr int clientHeight = 600;

	//ComPtrのnamespaceが長いのでエイリアス(別名)を設定
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//usingディレクティブ，　名前空間　DirectXを省略可能にする
	using namespace DirectX;

	/*-------------------------------------------------*/
	/* その他、FPS管理・キー入力                         */
	/*-------------------------------------------------*/
	ncc::FpsTimer fps_timer;	// ゲームのフレームレート管理
	std::unique_ptr<DirectX::Keyboard> keyboard;	// キーボードの入力管理
	Keyboard::KeyboardStateTracker kb_tracker;		// 詳細なキーの状態が取れる

	/*--------------------- */
	/* D3D12デバイス     */
	/*----------------------*/
	ncc::DeviceContext device_context;

	/*------------------------------*/
	/* ゲームクラス                    */
	/*------------------------------*/
	ncc::Game game;

	/*-------------------------------*/
	/* プロトタイプ宣言               */
	/*-------------------------------*/
	LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

	bool PrepareResources();
	void Render();
	void Update();

	/*---------------------------*/
	/* 関数定義                      */
	/*---------------------------*/

	/// @brief ウィンドウプロシージャ
	/// @param hwnd ウィンドウハンドル
	/// @param message メッセージの種類
	/// @param wparam メッセージのパラメータ
	/// @param lparam メッセージのパラメータ
	/// @return メッセージの戻り値
	LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		switch (message)
		{
		case WM_CLOSE:	//プログラムの終了シグナル
			// ウィンドウ破棄，WM_DESTORYが送出
			DestroyWindow(hwnd);
			break;

		case WM_DESTROY:	//ウィンドウの破棄シグナル
			// プログラムを終了する，WM_QUITが送出
			PostQuitMessage(0);
			break;

		case WM_ACTIVATEAPP:
			DirectX::Keyboard::ProcessMessage(message, wparam, lparam);
			break;

			// キーボード入力
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case  WM_KEYUP:
		case WM_SYSKEYUP:
			DirectX::Keyboard::ProcessMessage(message, wparam, lparam);
			break;

		default:
			// DefWindowProcでメッセージに応じたデフォルト処理になる
			return DefWindowProc(hwnd, message, wparam, lparam);
			break;
		}
		return 0;
	}

	/// @brief 描画処理
	void Render()
	{
		// アロケータ小窓リストをリセット
		// 前フレームで使ったデータを忘れて，メモリを再利用可能にする
		ID3D12GraphicsCommandList* command_list = device_context.command_list();
		{
			ID3D12CommandAllocator* allocator = device_context.current_command_allocator();
			allocator->Reset();
			command_list->Reset(allocator, nullptr);
		}

		// このフレームで使うレンダーターゲットへのリソースバリア
		{
			// バリア用データ作成
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.Subresource =
				D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			// 状態をチェックするリソース
			barrier.Transition.pResource = device_context.current_render_target();	// スワームで使うレンダーターゲット
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;	// スワップチェイン表示可能から
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;	// レンダーターゲット状態に遷移

			command_list->ResourceBarrier(1, &barrier);
		}

		// レンダーターゲットのディスクリプタハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE rtv = device_context.current_rtv();
		// デプスステンシルビューを取得，先頭の1個だけなのでそのまま使う
		D3D12_CPU_DESCRIPTOR_HANDLE dsv = device_context.dsv();

		// 描画対象のレンダーターゲットを設定するコマンド
		// デプスステンシルも設定
		command_list->OMSetRenderTargets(
			1,		// 設定するレンダーターゲットの数
			&rtv,		//レンダーターゲットへのハンドル
			FALSE,
			&dsv);	// デプスステンシルビュー

		// 塗りつぶす色, float4要素でRGBAを指定
		float clear_color[4]{ 0,0.5f,1.0f,1.0f };

		// 指定したレンダーターゲットを、特定の色で塗りつぶし
		command_list->ClearRenderTargetView(
			rtv,		// 対象のレンダーターゲットのハンドル
			clear_color,	// 塗りつぶす色
			0,
			nullptr);

		// デプスステンシルバッファの内容クリアする
		command_list->ClearDepthStencilView(
			dsv,		// 対象のデプスステンシルビュー
			D3D12_CLEAR_FLAG_DEPTH,	// クリアする対象のデータ
			1.0f,	// 設定するデプス値
			0,		// 設定するステンシル値
			0,
			nullptr);

		// バックバッファ全体に表示するビューポート / シザーの設定
		command_list->RSSetViewports(1, &device_context.screen_viewport());
		command_list->RSSetScissorRects(1, &device_context.scissor_rect());

		/*------------------------*/
		/* ゲーム描画処理開始    */
		/*------------------------*/

		game.Render();


		/*------------------------*/
		/* ゲームの描画処理終了 */
		/*------------------------*/

		// レンダーターゲットをバックバッファとして使えるようにする状態遷移
		{
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.Subresource =
				D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.pResource =
				device_context.current_render_target();		// 遷移するリソース
			barrier.Transition.StateBefore =
				D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter =
				D3D12_RESOURCE_STATE_PRESENT;	// スワップチェーン表示可能状態へ

			command_list->ResourceBarrier(1, &barrier);
		}

		// コマンド記録完了
		command_list->Close();

		// キューは ID3D12CommandList* 配列を受け取る, 今回は要素数１の配列として渡す
		ID3D12CommandList* command_lists[]{ command_list };

		// キューにあるコマンド実行命令
		device_context.command_queue()->ExecuteCommandLists(
			_countof(command_lists),	// 今回キューに渡すコマンドリストの数
			command_lists);			// コマンドリスト配列

		device_context.Present();
		device_context.WaitForPreviousFrame();
	}

	/// @brief リソース作成を行う
	/// @return 処理成功なら true / そうでなければ false
	bool PrepareResources()
	{
		if (!game.Initialize(&device_context, keyboard.get()))
		{
			return false;
		}

		return true;
	}

	/// @brief ゲームの更新処理
	void Update()
	{
		fps_timer.Tick();

		Keyboard::State key_state = keyboard->GetState();
		kb_tracker.Update(key_state);

		game.Update(fps_timer);
	}

} // namespace

/// @brief アプリのエントリーポイント
/// @param hInstance インスタンスハンドル
/// @param  hPrevInstance 不使用
/// @param  lpCmdLine コマンドライン引数が渡される
/// @param nCmdShow ウィンドウ表示設定が渡される
/// @return 終了コード
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE,
	_In_ LPWSTR, _In_ int nCmdShow)
{
#if _DEBUG
	// メモリリークの検出
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// COMオブジェクトを使うときには呼んでおく必要がある
	if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
	{
		return 1;
	}

	// CPUの機能がDirectXMathに対応しているかチェック
	if (!DirectX::XMVerifyCPUSupport())
	{
		return 1;
	}

	// ウィンドウクラス
	WNDCLASSEX wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIconW(hInstance, L"IDI_ICON");
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = windowClassName;

	// ウィンドウクラス登録
	if (!RegisterClassExW(&wc))
	{
		return 1;
	}

	// ウィンドウの表示スタイル，まずは一般的なウィンドウとして作成
	DWORD window_style = WS_OVERLAPPEDWINDOW;
	// 描画領域としてのサイズを指定
	RECT window_rect{ 0,0,clientWidth,clientHeight };
	// ウィンドウスタイルを含めたウィンドウサイズを計算する
	AdjustWindowRect(
		&window_rect,
		window_style,
		FALSE);

	// ウィンドウの実サイズを計算
	auto window_width = window_rect.right - window_rect.left;
	auto window_height = window_rect.bottom - window_rect.top;

	// ウィンドウ作成
	HWND hwnd = CreateWindowExW(
		0,
		windowClassName,	// RegisterClassExWで使ったウィンドウクラスと同じ名前を指定する
		windowTitle,			// タイトルバーに表示したいテキスト	
		window_style,		// ウィンドウスタイル
		CW_USEDEFAULT,		// ウィンドウの初期x座標，CW_USEDEFAULTでデフォルト位置
		CW_USEDEFAULT,		// ウィンドウの初期y座標，CW_USEDEFAULTでデフォルト位置
		window_width,		// ウィンドウの幅
		window_height,		// ウィンドウの高さ
		nullptr,
		nullptr,
		hInstance,		// RegisterClassExWで使ったインスタンスハンドル
		nullptr);

	if (!hwnd)
	{
		return 1;
	}

	// ウィンドウを表示
	ShowWindow(hwnd, nCmdShow);

	if (device_context.Initialize(hwnd, clientWidth, clientHeight) == false)
	{
		return 1;
	}

	keyboard = std::make_unique<DirectX::Keyboard>();


	//---------------------------------------------------------------------------
	// なぜかここのif文が入る

	if (PrepareResources() == false)
	{
		return 1;
	}

	// ここからプログラムのメインループ
	// WM_QUITが来たらプログラム終了
	for (MSG msg{}; WM_QUIT != msg.message;)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			// メッセージがあれば下の2行でメッセージプロシージャで処理
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Update();
			Render();
		}
	}

	device_context.Finalize();
	CoUninitialize();

	return 0;
}