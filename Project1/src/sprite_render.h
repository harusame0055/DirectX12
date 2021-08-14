#pragma once

#include<cstdint>
#include<unordered_map>
#include<vector>

#include<d3d12.h>
#include<DirectXMath.h>
#include<wrl.h>

#include"constant_buffer.h"
#include"descriptor_heap.h"
#include"sprite.h"

namespace ncc {

	/// @brief �X�v���C�g�`��I�u�W�F�N�g

	class SpriteRenderer {
	public:

		/// @brief �����_���̏�����
		/// @param device 
		/// @param buffering_count 
		/// @retval true ���������� 
		/// @retval false ���������s 
		bool Initialize(ID3D12Device* device, int buffering_count, const D3D12_VIEWPORT& viewport);

		/// @brief �����_���I������
		void Finalize();

		/// @brief �`��J�n�O����, Draw���ĂԑO�Ɉ�x�Ăяo��
		/// @param command_list 
		void Begin(int backbuffer_index, ID3D12GraphicsCommandList* command_list);


		/// @brief �X�v���C�g�̕`��o�^����,���ۂɂ͂܂��`�悵�ĂȂ��D
		/// @param sprite �`��X�v���C�g
		void Draw(Sprite& sprite);

		/// @brief �X�v���C�g�̕`��o�^����
		/// @param texture �`��e�N�X�`���̃n���h��
		/// @param texture_size �e�N�X�`���摜���̂̃T�C�Y
		/// @param position �X�v���C�g�̕`��ʒu
		/// @param rotation �X�v���C�g�̉�]�p
		/// @param color ��Z�J���[
		/// @param cell �`��Z�`
		void Draw(D3D12_GPU_DESCRIPTOR_HANDLE texture,
			const DirectX::XMUINT2& texture_size,
			const DirectX::XMFLOAT3& position,
			const float& rotation,
			const DirectX::XMFLOAT4& color,
			const Cell* cell);

		/// @brief �`��I������,
		void End();

	private:
		// �`��p�̃X�v���C�g�f�[�^
		struct SpriteData {
			DirectX::XMFLOAT3 pos;	// ���W
			DirectX::XMFLOAT4 color;	// ��Z�J���[
			DirectX::XMFLOAT4 texcoord; // �e�N�X�`���̌��_���W�ƃT�C�Y
			float rotation;			// ��]
			DirectX::XMFLOAT2 size;	// �`��|���S���̃T�C�Y
		};

		bool CreateConstans(ID3D12Device* device, int buffering_count);
		bool CreateIndexBuffer(ID3D12Device* device);
		bool CreatePipeline(ID3D12Device* device);
		bool CreateRootSignature(ID3D12Device* device);
		bool CreateShader();
		bool CreateVertexBuffer(ID3D12Device* device, int bufering_count);
		void MakeVertices();
		void Rendering();

		// �o�b�N�o�b�t�@�C���f�b�N�X
		int backbuffer_index_ = 0;

		// �r���[�|�[�g
		D3D12_VIEWPORT viewport_{};

		// �X�v���C�g�`�掞�ɍ��W�𒲐����邽�߂Ɏg��
		DirectX::XMFLOAT2 offset_position_{};

		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		// �X�v���C�g�p���_�o�b�t�@
		std::vector<ComPtr<ID3D12Resource>> vertex_buffers_;
		std::vector<D3D12_VERTEX_BUFFER_VIEW> vb_views_;
		std::vector<void*>vb_addrs_;

		// �X�v���C�g�p�C���f�b�N�X�o�b�t�@
		ComPtr<ID3D12Resource> index_buffer_;
		D3D12_INDEX_BUFFER_VIEW ib_view_;

		// �`�悷��X�v���C�g��
		int sprite_count_ = 0;

		// �`�悷��X�v���C�g�̃f�[�^�̎��Ԃ��i�[
		std::vector<SpriteData> sprites_;

		// �e�N�X�`�����ɃX�v���C�g�f�[�^�𕪂��ċL��
		using GpuHandlePtr = UINT64;
		std::unordered_map<GpuHandlePtr, std::vector<SpriteData*>> draw_lists_;

		// �`��Ɏg���R�}���h���X�g
		ID3D12GraphicsCommandList* command_list_ = nullptr;

		// ���ˉe�s��p�f�[�^�Q
		DescriptorHeap cbv_heap_;
		DirectX::XMFLOAT4X4 view_proj_mat_{};
		ConstantBuffer view_proj_buffer_;
		ConstantBufferView view_proj_cbview_;

		ComPtr<ID3D12RootSignature> root_signature_;
		ComPtr<ID3D12PipelineState>pso_;
		ComPtr<ID3DBlob>vs_;
		ComPtr<ID3DBlob>ps_;

	};

} // namespace ncc