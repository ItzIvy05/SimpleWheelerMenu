#include "KeyInput.h"

#include <atomic>

namespace KeyInput
{
	namespace
	{
		constexpr std::uint32_t kMouseOffset = 256;
		constexpr std::uint32_t kWheelUp = 264;
		constexpr std::uint32_t kGamepadOffset = 266;
		constexpr std::array<std::uint32_t, 16> kGamepadMasks{ 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 0x0100, 0x0200, 0x1000, 0x2000, 0x4000, 0x8000, 0x0009, 0x000A };

		std::atomic<State> g_state{ State::kIdle };
		std::atomic<std::uint32_t> g_code{ 0 };
		std::atomic<std::uint32_t> g_held{ 0 };

		std::uint32_t ToCode(RE::INPUT_DEVICE a_device, std::uint32_t a_id)
		{
			switch (a_device) {
			case RE::INPUT_DEVICE::kKeyboard:
				if (a_id < kMouseOffset) {
					return a_id;
				}
				return 0;
			case RE::INPUT_DEVICE::kMouse:
				if (a_id < kGamepadOffset - kMouseOffset) {
					return kMouseOffset + a_id;
				}
				return 0;
			case RE::INPUT_DEVICE::kGamepad:
				if (const auto it = std::ranges::find(kGamepadMasks, a_id); it != kGamepadMasks.end()) {
					return kGamepadOffset + static_cast<std::uint32_t>(it - kGamepadMasks.begin());
				}
				return 0;
			default:
				return 0;
			}
		}

		void Handle(const RE::ButtonEvent& a_button)
		{
			const auto code = ToCode(a_button.GetDevice(), a_button.GetIDCode());
			if (code == 0) {
				return;
			}
			if (a_button.IsUp() && g_held.load(std::memory_order_relaxed) == code) {
				g_held.store(0, std::memory_order_relaxed);
			}
			if (!a_button.IsDown() || g_state.load(std::memory_order_acquire) != State::kWaiting) {
				return;
			}
			auto expected = State::kWaiting;
			g_code.store(code, std::memory_order_relaxed);
			if (g_state.compare_exchange_strong(expected, State::kCaptured, std::memory_order_acq_rel) && (code < kWheelUp || code >= kGamepadOffset)) {
				g_held.store(code, std::memory_order_relaxed);
			}
		}
	}

	void Begin()
	{
		g_state.store(State::kWaiting, std::memory_order_release);
	}

	void Cancel()
	{
		auto expected = State::kWaiting;
		g_state.compare_exchange_strong(expected, State::kIdle, std::memory_order_acq_rel);
		g_held.store(0, std::memory_order_relaxed);
	}

	State Poll(std::uint32_t& a_code)
	{
		const auto state = g_state.load(std::memory_order_acquire);
		if (state == State::kCaptured) {
			a_code = g_code.load(std::memory_order_relaxed);
			g_state.store(State::kIdle, std::memory_order_release);
		}
		return state;
	}

	bool Process(const RE::InputEvent* const* a_events)
	{
		if (!a_events || (g_state.load(std::memory_order_acquire) != State::kWaiting && g_held.load(std::memory_order_relaxed) == 0)) {
			return false;
		}
		for (auto* event = *a_events; event; event = event->next) {
			if (const auto* button = event->AsButtonEvent()) {
				Handle(*button);
			}
		}
		return true;
	}
}
