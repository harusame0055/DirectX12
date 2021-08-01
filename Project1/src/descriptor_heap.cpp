#include"descriptor_heap.h"

using namespace Microsoft::WRL;

namespace ncc {
	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle::cpu_handle()
	{
		return cpu_handle_;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle::gpu_handle()
	{
		return gpu_handle_;
	}

	bool DescriptorHandle::IsValid()
	{
		// 有効ならheap_にはアドレスがあるはず
		if (heap_ == nullptr)
		{
			return false;
		}
		return true;
	}

	bool DescriptorHeap::Initialize(ComPtr<ID3D12Device>device,
		const D3D12_DESCRIPTOR_HEAP_DESC& desc)
	{
		// 不正引数なら失敗
		if (device == nullptr)
		{
			return false;
		}

		// デスクリプタヒープ作成
		if (FAILED(device->CreateDescriptorHeap(
			&desc, IID_PPV_ARGS(&heap_))))
		{
			return false;
		}
		heap_->SetName(L"DescriptorHeap");

		// ハンドル作成用のメンバを初期化
		increment_size_ = device->GetDescriptorHandleIncrementSize(desc.Type);
		cpu_handle_start_ = heap_->GetCPUDescriptorHandleForHeapStart();
		gpu_handle_start_ = heap_->GetGPUDescriptorHandleForHeapStart();

		// ヒープ設定を一応覚えておく
		heap_desc_ = desc;

		return true;
	}

	void DescriptorHeap::Finalize()
	{
		free_list_.clear();

		increment_size_ = 0;
		cpu_handle_start_.ptr = 0;
		gpu_handle_start_.ptr = 0;
		heap_.Reset();
	}

	bool DescriptorHeap::Allocate(DescriptorHandle* handle)
	{
		// 使えるハンドルがないときは失敗
		if (alloc_count_ >= heap_desc_.NumDescriptors)
		{
			return false;
		}

		// 使用するハンドルのインデックスを決める
		UINT index = heap_index_;

		if (free_list_.empty() == false)
		{
			// 返却されたハンドルがあれば、 そちらを優先的に使う
			index = free_list_.front();
			free_list_.pop_front();
		}
		else
		{
			// 新規に作るのでヒープの位置をずらす
			heap_index_++;
		}

		// ハンドル作成
		handle->cpu_handle_ = cpu_handle_start_;
		handle->gpu_handle_ = gpu_handle_start_;
		handle->cpu_handle_.ptr += index * increment_size_;
		handle->gpu_handle_.ptr *= index * increment_size_;
		handle->heap_ = heap_.Get();
		handle->index_ = index;

		// 使用数を1増やして終わり
		alloc_count_++;

		return true;
	}

	bool DescriptorHeap::Free(DescriptorHandle* handle)
	{
		// 不正なハンドルなので失敗する
		// ハンドルとヒープのID3DDescriptorHeapが一致しない時も失敗
		if (handle->heap_ == nullptr ||
			handle->heap_ != heap_)
		{
			return false;
		}

		// ハンドルが使っていた番号は空番号として覚える
		free_list_.emplace_back(handle->index_);

		// ハンドルの無効か
		handle->cpu_handle_.ptr = 0;
		handle->gpu_handle_.ptr = 0;
		handle->heap_ = nullptr;

		// 使用数を１減らして終わり
		alloc_count_--;

		return true;
	}


	ComPtr<ID3D12DescriptorHeap> DescriptorHeap::heap()
	{
		return heap_;
	}
}	//	namespace ncc