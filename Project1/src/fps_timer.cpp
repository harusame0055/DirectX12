#include "fps_timer.h"

namespace ncc {

/// @brief コンストラクタ
FpsTimer::FpsTimer()
{
  // タイマの周波数
  QueryPerformanceFrequency(&frequency_);
  // 生成時のカウンタを覚えておく
  QueryPerformanceCounter(&last_time_);
  // 許容するデルタタイムの最大
  max_delta_ = frequency_.QuadPart / 10;
}

/// @brief 1フレーム分の時間を進める. (時計の針が"ティック"タックすることからティック)
void FpsTimer::Tick()
{
  // 現在時間を取得
  LARGE_INTEGER current_time;
  QueryPerformanceCounter(&current_time);

  // 前回のTickからの経過時間
  std::uint64_t delta_time = current_time.QuadPart - last_time_.QuadPart;
  last_time_ = current_time;     // 現在時間を覚える
  second_counter_ += delta_time; // 経過時間更新

  // 何らかの理由で前回から時間が立ちすぎているときはmax_delta_をdelta_timeにする
  // そのまま使うと時間が大きすぎてゲームの処理が破綻するのを防ぐ
  if (delta_time > max_delta_)
  {
    delta_time = max_delta_;
  }
  // 時間として扱えるように計算
  delta_time = delta_time * TicksPerSecond / frequency_.QuadPart;

  // それぞれ更新
  delta_ticks_ = delta_time;
  total_ticks_ += delta_time;
  ++total_frame_count_;
  ++frame_this_count_;

  // 1秒経過したら呼び出されたTick()の数をfpsとする
  if (second_counter_ >= static_cast<std::uint64_t>(frequency_.QuadPart))
  {
    fps_ = frame_this_count_;
    // 次の1秒の為にリセット
    frame_this_count_ = 0;
    // 0にすると端数の時間が失われてタイマが徐々に狂う
    // 剰余で端数の時間を保存しておく
    second_counter_ %= frequency_.QuadPart;
  }
}

/// @brief タイマのリセット
void FpsTimer::ResetDeltaTime()
{
  QueryPerformanceCounter(&last_time_);
  frame_this_count_ = 0;
  fps_ = 0;
  second_counter_ = 0;
}

/// @brief 前回ティックからの経過時間を秒数で取得
/// @return 差分秒数
double FpsTimer::delta_seconds() const
{
  return TicksToSeconds(delta_ticks_);
}

/// @brief 前回ティックからの経過時間をティック数で取得
/// @return 差分ティック
std::uint64_t FpsTimer::delta_ticks() const
{
  return delta_ticks_;
}

/// @brief タイマ開始からの経過時間
/// @return 経過秒数
double FpsTimer::total_second() const
{
  return TicksToSeconds(total_ticks_);
}

/// @brief タイマ開始からの経過時間
/// @return 経過ティック数
std::uint64_t FpsTimer::total_ticks() const
{
  return total_ticks_;
}

/// @brief タイマ開始からのフレーム数
/// @return 総フレーム数
std::uint32_t FpsTimer::frame_count() const
{
  return total_frame_count_;
}

/// @brief 直近1秒のフレームレート
/// @return フレームレート
std::uint32_t FpsTimer::frames_per_second() const
{
  return fps_;
}

/// @brief ティック数を秒数に変換するヘルパー関数
/// @param ticks 変換したいティック数
/// @return 変換後の秒数
double FpsTimer::TicksToSeconds(const std::uint64_t ticks)
{
  return static_cast<double>(ticks) / TicksPerSecond;
}

/// @brief 秒数をティック数に変換するヘルパー関数
/// @param seconds 変換したい秒数
/// @return 変換後のティック数
std::uint64_t FpsTimer::SecondsToTicks(const double seconds)
{
  return static_cast<std::uint64_t>(seconds * TicksPerSecond);
}

} // namespace ncc
