#pragma once
#include<d3d12.h>
#include<dxgi1_6.h>
#include<dxcapi.h>
#include<wrl.h>

namespace ncc {

	/// @brief D3D12デバイスおよび最低限の描画に必要なものをまとめたオブジェクト
	class DeviceContext {
	public:
		/// @brief 初期化
		/// @param hwnd HWND
		/// @param backbuffer_width バックバッファ幅
		/// @param backbuffer_height バックバッファ高さ
		/// @retval true 成功
		/// @retval false 失敗
		bool Initialize(HWND hwnd, int backbuffer_width, int backbuffer_height);

		/// @brief 終了処理
		void Finalize();

		/// @brief GPUとの同期
		void WaitForGpu();

		/// @brief GPUの描画処理との同期
		void WaitForPreviousFrame();

		/// @brief スワップチェーンプレゼント
		void Present();

		/// @brief デバイス取得
		/// @return ID3D12Device*
		ID3D12Device* device();

		/// @brief スワップチェーン取得
		/// @return IDXGISwapChain4*
		IDXGISwapChain4* swap_chain();

		/// @brief グラフィックスコマンドリスト取得
		/// @return ID3D12GraphicsCommandList*
		ID3D12GraphicsCommandList* command_list();

		/// @brief カレントフレームで使うべきアロケータ取得
		/// @return ID3D12CommandAllocator*
		ID3D12CommandAllocator* current_command_allocator();

		/// @brief インデックスで指定したアロケータ取得
		/// @param index 0 ~ backbuffer -1 の間の数値
		/// @return ID3D12CommandAllocator*
		ID3D12CommandAllocator* command_allocator(int index);

		/// @brief コマンドキュー
		/// @return ID3D12CommandQueue*
		ID3D12CommandQueue* command_queue();

		/// @brief カレントフレームで使えるレンダーターゲット
		/// @return ID3D12Resource*
		ID3D12Resource* current_render_target();

		/// @brief カレントフレームで使えるレンダーターゲットビュー
		/// @return D3D12_CPU_DESCRIPTOR_HANDLE
		const D3D12_CPU_DESCRIPTOR_HANDLE current_rtv() const;

		/// @brief デプスステンシルビュー取得
		/// @return D3D12_CPU_DESCRIPTOR_HANDLE
		const D3D12_CPU_DESCRIPTOR_HANDLE dsv() const;

		/// @brief カレントフレームでのバックバッファインデックス
		/// @return バックバッファインデックス
		const int backbuffer_index() const;

		/// @brief 作成されたバックバッファ数取得
		/// @return int
		const int backbuffer_size() const;

		/// @brief バックバッファのビューポート取得
		/// @return 内部のビューポートの参照
		const D3D12_VIEWPORT& screen_viewport() const;

		/// @brief シザー矩形取得
		/// @return 内部のシザーの参照
		const D3D12_RECT& scissor_rect() const;

	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		bool CreateCommandAllocator(ID3D12Device* device);
		bool CreateCommandList(ID3D12Device* device, ID3D12CommandAllocator* allocator);
		bool CreateCommandQueue(ID3D12Device* device);
		bool CreateDepthStencil(ID3D12Device* device, int backbuffer_width, int backbuffer_height);
		bool CreateDevice(IDXGIFactory5* factory);
		bool CreateFactory();
		bool CreateFence(ID3D12Device* device);
		bool CreateRenderTarget(ID3D12Device* device, IDXGISwapChain4* swap_chain);
		bool CreateSwapChain(HWND hwnd, IDXGIFactory5* factory, int backbuffer_width, int backbuffer_height);

		// WARPアダプタ使用フラグ(デバッグビルドのみ有効)
		static constexpr bool useWarpAdapter = true;

		// バックバッファ最大値
		static constexpr int maxBackBufferSize = 3;

		// デバイス
		ComPtr<ID3D12Device> device_ = nullptr;

		// アダプタを取得するためのファクトリ作成
		ComPtr<IDXGIFactory5> dxgi_factory_;

		// スワップチェーン
		ComPtr<IDXGISwapChain4> swap_chain_ = nullptr;

		// バックバッファフォーマット
		const DXGI_FORMAT backbuffer_format_ = DXGI_FORMAT_R8G8B8A8_UNORM;
		// 作成する数
		int backbuffer_size_ = 2;
		// 現在のバックバッファインデックス
		int backbuffer_index_ = 0;

		// RTデスクリプタヒープ
		ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap_ = nullptr;
		// RTデスクリプタの1個当たりのサイズ
		UINT rtv_descriptor_size_ = 0;
		// バックバッファとして使うレンダーターゲット配列
		ComPtr<ID3D12CommandQueue> command_queue_ = nullptr;

		// コマンドリスト，GPUに対して処理手順をリストとして持つ
		ComPtr<ID3D12GraphicsCommandList> graphics_commnadlist_ = nullptr;
		// コマンドアロケータ，コマンドを作成するためのメモリを確保するオブジェクト
		ComPtr<ID3D12CommandAllocator> command_allocators_[maxBackBufferSize];
		// コマンドキュー，1つ以上のコマンドリストをキューに積んでGPUに送る
		ComPtr<ID3D12CommandQueue> command_queue_ = nullptr;

		// デプスステンシルフォーマット
		DXGI_FORMAT depth_stencil_format_ = DXGI_FORMAT_D24_UNORM_S8_UINT;
		// デプスステンシルデスクリプタヒープ
		ComPtr<ID3D12DescriptorHeap> dsv_descriptor_heap_ = nullptr;
		// デプスステンシルデスクリプタヒープサイズ
		UINT dsv_descriptor_size_ = 0;
		// デプスステンシルバッファ
		ComPtr<ID3D12Resource> depth_stencil_buffer_ = nullptr;

		// フェンスオブジェクト
		ComPtr<ID3D12Fence> d3d12_fence_ = nullptr;
		// フェンスに書き込む値.
		UINT fence_values_[maxBackBufferSize]{};
		// CPU. GPUの同期処理を楽にとるために使う
		Microsoft::WRL::Wrappers::Event fence_event_;

		// ビューポート
		D3D12_VIEWPORT screen_viewport_{};
		// シザー
		D3D12_RECT scissor_rect_{};
	};

}	// namespace nss