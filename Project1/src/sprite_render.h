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

	class SpriteRender {
	public:

		/// @brief �����_���̏�����
		/// @param device 
		/// @param buffering_count 
		/// @retval true ���������� 
		/// @retval false ���������s 
		bool Initialize(ID3D12Device* device, int buffering_count, const D3D12_VIEWPORT& viewport);

		/// @brief �����_���I������
		void Finalize();

		/// @brief �X�v���C�g�̕`��o�^����,���ۂɂ͂܂��`�悵�ĂȂ��D
		/// @param sprite �`��X�v���C�g
		void Draw(Sprite& sprite);

		/// @brief 
		/// @param texture 
		/// @param texture_size 
		/// @param position 
		/// @param rotation 
		/// @param color 
		/// @param cell 
		void Draw(D3D12_GPU_DESCRIPTOR_HANDLE texture,
			const DirectX::XMUINT2& texture_size,
			const DirectX::XMFLOAT3& position,
			const float& rotation,
			const DirectX::XMFLOAT4& color,
			const Cell* cell);
	};
}