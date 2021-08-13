#pragma once
#include<cstdint>
#include<vector>

#include<d3d12.h>
#include<DirectXMath.h>

namespace ncc {

	/// @brief �`��Z�`, �e�N�X�`���̈ꕔ��؂�o�����߂̍��W������
	struct Cell {
		std::uint32_t x;		//�e�N�X�`�����ł̕`��x���_ 
		std::uint32_t y;		//�e�N�X�`�����ł̕`��y���_
		std::uint32_t w;	//�`�悷�镝
		std::uint32_t h;	//�`�悷�鍂��
	};

	/// @brief �X�v���C�g
	class Sprite {
	public:
		// �A�j���[�V�����̍Đ����
		enum class AnimationState {
			Play,
			Pause,
			Reset,
			End,
			Size,
		};

		DirectX::XMFLOAT3 position{};		// ��ʍ�������_�Ƃ����`����W
		DirectX::XMFLOAT2 scale{ 1.0f,1.0f };	// �X�P�[��
		float rotatin = 0.0f;				// z����]
		DirectX::XMFLOAT4 color{ 1.0f,1.0f,1.0f,1.0f };	// �J���[

		/// @brief �X�V����,��ɃA�j���[�V�����������s��
		/// @param delta_time 1�t���[���̃f���^�^�C��
		void Update(float delta_time);

		/// @brief �`��Ɏg���e�N�X�`�������Z�b�g
		/// @param texture_view �e�N�X�`���r���[
		/// @param texture_size �e�N�X�`���̑傫��
		void SetTextureData(D3D12_GPU_DESCRIPTOR_HANDLE texture_view,
			const DirectX::XMUINT2& texture_size);


		/// @brief �`��p�Z���f�[�^���Z�b�g
		/// @param cell_data �Z���f�[�^�̔z��
		void cell_data(std::vector<Cell> cell_data)
		{
			cell_data_ = cell_data;
			cell_count_ = cell_data.size();
			animation_state(AnimationState::Pause);
		}

		/// @brief �`�悳���Z��
		/// @return Cell
		const Cell& current_cell() const
		{
			assert(cell_count_ > 0);
			return cell_data_[static_cast<int>(current_frame_)];
		}

		/// @brief ���݂̃X�v���C�g�̍Đ����Ԃ��擾
		/// @return �b
		const float current_frame() const {
			return current_frame_;
		}

		/// @brief �A�j���[�V������FPS��ݒ�
		/// @param fps 0�ȏ�̒l
		void fps(float fps)
		{
			assert(fps >= 0);
			fps_ = fps;
		}

		/// @brief �A�j���[�V�����̃��[�v�ݒ�
		/// @param loop true�Ń��[�v
		void is_loop(bool loop)
		{
			is_loop_ = loop;
		}

		/// @brief ���[�v�ݒ���擾
		/// @return bool�l
		bool is_loop() const
		{
			return is_loop_;
		}

		/// @brief �V�����A�j���[�V�����X�e�[�g��ݒ�
		/// @param new_state �V�����X�e�[�g
		void animation_state(AnimationState new_state)
		{
			assert(new_state != AnimationState::Size);
			animation_state_ = new_state;
			current_frame_ = 0.0f;
		}

		/// @brief ���݂̃A�j���[�V�����X�e�[�g
		/// @return �A�j���[�V�����X�e�[�g
		AnimationState animation_state() const
		{
			return animation_state_;
		}

		/// @brief �ݒ肳��Ă���e�N�X�`���r���[�̎擾
		/// @return D3D12�QGPU_DESCRIPTOR_HANDLE
		D3D12_GPU_DESCRIPTOR_HANDLE texture_view()
		{
			return texture_view_;
		}

		/// @brief �ݒ肳��Ă���e�N�X�`�����W���擾
		/// @return XMUINT2
		const DirectX::XMUINT2& texture_size()
		{
			return texture_size_;
		}

	private:
		// �e�N�X�`���r���[
		D3D12_GPU_DESCRIPTOR_HANDLE texture_view_{};

		// �e�N�X�`���̃T�C�Y
		DirectX::XMUINT2 texture_size_{};

		// �Z���̔z��
		std::vector<Cell> cell_data_;
		// �Z����, cell_data_.size()�̃L���b�V��
		std::size_t cell_count_;

		// �A�j���[�V�����X�e�[�g
		AnimationState animation_state_ = AnimationState::Play;

		// �A�j���[�V������FPS
		float fps_ = 12.0f;


		// �A�j���[�V�����Đ�����
		float current_frame_ = 0;

		//�@���[�v�Đ��ݒ�
		bool is_loop_ = false;

	};

}