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
		// 不正な状態で呼ばれたら失敗
		if (is_begin_recording_ == false)
		{
			return false;
		}

		// コマンドリストに閉じておく
		if (FAILED(command_list_->Close()))
		{
			return false;// Close処理は失敗の可能性もある
		}

		is_begin_recording_ = false; // 記録終了

		return true;
	}

	bool TextureUploadCommandList::Execute()
	{
		// 不正な状態なら失敗
		if (is_begin_recording_)
		{
			return false;
		}

		// テクスチャを１個も転送しないなら処理終了
		if (upload_list_.size() == 0)
		{
			return true; // １個も転送しないだけで失敗ではない
		}

		// アップロード実行用のキューとフェンスは使い捨てで作る
		ComPtr<ID3D12CommandQueue> queue;
		ComPtr<ID3D12Fence> fence;

		// フェンス作成
		if (FAILED(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
		{
			return false;
		}
		fence->SetName(L"TextureUploadCommandList");

		//キュー作成
		{
			D3D12_COMMAND_QUEUE_DESC desc{};
			desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // 一般的な描画
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

			if (FAILED(device_->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue))))
			{
				return false;
			}
			queue->SetName(L"TextureUploadCommandList");
		}

		// フェンスを監視するイベント
		Microsoft::WRL::Wrappers::Event event;
		event.Attach(CreateEventEx(
			nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
		if (!event.IsValid())
		{
			return false;
		}

		//コマンド実行
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

		// リソースとして使えるようになったので有効状態に設定
		for (std::size_t i = 0; i < upload_list_.size(); i++)
		{
			upload_list_[i]->is_upload_wait_ = false;
			upload_list_[i]->is_enabled_ = true;
		}

		// 転送完了後の片づけ処理
		upload_list_.clear();
		buffers_.clear();
		command_list_.Reset();
		allocator_.Reset();

		return true;
	}
}