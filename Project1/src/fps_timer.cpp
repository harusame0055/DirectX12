#include "fps_timer.h"

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

/// @brief 1�t���[�����̎��Ԃ�i�߂�. (���v�̐j��"�e�B�b�N"�^�b�N���邱�Ƃ���e�B�b�N)
void FpsTimer::Tick()
{
  // ���ݎ��Ԃ��擾
  LARGE_INTEGER current_time;
  QueryPerformanceCounter(&current_time);

  // �O���Tick����̌o�ߎ���
  std::uint64_t delta_time = current_time.QuadPart - last_time_.QuadPart;
  last_time_ = current_time;     // ���ݎ��Ԃ��o����
  second_counter_ += delta_time; // �o�ߎ��ԍX�V

  // ���炩�̗��R�őO�񂩂玞�Ԃ����������Ă���Ƃ���max_delta_��delta_time�ɂ���
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

  // 1�b�o�߂�����Ăяo���ꂽTick()�̐���fps�Ƃ���
  if (second_counter_ >= static_cast<std::uint64_t>(frequency_.QuadPart))
  {
    fps_ = frame_this_count_;
    // ����1�b�ׂ̈Ƀ��Z�b�g
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

/// @brief �O��e�B�b�N����̌o�ߎ��Ԃ�b���Ŏ擾
/// @return �����b��
double FpsTimer::delta_seconds() const
{
  return TicksToSeconds(delta_ticks_);
}

/// @brief �O��e�B�b�N����̌o�ߎ��Ԃ��e�B�b�N���Ŏ擾
/// @return �����e�B�b�N
std::uint64_t FpsTimer::delta_ticks() const
{
  return delta_ticks_;
}

/// @brief �^�C�}�J�n����̌o�ߎ���
/// @return �o�ߕb��
double FpsTimer::total_second() const
{
  return TicksToSeconds(total_ticks_);
}

/// @brief �^�C�}�J�n����̌o�ߎ���
/// @return �o�߃e�B�b�N��
std::uint64_t FpsTimer::total_ticks() const
{
  return total_ticks_;
}

/// @brief �^�C�}�J�n����̃t���[����
/// @return ���t���[����
std::uint32_t FpsTimer::frame_count() const
{
  return total_frame_count_;
}

/// @brief ����1�b�̃t���[�����[�g
/// @return �t���[�����[�g
std::uint32_t FpsTimer::frames_per_second() const
{
  return fps_;
}

/// @brief �e�B�b�N����b���ɕϊ�����w���p�[�֐�
/// @param ticks �ϊ��������e�B�b�N��
/// @return �ϊ���̕b��
double FpsTimer::TicksToSeconds(const std::uint64_t ticks)
{
  return static_cast<double>(ticks) / TicksPerSecond;
}

/// @brief �b�����e�B�b�N���ɕϊ�����w���p�[�֐�
/// @param seconds �ϊ��������b��
/// @return �ϊ���̃e�B�b�N��
std::uint64_t FpsTimer::SecondsToTicks(const double seconds)
{
  return static_cast<std::uint64_t>(seconds * TicksPerSecond);
}

} // namespace ncc
