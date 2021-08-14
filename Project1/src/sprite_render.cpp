#include"sprite_render.h"

#include<string>
#include"utility.h"

using namespace DirectX;

namespace ncc {

	// 1�|���S���ӂ�̃C���f�b�N�X��
	static constexpr int  indicesPerSprite = 6;
	// 1�|���S���ӂ�̒��_��
	static constexpr int verticesPerSprite = 4;
	// 1�X�v���C�g�V�X�e����������X�v���C�g�̍ő吔
	static constexpr int maxSpriteSize = 2024;

	// SpriteRenderer���g�����_�\��
	struct SpriteVertex {
		DirectX::XMFLOAT3 position;	//	3D���W
		DirectX::XMFLOAT4 color;		// ���_�J���[
		DirectX::XMFLOAT2 texcoord;	// �e�N�X�`��UV���W
	};

	// SpriteVertex�̑��჌�C�A�E�g
	constexpr D3D12_INPUT_ELEMENT_DESC spriteVertexLayouts[]
	{
		{// ���_���W
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0},
		{// ���_�J���[
			"COLOR",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0},
		{// UV���W
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

		// cell ���Ȃ���΃e�N�X�`���S�̂�`�悷��
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

			// �e�N�X�`�����W�̍\�z
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
		// ���_�f�[�^����C�ɍ\�z���ăR�}���h���쐬�����
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
		// �f�B�X�N���v�^�q�[�v�쐬
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = buffering_count;
		desc.NodeMask = 0;
		if (!cbv_heap_.Initialize(device, desc))
		{
			return false;
		}

		// ���ˉe�ϊ��s��
		{

			XMMATRIX orth = XMMatrixOrthographicLH(
				static_cast<float>(viewport_.Width),
				static_cast<float>(viewport_.Height),
				0.1f,
				100.0f);

			XMStoreFloat4x4(&view_proj_mat_, orth);
		}

		// �萔�o�b�t�@���\�[�X�r���[�ƃr���[���쐬
		view_proj_buffer_.Initialize(device, buffering_count, sizeof(view_proj_mat_));
		view_proj_cbview_.Create(device, &cbv_heap_, &view_proj_buffer_);

		for (int i = 0; i < buffering_count; i++)
		{
			// �萔�o�b�t�@�Ƀf�[�^��������
			view_proj_buffer_.UpdateBuffer(&view_proj_mat_, i);
		}
		return true;
	}

	/// @brief �C���f�b�N�X�o�b�t�@�쐬
	/// @param device ID3D12Device
	/// @retval true ����
	/// @retval false ���s
	bool SpriteRenderer::CreateIndexBuffer(ID3D12Device* device)
	{
		// maxSpriteSize * verticesPerSprite�̃C���f�b�N�X���ɍ\�z

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

	/// @brief �p�C�v���C���I�u�W�F�N�g�쐬
	/// @param device 
	/// @retval true ��������
	/// @retval false �������s
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

		// PSO�쐬
		{
			// �u�����h�X�e�[�g�̐ݒ�
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

			// ���X�^���C�U�[�X�e�[�g�ݒ�
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

			// �f�v�X�X�e���V���ݒ�
			D3D12_DEPTH_STENCIL_DESC ds_desc{};
			ds_desc.DepthEnable = TRUE;
			ds_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			ds_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			// �X�e���V���ݒ�
			constexpr D3D12_DEPTH_STENCILOP_DESC stencilop = {
				D3D12_STENCIL_OP_KEEP,
				D3D12_STENCIL_OP_KEEP,
				D3D12_STENCIL_OP_KEEP,
				D3D12_COMPARISON_FUNC_ALWAYS
			};
			ds_desc.FrontFace = stencilop;
			ds_desc.BackFace = stencilop;

			// ���_����
			D3D12_INPUT_LAYOUT_DESC input_layout = {
				spriteVertexLayouts,
				_countof(spriteVertexLayouts) };

			// �p�C�v���C���X�e�[�g�I�u�W�F�N�g
			D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc{};

			// ���[�g�V�O�l�`���ݒ�
			pso_desc.pRootSignature = root_signature_.Get();

			// �V�F�[�_�[
			pso_desc.VS.pShaderBytecode = vs_->GetBufferPointer();
			pso_desc.VS.BytecodeLength = vs_->GetBufferSize();
			pso_desc.PS.pShaderBytecode = ps_->GetBufferPointer();
			pso_desc.PS.BytecodeLength = ps_->GetBufferSize();

			// �u�����h�X�e�[�g�ݒ�
			pso_desc.BlendState = blend_desc;
			// ���X�^���C�U�[�X�e�[�g�ݒ�
			pso_desc.RasterizerState = raster_desc;

			// �����_�[�^�[�Q�b�g
			pso_desc.NumRenderTargets = 1;
			pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

			// ���_
			pso_desc.InputLayout = input_layout;
			pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			// �f�v�X�X�e���V���o�b�t�@�̐ݒ�
			pso_desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			pso_desc.DepthStencilState = ds_desc;

			pso_desc.SampleDesc = { 1,0 };
			pso_desc.SampleMask = UINT_MAX;

			// PSO�쐬
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