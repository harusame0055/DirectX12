#pragma once

#include <list>
#include <wrl.h>
#include <d3d12.h>

namespace ncc {

	/// @brief ディスクリプタハンドルを使いやすくしたクラス
	class DescriptorHandle {
	public:
		DescriptorHandle() = default;
		~DescriptorHandle() = default;

		/// @brief CPUハンドルの取得
		/// @return D3D12_CPU_DESCRIPTOR_HANDLE
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle();

		/// @brief GPUハンドルの取得
		/// @return D3D12_GPU_DESCRIPTOR_HANDLE
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle();

		/// @brief ハンドルが有効
		/// @retval true 有効
		/// @retval false 無効
		bool IsValid();

	private:
		friend class DescriptorHeap;

		D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle_{};

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
		UINT index_ = 0;
	};

	/// @brief ディスクリプタヒープを使いやすくするクラス
	class DescriptorHeap {
	public:
		DescriptorHeap() = default;
		~DescriptorHeap() = default;

		/// @brief 初期化
		/// @param device ComPtr<ID3D12Device>
		/// @param desc ヒープ記述子
		/// @retval true 成功
		/// @retval false 失敗
		bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, const D3D12_DESCRIPTOR_HEAP_DESC& desc);

		/// @brief 終了処理
		void Finalize();

		/// @brief デスクリプタ確保
		/// @param handle 確保したデスクリプタを書き込むハンドル
		/// @retval true 成功
		/// @retval false 失敗
		bool Allocate(DescriptorHandle* handle);

		/// @brief デスクリプタ解放
		/// @param handle 解放するデスクリプタ
		/// @retval true 成功
		/// @retval false 失敗
		bool Free(DescriptorHandle* handle);

		/// @brief デスクリプタヒープ本体
		/// @return ComPtr<ID3D12DescriptorHeap>
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap();

	private:
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc_{};
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
		UINT increment_size_ = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle_start_{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle_start_{};

		UINT heap_index_ = 0;
		UINT alloc_count_ = 0;

		std::list<UINT> free_list_;
	};
} //namespace ncc
