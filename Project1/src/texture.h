#pragma once

#include<cstddef>
#include<memory>
#include<string>
#include<vector>

#include<d3d12.h>
#include<wrl.h>

#include"descriptor_heap.h"

namespace ncc {

	class TextureResource;

	/// @brief �摜�t�@�C�����V�X�e���������Ƀ��[�h����
	///	GPU�������Ƀ��\�[�X�̈���m�ۂ���. �f�[�^�]���͂��Ȃ��̂Œ���
	/// @param device ID3D12Device�|�C���^
	/// @param texture TextureResource�̎Q��
	/// @return �����Ȃ�true,���s�Ȃ�false.
	bool LoadTextureFromFile(Microsoft::WRL::ComPtr<ID3D12Device> device,
		TextureResource* texture);

	/// @brief �e�N�X�`�����\�[�X���Ǘ�����N���X
	class TextureResource {
	public:
		TextureResource() = default;


		/// @brief �R�s�[�R���X�g���N�^
		/// @param o �R�s�[��
		TextureResource(const TextureResource& o) : TextureResource(o.name_)
		{}

		TextureResource(const std::wstring& name)
			:name_(name)
		{}

		~TextureResource();

		/// @brief �e�N�X�`���f�[�^��GPU�ɖ��A�b�v���[�h�ς݂�
		/// @return �A�b�v���[�h�ς݂Ȃ�true, �����łȂ����false.
		bool IsUploaded()
		{
			return !is_upload_wait_;
		}

		bool IsValid()
		{
			if (resource_ == nullptr ||
				is_upload_wait_ == true ||
				is_enabled_ == false)
			{
				return false;
			}
			return true;
		}

		/// @brief ���݂̃e�N�X�`�����J�����ĐV�����e�N�X�`�����\�[�X������悤�ɂ���
		/// @param name �V�����e�N�X�`����
		void Reset(const std::wstring& name)
		{
			Release();
			name_ = name;
		}

		/// @brief �I�u�W�F�N�g�̃f�[�^���J������
		void Release()
		{
			name_.clear();

			is_upload_wait_ = false;
			is_enabled_ = false;

			subresource_ = D3D12_SUBRESOURCE_DATA{};
			data_.reset();
			resource_.Reset();
		}

		/// @brief GPU�������ɍ쐬���ꂽ���\�[�X��Ԃ�
		/// @return �e�N�X�`�����\�[�X
		Microsoft::WRL::ComPtr<ID3D12Resource> resource()
		{
			return resource_;
		}

		/// @brief �e�N�X�`�����擾
		/// @return �e�N�X�`����
		const std::wstring& name()
		{
			return name_;
		}

	private:
		friend bool LoadTextureFromFile(Microsoft::WRL::ComPtr<ID3D12Device> device,
			TextureResource* texture);
		friend class TextureUploadComandList;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
		std::wstring name_;
		bool is_upload_wait_ = false;
		bool is_enabled_ = false;
		D3D12_SUBRESOURCE_DATA subresource_{};
		std::unique_ptr<std::uint8_t[]> data_;

	};

	/// @brief �V�X�e���������ɂ���e�N�X�`���f�[�^��GPU��������
	/// �A�b�v���[�h���邽�߂̏������s��
	class TextureUploadComandList
	{
	public :
		TextureUploadComandList() = default;
		~TextureUploadComandList() = default;

		/// @brief �e�N�X�`���A�b�v���[�h�R�}���h�L�^�J�n
		/// @param device ID3D12Device�|�C���^
		/// @return �����Ȃ�true / ���s�Ȃ�false
		bool BeginRecording(Microsoft::WRL::ComPtr<ID3D12Device> device);

		/// @brief �R�}���h�L�^�I��
		/// @return �����Ȃ�true / ���s�Ȃ�false
		bool EndRecording();

		/// @brief �o�b�t�@�f�[�^��GPU�������ɓ]��
		/// @return �����Ȃ�true /�@���s�Ȃ�false
		bool Execute();

		/// @brief GPU�������ɃA�b�v���[�h������TextureResource���R�}���h���X�g�ɒǉ�����
		/// @param texture �Ώۂ̃e�N�X�`��
		/// @return �����Ȃ�true /�@���s�Ȃ�false
		bool AddList(TextureResource* texture);

	private:
		Microsoft::WRL::ComPtr<ID3D12Device> device_;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>allocator_;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_;

		std::vector<TextureResource*>upload_list_;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>buffers_;

		bool is_begin_recording_ = false;
	};

	/// @brief �e�N�X�`�����\�[�X�̃V�F�[�_���\�[�X�r���[����������N���X
	class TextureView {
	public:
		TextureView() = default;

		/// @brief �f�X�g���N�^
		~TextureView();

		/// @brief �r���[�쐬
		/// @param device ComPtr<ID3D12Device>
		/// @param srv_heap �r���[���������ރq�[�v
		/// @param texture �쐬�Ώۃe�N�X�`�����\�[�X
		/// @retval true ����
		/// @retval false ���s
		bool Create(Microsoft::WRL::ComPtr<ID3D12Device> device,
			DescriptorHeap* srv_heap,
			TextureResource* texture);

		/// @brief �������
		/// @retval true ����
		/// @retval false ���s
		bool Release();

		/// @brief �r���[���w�����[�X���擾
		/// @return ComPtr<ID3D12Resource>
		Microsoft::WRL::ComPtr<ID3D12Resource> resource()
		{
			return texture_->resource();
		}

		/// @brief �r���[��GPU�n���h�����擾
		/// @return D3D12_GPU_DESCRIPTOR_HANDLE
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle()
		{
			return handle_.gpu_handle();
		}

		D3D12_CPU_DESCRIPTOR_HANDLE cpu_hadle()
		{
			return handle_.cpu_handle();
		}

	private:
		TextureResource* texture_ = nullptr;
		DescriptorHeap* heap_ = nullptr;
		DescriptorHandle handle_;
	};
}