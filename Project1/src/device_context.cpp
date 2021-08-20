#include"device_context.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include<cassert>
#include<string>

namespace ncc {

#pragma region public

	bool DeviceContext::Initialize(HWND hwnd, int backbuffer_width, int backbuffer_height)
	{
		if (!CreateFactory())
		{
			return false;
		}

		if (!CreateDevice(dxgi_factory_.Get()))
		{
			return false;
		}

		if (!CreateCommandQueue(device_.Get()))
		{
			return false;
		}

		if (!CreateSwapChain(hwnd, dxgi_factory_.Get(), backbuffer_width, backbuffer_height))
		{
			return false;
		}

		if (!CreateRenderTarget(device_.Get(), swap_chain_.Get()))
		{
			return false;
		}

		if (!CreateDepthStencil(device_.Get(), backbuffer_width, backbuffer_height))
		{
			return false;
		}

		if (!CreateCommandAllocator(device_.Get()))
		{
			return false;
		}

		if (!CreateCommandList(device_.Get(), command_allocators_[0].Get()))
		{
			return false;
		}

		if (!CreateFence(device_.Get()))
		{
			return false;
		}

		// ビューポート・シザー矩形設定
		screen_viewport_.TopLeftX = 0.0f;
		screen_viewport_.TopLeftY = 0.0f;
		screen_viewport_.Width = static_cast<FLOAT>(backbuffer_width);
		screen_viewport_.Height = static_cast<FLOAT>(backbuffer_height);
		screen_viewport_.MinDepth = D3D12_MIN_DEPTH;
		screen_viewport_.MaxDepth = D3D12_MAX_DEPTH;

		scissor_rect_.left = 0;
		scissor_rect_.top = 0;
		scissor_rect_.right = backbuffer_width;
		scissor_rect_.bottom = backbuffer_height;

		return true;
	}

	void DeviceContext::Finalize()
	{
		WaitForGpu();

#if _DEBUG
		{
			ComPtr<ID3D12DebugDevice> debug_interface;
			if (SUCCEEDED(
				device_->QueryInterface(IID_PPV_ARGS(&debug_interface))))
			{
				debug_interface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL |
					D3D12_RLDO_IGNORE_INTERNAL);
			}
		}
#endif
	}

	void DeviceContext::WaitForGpu()
	{
		// 現在のフェンス値をローカルにコピー
		const UINT64 current_value = fence_values_[backbuffer_index_];

		// キュー完了時に更新されるフェンスとフェンス値をセット
		if (FAILED(command_queue_->Signal(d3d12_fence_.Get(), current_value)))
		{
			return;
		}

		// フェンスにフェンス値更新時にイベントが実行されるように設定
		if (FAILED(
			d3d12_fence_->SetEventOnCompletion(current_value, fence_event_.Get())))
		{
			return;
		}

		// イベントが飛んでくるまで待機状態にする
		WaitForSingleObjectEx(fence_event_.Get(), INFINITE, FALSE);
		// 次回のフェンス用にフェンス値更新
		fence_values_[backbuffer_index_] = current_value + 1;
	}

	void DeviceContext::WaitForPreviousFrame()
	{
		// キューにシグナルを送る
		const auto current_value = fence_values_[backbuffer_index_];
		command_queue_->Signal(d3d12_fence_.Get(), current_value);

		// 次フレームのバックバッファインデックスをもらう
		backbuffer_index_ = swap_chain_->GetCurrentBackBufferIndex();

		// 次のフレームのためにフェンス値更新
		fence_values_[backbuffer_index_] = current_value + 1;

		// 描画処理によって到達した時点で描画が終わりフェンス値が更新されている可能性もある
		// その状態でWaitForSingleObjectExをすると無限に待つことになる為
		// そうならないようにGetCompletedValueでフェンスの現在値を確認する.
		// GetCompletedValueでフェンスの現在値を確認する.
		if (d3d12_fence_->GetCompletedValue() < current_value)
		{
			// まだ描画が終わっていないので同期
			d3d12_fence_->SetEventOnCompletion(current_value, fence_event_.Get());
			WaitForSingleObjectEx(fence_event_.Get(), INFINITE, FALSE);
		}
	}

	void DeviceContext::Present()
	{
		swap_chain_->Present(1, 0);
	}

	ID3D12Device* DeviceContext::device()
	{
		return device_.Get();
	}

	IDXGISwapChain4* DeviceContext::swap_chain()
	{
		return swap_chain_.Get();
	}

	ID3D12GraphicsCommandList* DeviceContext::command_list()
	{
		return graphics_commnadlist_.Get();
	}

	ID3D12CommandAllocator* DeviceContext::current_command_allocator()
	{
		return command_allocators_[backbuffer_index_].Get();
	}

	ID3D12CommandAllocator* DeviceContext::command_allocator(int index)
	{
		assert(index >= 0 && index <= backbuffer_size_);
		return command_allocators_[index].Get();
	}

	ID3D12CommandQueue* DeviceContext::command_queue()
	{
		return command_queue_.Get();
	}

	ID3D12Resource* DeviceContext::current_render_target()
	{
		return render_targets_[backbuffer_index_].Get();
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE DeviceContext::current_rtv() const
	{
		auto rtv
			= rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
		rtv.ptr += static_cast<SIZE_T>(static_cast<INT64>(backbuffer_index_) *
			static_cast<INT64>(rtv_descriptor_size_));
		return rtv;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE DeviceContext::dsv() const
	{
		return dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
	}

	const int DeviceContext::backbuffer_index() const
	{
		return backbuffer_index_;
	}

	const int DeviceContext::backbuffer_size() const
	{
		return backbuffer_size_;
	}

	const D3D12_VIEWPORT& DeviceContext::screen_viewport() const
	{
		return screen_viewport_;
	}

	const D3D12_RECT& DeviceContext::scissor_rect() const
	{
		return scissor_rect_;
	}

#pragma endregion

#pragma region private

	/// @brief ファクトリ作成フラグ
	/// @retval true 成功
	/// @retval false  失敗
	bool DeviceContext::CreateFactory()
	{
		// ファクトリ作成フラグ
		UINT dxgi_factory_flags = 0;

		// _DEBUGビルド時はD3D12のデバッグ機能の有効化する
#if _DEBUG
		ComPtr<ID3D12Debug> debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
		{
			debug->EnableDebugLayer();
		}

		// DXGIのデバッグ機能設定
		ComPtr<IDXGIInfoQueue> info_queue;
		if (SUCCEEDED(DXGIGetDebugInterface1(
			0, IID_PPV_ARGS(info_queue.GetAddressOf()))))
		{
			dxgi_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
			// DXGI に問題があった時にプログラムがブレーキするようにする
			info_queue->SetBreakOnSeverity(
				DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			info_queue->SetBreakOnSeverity(
				DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
#endif

		// アダプタを取得するためのファクトリ作成
		if (FAILED(CreateDXGIFactory2(dxgi_factory_flags,
			IID_PPV_ARGS(dxgi_factory_.ReleaseAndGetAddressOf()))))
		{
			return false;
		}

		return true;
	}

	/// @brief デバイス作成
	/// @param factory IDXGIFactory5*
	/// @retval true 成功
	/// @retval false 失敗
	bool DeviceContext::CreateDevice(IDXGIFactory5* factory)
	{
		// D3D12が使えるアダプタの探索処理
		ComPtr<IDXGIAdapter1> adapter; // アダプタオブジェクト
		{
			UINT adapter_index = 0;

			// dxgi_factory->EnumAdapters1 によりアダプタの性能をチェックしていく
			while (DXGI_ERROR_NOT_FOUND
				!= factory->EnumAdapters1(adapter_index, &adapter))
			{
				DXGI_ADAPTER_DESC1 desc{};
				adapter->GetDesc1(&desc);
				++adapter_index;

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;	//	CPUs処理は無視
				}

				// D3D12は使用可能か
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(),
					D3D_FEATURE_LEVEL_11_0,
					__uuidof(ID3D12Device), nullptr)))
				{
					break; // 使えるアダプタがあったので探索を中断して次の処理へ
				}
			}

#if _DEBUG
			// 適切なハードウェアアダプタがなく、useWarpAdapterがtrueなら
			// WARPアダプタを取得
			if (adapter == nullptr && useWarpAdapter)
			{
				// WARPはGPU機能の一部をCPU側で実行してくれるアダプタ
				if (FAILED(factory->EnumWarpAdapter(
					IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
				{
					return false;
				}
			}
#endif

			if (!adapter)
			{
				return false; // 使用可能なアダプターがなかった
			}
		}

		// ゲームで使うデバイス作成
		if (FAILED(D3D12CreateDevice(adapter.Get(), // デバイス生成に使うアダプタ
			D3D_FEATURE_LEVEL_11_0,	// D3Dの機能レベル
			IID_PPV_ARGS(&device_))))
		{
			return false;
		}
		device_->SetName(L"DeviceContext::device");

		return true;
	}

	/// @brief コマンドキュー作成
	/// @param device ID3D12Device
	/// @retval true  成功
	/// @retval false  失敗
	bool DeviceContext::CreateCommandQueue(ID3D12Device* device)
	{
		// キューの設定用構造体
		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // 一般的な描画はこれ
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		// キューの作成
		if (FAILED(device->CreateCommandQueue(
			&desc,
			IID_PPV_ARGS(command_queue_.ReleaseAndGetAddressOf()))))
		{
			return false;
		}
		command_queue_->SetName(L"DeviceContext::command_queue");

		return true;
	}

	/// @brief スワップチェイン作成
	/// @param hwnd HWND
	/// @param factory IDXGIFactory5*
	/// @param backbuffer_width バックバッファ幅
	/// @param backbuffer_height バックバッファ高さ
	/// @retval true 成功
	/// @retval false 失敗
	bool DeviceContext::CreateSwapChain(HWND hwnd, IDXGIFactory5* factory,
		int backbuffer_width, int backbuffer_height)
	{
		// スワップチェインの設定用構造体
		DXGI_SWAP_CHAIN_DESC1 desc{};
		desc.BufferCount = backbuffer_size_; //	バッファの数
		desc.Width = backbuffer_width;			//	バックバッファの幅
		desc.Height = backbuffer_height;		//	バックバッファの高さ
		desc.Format = backbuffer_format_;	// バックバッファフォーマット
		desc.BufferUsage =
			DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.Scaling = DXGI_SCALING_STRETCH;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		desc.Flags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		// スワップチェインオブジェクト作成
		ComPtr<IDXGISwapChain1> sc;
		if (FAILED(factory->CreateSwapChainForHwnd(
			command_queue_.Get(),	// スワップチェインを作るためのコマンドキュー
			hwnd,				// 表示先になるウィンドウハンドル
			&desc,				//	スワップチェインの設定
			nullptr,				//	フルスクリーン設定を指定しないのでnullptr
			nullptr,				//	マルチモニタ設定を指定しないのでnullptr
			sc.GetAddressOf())))
		{
			return false;
		}

		// IDXGISwapChain1::As関数からIDGISwapChain4インタフェースを取得
		if (FAILED(sc.As(&swap_chain_)))
		{
			return false;
		}

		// 初回描画に備えて、現在のバックバッファのインデックス取得
		backbuffer_index_ = swap_chain_->GetCurrentBackBufferIndex();

		return true;
	}

	/// @brief レンダーターゲット作成
	/// @param device ID3D12Device
	/// @param swap_chain IDXGISwapChain4*
	/// @retval true 成功
	/// @retval false 失敗
	bool DeviceContext::CreateRenderTarget(ID3D12Device* device, IDXGISwapChain4* swap_chain)
	{
		// レンダーターゲットデスクリプタヒープ作成

		{
			D3D12_DESCRIPTOR_HEAP_DESC desc{};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//	リソースの種類
			desc.NumDescriptors = backbuffer_size_;	//	RTの数だけの目盛るを確保
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			if (FAILED(device->CreateDescriptorHeap(
				&desc,
				IID_PPV_ARGS(rtv_descriptor_heap_.ReleaseAndGetAddressOf()))))
			{
				return false;
			}
			rtv_descriptor_heap_->SetName(L"DeviceContext::rtv_descriptor_heap");

			// デスクリプタヒープのメモリサイズを取得
			rtv_descriptor_size_ = device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_RTV);	//	HEAP_TYPE_RTVでRTのデスクリプタサイズを指定
		}

		// レンダーターゲットビューを作成
		{
			auto rtv_handle = rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();

			// バックバッファの数だけレンダーターゲットを作成
			for (int i = 0; i < backbuffer_size_; i++)
			{
				// スワップチェインからバックバッファを取得
				if (FAILED(swap_chain->GetBuffer(
					i,	//	バックバッファインデックス
					IID_PPV_ARGS(render_targets_[i].GetAddressOf()))))
				{
					return false;
				}

				// render_targetsに名前を設定するための文字列変数
				std::wstring name{ L"DeviceContext::render_targets[" + std::to_wstring(i) + L"]" };
				render_targets_[i]->SetName(name.c_str());

				// レンダーターゲットビューの設定用構造体
				D3D12_RENDER_TARGET_VIEW_DESC desc{};
				desc.Format = backbuffer_format_;
				desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

				// ビュー作成
				device_->CreateRenderTargetView(
					render_targets_[i].Get(),	// RT自体
					&desc,					// 上のRTの設定
					rtv_handle);				// 書込み先アドレス

				// ハンドルをずらす
				rtv_handle.ptr += rtv_descriptor_size_;
			}
		}

		return true;
	}

	/// @brief デプスステンシル作成
	/// @param device ID3D12Device
	/// @param backbuffer_width バックバッファ幅
	/// @param backvuffer_height バックバッファ高さ
	/// @retval true 成功
	/// @retval false 失敗
	bool DeviceContext::CreateDepthStencil(ID3D12Device* device,
		int backbuffer_width, int backbuffer_height)
	{
		// デプスステンシルバッファ本体
		{
			D3D12_RESOURCE_DESC resource_desc{};
			resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resource_desc.Width = backbuffer_width; // バックバッファ幅
			resource_desc.Height = backbuffer_height; // バックバッファ高さ
			resource_desc.DepthOrArraySize = 1;
			resource_desc.MipLevels = 0;
			resource_desc.SampleDesc.Count = 1;
			resource_desc.SampleDesc.Quality = 0;
			resource_desc.Format = depth_stencil_format_;	// DSフォーマット
			resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// DS用の設定

			// ヒープ設定
			D3D12_HEAP_PROPERTIES heap_prop{};
			heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
			heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heap_prop.CreationNodeMask = 1;
			heap_prop.VisibleNodeMask = 1;

			// デプスバッファをクリアするときの設定
			D3D12_CLEAR_VALUE clear_value{};
			clear_value.Format = depth_stencil_format_;	// デプスステンシルバッファフォーマット
			clear_value.DepthStencil.Depth = 1.0f;	// デプスは一番奥の値
			clear_value.DepthStencil.Stencil = 0;	// ステンシルはないので０

			// リソース作成
			if (FAILED(device_->CreateCommittedResource(
				&heap_prop,
				D3D12_HEAP_FLAG_NONE,
				&resource_desc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&clear_value,
				IID_PPV_ARGS(&depth_stencil_buffer_))))
			{
				return false;
			}
			depth_stencil_buffer_->SetName(L"DeviceContext::depth_stencil_buffer");
		}

		// デスクリプタヒープ・ビュー作成
		{
			D3D12_DESCRIPTOR_HEAP_DESC descriptor_desc{};
			descriptor_desc.NumDescriptors = 1;	//	このプログラムではデプスステンシルは1個
			descriptor_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	//	デプスステンシル用の値


			if (FAILED(device->CreateDescriptorHeap(
				&descriptor_desc,
				IID_PPV_ARGS(&dsv_descriptor_heap_))))
			{
				return false;
			}
			dsv_descriptor_heap_->SetName(L"DeviceContext::dsv_descriptor_heap");
			dsv_descriptor_size_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

			// ビュー作成
			D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
			dsv_desc.Format = depth_stencil_format_; // デプスバッファのフォーマット
			dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsv_desc.Flags = D3D12_DSV_FLAG_NONE;

			auto dsv = dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
			// ビュー作成，1個しかないので先頭に書き込む
			device->CreateDepthStencilView(
				depth_stencil_buffer_.Get(),
				&dsv_desc,
				dsv);
		}

		return true;
	}

	/// @brief コマンドアロケータ作成
	/// @param device ID3D12Device*
	/// @retval true 成功
	/// @retval false 失敗
	bool DeviceContext::CreateCommandAllocator(ID3D12Device* device)
	{
		for (int i = 0; i < backbuffer_size_; i++)
		{
			if (FAILED(device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(command_allocators_[i].ReleaseAndGetAddressOf()))))
			{
				return false;
			}
			std::wstring name{ L"DeviceContext::command_allocators[" + std::to_wstring(i) + L"]" };
			command_allocators_[i]->SetName(name.c_str());
		}

		return true;
	}

	/// @brief グラフィックスコマンド作成
	/// @param device ID3D12Device*
	/// @param allocator ID3D12CommandAllocator*
	/// @retval true 成功
	/// @retval false 失敗
	bool DeviceContext::CreateCommandList(ID3D12Device* device, ID3D12CommandAllocator* allocator)
	{
		if (FAILED(device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			allocator,
			nullptr,
			IID_PPV_ARGS(graphics_commnadlist_.ReleaseAndGetAddressOf()))))
		{
			return false;
		}
		graphics_commnadlist_->SetName(L"DeviceContext::graphics_commandlist");

		graphics_commnadlist_->Close();

		return true;
	}











#pragma endregion

}