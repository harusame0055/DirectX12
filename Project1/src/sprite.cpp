#include"sprite.h"

namespace ncc {

	void Sprite::Update(float delta_time)
	{
		assert(cell_count_ >= 1);

		if (cell_count_ == 1)return;

		// �X�e�[�g���ɏ���
		switch (animation_state_)
		{
		case AnimationState::Play:
				current_frame_ += fps_ * delta_time;

				// �S�t���[���\�������������̏���
				if (current_frame_ >= cell_count_)
				{
					// ���[�v����ꍇ
					if (is_loop_)
					{
						// �A�j���[�V�����t���[���������Ԃ����炷
						do
						{
							current_frame_ -= static_cast<float>(cell_count_);
						} while (current_frame_ >= static_cast<float>(cell_count_));
					}
					else
					{
						// ���[�v���Ȃ��Ȃ�End�ɂ��čĐ���~
						animation_state_ = AnimationState::End;
						current_frame_ = static_cast<float>(cell_count_ - 1);
					}
				}
				break;

		case AnimationState::Pause:
			// �ꎞ��~�̎��͉������Ȃ�
			break;

		case AnimationState::Reset:
			// �J�����g�t���[�����O�ɖ߂��čĐ��J�n
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