#include"texture.h"

#include"WICTextureLoader12.h"

using namespace Microsoft::WRL;
namespace ncc {

	bool LoadTextureFromFile(ComPtr<ID3D12Device> device, TextureResource* texture)
	{
		//不正な引数なら失敗
		if (device == nullptr || texture->name_.empty())
		{
			return false;
		}

		texture->data_.reset();

		// 画像ファイルをシステムメモリにロードしてGPUメモリにリソース領域を確保
		if (FAILED(DirectX::LoadWICTextureFromFile(
			device.Get(),
			texture->name_.c_str(),
			&texture->resource_,
			texture->data_,
			texture->subresource_)))
		{
			return false;
		}

		//未転送状態
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
		//無効なデバイスが渡されるか、 すでに記録開始状態ならば失敗
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

		is_begin_recording_ = true; // コマンド記録開始状態にする

		return true;
	}


	bool TextureUploadCommandList::EndRecording()
	{

	}
}