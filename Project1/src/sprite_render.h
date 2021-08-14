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

	class SpriteRenderer {
	public:

		/// @brief レンダラの初期化
		/// @param device 
		/// @param buffering_count 
		/// @retval true 初期化成功 
		/// @retval false 初期化失敗 
		bool Initialize(ID3D12Device* device, int buffering_count, const D3D12_VIEWPORT& viewport);

		/// @brief レンダラ終了処理
		void Finalize();

		/// @brief 描画開始前処理, Drawを呼ぶ前に一度呼び出す
		/// @param command_list 
		void Begin(int backbuffer_index, ID3D12GraphicsCommandList* command_list);


		/// @brief スプライトの描画登録処理,実際にはまだ描画してない．
		/// @param sprite 描画スプライト
		void Draw(Sprite& sprite);

		/// @brief スプライトの描画登録処理
		/// @param texture 描画テクスチャのハンドル
		/// @param texture_size テクスチャ画像自体のサイズ
		/// @param position スプライトの描画位置
		/// @param rotation スプライトの回転角
		/// @param color 乗算カラー
		/// @param cell 描画短形
		void Draw(D3D12_GPU_DESCRIPTOR_HANDLE texture,
			const DirectX::XMUINT2& texture_size,
			const DirectX::XMFLOAT3& position,
			const float& rotation,
			const DirectX::XMFLOAT4& color,
			const Cell* cell);

		/// @brief 描画終了処理,
		void End();

	private:
		// 描画用のスプライトデータ
		struct SpriteData {
			DirectX::XMFLOAT3 pos;	// 座標
			DirectX::XMFLOAT4 color;	// 乗算カラー
			DirectX::XMFLOAT4 texcoord; // テクスチャの原点座標とサイズ
			float rotation;			// 回転
			DirectX::XMFLOAT2 size;	// 描画ポリゴンのサイズ
		};

		bool CreateConstans(ID3D12Device* device, int buffering_count);
		bool CreateIndexBuffer(ID3D12Device* device);
		bool CreatePipeline(ID3D12Device* device);
		bool CreateRootSignature(ID3D12Device* device);
		bool CreateShader();
		bool CreateVertexBuffer(ID3D12Device* device, int bufering_count);
		void MakeVertices();
		void Rendering();

		// バックバッファインデックス
		int backbuffer_index_ = 0;

		// ビューポート
		D3D12_VIEWPORT viewport_{};

		// スプライト描画時に座標を調整するために使う
		DirectX::XMFLOAT2 offset_position_{};

		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		// スプライト用頂点バッファ
		std::vector<ComPtr<ID3D12Resource>> vertex_buffers_;
		std::vector<D3D12_VERTEX_BUFFER_VIEW> vb_views_;
		std::vector<void*>vb_addrs_;

		// スプライト用インデックスバッファ
		ComPtr<ID3D12Resource> index_buffer_;
		D3D12_INDEX_BUFFER_VIEW ib_view_;

		// 描画するスプライト数
		int sprite_count_ = 0;

		// 描画するスプライトのデータの実態を格納
		std::vector<SpriteData> sprites_;

		// テクスチャ毎にスプライトデータを分けて記憶
		using GpuHandlePtr = UINT64;
		std::unordered_map<GpuHandlePtr, std::vector<SpriteData*>> draw_lists_;

		// 描画に使うコマンドリスト
		ID3D12GraphicsCommandList* command_list_ = nullptr;

		// 正射影行列用データ群
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