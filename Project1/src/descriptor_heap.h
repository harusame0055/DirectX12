#pragma once

#include <list>
#include <wrl.h>
#include <d3d12.h>

namespace ncc {

	/// @brief �f�B�X�N���v�^�n���h�����g���₷�������N���X
	class DescriptorHandle {
	public:
		DescriptorHandle() = default;
		~DescriptorHandle() = default;

		/// @brief CPU�n���h���̎擾
		/// @return D3D12_CPU_DESCRIPTOR_HANDLE
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle();

		/// @brief GPU�n���h���̎擾
		/// @return D3D12_GPU_DESCRIPTOR_HANDLE
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle();

		/// @brief �n���h�����L��
		/// @retval true �L��
		/// @retval false ����
		bool IsValid();

	private:
		friend class DescriptorHeap;

		D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle_{};

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
		UINT index_ = 0;
	};

	/// @brief �f�B�X�N���v�^�q�[�v���g���₷������N���X
	class DescriptorHeap {
	public:
		DescriptorHeap() = default;
		~DescriptorHeap() = default;

		/// @brief ������
		/// @param device ComPtr<ID3D12Device>
		/// @param desc �q�[�v�L�q�q
		/// @retval true ����
		/// @retval false ���s
		bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, const D3D12_DESCRIPTOR_HEAP_DESC& desc);

		/// @brief �I������
		void Finalize();

		/// @brief �f�X�N���v�^�m��
		/// @param handle �m�ۂ����f�X�N���v�^���������ރn���h��
		/// @retval true ����
		/// @retval false ���s
		bool Allocate(DescriptorHandle* handle);

		/// @brief �f�X�N���v�^���
		/// @param handle �������f�X�N���v�^
		/// @retval true ����
		/// @retval false ���s
		bool Free(DescriptorHandle* handle);

		/// @brief �f�X�N���v�^�q�[�v�{��
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
