#include "ModSettings.h"

#include <cmath>
#include <cstring>
#include <fstream>
#include <stack>

#include <Xinput.h>

#include "SimpleIni.h"
#include "logger.h"

#pragma comment(lib, "Xinput.lib")

namespace im = ImGuiMCP;

static RE::GameSettingCollection* gsc = nullptr;

bool ModSettings::entry_base::Control::Req::satisfied()
{
	bool val = false;
	if (type == kReqType_Checkbox) {
		auto it = ModSettings::m_checkbox_toggle.find(id);
		if (it == ModSettings::m_checkbox_toggle.end()) {
			return false;
		}
		val = it->second->value;
	} else if (type == kReqType_GameSetting) {
		if (!gsc) {
			gsc = RE::GameSettingCollection::GetSingleton();
			if (!gsc)
				return false;
		}
		auto setting = gsc->GetSetting(id.c_str());
		if (!setting)
			return false;
		val = setting->GetBool();
	}
	return this->_not ? !val : val;
}

bool ModSettings::entry_base::Control::satisfied()
{
	for (auto& req : reqs) {
		if (!req.satisfied()) {
			return false;
		}
	}
	return true;
}

using json = nlohmann::json;

void ModSettings::SendSettingsUpdateEvent(std::string& modName)
{
	auto eventSource = SKSE::GetModCallbackEventSource();
	if (!eventSource) {
		return;
	}
	SKSE::ModCallbackEvent callbackEvent;
	callbackEvent.eventName = "dmenu_updateSettings";
	callbackEvent.strArg = modName;
	eventSource->SendEvent(&callbackEvent);
}

void ModSettings::send_mod_callback_event(std::string& mod_name, std::string& str_arg)
{
	auto eventSource = SKSE::GetModCallbackEventSource();
	if (!eventSource) {
		return;
	}
	SKSE::ModCallbackEvent callbackEvent;
	callbackEvent.eventName = mod_name.data();
	callbackEvent.strArg = str_arg.data();
	eventSource->SendEvent(&callbackEvent);
}

std::string ModSettings::get_type_str(entry_type t)
{
	switch (t) {
	case entry_type::kEntryType_Checkbox:
		return "checkbox";
	case entry_type::kEntryType_Slider:
		return "slider";
	case entry_type::kEntryType_Textbox:
		return "textbox";
	case entry_type::kEntryType_Dropdown:
		return "dropdown";
	case entry_type::kEntryType_Text:
		return "text";
	case entry_type::kEntryType_Group:
		return "group";
	case entry_type::kEntryType_Color:
		return "color";
	case entry_type::kEntryType_Keymap:
		return "keymap";
	case entry_type::kEntryType_Button:
		return "button";
	default:
		return "invalid";
	}
}

static void HoverNote(const char* text)
{
	im::TextDisabled("(?)");
	if (im::IsItemHovered()) {
		im::SetTooltip("%s", text);
	}
}

static bool SliderFloatWithSteps(const char* label, float* v, float v_min, float v_max, float step, uint8_t precision)
{
	char fmt[16];
	std::snprintf(fmt, sizeof(fmt), "%%.%df", static_cast<int>(precision));
	float val = *v;
	im::SliderFloat(label, &val, v_min, v_max, fmt, 0);
	if (step > 0.0f) {
		val = v_min + step * std::round((val - v_min) / step);
	}
	if (val != *v) {
		*v = val;
		return true;
	}
	return false;
}

static bool s_captureArmed = false;
static WORD s_prevPadButtons = 0;
static bool s_prevLT = false;
static bool s_prevRT = false;

static void ResetGamepadCapture()
{
	XINPUT_STATE state{};
	if (XInputGetState(0, &state) == ERROR_SUCCESS) {
		s_prevPadButtons = state.Gamepad.wButtons;
		s_prevLT = state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
		s_prevRT = state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
	} else {
		s_prevPadButtons = 0;
		s_prevLT = false;
		s_prevRT = false;
	}
}

static int PollCapturedInput()
{
	struct KeyMap
	{
		int key;
		int dik;
	};
	static const KeyMap keys[] = {
		{ im::ImGuiKey_Escape, 1 }, { im::ImGuiKey_1, 2 }, { im::ImGuiKey_2, 3 }, { im::ImGuiKey_3, 4 },
		{ im::ImGuiKey_4, 5 }, { im::ImGuiKey_5, 6 }, { im::ImGuiKey_6, 7 }, { im::ImGuiKey_7, 8 },
		{ im::ImGuiKey_8, 9 }, { im::ImGuiKey_9, 10 }, { im::ImGuiKey_0, 11 }, { im::ImGuiKey_Minus, 12 },
		{ im::ImGuiKey_Equal, 13 }, { im::ImGuiKey_Backspace, 14 }, { im::ImGuiKey_Tab, 15 },
		{ im::ImGuiKey_Q, 16 }, { im::ImGuiKey_W, 17 }, { im::ImGuiKey_E, 18 }, { im::ImGuiKey_R, 19 },
		{ im::ImGuiKey_T, 20 }, { im::ImGuiKey_Y, 21 }, { im::ImGuiKey_U, 22 }, { im::ImGuiKey_I, 23 },
		{ im::ImGuiKey_O, 24 }, { im::ImGuiKey_P, 25 }, { im::ImGuiKey_LeftBracket, 26 }, { im::ImGuiKey_RightBracket, 27 },
		{ im::ImGuiKey_Enter, 28 }, { im::ImGuiKey_LeftCtrl, 29 }, { im::ImGuiKey_A, 30 }, { im::ImGuiKey_S, 31 },
		{ im::ImGuiKey_D, 32 }, { im::ImGuiKey_F, 33 }, { im::ImGuiKey_G, 34 }, { im::ImGuiKey_H, 35 },
		{ im::ImGuiKey_J, 36 }, { im::ImGuiKey_K, 37 }, { im::ImGuiKey_L, 38 }, { im::ImGuiKey_Semicolon, 39 },
		{ im::ImGuiKey_Apostrophe, 40 }, { im::ImGuiKey_GraveAccent, 41 }, { im::ImGuiKey_LeftShift, 42 }, { im::ImGuiKey_Backslash, 43 },
		{ im::ImGuiKey_Z, 44 }, { im::ImGuiKey_X, 45 }, { im::ImGuiKey_C, 46 }, { im::ImGuiKey_V, 47 },
		{ im::ImGuiKey_B, 48 }, { im::ImGuiKey_N, 49 }, { im::ImGuiKey_M, 50 }, { im::ImGuiKey_Comma, 51 },
		{ im::ImGuiKey_Period, 52 }, { im::ImGuiKey_Slash, 53 }, { im::ImGuiKey_RightShift, 54 }, { im::ImGuiKey_KeypadMultiply, 55 },
		{ im::ImGuiKey_LeftAlt, 56 }, { im::ImGuiKey_Space, 57 }, { im::ImGuiKey_CapsLock, 58 },
		{ im::ImGuiKey_F1, 59 }, { im::ImGuiKey_F2, 60 }, { im::ImGuiKey_F3, 61 }, { im::ImGuiKey_F4, 62 },
		{ im::ImGuiKey_F5, 63 }, { im::ImGuiKey_F6, 64 }, { im::ImGuiKey_F7, 65 }, { im::ImGuiKey_F8, 66 },
		{ im::ImGuiKey_F9, 67 }, { im::ImGuiKey_F10, 68 }, { im::ImGuiKey_NumLock, 69 }, { im::ImGuiKey_ScrollLock, 70 },
		{ im::ImGuiKey_Keypad7, 71 }, { im::ImGuiKey_Keypad8, 72 }, { im::ImGuiKey_Keypad9, 73 }, { im::ImGuiKey_KeypadSubtract, 74 },
		{ im::ImGuiKey_Keypad4, 75 }, { im::ImGuiKey_Keypad5, 76 }, { im::ImGuiKey_Keypad6, 77 }, { im::ImGuiKey_KeypadAdd, 78 },
		{ im::ImGuiKey_Keypad1, 79 }, { im::ImGuiKey_Keypad2, 80 }, { im::ImGuiKey_Keypad3, 81 }, { im::ImGuiKey_Keypad0, 82 },
		{ im::ImGuiKey_KeypadDecimal, 83 }, { im::ImGuiKey_F11, 87 }, { im::ImGuiKey_F12, 88 }, { im::ImGuiKey_KeypadEnter, 156 },
		{ im::ImGuiKey_RightCtrl, 157 }, { im::ImGuiKey_KeypadDivide, 181 }, { im::ImGuiKey_PrintScreen, 183 }, { im::ImGuiKey_RightAlt, 184 },
		{ im::ImGuiKey_Pause, 197 }, { im::ImGuiKey_Home, 199 }, { im::ImGuiKey_UpArrow, 200 }, { im::ImGuiKey_PageUp, 201 },
		{ im::ImGuiKey_LeftArrow, 203 }, { im::ImGuiKey_RightArrow, 205 }, { im::ImGuiKey_End, 207 }, { im::ImGuiKey_DownArrow, 208 },
		{ im::ImGuiKey_PageDown, 209 }, { im::ImGuiKey_Insert, 210 }, { im::ImGuiKey_Delete, 211 }
	};
	for (auto& m : keys) {
		if (im::IsKeyPressed(static_cast<ImGuiMCP::ImGuiKey>(m.key), false)) {
			return m.dik;
		}
	}
	if (im::IsMouseClicked(ImGuiMCP::ImGuiMouseButton_Left, false)) {
		return 256;
	}
	if (im::IsMouseClicked(ImGuiMCP::ImGuiMouseButton_Right, false)) {
		return 257;
	}
	if (im::IsMouseClicked(ImGuiMCP::ImGuiMouseButton_Middle, false)) {
		return 258;
	}
	if (im::IsMouseClicked(3, false)) {
		return 259;
	}
	if (im::IsMouseClicked(4, false)) {
		return 260;
	}
	auto io = im::GetIO();
	if (io) {
		if (io->MouseWheel > 0.0f) {
			return 264;
		}
		if (io->MouseWheel < 0.0f) {
			return 265;
		}
	}

	XINPUT_STATE state{};
	if (XInputGetState(0, &state) == ERROR_SUCCESS) {
		struct PadMap
		{
			WORD mask;
			int code;
		};
		static const PadMap pads[] = {
			{ XINPUT_GAMEPAD_DPAD_UP, 266 }, { XINPUT_GAMEPAD_DPAD_DOWN, 267 },
			{ XINPUT_GAMEPAD_DPAD_LEFT, 268 }, { XINPUT_GAMEPAD_DPAD_RIGHT, 269 },
			{ XINPUT_GAMEPAD_START, 270 }, { XINPUT_GAMEPAD_BACK, 271 },
			{ XINPUT_GAMEPAD_LEFT_THUMB, 272 }, { XINPUT_GAMEPAD_RIGHT_THUMB, 273 },
			{ XINPUT_GAMEPAD_LEFT_SHOULDER, 274 }, { XINPUT_GAMEPAD_RIGHT_SHOULDER, 275 },
			{ XINPUT_GAMEPAD_A, 276 }, { XINPUT_GAMEPAD_B, 277 },
			{ XINPUT_GAMEPAD_X, 278 }, { XINPUT_GAMEPAD_Y, 279 }
		};
		WORD buttons = state.Gamepad.wButtons;
		bool lt = state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
		bool rt = state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
		int result = -1;
		for (auto& p : pads) {
			if ((buttons & p.mask) && !(s_prevPadButtons & p.mask)) {
				result = p.code;
			}
		}
		if (lt && !s_prevLT) {
			result = 280;
		}
		if (rt && !s_prevRT) {
			result = 281;
		}
		s_prevPadButtons = buttons;
		s_prevLT = lt;
		s_prevRT = rt;
		if (result >= 0) {
			return result;
		}
	}
	return -1;
}

void ModSettings::show_saveButton()
{
	bool unsaved_changes = !ini_dirty_mods.empty();

	if (unsaved_changes) {
		im::PushStyleColor(ImGuiMCP::ImGuiCol_Button, ImGuiMCP::ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
		im::PushStyleColor(ImGuiMCP::ImGuiCol_ButtonHovered, ImGuiMCP::ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
		im::PushStyleColor(ImGuiMCP::ImGuiCol_ButtonActive, ImGuiMCP::ImVec4(0.4f, 0.9f, 0.5f, 1.0f));
	}

	if (im::Button("Save Changes")) {
		for (auto& mod : ini_dirty_mods) {
			flush_ini(mod);
			for (auto callback : mod->callbacks) {
				callback();
			}
			SendSettingsUpdateEvent(mod->name);
		}
		ini_dirty_mods.clear();
	}

	if (unsaved_changes) {
		im::PopStyleColor(3);
	}
}

void ModSettings::show_cancelButton()
{
	if (im::Button("Revert")) {
		for (auto& mod : ini_dirty_mods) {
			load_ini(mod);
		}
		ini_dirty_mods.clear();
	}
}

void ModSettings::show_entry(entry_base* entry, mod_setting* mod)
{
	im::PushID(entry);
	bool edited = false;

	bool available = entry->control.satisfied();
	if (!available && entry->control.failAction == entry_base::Control::FailAction::kFailAction_Hide) {
		im::PopID();
		return;
	}
	if (!available) {
		im::BeginDisabled();
	}

	im::ImVec2 avail;
	im::GetContentRegionAvail(&avail);
	float width = avail.x * 0.5f;

	switch (entry->type) {
	case kEntryType_Checkbox:
		{
			setting_checkbox* checkbox = dynamic_cast<setting_checkbox*>(entry);
			if (im::Checkbox(checkbox->name.get(), &checkbox->value)) {
				edited = true;
			}
			if (im::IsItemHovered() && im::IsKeyPressed(ImGuiMCP::ImGuiKey_R)) {
				edited |= checkbox->reset();
			}
			if (!checkbox->desc.empty()) {
				im::SameLine();
				HoverNote(checkbox->desc.get());
			}
		}
		break;

	case kEntryType_Slider:
		{
			setting_slider* slider = dynamic_cast<setting_slider*>(entry);
			im::SetNextItemWidth(width);
			if (SliderFloatWithSteps(slider->name.get(), &slider->value, slider->min, slider->max, slider->step, slider->precision)) {
				edited = true;
			}
			if (im::IsItemHovered()) {
				if (im::IsKeyPressed(ImGuiMCP::ImGuiKey_LeftArrow) && slider->value > slider->min) {
					slider->value -= slider->step;
					edited = true;
				}
				if (im::IsKeyPressed(ImGuiMCP::ImGuiKey_RightArrow) && slider->value < slider->max) {
					slider->value += slider->step;
					edited = true;
				}
				if (im::IsKeyPressed(ImGuiMCP::ImGuiKey_R)) {
					edited |= slider->reset();
				}
			}
			if (!slider->desc.empty()) {
				im::SameLine();
				HoverNote(slider->desc.get());
			}
		}
		break;

	case kEntryType_Textbox:
		{
			setting_textbox* textbox = dynamic_cast<setting_textbox*>(entry);
			char buf[256];
			std::strncpy(buf, textbox->value.c_str(), sizeof(buf) - 1);
			buf[sizeof(buf) - 1] = '\0';
			im::SetNextItemWidth(width);
			if (im::InputText(textbox->name.get(), buf, sizeof(buf))) {
				textbox->value = buf;
				edited = true;
			}
			if (im::IsItemHovered() && im::IsKeyPressed(ImGuiMCP::ImGuiKey_R)) {
				edited |= textbox->reset();
			}
			if (!textbox->desc.empty()) {
				im::SameLine();
				HoverNote(textbox->desc.get());
			}
		}
		break;

	case kEntryType_Dropdown:
		{
			setting_dropdown* dropdown = dynamic_cast<setting_dropdown*>(entry);
			int selected = dropdown->value;
			const char* preview_value = "";
			if (selected >= 0 && selected < static_cast<int>(dropdown->options.size())) {
				preview_value = dropdown->options[selected].c_str();
			}
			im::SetNextItemWidth(width);
			if (im::BeginCombo(dropdown->name.get(), preview_value)) {
				for (int i = 0; i < static_cast<int>(dropdown->options.size()); i++) {
					bool is_selected = (selected == i);
					if (im::Selectable(dropdown->options[i].c_str(), is_selected)) {
						dropdown->value = i;
						edited = true;
					}
					if (is_selected) {
						im::SetItemDefaultFocus();
					}
				}
				im::EndCombo();
			}
			if (im::IsItemHovered() && im::IsKeyPressed(ImGuiMCP::ImGuiKey_R)) {
				edited |= dropdown->reset();
			}
			if (!dropdown->desc.empty()) {
				im::SameLine();
				HoverNote(dropdown->desc.get());
			}
		}
		break;

	case kEntryType_Text:
		{
			entry_text* t = dynamic_cast<entry_text*>(entry);
			im::TextColored(t->_color, "%s", t->name.get());
		}
		break;

	case kEntryType_Group:
		{
			entry_group* g = dynamic_cast<entry_group*>(entry);
			im::PushStyleColor(ImGuiMCP::ImGuiCol_Header, ImGuiMCP::ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			if (im::CollapsingHeader(g->name.get())) {
				if (im::IsItemHovered() && !g->desc.empty()) {
					im::SetTooltip("%s", g->desc.get());
				}
				im::PopStyleColor();
				show_entries(g->entries, mod);
			} else {
				im::PopStyleColor();
			}
		}
		break;

	case kEntryType_Keymap:
		{
			setting_keymap* k = dynamic_cast<setting_keymap*>(entry);
			std::string popupId = "Remap Key###" + std::to_string(reinterpret_cast<uintptr_t>(k));
			if (im::Button("Remap")) {
				im::OpenPopup(popupId.data());
				keyMapListening = k;
				s_captureArmed = false;
				ResetGamepadCapture();
			}
			im::SameLine();
			if (im::Button("Unmap")) {
				k->value = 0;
				edited = true;
			}
			im::SameLine();
			im::Text("%s:", k->name.get());
			im::SameLine();
			im::Text("%s", setting_keymap::keyid_to_str(k->value));

			if (!k->desc.empty()) {
				im::SameLine();
				HoverNote(k->desc.get());
			}

			if (im::BeginPopupModal(popupId.data())) {
				im::Text("Press any key, mouse button, wheel, or controller button to bind.");
				im::Text("(Esc binds Escape.)");
				if (keyMapListening == k) {
					if (!s_captureArmed) {
						s_captureArmed = true;
					} else {
						int code = PollCapturedInput();
						if (code >= 0) {
							k->value = code;
							keyMapListening = nullptr;
							edited = true;
						}
					}
				}
				if (keyMapListening != k) {
					im::CloseCurrentPopup();
				}
				im::EndPopup();
			}
		}
		break;

	case kEntryType_Color:
		{
			setting_color* color = dynamic_cast<setting_color*>(entry);
			im::SetNextItemWidth(width);
			float colorArray[4] = { color->color.x, color->color.y, color->color.z, color->color.w };
			if (im::ColorEdit4(color->name.get(), colorArray)) {
				color->color = ImGuiMCP::ImVec4(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
				edited = true;
			}
			if (im::IsItemHovered() && im::IsKeyPressed(ImGuiMCP::ImGuiKey_R)) {
				edited |= color->reset();
			}
			if (!color->desc.empty()) {
				im::SameLine();
				HoverNote(color->desc.get());
			}
		}
		break;

	case kEntryType_Button:
		{
			entry_button* b = dynamic_cast<entry_button*>(entry);
			if (im::Button(b->name.get())) {
				std::string custom_event_name = "dmenu_buttonCallback";
				send_mod_callback_event(custom_event_name, b->id);
			}
			if (!b->desc.empty()) {
				im::SameLine();
				HoverNote(b->desc.get());
			}
		}
		break;

	default:
		break;
	}

	if (!available) {
		im::EndDisabled();
	}
	if (edited) {
		ini_dirty_mods.insert(mod);
	}
	im::PopID();
}

void ModSettings::show_entries(std::vector<entry_base*>& entries, mod_setting* mod)
{
	im::PushID(&entries);
	for (auto& entry : entries) {
		im::PushID(entry);
		im::Indent();
		show_entry(entry, mod);
		im::Unindent();
		im::PopID();
	}
	im::PopID();
}

void ModSettings::show_modSetting(mod_setting* mod)
{
	show_entries(mod->entries, mod);
}

void ModSettings::show()
{
	show_saveButton();
	im::SameLine();
	show_cancelButton();
	im::Separator();

	const float padding = 8.0f;
	const float spacing = 8.0f;
	im::PushStyleVar(ImGuiMCP::ImGuiStyleVar_WindowPadding, ImGuiMCP::ImVec2(padding, padding));
	im::PushStyleVar(ImGuiMCP::ImGuiStyleVar_ItemSpacing, ImGuiMCP::ImVec2(spacing, spacing));

	im::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	for (auto& mod : mods) {
		if (im::CollapsingHeader(mod->name.c_str())) {
			show_modSetting(mod);
		}
	}
	im::PopStyleColor();
	im::PopStyleVar(2);
}

static const std::string SETTINGS_DIR = "Data\\SKSE\\Plugins\\dmenu\\customSettings";

void ModSettings::init()
{
	try {
		logger::info("Loading .json configurations...");
		std::error_code ec;
		if (!std::filesystem::exists(SETTINGS_DIR, ec)) {
			logger::warn("Settings directory {} not found; no configs loaded.", SETTINGS_DIR);
			return;
		}
		for (auto& file : std::filesystem::directory_iterator(SETTINGS_DIR)) {
			if (file.path().extension() == ".json" && file.path().stem() == "Wheeler Controls") {
				load_json(file.path());
			}
		}

		logger::info("Loading .ini config serializations...");
		for (auto& mod : mods) {
			load_ini(mod);
		}
		logger::info("Mod settings initialized");
	} catch (const std::exception& e) {
		logger::error("ModSettings::init aborted: {}", e.what());
	}
}

ModSettings::entry_base* ModSettings::load_json_non_group(nlohmann::json& json)
{
	entry_base* e = nullptr;
	std::string type_str = json["type"].get<std::string>();
	if (type_str == "checkbox") {
		setting_checkbox* scb = new setting_checkbox();
		scb->value = json.contains("default") ? json["default"].get<bool>() : false;
		scb->default_value = scb->value;
		if (json.contains("control")) {
			if (json["control"].contains("id")) {
				scb->control_id = json["control"]["id"].get<std::string>();
				m_checkbox_toggle[scb->control_id] = scb;
			}
		}
		e = scb;
	} else if (type_str == "slider") {
		setting_slider* ssl = new setting_slider();
		ssl->value = json.contains("default") ? json["default"].get<float>() : 0;
		ssl->min = json["style"]["min"].get<float>();
		ssl->max = json["style"]["max"].get<float>();
		ssl->step = json["style"]["step"].get<float>();
		ssl->default_value = ssl->value;
		e = ssl;
	} else if (type_str == "textbox") {
		setting_textbox* stb = new setting_textbox();
		stb->value = json.contains("default") ? json["default"].get<std::string>() : "";
		stb->default_value = stb->value;
		e = stb;
	} else if (type_str == "dropdown") {
		setting_dropdown* sdd = new setting_dropdown();
		sdd->value = json.contains("default") ? json["default"].get<int>() : 0;
		for (auto& option_json : json["options"]) {
			sdd->options.push_back(option_json.get<std::string>());
		}
		sdd->default_value = sdd->value;
		e = sdd;
	} else if (type_str == "text") {
		entry_text* et = new entry_text();
		if (json.contains("style") && json["style"].contains("color")) {
			et->_color = ImGuiMCP::ImVec4(json["style"]["color"]["r"].get<float>(), json["style"]["color"]["g"].get<float>(), json["style"]["color"]["b"].get<float>(), json["style"]["color"]["a"].get<float>());
		}
		e = et;
	} else if (type_str == "color") {
		setting_color* sc = new setting_color();
		sc->default_color = ImGuiMCP::ImVec4(json["default"]["r"].get<float>(), json["default"]["g"].get<float>(), json["default"]["b"].get<float>(), json["default"]["a"].get<float>());
		sc->color = sc->default_color;
		e = sc;
	} else if (type_str == "keymap") {
		setting_keymap* skm = new setting_keymap();
		skm->default_value = json["default"].get<int>();
		skm->value = skm->default_value;
		e = skm;
	} else if (type_str == "button") {
		entry_button* sb = new entry_button();
		sb->id = json["id"].get<std::string>();
		e = sb;
	} else {
		logger::info("Unknown setting type: {}", type_str);
		return nullptr;
	}

	if (e->is_setting()) {
		setting_base* s = dynamic_cast<setting_base*>(e);
		s->ini_section = json["ini"]["section"].get<std::string>();
		s->ini_id = json["ini"]["id"].get<std::string>();
	}
	return e;
}

ModSettings::entry_group* ModSettings::load_json_group(nlohmann::json& group_json)
{
	entry_group* group = new entry_group();
	for (auto& entry_json : group_json["entries"]) {
		entry_base* entry = load_json_entry(entry_json);
		if (entry) {
			group->entries.push_back(entry);
		}
	}
	return group;
}

ModSettings::entry_base* ModSettings::load_json_entry(nlohmann::json& entry_json)
{
	ModSettings::entry_base* entry = nullptr;
	if (entry_json["type"].get<std::string>() == "group") {
		entry = load_json_group(entry_json);
	} else {
		entry = load_json_non_group(entry_json);
	}
	if (entry == nullptr) {
		logger::info("ERROR: Failed to load json entry.");
		return nullptr;
	}

	entry->name.def = entry_json["text"]["name"].get<std::string>();
	if (entry_json["text"].contains("desc")) {
		entry->desc.def = entry_json["text"]["desc"].get<std::string>();
	}

	if (entry_json.contains("translation")) {
		if (entry_json["translation"].contains("name")) {
			entry->name.key = entry_json["translation"]["name"].get<std::string>();
		}
		if (entry_json["translation"].contains("desc")) {
			entry->desc.key = entry_json["translation"]["desc"].get<std::string>();
		}
	}
	entry->control.failAction = entry_base::Control::kFailAction_Disable;

	if (entry_json.contains("control")) {
		if (entry_json["control"].contains("requirements")) {
			for (auto& req_json : entry_json["control"]["requirements"]) {
				entry_base::Control::Req req;
				req.id = req_json["id"].get<std::string>();
				req._not = !req_json["value"].get<bool>();
				std::string req_type = req_json["type"].get<std::string>();
				if (req_type == "checkbox") {
					req.type = entry_base::Control::Req::ReqType::kReqType_Checkbox;
				} else if (req_type == "gameSetting") {
					req.type = entry_base::Control::Req::ReqType::kReqType_GameSetting;
				} else {
					logger::info("Error: unknown requirement type: {}", req_type);
					continue;
				}
				entry->control.reqs.push_back(req);
			}
		}
		if (entry_json["control"].contains("failAction")) {
			std::string failAction = entry_json["control"]["failAction"].get<std::string>();
			if (failAction == "disable") {
				entry->control.failAction = entry_base::Control::kFailAction_Disable;
			} else if (failAction == "hide") {
				entry->control.failAction = entry_base::Control::kFailAction_Hide;
			}
		}
	}

	return entry;
}

void ModSettings::load_json(std::filesystem::path path)
{
	std::string mod_path = path.string();
	std::ifstream json_file(mod_path);
	if (!json_file.is_open()) {
		return;
	}

	nlohmann::json mod_json;
	try {
		json_file >> mod_json;
	} catch (const nlohmann::json::exception&) {
		return;
	}
	mod_setting* mod = new mod_setting();

	mod->name = path.stem().string();
	mod->json_path = SETTINGS_DIR + "\\" + path.filename().string();
	try {
		if (mod_json.contains("ini")) {
			mod->ini_path = mod_json["ini"].get<std::string>();
		} else {
			mod->ini_path = SETTINGS_DIR + "\\ini\\" + mod->name.data() + ".ini";
		}

		for (auto& entry_json : mod_json["data"]) {
			entry_base* entry = load_json_entry(entry_json);
			if (entry) {
				mod->entries.push_back(entry);
			}
		}
		mods.push_back(mod);
	} catch (const nlohmann::json::exception& e) {
		logger::info("Exception parsing {} : {}", mod_path, e.what());
		return;
	}
	logger::info("Loaded mod {}", mod->name);
}

void ModSettings::get_all_settings(mod_setting* mod, std::vector<ModSettings::setting_base*>& r_vec)
{
	std::stack<entry_group*> group_stack;
	for (auto& entry : mod->entries) {
		if (entry->is_group()) {
			group_stack.push(dynamic_cast<entry_group*>(entry));
		} else if (entry->is_setting()) {
			r_vec.push_back(dynamic_cast<setting_base*>(entry));
		}
	}
	while (!group_stack.empty()) {
		auto group = group_stack.top();
		group_stack.pop();
		for (auto& entry : group->entries) {
			if (entry->is_group()) {
				group_stack.push(dynamic_cast<entry_group*>(entry));
			} else if (entry->is_setting()) {
				r_vec.push_back(dynamic_cast<setting_base*>(entry));
			}
		}
	}
}

void ModSettings::load_ini(mod_setting* mod)
{
	logger::info("loading .ini for {}", mod->name);
	CSimpleIniA ini;
	ini.SetUnicode();
	SI_Error rc = ini.LoadFile(mod->ini_path.c_str());
	if (rc != SI_OK) {
		logger::info(".ini file for {} not found. Creating a new .ini file.", mod->name);
		flush_ini(mod);
		return;
	}

	std::vector<ModSettings::setting_base*> settings;
	get_all_settings(mod, settings);
	for (auto& setting_ptr : settings) {
		if (setting_ptr->ini_id.empty() || setting_ptr->ini_section.empty()) {
			logger::error("Undefined .ini serialization for setting {}; failed to load value.", setting_ptr->name.def);
			continue;
		}
		std::string value;
		bool use_default = false;
		if (ini.KeyExists(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str())) {
			value = ini.GetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), "");
		} else {
			use_default = true;
		}
		if (use_default) {
			if (setting_ptr->type == kEntryType_Checkbox) {
				dynamic_cast<setting_checkbox*>(setting_ptr)->value = dynamic_cast<setting_checkbox*>(setting_ptr)->default_value;
			} else if (setting_ptr->type == kEntryType_Slider) {
				dynamic_cast<setting_slider*>(setting_ptr)->value = dynamic_cast<setting_slider*>(setting_ptr)->default_value;
			} else if (setting_ptr->type == kEntryType_Textbox) {
				dynamic_cast<setting_textbox*>(setting_ptr)->value = dynamic_cast<setting_textbox*>(setting_ptr)->default_value;
			} else if (setting_ptr->type == kEntryType_Dropdown) {
				dynamic_cast<setting_dropdown*>(setting_ptr)->value = dynamic_cast<setting_dropdown*>(setting_ptr)->default_value;
			} else if (setting_ptr->type == kEntryType_Color) {
				dynamic_cast<setting_color*>(setting_ptr)->color = dynamic_cast<setting_color*>(setting_ptr)->default_color;
			} else if (setting_ptr->type == kEntryType_Keymap) {
				dynamic_cast<setting_keymap*>(setting_ptr)->value = dynamic_cast<setting_keymap*>(setting_ptr)->default_value;
			}
		} else {
			try {
				if (setting_ptr->type == kEntryType_Checkbox) {
					dynamic_cast<setting_checkbox*>(setting_ptr)->value = (value == "true");
				} else if (setting_ptr->type == kEntryType_Slider) {
					dynamic_cast<setting_slider*>(setting_ptr)->value = std::stof(value);
				} else if (setting_ptr->type == kEntryType_Textbox) {
					dynamic_cast<setting_textbox*>(setting_ptr)->value = value;
				} else if (setting_ptr->type == kEntryType_Dropdown) {
					dynamic_cast<setting_dropdown*>(setting_ptr)->value = std::stoi(value);
				} else if (setting_ptr->type == kEntryType_Color) {
					uint32_t colUInt = std::stoul(value);
					setting_color* sc = dynamic_cast<setting_color*>(setting_ptr);
					sc->color.x = ((colUInt >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f;
					sc->color.y = ((colUInt >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f;
					sc->color.z = ((colUInt >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f;
					sc->color.w = ((colUInt >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
				} else if (setting_ptr->type == kEntryType_Keymap) {
					dynamic_cast<setting_keymap*>(setting_ptr)->value = std::stoi(value);
				}
			} catch (const std::exception& e) {
				logger::warn("Invalid .ini value \"{}\" for setting {}; keeping default. ({})", value, setting_ptr->name.def, e.what());
			}
		}
	}
	logger::info(".ini loaded.");
}

void ModSettings::flush_ini(mod_setting* mod)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	std::vector<ModSettings::setting_base*> settings;
	get_all_settings(mod, settings);
	for (auto& setting : settings) {
		if (setting->ini_id.empty() || setting->ini_section.empty()) {
			logger::error("Undefined .ini serialization for setting {}; failed to save value.", setting->name.def);
			continue;
		}
		std::string value;
		if (setting->type == kEntryType_Checkbox) {
			value = dynamic_cast<setting_checkbox*>(setting)->value ? "true" : "false";
		} else if (setting->type == kEntryType_Slider) {
			value = std::to_string(dynamic_cast<setting_slider*>(setting)->value);
		} else if (setting->type == kEntryType_Textbox) {
			value = dynamic_cast<setting_textbox*>(setting)->value;
		} else if (setting->type == kEntryType_Dropdown) {
			value = std::to_string(dynamic_cast<setting_dropdown*>(setting)->value);
		} else if (setting->type == kEntryType_Color) {
			auto sc = dynamic_cast<setting_color*>(setting);
			uint32_t r = static_cast<uint32_t>(sc->color.x * 255.0f);
			uint32_t g = static_cast<uint32_t>(sc->color.y * 255.0f);
			uint32_t b = static_cast<uint32_t>(sc->color.z * 255.0f);
			uint32_t a = static_cast<uint32_t>(sc->color.w * 255.0f);
			std::uint32_t col = IM_COL32(r, g, b, a);
			value = std::to_string(col);
		} else if (setting->type == kEntryType_Keymap) {
			value = std::to_string(dynamic_cast<setting_keymap*>(setting)->value);
		}
		ini.SetValue(setting->ini_section.c_str(), setting->ini_id.c_str(), value.c_str());
	}

	ini.SaveFile(mod->ini_path.c_str());
}

const char* ModSettings::setting_keymap::keyid_to_str(int key_id)
{
	switch (key_id) {
	case 0: return "Unmapped";
	case 1: return "Escape";
	case 2: return "1";
	case 3: return "2";
	case 4: return "3";
	case 5: return "4";
	case 6: return "5";
	case 7: return "6";
	case 8: return "7";
	case 9: return "8";
	case 10: return "9";
	case 11: return "0";
	case 12: return "Minus";
	case 13: return "Equals";
	case 14: return "Backspace";
	case 15: return "Tab";
	case 16: return "Q";
	case 17: return "W";
	case 18: return "E";
	case 19: return "R";
	case 20: return "T";
	case 21: return "Y";
	case 22: return "U";
	case 23: return "I";
	case 24: return "O";
	case 25: return "P";
	case 26: return "Left Bracket";
	case 27: return "Right Bracket";
	case 28: return "Enter";
	case 29: return "Left Control";
	case 30: return "A";
	case 31: return "S";
	case 32: return "D";
	case 33: return "F";
	case 34: return "G";
	case 35: return "H";
	case 36: return "J";
	case 37: return "K";
	case 38: return "L";
	case 39: return "Semicolon";
	case 40: return "Apostrophe";
	case 41: return "~ (Console)";
	case 42: return "Left Shift";
	case 43: return "Back Slash";
	case 44: return "Z";
	case 45: return "X";
	case 46: return "C";
	case 47: return "V";
	case 48: return "B";
	case 49: return "N";
	case 50: return "M";
	case 51: return "Comma";
	case 52: return "Period";
	case 53: return "Forward Slash";
	case 54: return "Right Shift";
	case 55: return "NUM*";
	case 56: return "Left Alt";
	case 57: return "Spacebar";
	case 58: return "Caps Lock";
	case 59: return "F1";
	case 60: return "F2";
	case 61: return "F3";
	case 62: return "F4";
	case 63: return "F5";
	case 64: return "F6";
	case 65: return "F7";
	case 66: return "F8";
	case 67: return "F9";
	case 68: return "F10";
	case 69: return "Num Lock";
	case 70: return "Scroll Lock";
	case 71: return "NUM7";
	case 72: return "NUM8";
	case 73: return "NUM9";
	case 74: return "NUM-";
	case 75: return "NUM4";
	case 76: return "NUM5";
	case 77: return "NUM6";
	case 78: return "NUM+";
	case 79: return "NUM1";
	case 80: return "NUM2";
	case 81: return "NUM3";
	case 82: return "NUM0";
	case 83: return "NUM.";
	case 87: return "F11";
	case 88: return "F12";
	case 156: return "NUM Enter";
	case 157: return "Right Control";
	case 181: return "NUM/";
	case 183: return "SysRq / PtrScr";
	case 184: return "Right Alt";
	case 197: return "Pause";
	case 199: return "Home";
	case 200: return "Up Arrow";
	case 201: return "PgUp";
	case 203: return "Left Arrow";
	case 205: return "Right Arrow";
	case 207: return "End";
	case 208: return "Down Arrow";
	case 209: return "PgDown";
	case 210: return "Insert";
	case 211: return "Delete";
	case 256: return "Left Mouse Button";
	case 257: return "Right Mouse Button";
	case 258: return "Middle/Wheel Mouse Button";
	case 259: return "Mouse Button 3";
	case 260: return "Mouse Button 4";
	case 261: return "Mouse Button 5";
	case 262: return "Mouse Button 6";
	case 263: return "Mouse Button 7";
	case 264: return "Mouse Wheel Up";
	case 265: return "Mouse Wheel Down";
	case 266: return "DPAD_UP";
	case 267: return "DPAD_DOWN";
	case 268: return "DPAD_LEFT";
	case 269: return "DPAD_RIGHT";
	case 270: return "START";
	case 271: return "BACK";
	case 272: return "LEFT_THUMB";
	case 273: return "RIGHT_THUMB";
	case 274: return "LEFT_SHOULDER";
	case 275: return "RIGHT_SHOULDER";
	case 276: return "A";
	case 277: return "B";
	case 278: return "X";
	case 279: return "Y";
	case 280: return "LT";
	case 281: return "RT";
	default: return "Unknown Key";
	}
}
