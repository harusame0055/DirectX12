#include"texture.h"

#include"WICTextureLoader12.h"

using namespace Microsoft::WRL;
namespace ncc {

	bool LoadTextureFromFile(ComPtr<ID3D12Device> device, TextureResource* texture)
	{
		//�s���Ȉ����Ȃ玸�s
		if (device == nullptr || texture->name_.empty())
		{
			return false;
		}

		texture->data_.reset();

		// �摜�t�@�C�����V�X�e���������Ƀ��[�h����GPU�������Ƀ��\�[�X�̈���m��
		if (FAILED(DirectX::LoadWICTextureFromFile(
			device.Get(),
			texture->name_.c_str(),
			&texture->resource_,
			texture->data_,
			texture->subresource_)))
		{
			return false;
		}

		//���]�����
		texture->is_upload_wait_ = true;
	}

	/*-------------------------------------------*/
	/* TextureResource										*/
	/*-------------------------------------------*/
	TextureResource::~TextureResource()
	{
		Release();
	}

	/*-------------------------------------------*/
	/* TextureUploadCommandList                  */
	/*-------------------------------------------*/
	bool TextureUploadCommandList::BeginRecording(ComPtr<ID3D12Device> device)
	{
		//�����ȃf�o�C�X���n����邩�A ���łɋL�^�J�n��ԂȂ�Ύ��s
		if (device == nullptr || is_begin_recording_)
		{
			return false;
		}
		device_ = device;

		if (FAILED(device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator_))))
		{
			return false;
		}

		command_list_->SetName(L"TextureUploadCommandList");

		is_begin_recording_ = true; // �R�}���h�L�^�J�n��Ԃɂ���

		return true;
	}


	bool TextureUploadCommandList::EndRecording()
	{
		// �s���ȏ�ԂŌĂ΂ꂽ�玸�s
		if (is_begin_recording_ == false)
		{
			return false;
		}

		// �R�}���h���X�g�ɕ��Ă���
		if (FAILED(command_list_->Close()))
		{
			return false;// Close�����͎��s�̉\��������
		}

		is_begin_recording_ = false; // �L�^�I��

		return true;
	}

	bool TextureUploadCommandList::Execute()
	{
		// �s���ȏ�ԂȂ玸�s
		if (is_begin_recording_)
		{
			return false;
		}

		// �e�N�X�`�����P���]�����Ȃ��Ȃ珈���I��
		if (upload_list_.size() == 0)
		{
			return true; // �P���]�����Ȃ������Ŏ��s�ł͂Ȃ�
		}

		// �A�b�v���[�h���s�p�̃L���[�ƃt�F���X�͎g���̂Ăō��
		ComPtr<ID3D12CommandQueue> queue;
		ComPtr<ID3D12Fence> fence;

		// �t�F���X�쐬
		if (FAILED(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
		{
			return false;
		}
		fence->SetName(L"TextureUploadCommandList");

		//�L���[�쐬
		{
			D3D12_COMMAND_QUEUE_DESC desc{};
			desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // ��ʓI�ȕ`��
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

			if (FAILED(device_->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue))))
			{
				return false;
			}
			queue->SetName(L"TextureUploadCommandList");
		}

		// �t�F���X���Ď�����C�x���g
		Microsoft::WRL::Wrappers::Event event;
		event.Attach(CreateEventEx(
			nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
		if (!event.IsValid())
		{
			return false;
		}

		//�R�}���h���s
		const UINT64 complete_value = 1;
		ID3D12CommandList* commands[] = { command_list_.Get() };
		queue->ExecuteCommandLists(_countof(commands), commands);
		if (FAILED(queue->Signal(fence.Get(), complete_value)))
		{
			return false;
		}

		if (SUCCEEDED(fence->SetEventOnCompletion(complete_value, event.Get())))
		{
			WaitForSingleObjectEx(event.Get(), INFINITE, FALSE);
		}

		// ���\�[�X�Ƃ��Ďg����悤�ɂȂ����̂ŗL����Ԃɐݒ�
		for (std::size_t i = 0; i < upload_list_.size(); i++)
		{
			upload_list_[i]->is_upload_wait_ = false;
			upload_list_[i]->is_enabled_ = true;
		}

		// �]��������̕ЂÂ�����
		upload_list_.clear();
		buffers_.clear();
		command_list_.Reset();
		allocator_.Reset();

		return true;
	}
}