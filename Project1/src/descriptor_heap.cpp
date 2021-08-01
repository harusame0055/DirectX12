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
		// �L���Ȃ�heap_�ɂ̓A�h���X������͂�
		if (heap_ == nullptr)
		{
			return false;
		}
		return true;
	}

	bool DescriptorHeap::Initialize(ComPtr<ID3D12Device>device,
		const D3D12_DESCRIPTOR_HEAP_DESC& desc)
	{
		// �s�������Ȃ玸�s
		if (device == nullptr)
		{
			return false;
		}

		// �f�X�N���v�^�q�[�v�쐬
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
		// �g����n���h�����Ȃ��Ƃ��͎��s
		if (alloc_count_ >= heap_desc_.NumDescriptors)
		{
			return false;
		}

		// �g�p����n���h���̃C���f�b�N�X�����߂�
		UINT index = heap_index_;

		if (free_list_.empty() == false)
		{
			// �ԋp���ꂽ�n���h��������΁A �������D��I�Ɏg��
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
		handle->gpu_handle_.ptr *= index * increment_size_;
		handle->heap_ = heap_.Get();
		handle->index_ = index;

		// �g�p����1���₵�ďI���
		alloc_count_++;

		return true;
	}

	bool DescriptorHeap::Free(DescriptorHandle* handle)
	{
		// �s���ȃn���h���Ȃ̂Ŏ��s����
		// �n���h���ƃq�[�v��ID3DDescriptorHeap����v���Ȃ��������s
		if (handle->heap_ == nullptr ||
			handle->heap_ != heap_)
		{
			return false;
		}

		// �n���h�����g���Ă����ԍ��͋�ԍ��Ƃ��Ċo����
		free_list_.emplace_back(handle->index_);

		// �n���h���̖�����
		handle->cpu_handle_.ptr = 0;
		handle->gpu_handle_.ptr = 0;
		handle->heap_ = nullptr;

		// �g�p�����P���炵�ďI���
		alloc_count_--;

		return true;
	}


	ComPtr<ID3D12DescriptorHeap> DescriptorHeap::heap()
	{
		return heap_;
	}
}	//	namespace ncc