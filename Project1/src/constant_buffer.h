#pragma once

#include<cstdint>
#include<memory>
#include<string>
#include<vector>

#include<d3d12.h>
#include<wrl.h>

#include"descriptor_heap.h"

namespace ncc {

	/// @brief �萔�o�b�t�@�̃��\�[�X�f�[�^���Ǘ�����N���X
	/// ���t���[���X�V����悤�Ȓ萔�f�[�^�̂��߂Ƀo�b�t�@�����O�Ή�
	class ConstantBuffer {
	public:
		ConstantBuffer() = default;

		/// @brief �f�X�g���N�^
		~ConstantBuffer();

		/// @brief �������B�g���O�ɕK�����s
		/// @param device D3D12Device
		/// @param buffering_count �萔�o�b�t�@�̃o�b�t�@�����O��
		/// @param byte_size �������ރf�[�^�T�C�Y
		/// @retval true ����I��
		/// retval false ���s
		bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, int buffering_count, std::size_t byte_size);

		/// @brief �������
		void Release();

		/// @brief �o�b�t�@�փf�[�^��������
		/// @param data �������݃f�[�^�̐擪�A�h���X
		/// @param index �o�b�t�@�����O����Ă���̂ŁA�������ރC���f�b�N�X���w��
		void UpdateBuffer(const void* data, int index);

		/// @brief �萔�o�b�t�@���\�[�X�̃|�C���^��Ԃ�
		/// @param index �v�f�̃C���f�b�N�X
		/// @return ID3D12Resource�|�C���^
		Microsoft::WRL::ComPtr<ID3D12Resource> resource(int index)
		{
			return resources_[index];
		}

		/// @brief ���������ɍ쐬�������\�[�X����Ԃ�
		/// @return �쐬���Ă���o�b�t�@��
		std::size_t buffer_count() const
		{
			return resources_.size();
		}

		/// @brief �萔�o�b�t�@��D3D12_RESOURCE_DESC
		/// @return D3D12_RESOURCE_DESC�̃R�s�[
		D3D12_RESOURCE_DESC resource_desc() const
		{
			return resource_desc_;
		}

	private:
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resources_;
		D3D12_HEAP_PROPERTIES heap_props_{};
		D3D12_RESOURCE_DESC resource_desc_{};
	};

	/// @brief �萔�o�b�t�@�r���[
	class ConstantBufferView {
	public:
		ConstantBufferView() = default;

		/// @brief �f�X�g���N�^
		~ConstantBufferView();

		/// @brief �r���[�쐬
		/// @param device ComPtr<ID3D12Device>
		/// @param heap �r���[�쐬����q�[�v
		/// @param cbuffer �r���[���쐬����o�b�t�@
		/// @retval true ����
		/// @return false ���s
		bool Create(Microsoft::WRL::ComPtr<ID3D12Device> device,
			DescriptorHeap* heap,
			ConstantBuffer* cbuffer);

		/// @brief �������
		void Release();

		/// @brief ���������ɍ쐬�������\�[�X����Ԃ�
		/// @return �쐬���Ă���o�b�t�@��
		std::size_t view_count() const
		{
			return handles_.size();
		}

		/// @brief �萔�o�b�t�@���\�[�X�̃f�B�X�N���v�^�n���h����Ԃ�
		/// @param index �擾����v�f�̃C���f�b�N�X
		/// @return DescriptorHandle�̃R�s�[
		DescriptorHandle handle(int index)
		{
			return handles_[index];
		}

	private:
		std::vector<DescriptorHandle> handles_;

		DescriptorHeap* heap_ = nullptr;
	};

}