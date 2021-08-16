#include"fps_timer.h"

namespace ncc {

	/// @brief �R���X�g���N�^
	FpsTimer::FpsTimer()
	{
		// �^�C�}�̎��g��
		QueryPerformanceFrequency(&frequency_);
		// �������̃J�E���^���o���Ă���
		QueryPerformanceCounter(&last_time_);
		// ���e����f���^�^�C���̍ő�
		max_delta_ = frequency_.QuadPart / 10;
	}

	/// @brief 1�t���[�����̎��Ԃ�i�߂�D
	void FpsTimer::Tick()
	{
		// ���ݎ��Ԃ��擾
		LARGE_INTEGER current_time;
		QueryPerformanceCounter(&current_time);

		// �O�񂩂�̌o�ߎ���
		std::uint64_t delta_time = current_time.QuadPart - last_time_.QuadPart;
		last_time_ = current_time;	// ���݂̎��Ԃ��o����
		second_counter_ += delta_time;	//�o�ߎ��ԍX�V

		// ���炩�̗��R�őO�񂩂玞�Ԃ��o�������Ă���Ƃ��� max_delta_ �� delta_time �ɂ���
		// ���̂܂܎g���Ǝ��Ԃ��傫�����ăQ�[���̏������j�]����̂�h��
		if (delta_time > max_delta_)
		{
			delta_time = max_delta_;
		}
		// ���ԂƂ��Ĉ�����悤�Ɍv�Z
		delta_time = delta_time * TicksPerSecond / frequency_.QuadPart;

		// ���ꂼ��X�V
		delta_ticks_ = delta_time;
		total_ticks_ += delta_time;
		++total_frame_count_;
		++frame_this_count_;

		// 1�a�^��������Ăяo���ꂽTick()�̐���fps�Ƃ���
		if (second_counter_ > static_cast<std::uint64_t>(frequency_.QuadPart))
		{
			fps_ = frame_this_count_;
			// ����1�b�̂��߂Ƀ��Z�b�g
			frame_this_count_ = 0;
			// 0�ɂ���ƒ[���̎��Ԃ������ă^�C�}�����X�ɋ���
			// ��]�Œ[���̎��Ԃ�ۑ����Ă���
			second_counter_ %= frequency_.QuadPart;
		}
	}

	/// @brief �^�C�}�̃��Z�b�g
	void FpsTimer::ResetDeltaTime()
	{
		QueryPerformanceCounter(&last_time_);
		frame_this_count_ = 0;
		fps_ = 0;
		second_counter_ = 0;
	}

	/// @brief �O�� Tick ����̌o�ߎ��Ԃ�b���Ŏ擾
	/// @return �����b��
	float FpsTimer::delta_seconds() const
	{
		return static_cast<float>(TicksToSeconds(delta_ticks_));
	}

	/// @brief �O�� Tick ����̌o�ߎ��Ԃ� Tick ���Ŏ擾
	/// @return ���� Tick
	std::uint64_t FpsTimer::delta_ticks() const
	{
		return delta_ticks_;
	}

}