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

	/// @brief スプライト描画オブジェクト

	class SpriteRender {
	public:

		/// @brief レンダラの初期化
		/// @param device 
		/// @param buffering_count 
		/// @retval true 初期化成功 
		/// @retval false 初期化失敗 
		bool Initialize(ID3D12Device* device, int buffering_count, const D3D12_VIEWPORT& viewport);

		/// @brief レンダラ終了処理
		void Finalize();

		/// @brief スプライトの描画登録処理,実際にはまだ描画してない．
		/// @param sprite 描画スプライト
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