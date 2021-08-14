#include"sprite_render.h"

#include<string>
#include"utility.h"

using namespace DirectX;

namespace ncc {

	// 1ポリゴン辺りのインデックス数
	static constexpr int  indicesPerSprite = 6;
	// 1ポリゴン辺りの頂点数
	static constexpr int verticesPerSprite = 4;
	// 1スプライトシステムが扱えるスプライトの最大数
	static constexpr int maxSpriteSize = 2024;

	// SpriteRendererが使う頂点構造
	struct SpriteVertex {
		DirectX::XMFLOAT3 position;	//	3D座標
		DirectX::XMFLOAT4 color;		// 頂点カラー
		DirectX::XMFLOAT2 texcoord;	// テクスチャUV座標
	};

	// SpriteVertexの属例レイアウト
	constexpr D3D12_INPUT_ELEMENT_DESC spriteVertexLayouts[]
	{
		{// 頂点座標
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0},
		{// 頂点カラー
			"COLOR",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0},
		{// UV座標
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0},
	};

}// namespace ncc

namespace ncc {
#pragma region public

	void SpriteRenderer::Begin(int backbuffer_index, ID3D12GraphicsCommandList* command_list)
	{
		assert(command_list);

		assert(command_list_ == nullptr);

		backbuffer_index_ = backbuffer_index;
		command_list_ = command_list;
		sprite_count_ = 0;
		draw_lists_.clear();
	}

	void SpriteRenderer::Draw(Sprite& sprite)
	{
		Draw(sprite.texture_view(), sprite.texture_size(), sprite.position, sprite.rotatin, sprite.color, &sprite.current_cell());
	}

	void SpriteRenderer::Draw(D3D12_GPU_DESCRIPTOR_HANDLE texture,
		const XMUINT2& texture_size,
		const XMFLOAT3& position,
		const float& rotation,
		const DirectX::XMFLOAT4& color,
		const Cell* cell)
	{
		assert(sprite_count_ <= maxSpriteSize);

		auto sprite = &sprites_[sprite_count_];
		sprite->pos.x = position.x;
		sprite->pos.y = position.y;
		sprite->pos.z = position.z;
		sprite->color = color;
		sprite->rotation = rotation;

		// cell がなければテクスチャ全体を描画する
		if (cell == nullptr)
		{
			sprite->size = { static_cast<float>(texture_size.x), static_cast<float>(texture_size.y) };
			sprite->texcoord = XMFLOAT4{ 0.0f,0.0f,1.0f,1.0f };
		}
		else
		{
			sprite->size = { static_cast<float>(cell->w),static_cast<float>(cell->h) };

			float w = static_cast<float>(texture_size.x);
			float h = static_cast<float>(texture_size.y);

			// テクスチャ座標の構築
			sprite->texcoord.x = cell->x / w;
			sprite->texcoord.y = cell->y / h;
			sprite->texcoord.x = cell->w / w + sprite->texcoord.x;
			sprite->texcoord.w = cell->h / h + sprite->texcoord.y;
		}

		draw_lists_[texture.ptr].emplace_back(sprite);
		sprite_count_++;
	}

	void SpriteRenderer::End()
	{
		// 頂点データを一気に構築してコマンドを作成するよ
		MakeVertices();
		Rendering();
		command_list_ = nullptr;
	}

	void SpriteRenderer::Finalize()
	{
		command_list_ = nullptr;
		sprite_count_ = 0;
		draw_lists_.clear();
		sprites_.clear();
	}

	bool SpriteRenderer::Initialize(ID3D12Device* device, const int buffering_count, const D3D12_VIEWPORT& viewport)
	{
		assert(device != nullptr);

		viewport_ = viewport;
		offset_position_.x = -viewport.Width * 0.5f;
		offset_position_.y = viewport.Height * 0.5f;

		command_list_ = nullptr;

		if (!CreatePipeline(device))
		{
			return false;
		}

		if (!CreateIndexBuffer(device))
		{
			return false;
		}

		if (!CreateVertexBuffer(device, buffering_count))
		{
			return false;
		}

		if (!CreateConstans(device, buffering_count))
		{
			return false;
		}

		draw_lists_.clear();
		sprite_count_ = 0;
		sprites_.clear();
		sprites_.resize(maxSpriteSize);

		return true;
	}
#pragma endregion

#pragma region private

	bool SpriteRenderer::CreateConstans(ID3D12Device* device, const int buffering_count)
	{
		// ディスクリプタヒープ作成
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = buffering_count;
		desc.NodeMask = 0;
		if (!cbv_heap_.Initialize(device, desc))
		{
			return false;
		}

		// 正射影変換行列
		{

			XMMATRIX orth = XMMatrixOrthographicLH(
				static_cast<float>(viewport_.Width),
				static_cast<float>(viewport_.Height),
				0.1f,
				100.0f);

			XMStoreFloat4x4(&view_proj_mat_, orth);
		}

		// 定数バッファリソースビューとビューを作成
		view_proj_buffer_.Initialize(device, buffering_count, sizeof(view_proj_mat_));
		view_proj_cbview_.Create(device, &cbv_heap_, &view_proj_buffer_);

		for (int i = 0; i < buffering_count; i++)
		{
			// 定数バッファにデータ書き込み
			view_proj_buffer_.UpdateBuffer(&view_proj_mat_, i);
		}
		return true;
	}

	/// @brief インデックスバッファ作成
	/// @param device ID3D12Device
	/// @retval true 成功
	/// @retval false 失敗
	bool SpriteRenderer::CreateIndexBuffer(ID3D12Device* device)
	{
		// maxSpriteSize * verticesPerSpriteのインデックスを先に構築

		std::vector<std::uint16_t> indices;
		const std::uint16_t len = maxSpriteSize * verticesPerSprite;

		for (std::uint16_t i = 0; i < len; i += verticesPerSprite)
		{
			std::uint16_t j = i;
			indices.emplace_back(static_cast<std::uint16_t>(j + 0));
			indices.emplace_back(static_cast<std::uint16_t>(j + 1));
			indices.emplace_back(static_cast<std::uint16_t>(j + 2));
			indices.emplace_back(static_cast<std::uint16_t>(j + 1));
			indices.emplace_back(static_cast<std::uint16_t>(j + 3));
			indices.emplace_back(static_cast<std::uint16_t>(j + 2));
		}

		UINT buffer_size = indices.size() * sizeof(indices[0]);

		D3D12_HEAP_PROPERTIES properties{};
		properties.Type = D3D12_HEAP_TYPE_UPLOAD;
		properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		properties.CreationNodeMask = 0;
		properties.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = buffer_size;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		if (FAILED(device->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(index_buffer_.ReleaseAndGetAddressOf()))))
		{
			return false;
		}
		index_buffer_->SetName(L"SpriteRenderer IndexBuffer");

		void* mapped = nullptr;
		index_buffer_->Map(0, nullptr, &mapped);
		std::memcpy(mapped, indices.data(), buffer_size);
		index_buffer_->Unmap(0, nullptr);

		ib_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
		ib_view_.SizeInBytes = buffer_size;
		ib_view_.Format = DXGI_FORMAT_R16_UINT;

		return true;
	}

	/// @brief パイプラインオブジェクト作成
	/// @param device 
	/// @retval true 処理成功
	/// @retval false 処理失敗
	bool SpriteRenderer::CreatePipeline(ID3D12Device* device)
	{
		if (!CreateShader())
		{
			return false;
		}

		if (!CreateRootSignature(device))
		{
			return false;
		}

		// PSO作成
		{
			// ブレンドステートの設定
			D3D12_BLEND_DESC blend_desc{};
			blend_desc.AlphaToCoverageEnable = TRUE;
			blend_desc.IndependentBlendEnable = FALSE;
			blend_desc.RenderTarget[0] = D3D12_RENDER_TARGET_BLEND_DESC{
				TRUE,
				FALSE,
				D3D12_BLEND_SRC_ALPHA,
				D3D12_BLEND_INV_SRC_ALPHA,
				D3D12_BLEND_OP_ADD,
				D3D12_BLEND_ONE,
				D3D12_BLEND_ZERO,
				D3D12_BLEND_OP_ADD,
				D3D12_LOGIC_OP_NOOP,
				D3D12_COLOR_WRITE_ENABLE_ALL,
			};

			// ラスタライザーステート設定
			D3D12_RASTERIZER_DESC raster_desc = {
				D3D12_FILL_MODE_SOLID,
				D3D12_CULL_MODE_BACK,
				FALSE,
				D3D12_DEFAULT_DEPTH_BIAS,
				D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
				D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
				TRUE,
				FALSE,
				FALSE,
				0,
				D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
			};

			// デプスステンシル設定
			D3D12_DEPTH_STENCIL_DESC ds_desc{};
			ds_desc.DepthEnable = TRUE;
			ds_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			ds_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			// ステンシル設定
			constexpr D3D12_DEPTH_STENCILOP_DESC stencilop = {
				D3D12_STENCIL_OP_KEEP,
				D3D12_STENCIL_OP_KEEP,
				D3D12_STENCIL_OP_KEEP,
				D3D12_COMPARISON_FUNC_ALWAYS
			};
			ds_desc.FrontFace = stencilop;
			ds_desc.BackFace = stencilop;

			// 頂点属性
			D3D12_INPUT_LAYOUT_DESC input_layout = {
				spriteVertexLayouts,
				_countof(spriteVertexLayouts) };

			// パイプラインステートオブジェクト
			D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc{};

			// ルートシグネチャ設定
			pso_desc.pRootSignature = root_signature_.Get();

			// シェーダー
			pso_desc.VS.pShaderBytecode = vs_->GetBufferPointer();
			pso_desc.VS.BytecodeLength = vs_->GetBufferSize();
			pso_desc.PS.pShaderBytecode = ps_->GetBufferPointer();
			pso_desc.PS.BytecodeLength = ps_->GetBufferSize();

			// ブレンドステート設定
			pso_desc.BlendState = blend_desc;
			// ラスタライザーステート設定
			pso_desc.RasterizerState = raster_desc;

			// レンダーターゲット
			pso_desc.NumRenderTargets = 1;
			pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

			// 頂点
			pso_desc.InputLayout = input_layout;
			pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			// デプスステンシルバッファの設定
			pso_desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			pso_desc.DepthStencilState = ds_desc;

			pso_desc.SampleDesc = { 1,0 };
			pso_desc.SampleMask = UINT_MAX;

			// PSO作成
			if (FAILED(device->CreateGraphicsPipelineState(
				&pso_desc,
				IID_PPV_ARGS(&pso_))))
			{
				return false;
			}
		}

		return true;
	}

#pragma endregion


}