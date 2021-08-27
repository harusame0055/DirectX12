#include"constant_buffer.h"

namespace ncc {

#pragma region ConstantBuffer

	ConstantBuffer::~ConstantBuffer()
	{
		Release();
	}

	bool ConstantBuffer::Initialize(
		Microsoft::WRL::ComPtr<ID3D12Device>device,
		int buffering_count,
		std::size_t byte_size)
	{
		if (device == nullptr ||
			buffering_count <= 0 ||
			byte_size == 0)
		{
			return false;
		}

		// バックバッファ数分、定数バッファリソースを確保する
		resources_.resize(buffering_count);

		// 定数バッファリソースはデータサイズが256の倍数でないとダメ
		// バッファサイズを256でアライメント
		const UINT buffer_size = byte_size + 255 & ~255;

		heap_props_.Type = D3D12_HEAP_TYPE_UPLOAD;
		heap_props_.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heap_props_.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		resource_desc_.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resource_desc_.Alignment = 0;
		resource_desc_.Width = buffer_size;
		resource_desc_.Height = 1;
		resource_desc_.DepthOrArraySize = 1;
		resource_desc_.MipLevels = 1;
		resource_desc_.Format = DXGI_FORMAT_UNKNOWN;
		resource_desc_.SampleDesc.Count = 1;
		resource_desc_.SampleDesc.Quality = 0;
		resource_desc_.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resource_desc_.Flags = D3D12_RESOURCE_FLAG_NONE;

		for (int i = 0; i < buffering_count; i++)
		{
			if (FAILED(device->CreateCommittedResource(
				&heap_props_, D3D12_HEAP_FLAG_NONE, &resource_desc_,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				IID_PPV_ARGS(&resources_[i]))))
			{
				return false;
			}
		}

		return true;
	}

	void ConstantBuffer::Release()
	{
		const auto length = resources_.size();
		for (std::size_t i = 0; i < length; i++)
		{
			resources_[i].Reset();
		}
	}

	void ConstantBuffer::UpdateBuffer(const void* data, int index)
	{
		if (data == nullptr)
		{
			return;
		}

		void* mapped = nullptr;
		if (SUCCEEDED(resources_[index]->Map(0, nullptr, &mapped)))
		{
			std::memcpy(mapped, data, static_cast<std::size_t>(resource_desc_.Width));
			resources_[index]->Unmap(0, nullptr);
		}
	}

#pragma endregion

#pragma region ConstantBufferView

	ConstantBufferView::~ConstantBufferView()
	{
		Release();
	}

	bool ConstantBufferView::Create(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		DescriptorHeap* heap,
		ConstantBuffer* cbuffer)
	{
		if (device == nullptr ||
			heap == nullptr ||
			cbuffer == nullptr)
		{
			return false;
		}
		heap_ = heap;

		std::size_t size = cbuffer->buffer_count();
		handles_.resize(size);

		// ビューの作成
		for (std::size_t i = 0; i < size; i++)
		{
			// ディスクリプタハンドル確保

			if (!heap->Allocate(&handles_[i]))
			{
				return false;
			}

			D3D12_CONSTANT_BUFFER_VIEW_DESC view_desc{};
			// バッファのGPUアドレス
			view_desc.BufferLocation = cbuffer->resource(i)->GetGPUVirtualAddress();
			// バッファサイズ
			view_desc.SizeInBytes = static_cast<UINT>(cbuffer->resource_desc().Width);
			// ビュー作成
			device->CreateConstantBufferView(&view_desc, handles_[i].cpu_handle());
		}

		return true;
	}

	void ConstantBufferView::Release()
	{
		std::size_t size = handles_.size();
		for (std::size_t i = 0; i < size; i++)
		{
			heap_->Free(&handles_[i]);
		}
	}
#pragma endregion

} // namespace ncc