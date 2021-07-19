#pragma once

#include <cstdint>
#include <Windows.h>

namespace ncc {

/// @brief デルタタイムとフレームレート計算をする.
///   FPSの固定機能はなし.
///   MSのDirectXサンプルにあるStepTimerの簡易版.
class FpsTimer {
public:
  FpsTimer();
  ~FpsTimer() = default;

  void Tick();
  void ResetDeltaTime();
  double delta_seconds() const;
  std::uint64_t delta_ticks() const;
  double total_second() const;
  std::uint64_t total_ticks() const;
  std::uint32_t frame_count() const;
  std::uint32_t frames_per_second() const;

  static std::uint64_t SecondsToTicks(const double seconds);
  static double TicksToSeconds(const std::uint64_t ticks);

  static constexpr std::uint64_t TicksPerSecond = 10'000'000;

private:
  LARGE_INTEGER frequency_;
  LARGE_INTEGER last_time_;

  std::uint64_t max_delta_;
  std::uint64_t delta_ticks_ = 0;
  std::uint64_t total_ticks_ = 0;

  std::uint32_t total_frame_count_ = 0;
  std::uint32_t frame_this_count_ = 0;
  std::uint32_t fps_ = 0;

  std::uint64_t second_counter_ = 0;
};

} // namespace ncc
