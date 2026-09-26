#pragma once

namespace KeyInput
{
	enum class State : std::uint8_t
	{
		kIdle,
		kWaiting,
		kCaptured
	};

	void Begin();
	void Cancel();
	State Poll(std::uint32_t& a_code);
	bool Process(const RE::InputEvent* const* a_events);
}
