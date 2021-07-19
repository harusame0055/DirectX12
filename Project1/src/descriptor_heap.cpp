#include "descriptor_heap.h"

using namespace Microsoft::WRL;

namespace ncc {

/// @brief �f�B�X�N���v�^��CPU�n���h���擾
/// @return D3D12_CPU_DESCRIPTOR_HANDLE
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle::cpu_handle()
{
  return cpu_handle_;
}

/// @brief �f�B�X�N���v�^��GPU�n���h���擾
/// @return D3D12_GPU_DESCRIPTOR_HANDLE
D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle::gpu_handle()
{
  return gpu_handle_;
}

/// @brief ���̃n���h�����L����
/// @return �L���Ȃ�true, �����Ȃ�false.
bool DescriptorHandle::IsValid()
{
  // �L���Ȃ� heap_ �ɂ̓A�h���X������͂�
  if (heap_ == nullptr)
  {
    return false;
  }
  return true;
}

/// @brief �f�B�X�N���v�^�q�[�v�N���X�̏�����
/// @param device D3D12�f�o�C�X
/// @param desc �쐬�������q�[�v�̋L�q
/// @return �����Ȃ�true, ���s�Ȃ�false.
bool DescriptorHeap::Initialize(ComPtr<ID3D12Device> device,
                                const D3D12_DESCRIPTOR_HEAP_DESC& desc)
{
  // �s���Ȉ����Ȃ玸�s
  if (device == nullptr)
  {
    return false;
  }

  // �f�B�X�N���v�^�q�[�v�쐬
  if (FAILED(device->CreateDescriptorHeap(
    &desc, IID_PPV_ARGS(&heap_))))
  {
    return false;
  }
  heap_->SetName(L"DescriptorHeap");

  // �n���h���쐬�p�̃����o��������
  increment_size_ = device->GetDescriptorHandleIncrementSize(desc.Type);
  cpu_handle_start_ = heap_->GetCPUDescriptorHandleForHeapStart();
  gpu_handle_start_ = heap_->GetGPUDescriptorHandleForHeapStart();

  // �q�[�v�ݒ���ꉞ�o���Ă���
  heap_desc_ = desc;

  return true;
}

/// @brief �I������
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
  // �g����n���h���������Ƃ��͎��s
  if (alloc_count_ >= heap_desc_.NumDescriptors)
  {
    return false;
  }

  // �g�p����n���h���̃C���f�b�N�X�����߂�
  UINT index = heap_index_;

  if (free_list_.empty() == false)
  {
    // �ԋp���ꂽ�n���h��������΁A�������D��I�Ɏg��
    index = free_list_.front();
    free_list_.pop_front();
  }
  else
  {
    // �V�K�ɍ��̂Ńq�[�v�̈ʒu�����炷
    heap_index_++;
  }

  // �n���h���쐬
  handle->cpu_handle_ = cpu_handle_start_;
  handle->gpu_handle_ = gpu_handle_start_;
  handle->cpu_handle_.ptr += index * increment_size_;
  handle->gpu_handle_.ptr += index * increment_size_;
  handle->heap_ = heap_.Get();
  handle->index_ = index;

  // �g�p����1���₵�ďI���
  alloc_count_++;

  return true;
}

/// @brief �n���h���̉��
/// @param handle ����������n���h��
/// @return �����Ȃ�true, ���s�Ȃ�false.
bool DescriptorHeap::Free(DescriptorHandle* handle)
{
  // �s���ȃn���h���Ȃ̂Ŏ��s����
  // �n���h���ƃq�[�v�� ID3D12DescriptorHeap����v���Ȃ��Ƃ������s
  if (handle->heap_ == nullptr ||
      handle->heap_ != heap_)
  {
    return false;
  }

  // �n���h�����g���Ă����ԍ��͏H�ԍ��Ƃ��Ċo����
  free_list_.emplace_back(handle->index_);

  // �n���h��������
  handle->cpu_handle_.ptr = 0;
  handle->gpu_handle_.ptr = 0;
  handle->heap_ = nullptr;

  // �g�p����1���炵�ďI���
  alloc_count_--;

  return true;
}

/// @brief ���g�� ID3D12DescriptorHeap ��Ԃ�
/// @return ComPtr<ID3D12DescriptorHeap>
ComPtr<ID3D12DescriptorHeap> DescriptorHeap::heap()
{
  return heap_;
}

} //namespace ncc
