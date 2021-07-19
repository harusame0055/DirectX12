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

  D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle();
  D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle();
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

  bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, const D3D12_DESCRIPTOR_HEAP_DESC& desc);
  void Finalize();

  bool Allocate(DescriptorHandle* handle);
  bool Free(DescriptorHandle* handle);

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
