#include "KeyInput.h"

#include <atomic>

namespace KeyInput
{
	namespace
	{
		constexpr std::uint32_t kMouseOffset = 256;
		constexpr std::uint32_t kGamepadOffset = 266;
		constexpr std::uint32_t kLeftStickOffset = 282;
		constexpr std::uint32_t kCodeCount = 286;

		constexpr std::array<std::uint32_t, 16> kGamepadMasks{ 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 0x0100, 0x0200, 0x1000, 0x2000, 0x4000, 0x8000, 0x0009, 0x000A };
		constexpr std::array<std::uint32_t, 3> kPlayStationUnreachable{ 266, 274, 275 };

		constexpr std::array<std::string_view, 16> kPlayStationNames{
			"D-Pad Up (Xbox)",
			"L3",
			"R3",
			"Options",
			"D-Pad Up",
			"D-Pad Right",
			"D-Pad Down",
			"D-Pad Left",
			"LB (Xbox)",
			"RB (Xbox)",
			"Triangle",
			"Circle",
			"Cross",
			"Square",
			"L2",
			"R2"
		};

		struct KeyName
		{
			std::uint32_t code;
			std::string_view name;
		};

		constexpr std::array kKeyNames{
			KeyName{ 0, "Unmapped" },
			KeyName{ 1, "Escape" },
			KeyName{ 2, "1" },
			KeyName{ 3, "2" },
			KeyName{ 4, "3" },
			KeyName{ 5, "4" },
			KeyName{ 6, "5" },
			KeyName{ 7, "6" },
			KeyName{ 8, "7" },
			KeyName{ 9, "8" },
			KeyName{ 10, "9" },
			KeyName{ 11, "0" },
			KeyName{ 12, "Minus" },
			KeyName{ 13, "Equals" },
			KeyName{ 14, "Backspace" },
			KeyName{ 15, "Tab" },
			KeyName{ 16, "Q" },
			KeyName{ 17, "W" },
			KeyName{ 18, "E" },
			KeyName{ 19, "R" },
			KeyName{ 20, "T" },
			KeyName{ 21, "Y" },
			KeyName{ 22, "U" },
			KeyName{ 23, "I" },
			KeyName{ 24, "O" },
			KeyName{ 25, "P" },
			KeyName{ 26, "Left Bracket" },
			KeyName{ 27, "Right Bracket" },
			KeyName{ 28, "Enter" },
			KeyName{ 29, "Left Control" },
			KeyName{ 30, "A" },
			KeyName{ 31, "S" },
			KeyName{ 32, "D" },
			KeyName{ 33, "F" },
			KeyName{ 34, "G" },
			KeyName{ 35, "H" },
			KeyName{ 36, "J" },
			KeyName{ 37, "K" },
			KeyName{ 38, "L" },
			KeyName{ 39, "Semicolon" },
			KeyName{ 40, "Apostrophe" },
			KeyName{ 41, "~ (Console)" },
			KeyName{ 42, "Left Shift" },
			KeyName{ 43, "Back Slash" },
			KeyName{ 44, "Z" },
			KeyName{ 45, "X" },
			KeyName{ 46, "C" },
			KeyName{ 47, "V" },
			KeyName{ 48, "B" },
			KeyName{ 49, "N" },
			KeyName{ 50, "M" },
			KeyName{ 51, "Comma" },
			KeyName{ 52, "Period" },
			KeyName{ 53, "Forward Slash" },
			KeyName{ 54, "Right Shift" },
			KeyName{ 55, "NUM*" },
			KeyName{ 56, "Left Alt" },
			KeyName{ 57, "Spacebar" },
			KeyName{ 58, "Caps Lock" },
			KeyName{ 59, "F1" },
			KeyName{ 60, "F2" },
			KeyName{ 61, "F3" },
			KeyName{ 62, "F4" },
			KeyName{ 63, "F5" },
			KeyName{ 64, "F6" },
			KeyName{ 65, "F7" },
			KeyName{ 66, "F8" },
			KeyName{ 67, "F9" },
			KeyName{ 68, "F10" },
			KeyName{ 69, "Num Lock" },
			KeyName{ 70, "Scroll Lock" },
			KeyName{ 71, "NUM7" },
			KeyName{ 72, "NUM8" },
			KeyName{ 73, "NUM9" },
			KeyName{ 74, "NUM-" },
			KeyName{ 75, "NUM4" },
			KeyName{ 76, "NUM5" },
			KeyName{ 77, "NUM6" },
			KeyName{ 78, "NUM+" },
			KeyName{ 79, "NUM1" },
			KeyName{ 80, "NUM2" },
			KeyName{ 81, "NUM3" },
			KeyName{ 82, "NUM0" },
			KeyName{ 83, "NUM." },
			KeyName{ 86, "< > (ISO)" },
			KeyName{ 87, "F11" },
			KeyName{ 88, "F12" },
			KeyName{ 156, "NUM Enter" },
			KeyName{ 157, "Right Control" },
			KeyName{ 181, "NUM/" },
			KeyName{ 183, "SysRq / PtrScr" },
			KeyName{ 184, "Right Alt" },
			KeyName{ 197, "Pause" },
			KeyName{ 199, "Home" },
			KeyName{ 200, "Up Arrow" },
			KeyName{ 201, "PgUp" },
			KeyName{ 203, "Left Arrow" },
			KeyName{ 205, "Right Arrow" },
			KeyName{ 207, "End" },
			KeyName{ 208, "Down Arrow" },
			KeyName{ 209, "PgDown" },
			KeyName{ 210, "Insert" },
			KeyName{ 211, "Delete" },
			KeyName{ 219, "Left Windows" },
			KeyName{ 220, "Right Windows" },
			KeyName{ 221, "Menu" },
			KeyName{ 256, "Left Mouse Button" },
			KeyName{ 257, "Right Mouse Button" },
			KeyName{ 258, "Middle/Wheel Mouse Button" },
			KeyName{ 259, "Mouse Button 3" },
			KeyName{ 260, "Mouse Button 4" },
			KeyName{ 261, "Mouse Button 5" },
			KeyName{ 262, "Mouse Button 6" },
			KeyName{ 263, "Mouse Button 7" },
			KeyName{ 264, "Mouse Wheel Up" },
			KeyName{ 265, "Mouse Wheel Down" },
			KeyName{ 266, "D-Pad Up" },
			KeyName{ 267, "D-Pad Down" },
			KeyName{ 268, "D-Pad Left" },
			KeyName{ 269, "D-Pad Right" },
			KeyName{ 270, "Start / Options" },
			KeyName{ 271, "Back / Share" },
			KeyName{ 272, "LS / L3" },
			KeyName{ 273, "RS / R3" },
			KeyName{ 274, "LB / L1" },
			KeyName{ 275, "RB / R1" },
			KeyName{ 276, "A / Cross" },
			KeyName{ 277, "B / Circle" },
			KeyName{ 278, "X / Square" },
			KeyName{ 279, "Y / Triangle" },
			KeyName{ 280, "LT / L2" },
			KeyName{ 281, "RT / R2" },
			KeyName{ 282, "Left Stick Up" },
			KeyName{ 283, "Left Stick Down" },
			KeyName{ 284, "Left Stick Left" },
			KeyName{ 285, "Left Stick Right" }
		};

		consteval std::array<std::string_view, kCodeCount> BuildNameTable()
		{
			std::array<std::string_view, kCodeCount> table{};
			for (const auto& entry : kKeyNames) {
				table[entry.code] = entry.name;
			}
			return table;
		}

		constexpr auto kNameTable = BuildNameTable();

		std::atomic<State> g_state{ State::kIdle };
		std::atomic<std::uint32_t> g_code{ 0 };

		bool IsGamepad(std::uint32_t a_code)
		{
			return a_code >= kGamepadOffset && a_code < kCodeCount;
		}

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

		bool __stdcall OnInput(RE::InputEvent* a_event)
		{
			if (g_state.load(std::memory_order_acquire) != State::kWaiting) {
				return false;
			}
			const auto* button = a_event->AsButtonEvent();
			if (!button || !button->IsPressed()) {
				return false;
			}
			const auto code = ToCode(button->GetDevice(), button->GetIDCode());
			if (code == 0) {
				return false;
			}
			if (button->IsDown()) {
				auto expected = State::kWaiting;
				g_code.store(code, std::memory_order_relaxed);
				g_state.compare_exchange_strong(expected, State::kCaptured, std::memory_order_acq_rel);
			}
			return true;
		}

		void __stdcall OnMenuEvent(SKSEMenuFramework::Model::EventType a_type)
		{
			auto expected = State::kWaiting;
			if (a_type == SKSEMenuFramework::Model::EventType::kCloseMenu) {
				g_state.compare_exchange_strong(expected, State::kIdle, std::memory_order_acq_rel);
			}
		}
	}

	void Install()
	{
		SKSEMenuFramework::AddInputEvent(OnInput);
		SKSEMenuFramework::AddEvent(OnMenuEvent, 0.0f);
	}

	void Begin()
	{
		g_state.store(State::kWaiting, std::memory_order_release);
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

	bool IsPlayStation()
	{
		const auto* controlMap = RE::ControlMap::GetSingleton();
		return controlMap && controlMap->GetGamePadType() == RE::PC_GAMEPAD_TYPE::kOrbis;
	}

	std::string_view Name(std::uint32_t a_code, bool a_playStation)
	{
		if (a_playStation && a_code >= kGamepadOffset && a_code < kLeftStickOffset) {
			return kPlayStationNames[a_code - kGamepadOffset];
		}
		if (a_code < kCodeCount && !kNameTable[a_code].empty()) {
			return kNameTable[a_code];
		}
		return "Unknown Key"sv;
	}

	std::string_view Problem(std::uint32_t a_code, bool a_gamepad, bool a_playStation)
	{
		if (a_code == 0) {
			return {};
		}
		if (IsGamepad(a_code) != a_gamepad) {
			return "Wheeler reads this list from a different device. Remap it."sv;
		}
		if (a_playStation && std::ranges::contains(kPlayStationUnreachable, a_code)) {
			return "No PlayStation button triggers this binding in Wheeler. Remap it."sv;
		}
		return {};
	}
}
