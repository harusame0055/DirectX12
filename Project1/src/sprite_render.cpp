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

	/// @brief ���[�g�V�O�l�`���쐬
	/// @param device 
	/// @retval true ��������
	/// @retval false �������s
	bool SpriteRenderer::CreateRootSignature(ID3D12Device* device)
	{
		// �ÓI�T���v���[�̐ݒ�
		D3D12_STATIC_SAMPLER_DESC sampler{};
		// �e�N�X�`���t�B���^�����O
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		// �e�N�X�`���A�h���b�V���O
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;

		// �V�F�[�_�[�w��
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		/*----------------------------------------------*/
		/* D3D12_DESCRIPTOR_RANGE�쐬          */
		/*----------------------------------------------*/

		// srv�p��Range
		D3D12_DESCRIPTOR_RANGE srv{};

		// �e�N�X�`�����\�[�X1�̃p�����[�^�ݒ�
		srv.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srv.NumDescriptors = 1;
		srv.BaseShaderRegister = 0;
		srv.RegisterSpace = 0;
		srv.OffsetInDescriptorsFromTableStart =
			D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		/*----------------------------------------------*/
		/* �V�[���S�̂ŋ��ʂ���萔�o�b�t�@�p��Range*/
		/*----------------------------------------------*/
		D3D12_DESCRIPTOR_RANGE scene_cbv{};
		scene_cbv.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		scene_cbv.NumDescriptors = 1;
		scene_cbv.BaseShaderRegister = 0;
		scene_cbv.RegisterSpace = 0;
		scene_cbv.OffsetInDescriptorsFromTableStart =
			D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		/*----------------------------------------------*/
		/* �I�u�W�F�N�g���̒萔�o�b�t�@�p��Range  */
		/*----------------------------------------------*/
		D3D12_DESCRIPTOR_RANGE obj_cbv{};
		obj_cbv.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		obj_cbv.NumDescriptors = 1;
		obj_cbv.BaseShaderRegister = 1;
		obj_cbv.RegisterSpace = 0;
		obj_cbv.OffsetInDescriptorsFromTableStart =
			D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		/*----------------------------------------------*/
		/* D3D12_ROOT_PARAMETER�ݒ�             */
		/*----------------------------------------------*/
		D3D12_ROOT_PARAMETER root_param[2]{};
		// srv
		root_param[0].ParameterType =
			D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		root_param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		// �p�����[�^�̔z��
		root_param[0].DescriptorTable.NumDescriptorRanges = 1;
		root_param[0].DescriptorTable.pDescriptorRanges = &srv; // �擪�A�h���X
		/*--------------*/
		/* scene_cbv  */
		/*--------------*/
		root_param[1].ParameterType =
			D3D12_ROOT_PARAMETER_TYPE_CBV;
		// ���_�V�F�[�_�Ŏg������ SHADER_VISIBILITY_VERTEX
		root_param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		root_param[1].Descriptor.RegisterSpace = 0;
		root_param[1].Descriptor.ShaderRegister = 0;

		// �ݒ肵����ROOT_PARAMETER��n��
		D3D12_ROOT_SIGNATURE_DESC rs_desc{};
		rs_desc.NumParameters = _countof(root_param);
		// ROOT_PARAMETER�̐擪�v�f�̃A�h���X
		rs_desc.pParameters = root_param;
		rs_desc.NumStaticSamplers = 1;
		// �X�^�e�B�b�N�T���v���z��̐擪�v�f�A�h���X
		rs_desc.pStaticSamplers = &sampler;
		rs_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


		// D3D12_ROOT_SIGNATURE_DESC�� GPU �������ł���`���ɕϊ�
		ComPtr<ID3DBlob>signature, error;
		if (FAILED(D3D12SerializeRootSignature(
			&rs_desc,
			D3D_ROOT_SIGNATURE_VERSION_1_0,
			&signature,
			nullptr)))
		{
			// ���s������G���[���o��
			OutputDebugStringA((const char*)error->GetBufferPointer());
			OutputDebugStringA("CreateRootSignature Failed.");
			return false;
		}

		// ���[�g�V�O�l�`���I�u�W�F�N�g�쐬
		if (FAILED(device->CreateRootSignature(
			0,
			signature->GetBufferPointer(),
			signature->GetBufferSize(),
			IID_PPV_ARGS(&root_signature_))))
		{
			return false;
		}

		return true;
	}

	/// @brief �V�F�[�_�[�u���u�쐬 
	bool SpriteRenderer::CreateShader()
	{
		bool ret = true;
		ret = CreateShaderBlob(L"shader/VertexShader.hlsl",
			L"vs_6_0",
			vs_);
		if (!ret)
		{
			return ret;
		}

		ret = CreateShaderBlob(L"shader/PixelShader.hlsl",
			L"ps_6_0",
			ps_);
		if (!ret)
		{
			return ret;
		}

		return ret;
	}

	/// @brief ���_�o�b�t�@�쐬�C���_�o�b�t�@�͖��t���[����蒼���̂Ńo�b�N�o�b�t�@���Ńo�b�t�@�����O
	/// @param device ID3D12Device
	/// @param buffering_count ���_�o�b�t�@�̃o�b�t�@�����O��
	/// @retval true ����
	/// @retval false ���s
	bool SpriteRenderer::CreateVertexBuffer(ID3D12Device* device, const int buffering_count)
	{
		// �f�[�^�͖��t���[���\�z����

		UINT buffer_size = maxSpriteSize * verticesPerSprite * sizeof(SpriteVertex);

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

		vertex_buffers_.clear();
		vertex_buffers_.resize(buffering_count);
		vb_views_.clear();
		vb_views_.resize(buffering_count);

		vb_addrs_.clear();
		vb_addrs_.resize(buffering_count);

		for (int i = 0; i < buffering_count; i++)
		{
			if (FAILED(device->CreateCommittedResource(
				&properties,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(vertex_buffers_[i].ReleaseAndGetAddressOf()))))
			{
				return false;
			}
			std::wstring str = L"SpriteRenderer VertexBuffer_" + std::to_wstring(i);
			vertex_buffers_[i]->SetName(str.c_str());

			if (FAILED(vertex_buffers_[i]->Map(0, nullptr, &vb_addrs_[i])))
			{
				return false;
			}

			vb_views_[i].BufferLocation = vertex_buffers_[i]->GetGPUVirtualAddress();
			vb_views_[i].SizeInBytes = static_cast<UINT>(buffer_size);
			vb_views_[i].StrideInBytes = sizeof(SpriteVertex);
		}

		return true;
	}

	/// @brief ���_�f�[�^�쐬
	void SpriteRenderer::MakeVertices()
	{
		std::vector<GpuHandlePtr> handles;

		// ����t���[���Ŏg�����_�o�b�t�@���擾
		SpriteVertex* vertex = static_cast<SpriteVertex*>(vb_addrs_[backbuffer_index_]);

		// 1�X�v�^�C�g(4���_)�����_�����
		// �e�N�X�`���P�ʂŃX�v�^�C�g�����X�g���C�܂Ƃ܂Ă���

		int index = 0;	//	���_�o�b�t�@�̏����݈ʒu
		for (const auto& list : draw_lists_)
		{
			handles.emplace_back(list.first);

			for (const auto sprite : list.second)
			{
				auto half_w = sprite->size.x * 0.5f;
				auto half_h = sprite->size.y * 0.5f;

				// �R�T�C���E�T�C���̌v�Z
				auto cos = XMScalarCos(sprite->rotation);
				auto sin = XMScalarSin(sprite->rotation);

				auto x = offset_position_.x + sprite->pos.x;
				auto y = offset_position_.y - sprite->pos.y;

				// �X�v���C�g���㒸�_
				vertex[index].position.x = ((-half_w) * cos) - ((half_h)*sin) + x;
				vertex[index].position.y = ((-half_w) * sin) + ((half_h)*cos) + y;
				vertex[index].position.z = sprite->pos.z;
				vertex[index].color = sprite->color;
				vertex[index].texcoord.x = sprite->texcoord.x;
				vertex[index].texcoord.y = sprite->texcoord.y;
				// �X�v���C�g�E�㒸�_
				vertex[index + 1].position.x = ((half_w)*cos) - ((half_h)*sin) + x;
				vertex[index + 1].position.y = ((half_w)*sin) + ((half_h)*cos) + y;
				vertex[index + 1].position.z = sprite->pos.z;
				vertex[index + 1].color = sprite->color;
				vertex[index + 1].texcoord.x = sprite->texcoord.z;
				vertex[index + 1].texcoord.y = sprite->texcoord.y;
				// �X�v���C�g�������_
				vertex[index + 2].position.x = ((-half_w) * cos) - ((-half_h) * sin) + x;
				vertex[index + 2].position.y = ((-half_w) * sin) + ((-half_h) * cos) + y;
				vertex[index + 2].position.z = sprite->pos.z;
				vertex[index + 2].color = sprite->color;
				vertex[index + 2].texcoord.x = sprite->texcoord.x;
				vertex[index + 2].texcoord.y = sprite->texcoord.w;
				// �X�v���C�g�E�����_
				vertex[index + 3].position.x = ((half_w)*cos) - ((-half_h) * sin) + x;
				vertex[index + 3].position.y = ((half_w)*sin) + ((-half_h) * cos) + y;
				vertex[index + 3].position.z = sprite->pos.z;
				vertex[index + 3].color = sprite->color;
				vertex[index + 3].texcoord.x = sprite->texcoord.z;
				vertex[index + 3].texcoord.y = sprite->texcoord.w;

				// ���̃X�v���C�g�̂��߂ɃC���f�b�N�X��i�߂�
				index += 4;
			}
		}
	}

	/// @brief �`��R�}���h�쐬
	void SpriteRenderer::Rendering()
	{
		command_list_->SetPipelineState(pso_.Get());
		command_list_->SetGraphicsRootSignature(root_signature_.Get());
		command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		command_list_->IASetIndexBuffer(&ib_view_);
		command_list_->IASetVertexBuffers(0, 1, &vb_views_[backbuffer_index_]);
		command_list_->SetGraphicsRootConstantBufferView(1, view_proj_buffer_.resource(backbuffer_index_)->GetGPUVirtualAddress());

		std::size_t draw_count = 0;
		for (const auto& list : draw_lists_)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE srv{ list.first };
			command_list_->SetGraphicsRootDescriptorTable(0, srv);

			auto count = list.second.size();
			command_list_->DrawIndexedInstanced(
				count * indicesPerSprite,			// �`��Ɏg���C���f�b�N�X��
				1,
				draw_count * indicesPerSprite,	// ���_�o�b�t�@�̓ǂݎ��ʒu
				0, 0);

			// ���̕`��p�ɕ`�搔���X�V
			draw_count += count;
		}
	}
#pragma endregion

} // namespace ncc