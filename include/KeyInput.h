#pragma once

namespace KeyInput
{
	enum class State : std::uint8_t
	{
		kIdle,
		kWaiting,
		kCaptured
	};

	void Install();
	void Begin();
	State Poll(std::uint32_t& a_code);

	bool IsPlayStation();
	std::string_view Name(std::uint32_t a_code, bool a_playStation);
	std::string_view Problem(std::uint32_t a_code, bool a_gamepad, bool a_playStation);
}
