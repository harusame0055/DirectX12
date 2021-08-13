#pragma once
#include<cstdint>
#include<vector>

#include<d3d12.h>
#include<DirectXMath.h>

namespace ncc {

	/// @brief 描画短形, テクスチャの一部を切り出すための座標を扱う
	struct Cell {
		std::uint32_t x;		//テクスチャ内での描画x原点 
		std::uint32_t y;		//テクスチャ内での描画y原点
		std::uint32_t w;	//描画する幅
		std::uint32_t h;	//描画する高さ
	};

	/// @brief スプライト
	class Sprite {
	public:
		// アニメーションの再生状態
		enum class AnimationState {
			Play,
			Pause,
			Reset,
			End,
			Size,
		};

		DirectX::XMFLOAT3 position{};		// 画面左上を原点とした描画座標
		DirectX::XMFLOAT2 scale{ 1.0f,1.0f };	// スケール
		float rotatin = 0.0f;				// z軸回転
		DirectX::XMFLOAT4 color{ 1.0f,1.0f,1.0f,1.0f };	// カラー

		/// @brief 更新処理,主にアニメーション処理を行う
		/// @param delta_time 1フレームのデルタタイム
		void Update(float delta_time);

		/// @brief 描画に使うテクスチャ情報をセット
		/// @param texture_view テクスチャビュー
		/// @param texture_size テクスチャの大きさ
		void SetTextureData(D3D12_GPU_DESCRIPTOR_HANDLE texture_view,
			const DirectX::XMUINT2& texture_size);


		/// @brief 描画用セルデータをセット
		/// @param cell_data セルデータの配列
		void cell_data(std::vector<Cell> cell_data)
		{
			cell_data_ = cell_data;
			cell_count_ = cell_data.size();
			animation_state(AnimationState::Pause);
		}

		/// @brief 描画されるセル
		/// @return Cell
		const Cell& current_cell() const
		{
			assert(cell_count_ > 0);
			return cell_data_[static_cast<int>(current_frame_)];
		}

		/// @brief 現在のスプライトの再生時間を取得
		/// @return 秒
		const float current_frame() const {
			return current_frame_;
		}

		/// @brief アニメーションのFPSを設定
		/// @param fps 0以上の値
		void fps(float fps)
		{
			assert(fps >= 0);
			fps_ = fps;
		}

		/// @brief アニメーションのループ設定
		/// @param loop trueでループ
		void is_loop(bool loop)
		{
			is_loop_ = loop;
		}

		/// @brief ループ設定を取得
		/// @return bool値
		bool is_loop() const
		{
			return is_loop_;
		}

		/// @brief 新しいアニメーションステートを設定
		/// @param new_state 新しいステート
		void animation_state(AnimationState new_state)
		{
			assert(new_state != AnimationState::Size);
			animation_state_ = new_state;
			current_frame_ = 0.0f;
		}

		/// @brief 現在のアニメーションステート
		/// @return アニメーションステート
		AnimationState animation_state() const
		{
			return animation_state_;
		}

		/// @brief 設定されているテクスチャビューの取得
		/// @return D3D12＿GPU_DESCRIPTOR_HANDLE
		D3D12_GPU_DESCRIPTOR_HANDLE texture_view()
		{
			return texture_view_;
		}

		/// @brief 設定されているテクスチャ座標を取得
		/// @return XMUINT2
		const DirectX::XMUINT2& texture_size()
		{
			return texture_size_;
		}

	private:
		// テクスチャビュー
		D3D12_GPU_DESCRIPTOR_HANDLE texture_view_{};

		// テクスチャのサイズ
		DirectX::XMUINT2 texture_size_{};

		// セルの配列
		std::vector<Cell> cell_data_;
		// セル数, cell_data_.size()のキャッシュ
		std::size_t cell_count_;

		// アニメーションステート
		AnimationState animation_state_ = AnimationState::Play;

		// アニメーションのFPS
		float fps_ = 12.0f;


		// アニメーション再生時間
		float current_frame_ = 0;

		//　ループ再生設定
		bool is_loop_ = false;

	};

}