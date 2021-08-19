#include"fps_timer.h"

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

	/// @brief 1フレーム分の時間を進める．
	void FpsTimer::Tick()
	{
		// 現在時間を取得
		LARGE_INTEGER current_time;
		QueryPerformanceCounter(&current_time);

		// 前回からの経過時間
		std::uint64_t delta_time = current_time.QuadPart - last_time_.QuadPart;
		last_time_ = current_time;	// 現在の時間を覚える
		second_counter_ += delta_time;	//経過時間更新

		// 何らかの理由で前回から時間が経ちすぎているときは max_delta_ を delta_time にする
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

		// 1病型化したら呼び出されたTick()の数をfpsとする
		if (second_counter_ > static_cast<std::uint64_t>(frequency_.QuadPart))
		{
			fps_ = frame_this_count_;
			// 次の1秒のためにリセット
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

	/// @brief 前回 Tick からの経過時間を秒数で取得
	/// @return 差分秒数
	float FpsTimer::delta_seconds() const
	{
		return static_cast<float>(TicksToSeconds(delta_ticks_));
	}

	/// @brief 前回 Tick からの経過時間を Tick 数で取得
	/// @return 差分 Tick
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
	/// @return 経過 Tick 数
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

	/// @brief Tick数を秒数に変換するヘルパー関数
	/// @param ticks 変換したい Tick 数
	/// @return 変換後の秒数
	double FpsTimer::TicksToSeconds(const std::uint64_t ticks)
	{
		return static_cast<double>(ticks) / TicksPerSecond;
	}

	/// @brief 秒数を Tick 数に変換するヘルパー関数
	/// @param seconds 変換したい秒数
	/// @return 変換後の Tick 数
	std::uint64_t FpsTimer::SecondsToTicks(const double seconds)
	{
		return static_cast<std::uint64_t>(seconds * TicksPerSecond);
	}

} //namespace ncc