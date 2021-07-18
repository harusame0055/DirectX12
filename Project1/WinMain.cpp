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
#include<filesystem>
#include<fstream>
#include<vector>

/*----------------------------------------------*/
/*  DirectX関連ヘッダ                                     */
/*----------------------------------------------*/
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<dxcapi.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

/*----------------------------------------------*/
/*  DirectX関連のライブラリ                          */
/*----------------------------------------------*/
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")


//無名名前空間
namespace {

	//ウィンドウクラスネームの文字配列
	constexpr wchar_t kWindowClassName[]{ L"DirecrtX12WindowClass" };

	//タイトルバーの表示される文字列
	constexpr wchar_t kWindowTitle[]{ L"DirectX12 Application" };

	//デフォルトのクライアント領域
	constexpr int clienWidth = 800;
	constexpr int clienHeight = 600;

	//ComPtrのnamespaceが長いのでエイリアス(別名)を設定
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//usingディレクティブ，　名前空間　DirectXを省略可能にする
	using namespace DirectX;

	/*-------------------------------------------*/
	/*  頂点データ                                             */
	/*-------------------------------------------*/
	//頂点データ構造体
	struct Vertex
	{
		DirectX::XMFLOAT3 position;		//3D座標, float xyz 3要素
		DirectX::XMFLOAT4 color;			//頂点カラー, float rgba 4要素
	};

	//Vertex構造体の " 意味 " をシャーダに伝えるデータ配列
	constexpr D3D12_INPUT_ELEMENT_DESC vertexLayouts[]
	{
		{
			//DirectX::XMFLOAT3 position; の記述
			"POSITION",					//POSITION＝座標
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,		//32bit float型 3要素
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0},

		{
			//DirectX::XMFLOAT4 color;　の記述
			"COLOR",						//COLOR＝頂点カラー
			0,DXGI_FORMAT_R32G32B32A32_FLOAT,	//32bit float型 4要素
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0},
	};


	/*------------------------------------------------*/
	/*  D３Dオブジェクト関連                                 */
	/*------------------------------------------------*/
	/*D3D12デバイス                                               */
	ComPtr<ID3D12Device> d3d12_device = nullptr;
	//WARPアダプタ仕様フラグ(デバッグビルドのみ有効)
	constexpr bool useWarpAdapter = true;
	//バックバッファの最大値
	constexpr int maxBackBufferSize = 3;


	//コマンドリスト,GPUに対して処理手順をリストとして持つ
	ComPtr<ID3D12GraphicsCommandList> graphics_commandlist = nullptr;
	//コマンドアロケータ, コマンドを作成するためのメモリを確保するオブジェクト
	ComPtr<ID3D12CommandAllocator> command_allocators[maxBackBufferSize];
	//コマンドキュー，1つ以上のコマンドリストをキューに積んでGPUに送る
	ComPtr<ID3D12CommandQueue> command_queue = nullptr;


	//レンダーターゲットの設定(Descriptor)をVRAMに保存するため必要
	ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap = nullptr;
	//上のレンダーターゲットデスクリプタの1個当たりのサイズを覚える
	UINT rtv_descriptor_size = 0;

	//GPU処理が終わっているかを確認するために必要なフェンスオブジェクト
	ComPtr<ID3D12Fence> d3d12_fence = nullptr;
	//フェンスに書き込む値,バックバッファと同じ数を用意しておく
	UINT64 fence_values[maxBackBufferSize]{};
	//CPU. GPUの同期処理を楽にとるために使う
	Microsoft::WRL::Wrappers::Event fence_event;

	/*---------------------------------------------------------------------*/
	/*  バックバッファ関連                                                                      */
	/*---------------------------------------------------------------------*/
	//スワップチェインオブジェクト. 描画結果を画面へ出力してくれる
	ComPtr<IDXGISwapChain4> swap_chain = nullptr;
	//バックバッファとして使うレンダーターゲット配列
	ComPtr<ID3D12Resource> render_targets[maxBackBufferSize]{};
	//バックバッファテクスチャのフォーマット. 1pxをRGBA各8bitずつの32bit
	DXGI_FORMAT backbuffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//作成するバックバッファ数
	int backbuffer_size = 2;
	//現在のバックバッファのインデックス
	int backbuffer_index = 0;

	/*-------------------------------------------------*/
	/*  パイプラインオブジェクト                          */
	/*-------------------------------------------------*/
	ComPtr<ID3DBlob> simple_vs;
	ComPtr<ID3DBlob> simple_ps;
	ComPtr<ID3D12RootSignature> simple_rs;
	ComPtr<ID3D12PipelineState> simple_pso;


	/*-------------------------------------------------*/
	/*  描画用データ                                           */
	/*------------------------------------------------*/
	//頂点配列
	Vertex vertices[]
	{
		//1頂点{ XMFLOAT3 {座標} ，XMFLOAT4 {色} }，
		{XMFLOAT3{-0.5f,-0.5f,0.5f},XMFLOAT4{1.0f,0.0f,0.0f,1.0f}},
		{XMFLOAT3{0.0f,0.5f,0.5f},XMFLOAT4{0.0f,1.0f,0.0f,1.0f}},
		{XMFLOAT3{0.5f,-0.5f,0.5f},XMFLOAT4{0.0f,0.0f,1.0f,1.0f}},
	};

	//verticesを頂点バッファ化したオブジェクトを格納
	ComPtr<ID3D12Resource> vertex_buffer;
	//vertex_buffer変数の頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW vb_view;

	/*-------------------------------------------*/
	/*  プロトタイプ宣言                                   */
	/*-------------------------------------------*/

	LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	bool InitializeD3D(HWND hwnd);
	void FinalizeD3D();
	void WaitForGpu();
	void Render();
	void WaitForPreviousFrame();
	HRESULT CompileShaderFromFile(
		const std::wstring& filename,
		const std::wstring& profile,
		ComPtr<ID3DBlob>& shader_blob,
		ComPtr<ID3DBlob>& error_blob);
	bool CreateShaderBlob(
		const std::wstring& filename,
		const std::wstring& profile,
		ComPtr<ID3DBlob>& shader_blob);
	bool CreateSimplePSO();
	bool PrepareResources();

	/*-------------------------------------------*/
	/*  関数定義                                                 */
	/*-------------------------------------------*/
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
		//適切なハードウェアアダプタがなくて、useWarpAdapterがtrueなら
		//WARPアダプタを取得(WARP = Windows Advanced Rasterization + Platform)
		if (adapter == nullptr && useWarpAdapter)
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

		//スワップチェイン
		{
			//スワップチェインの設定用構造体
			DXGI_SWAP_CHAIN_DESC1 desc{}; desc.BufferCount = backbuffer_size; // バッファの数
			desc.Width = clienWidth; // バックバッファの幅
			desc.Height = clienHeight; // バックバッファの⾼さ
			desc.Format = backbuffer_format; // バックバッファフォーマット
			desc.BufferUsage =
				+DXGI_USAGE_RENDER_TARGET_OUTPUT; // レンダーターゲットの場合はこれ
			desc.Scaling = DXGI_SCALING_STRETCH;
			desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			desc.Flags = 0;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;

			// スワップチェインオブジェクト作成
			ComPtr<IDXGISwapChain1> sc;
			hr = dxgi_factory->CreateSwapChainForHwnd(
				command_queue.Get(), // スワップチェインを作るためのコマンドキュー
				hwnd, // 表⽰先になるウィンドウハンドル
				&desc, // スワップチェインの設定
				nullptr, // フルスクリーン設定 (指定しないときはnullptr)
				nullptr, // マルチモニタ設定 (指定しないときはnullptr)
				sc.GetAddressOf()); // 受け取る変数
			if (FAILED(hr))
			{
				return false;
			}

			//IDXGISwapChain1::As関数からIDXGISwapChain4インターフェースを取得
			hr = sc.As(&swap_chain);
			if (FAILED(hr))
			{
				return false;
			}
		}

		//レンダーターゲット用のデスクリプタヒープの作成
		{
			//レンダーターゲット用デスクリプタヒープの設定構造体
			D3D12_DESCRIPTOR_HEAP_DESC desc{};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			desc.NumDescriptors = backbuffer_size;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			//descの内容でメモリに動的に確保してもらう
			hr = d3d12_device->CreateDescriptorHeap(
				&desc,
				IID_PPV_ARGS(rtv_descriptor_heap.ReleaseAndGetAddressOf()));
			if (FAILED(hr))
			{
				return false;
			}
			rtv_descriptor_heap->SetName(L"rtv_descriptor_heap");

			//デスクリプタヒープのメモリサイズを取得
			rtv_descriptor_size = d3d12_device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		//レンダービューを作成
		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle =
				rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();

			//バックバッファの数だけレンダーターゲットを作成
			for (int i = 0; i < backbuffer_size; i++)
			{
				//スワップチェインからバックバッファを取得してくる
				hr = swap_chain->GetBuffer(
					i,
					IID_PPV_ARGS(render_targets[i].GetAddressOf()));
				if (FAILED(hr))
				{
					return false;
				}
				//render_targetsに名前を設定するための文字列変数
				std::wstring name{ L"render_targets[" + std::to_wstring(i) + L"]" };
				render_targets[i]->SetName(name.c_str());

				//レンダーターゲットビューの設定用構造体
				D3D12_RENDER_TARGET_VIEW_DESC desc = {};
				desc.Format = backbuffer_format;
				desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

				//RTについての情報を書き込んでRTビュー作成
				d3d12_device->CreateRenderTargetView(
					render_targets[i].Get(),
					&desc,
					rtv_handle);
				rtv_descriptor_heap->SetName(L"rtv_descriptor_heap");

				//次のループに備えて書き込み先のメモリ位置をずらす
				rtv_handle.ptr += rtv_descriptor_size;
			}
		}

		//初回描画に備えて、現在のバックバッファの員デスク取得
		backbuffer_index = swap_chain->GetCurrentBackBufferIndex();

		//コマンドアロケータ作成　
		{
			//使用するバックバッファと同じ数だけ作成してみる
			for (int i = 0; i < backbuffer_size; i++)
			{
				hr = d3d12_device->CreateCommandAllocator(
					D3D12_COMMAND_LIST_TYPE_DIRECT,
					IID_PPV_ARGS(command_allocators[i].ReleaseAndGetAddressOf()));
				if (FAILED(hr))
				{
					return false;
				}

				std::wstring name{ L"command_allocators[" + std::to_wstring(i) + L"]" };
				command_allocators[i]->SetName(name.c_str());
			}
		}

		//コマンドリスト作成
		{
			hr = d3d12_device->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				command_allocators[backbuffer_index].Get(),
				nullptr,
				IID_PPV_ARGS(graphics_commandlist.ReleaseAndGetAddressOf()));
			if (FAILED(hr))
			{
				return false;
			}
			graphics_commandlist->SetName(L"graphics_commandlist");

			graphics_commandlist->Close();
		}


		//フェンスオブジェクト
		{
			for (int i = 0; i < backbuffer_size; i++)
			{
				fence_values[i] = 0;
			}
			//フェンス作成
			hr = d3d12_device->CreateFence(
				fence_values[backbuffer_index],
				D3D12_FENCE_FLAG_NONE,
				IID_PPV_ARGS(d3d12_fence.ReleaseAndGetAddressOf()));
			if (FAILED(hr))
			{
				return false;
			}
			d3d12_fence->SetName(L"d3d12_fence");

			//次回同期のためにフェンス値を設定
			++fence_values[backbuffer_index];

			//フェンスの状態を確認するイベントを作る
			fence_event.Attach(
				CreateEventEx(
					nullptr,
					nullptr,
					0,
					EVENT_MODIFY_STATE | SYNCHRONIZE)
			);

			//イベントが正しく設定できたことをチェック
			if (!fence_event.IsValid())
			{
				return false;
			}
		}
		return true;
	}

	/// @brief  D3Dオブジェクト解放処理
	void FinalizeD3D()
	{
		WaitForGpu();
	}

	/// @brief GPUの処理完了を確認する
	void WaitForGpu()
	{
		//現在のフェンス値をローカルにコピー
		const UINT64 current_value = fence_values[backbuffer_index];

		//キュー完了時に更新されるフェンスとフェンス値をセット
		if (FAILED(command_queue->Signal(d3d12_fence.Get(), current_value)))
		{
			return;
		}

		//フェンスにフェンス値更新時にイベントが実行されるように設定
		if (FAILED(
			d3d12_fence->SetEventOnCompletion(current_value,
				fence_event.Get())))
		{
			return;
		}

		//イベントに飛んでくるまでこのプログラムを待機状態にする
		WaitForSingleObjectEx(fence_event.Get(), INFINITE, FALSE);

		//次回のフェンス用にフェンス値更新
		fence_values[backbuffer_index] = current_value + 1;


	}

	/// @brief 描画処理
	void Render()
	{
		//アロケータとコマンドリストをリセット
		//前フレームで使ったデータを忘れて、メモリを再利用可能にする
		command_allocators[backbuffer_index]->Reset();
		graphics_commandlist->Reset(
			command_allocators[backbuffer_index].Get(),
			nullptr);

		//このフレームで使うレンダーターゲットへのリソースバリア
		//リソースバリアはリソースの状態を遷移させ、線移管料までリソースバリアする仕組み
		{
			//バリア用データ作成
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.Subresource =
				D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			//状態をチェックするリソース
			barrier.Transition.pResource
				= render_targets[backbuffer_index].Get();

			//遷移前(Before)の状態
			barrier.Transition.StateBefore
				= D3D12_RESOURCE_STATE_PRESENT;
			//遷移後(After)の状態
			barrier.Transition.StateAfter
				= D3D12_RESOURCE_STATE_RENDER_TARGET;

			//リソースバリアコマンド作成
			graphics_commandlist->ResourceBarrier(1, &barrier);
		}

		//レンダーターゲットのディスクリプタハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE rtv
			= rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
		rtv.ptr += static_cast<SIZE_T>(static_cast<INT64>(backbuffer_index) *
			static_cast<INT64>
			(rtv_descriptor_size));

		//描画対象のレンダーターゲットを設定するコマンド
		graphics_commandlist->OMSetRenderTargets(
			1,
			&rtv,
			FALSE,
			nullptr);


		//塗りつぶす色. float4要素でRGBAを指定
		float clear_color[4]{ 0,0.5f,1.0f,1.0f };

		//指定したレンダーターゲットを、特定の色で塗りつぶし
		graphics_commandlist->ClearRenderTargetView(
			rtv,
			clear_color,
			0,
			nullptr);

		/*-----------------------------------------*/
		/*  ゲームの描画処理開始                     */
		/*-----------------------------------------*/



		/*-----------------------------------------*/
		/*  ゲームの描画処理終了                         */
		/*-----------------------------------------*/


		//レンダーターゲットをバックバッファとして使えるようにする状態遷移
		{
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.Subresource =
				D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			barrier.Transition.pResource
				= render_targets[backbuffer_index].Get();

			barrier.Transition.StateBefore
				= D3D12_RESOURCE_STATE_RENDER_TARGET;

			barrier.Transition.StateAfter
				= D3D12_RESOURCE_STATE_PRESENT;

			graphics_commandlist->ResourceBarrier(1, &barrier);
		}

		//コマンド記録完了
		graphics_commandlist->Close();

		//キューは ID3D12CommandList * 配列を受け取る, 今回は要素数1の配列として渡す
		ID3D12CommandList* command_lists[]{
			graphics_commandlist.Get()
		};

		//キューにあるコマンド実行命令
		command_queue->ExecuteCommandLists(
			_countof(command_lists),
			command_lists);

		//バックバッファをフロントバッファに入れ替え指示
		swap_chain->Present(1, 0);

		WaitForPreviousFrame();
	}

	/// @brief 描画用同期処理
	void WaitForPreviousFrame()
	{
		//キューにシグナルを送る
		const UINT64 current_value = fence_values[backbuffer_index];
		command_queue->Signal(d3d12_fence.Get(), current_value);

		//次のフレームのバックバッファインデックスをもらう
		backbuffer_index = swap_chain->GetCurrentBackBufferIndex();

		//GetCompletedValueでフェンスの現在地を確認する.
		if (d3d12_fence->GetCompletedValue() < current_value)
		{
			//まだ描画が終わっていないので同期
			d3d12_fence->SetEventOnCompletion(current_value, fence_event.Get());
			WaitForSingleObjectEx(fence_event.Get(), INFINITE, FALSE);
		}

		//次のフレームのためにフェンス値更新
		fence_values[backbuffer_index] = current_value + 1;
	}

	/// @brief HLSLをコンパイルして実行可能なオブジェクトを作る
	/// @param filename コンパイルするHLSLファイルパス
	/// @param profile シェーダーポロファイル（シェーダーの種類）
	/// @param shader_blob コンパイル後のHLSLデータを受け取る
	/// @param error_blob 失敗時はこっちにエラー情報オブジェクトを入れる
	/// @return 処理成功ならS_OK / そうでなければS_FALSE
	HRESULT CompileShaderFromFile(
		const std::wstring& filename,
		const std::wstring& profile,
		ComPtr<ID3DBlob>& shader_blob,
		ComPtr<ID3DBlob>& error_blob)
	{
		HRESULT hr = S_FALSE;

		//ファイル名からパスを作る
		std::filesystem::path file_path(filename);

		//ファイルを読み取るオブジェクト
		std::ifstream in_file(file_path);

		//ファイルがなければ失敗
		if (!in_file)
		{
			return hr;
		}

		//ファイルを書き込む動的配列
		std::vector<char> shader_src;
		//shader_srcのサイズをファイルに合わせる
		std::size_t file_size = static_cast<std::size_t>
			(in_file.seekg(0, in_file.end).tellg());
		shader_src.resize(file_size);

		//ファイル全体をshader_srcに読み込む
		in_file.seekg(0, in_file.beg).read(shader_src.data(), shader_src.size());

		//読み込んだファイル（バイトデータ）をシェーダーソースに変換するオブジェクト
		ComPtr<IDxcLibrary> library;
		DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));

		//シェーダソースを受け取るオブジェクト
		ComPtr<IDxcBlobEncoding> source;
		//バイトデータからシェーダーソースに変換
		library->CreateBlobWithEncodingFromPinned(
			shader_src.data(),
			static_cast<UINT32>(shader_src.size()),
			CP_UTF8,
			&source);

		//DXCコンパイルフラグ
		LPCWSTR compiler_flags[]{
#if _DEBUG
			//デバッグ用フラグ
			L"/Zi",L"/O0",
#else
			L"/02"
#endif
		};

		//シェーダー内のincludeを解決してくれる
		ComPtr<IDxcIncludeHandler>include_handler;
		library->CreateIncludeHandler(include_handler.ReleaseAndGetAddressOf());

		//コンパイルの結果を受け取るオブジェクト
		ComPtr<IDxcOperationResult> result;

		//シェーダコンパイラ
		ComPtr<IDxcCompiler> compiler;
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

		//シェーダデータを実行可能な状態にコンパイル
		compiler->Compile(
			source.Get(),
			file_path.wstring().c_str(),
			L"main",
			profile.c_str(),
			compiler_flags,
			_countof(compiler_flags),
			nullptr,
			0,
			include_handler.Get(),
			&result);

		//コンパイルの結果をチェック
		result->GetStatus(&hr);
		if (SUCCEEDED(hr))
		{
			//コンパイル成功
			//shader_blobにコンパイル後のシェーダデータを入れる
			result->GetResult(
				reinterpret_cast<IDxcBlob**>(shader_blob.GetAddressOf()));
		}
		else
		{
			//コンパイル失敗
			//エラーを渡す
			result->GetErrorBuffer(
				reinterpret_cast<IDxcBlobEncoding**>(error_blob.GetAddressOf()));
		}

		return hr;
	}

	/// @brief HLSLファイルからBlobオブジェクトを作る
	/// @param filename Blob化するHLSLファイルパス
	/// @param profile シェーダープロファイル(シェーダーの種類)
	/// @param shader_blob コンパいつ後のHLSLデータを受け取る
	/// @return 処理成功ならtrue / そうでなければfalse
	bool CreateShaderBlob(
		const std::wstring& filename,
		const std::wstring& profile,
		ComPtr<ID3DBlob>& shader_blob)
	{
		bool ret = true;

		//シェーダープログラムは事前コンパイルしておく方法もあるが今回は都度コンパイル
		ComPtr<ID3DBlob> error;

		//上で作った関数デコンパイル
		auto hr = CompileShaderFromFile(filename, profile, shader_blob, error);

		//結果をチェック
		if (FAILED(hr))
		{
			//errorの時はVSの出力にメッセージを出しておく	
			OutputDebugStringA((const char*)error->GetBufferPointer());
		}
		return ret;
	}

	/// @brief シンプルな描画を実現するPSOを作る
	/// @return 処理成功ならtrue / そうでなければfalse
	bool CreateSimplePSO()
	{
		//シェーダーオブジェクト作成
		{
			bool ret = true;
			ret = CreateShaderBlob(
				L"shader/VertexShader.hlsl",
				L"vs_6_0",
				simple_vs);

			if (!ret)
			{
				return false;
			}

			ret = CreateShaderBlob(
				L"shader/PixelShader.hlsl",
				L"ps_6_0",
				simple_ps);

			if (!ret)
			{
				return false;
			}
			return true;
		}

		//ルートシグネチャ作成
		{
			//ルートパラメータを定義
			D3D12_ROOT_SIGNATURE_DESC rs_desc{
				0,
				nullptr,
				0,
				nullptr,
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			};

			ComPtr<ID3DBlob> signature, error;
			if (FAILED(D3D12SerializeRootSignature(
				&rs_desc,
				D3D_ROOT_SIGNATURE_VERSION_1_0,
				&signature,
				nullptr)))
			{
				//失敗したらエラーを出力
				OutputDebugStringA((const char*)error->GetBufferPointer());
				OutputDebugStringA("CreateRootSignature Failed.");
				return false;
			}

			//ルートシグネチャオブジェクト作成
			if (FAILED(d3d12_device->CreateRootSignature(
				0,
				signature->GetBufferPointer(),
				signature->GetBufferSize(),
				IID_PPV_ARGS(&simple_rs))))
			{
				return false;
			}
		}

		//PSO作成
		{
			//パイプラインの動作設定するパラメータの準備
			/*-------------------------------------------*/
			//アウトプットマージャの設定
			//不透明な描画
			D3D12_BLEND_DESC blend_desc{};
			//透明なピクセルをレンダリングした時の処理について
			blend_desc.AlphaToCoverageEnable = false;
			//レンダーターゲット度に個別設定するか
			blend_desc.IndependentBlendEnable = false;
			//ブレンディングの設定.MSのドキュメント等も参照すること
			blend_desc.RenderTarget[0] = D3D12_RENDER_TARGET_BLEND_DESC{
				FALSE,
				FALSE,
				D3D12_BLEND_ONE,
				D3D12_BLEND_ZERO,
				D3D12_BLEND_OP_ADD,
				D3D12_BLEND_ONE,
				D3D12_BLEND_ZERO,
				D3D12_BLEND_OP_ADD,
				D3D12_LOGIC_OP_NOOP,
				D3D12_COLOR_WRITE_ENABLE_ALL,
			};

			//ラスタライザステート設定
			D3D12_RASTERIZER_DESC  raster_desc = {
				D3D12_FILL_MODE_SOLID,
				D3D12_CULL_MODE_BACK,
				FALSE,
				D3D12_DEFAULT_DEPTH_BIAS,
				D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
				D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
				TRUE,
				FALSE,
				FALSE,
				0,
				D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF };


			//デプスステンシル
			D3D12_DEPTH_STENCIL_DESC ds_desc{};
			ds_desc.DepthEnable = FALSE;

			//頂点レイアウト
			D3D12_INPUT_LAYOUT_DESC input_layout = {
				vertexLayouts,
				_countof(vertexLayouts) };

			/*-------------------------------------------*/


			//パイプラインステートオブジェクトの定義を作成
			D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc{};

			//ルートシグネチャ設定
			pso_desc.pRootSignature = simple_rs.Get();

			//頂点シェーダー設定
			pso_desc.VS.pShaderBytecode = simple_vs->GetBufferPointer();
			pso_desc.VS.BytecodeLength = simple_vs->GetBufferSize();

			//ピクセルシェーダ設定
			pso_desc.PS.pShaderBytecode = simple_ps->GetBufferPointer();
			pso_desc.PS.BytecodeLength = simple_ps->GetBufferSize();

			//ブレンドステート設定
			pso_desc.BlendState = blend_desc;
			//ラスタライザーステート設定
			pso_desc.RasterizerState = raster_desc;

			//ピクセルシェーダ出力先のレンダーターゲットの数
			pso_desc.NumRenderTargets = 1;
			//ピクセルシェーダが出力するレンダーターゲットのフォーマット
			pso_desc.RTVFormats[0] = backbuffer_format;

			//インプットアセンブラが使う頂点属性レイアウト
			pso_desc.InputLayout = input_layout;
			//プリミティブトポロジ設定
			pso_desc.PrimitiveTopologyType =
				D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			//デプスバッファとステンシルのフォーマットを設定
			pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			pso_desc.DepthStencilState = ds_desc;

			//マルチサンプル設定
			pso_desc.SampleDesc = { 1,0 };
			pso_desc.SampleMask = UINT_MAX;

			//PSO作成
			if (FAILED(d3d12_device->CreateGraphicsPipelineState(
				&pso_desc,
				IID_PPV_ARGS(&simple_pso))))
			{
				return false;
			}
		}
		return true;
	}


	/// @brief リソース作成を行う
	/// @return 処理成功なら true / そうでなければ false
	bool PrepareResources()
	{
		//頂点バッファ作成
		{
			//頂点データサイズ
			UINT64 buffer_size = _countof(vertices) * sizeof(vertices[0]);

			//GPUメモリに確保するリソースのメモリ設定
			//CPU / GPUの読取り , 書込みなど可否設定
			//リソースが頂点バッファなら以下でOK
			D3D12_HEAP_PROPERTIES properties{};
			properties.Type = D3D12_HEAP_TYPE_UPLOAD;
			properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			properties.CreationNodeMask = 0;
			properties.VisibleNodeMask = 0;
		}
		return true;
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

	//CPUの機能がDirectXMathに対応しているかチェック
	if (!DirectX::XMVerifyCPUSupport())
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
		clienWidth,
		clienHeight };

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

	if (CreateSimplePSO() == false)
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
			Render();
		}
	}

	FinalizeD3D();
	CoUninitialize();

	return 0;
}