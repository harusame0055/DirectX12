#include "descriptor_heap.h"

using namespace Microsoft::WRL;

namespace ncc {

/// @brief ディスクリプタのCPUハンドル取得
/// @return D3D12_CPU_DESCRIPTOR_HANDLE
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle::cpu_handle()
{
  return cpu_handle_;
}

/// @brief ディスクリプタのGPUハンドル取得
/// @return D3D12_GPU_DESCRIPTOR_HANDLE
D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle::gpu_handle()
{
  return gpu_handle_;
}

/// @brief このハンドルが有効か
/// @return 有効ならtrue, 無効ならfalse.
bool DescriptorHandle::IsValid()
{
  // 有効なら heap_ にはアドレスがあるはず
  if (heap_ == nullptr)
  {
    return false;
  }
  return true;
}

/// @brief ディスクリプタヒープクラスの初期化
/// @param device D3D12デバイス
/// @param desc 作成したいヒープの記述
/// @return 成功ならtrue, 失敗ならfalse.
bool DescriptorHeap::Initialize(ComPtr<ID3D12Device> device,
                                const D3D12_DESCRIPTOR_HEAP_DESC& desc)
{
  // 不正な引数なら失敗
  if (device == nullptr)
  {
    return false;
  }

  // ディスクリプタヒープ作成
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

/// @brief 終了処理
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
  // 使えるハンドルが無いときは失敗
  if (alloc_count_ >= heap_desc_.NumDescriptors)
  {
    return false;
  }

  // 使用するハンドルのインデックスを決める
  UINT index = heap_index_;

  if (free_list_.empty() == false)
  {
    // 返却されたハンドルがあれば、そちらを優先的に使う
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
  handle->gpu_handle_.ptr += index * increment_size_;
  handle->heap_ = heap_.Get();
  handle->index_ = index;

  // 使用数を1増やして終わり
  alloc_count_++;

  return true;
}

/// @brief ハンドルの解放
/// @param handle 解放したいハンドル
/// @return 成功ならtrue, 失敗ならfalse.
bool DescriptorHeap::Free(DescriptorHandle* handle)
{
  // 不正なハンドルなので失敗する
  // ハンドルとヒープの ID3D12DescriptorHeapが一致しないときも失敗
  if (handle->heap_ == nullptr ||
      handle->heap_ != heap_)
  {
    return false;
  }

  // ハンドルが使っていた番号は秋番号として覚える
  free_list_.emplace_back(handle->index_);

  // ハンドル無効化
  handle->cpu_handle_.ptr = 0;
  handle->gpu_handle_.ptr = 0;
  handle->heap_ = nullptr;

  // 使用数を1減らして終わり
  alloc_count_--;

  return true;
}

/// @brief 自身の ID3D12DescriptorHeap を返す
/// @return ComPtr<ID3D12DescriptorHeap>
ComPtr<ID3D12DescriptorHeap> DescriptorHeap::heap()
{
  return heap_;
}

} //namespace ncc
