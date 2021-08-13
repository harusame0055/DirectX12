#include"sprite.h"

namespace ncc {

	void Sprite::Update(float delta_time)
	{
		assert(cell_count_ >= 1);

		if (cell_count_ == 1)return;

		// ステート毎に処理
		switch (animation_state_)
		{
		case AnimationState::Play:
				current_frame_ += fps_ * delta_time;

				// 全フレーム表示しきった時の処理
				if (current_frame_ >= cell_count_)
				{
					// ループする場合
					if (is_loop_)
					{
						// アニメーションフレーム数ずつ時間を減らす
						do
						{
							current_frame_ -= static_cast<float>(cell_count_);
						} while (current_frame_ >= static_cast<float>(cell_count_));
					}
					else
					{
						// ループしないならEndにして再生停止
						animation_state_ = AnimationState::End;
						current_frame_ = static_cast<float>(cell_count_ - 1);
					}
				}
				break;

		case AnimationState::Pause:
			// 一時停止の時は何もしない
			break;

		case AnimationState::Reset:
			// カレントフレームを０に戻して再生開始
			current_frame_ = 0;
			animation_state_ = AnimationState::Play;
			break;

		case AnimationState::End:
			break;

		case AnimationState::Size:
		default:
			assert(false);
			break;
		}
	}

	void Sprite::SetTextureData(D3D12_GPU_DESCRIPTOR_HANDLE texture_view, const DirectX::XMUINT2& texture_size)
	{
		texture_view_ = texture_view;
		texture_size_ = texture_size;
	}

}// namespace ncc