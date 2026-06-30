#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SKSEMCP/SKSEMenuFramework.hpp"
#include "nlohmann/json.hpp"

struct Translatable
{
	std::string def;
	std::string key;

	Translatable() = default;
	Translatable(std::string a_def) :
		def(std::move(a_def)) {}

	const char* get() const { return def.c_str(); }
	bool empty() const { return def.empty(); }
};

class ModSettings
{
public:
	class setting_base;
	class setting_checkbox;
	class setting_slider;
	class setting_keymap;

private:
	static inline std::unordered_map<std::string, setting_checkbox*> m_checkbox_toggle;

public:
	static inline setting_keymap* keyMapListening = nullptr;

	enum entry_type
	{
		kEntryType_Checkbox,
		kEntryType_Slider,
		kEntryType_Textbox,
		kEntryType_Dropdown,
		kEntryType_Text,
		kEntryType_Group,
		kEntryType_Keymap,
		kEntryType_Color,
		kEntryType_Button,
		kSettingType_Invalid
	};

	static std::string get_type_str(entry_type t);

	class entry_base
	{
	public:
		class Control
		{
		public:
			class Req
			{
			public:
				enum ReqType
				{
					kReqType_Checkbox,
					kReqType_GameSetting
				};
				ReqType type;
				std::string id;
				bool _not = false;
				bool satisfied();
				Req()
				{
					id = "New Requirement";
					type = kReqType_Checkbox;
				}
			};
			enum FailAction
			{
				kFailAction_Disable,
				kFailAction_Hide,
			};
			FailAction failAction;
			std::vector<Req> reqs;
			bool satisfied();
		};

		entry_type type;
		Translatable name;
		Translatable desc;
		Control control;
		virtual bool is_setting() const { return false; }
		virtual ~entry_base() = default;
		virtual bool is_group() const { return false; }
	};

	class entry_text : public entry_base
	{
	public:
		ImGuiMCP::ImVec4 _color;

		entry_text()
		{
			type = kEntryType_Text;
			name = Translatable("New Text");
			_color = ImGuiMCP::ImVec4(1, 1, 1, 1);
		}
	};

	class entry_group : public entry_base
	{
	public:
		std::vector<entry_base*> entries;

		entry_group()
		{
			type = kEntryType_Group;
			name = Translatable("New Group");
		}

		bool is_group() const override { return true; }
	};

	class setting_base : public entry_base
	{
	public:
		std::string ini_section;
		std::string ini_id;

		bool is_setting() const override { return true; }
		virtual ~setting_base() = default;
		virtual bool reset() { return false; };
	};

	class setting_checkbox : public setting_base
	{
	public:
		setting_checkbox()
		{
			type = kEntryType_Checkbox;
			name = Translatable("New Checkbox");
			value = true;
			default_value = true;
		}
		bool value;
		bool default_value;
		std::string control_id;
		bool reset() override
		{
			bool changed = value != default_value;
			value = default_value;
			return changed;
		}
	};

	class setting_slider : public setting_base
	{
	public:
		setting_slider()
		{
			type = kEntryType_Slider;
			name = Translatable("New Slider");
			value = 0.0f;
			min = 0.0f;
			max = 1.0f;
			step = 0.1f;
			default_value = 0.f;
		}
		float value;
		float min;
		float max;
		float step;
		float default_value;
		uint8_t precision = 2;
		bool reset() override
		{
			bool changed = value != default_value;
			value = default_value;
			return changed;
		}
	};

	class setting_textbox : public setting_base
	{
	public:
		std::string value;
		std::string default_value;
		setting_textbox()
		{
			type = kEntryType_Textbox;
			name = Translatable("New Textbox");
			value = "";
			default_value = "";
		}
		bool reset() override
		{
			bool changed = value != default_value;
			value = default_value;
			return changed;
		}
	};

	class setting_dropdown : public setting_base
	{
	public:
		setting_dropdown()
		{
			type = kEntryType_Dropdown;
			name = Translatable("New Dropdown");
			value = 0;
			default_value = 0;
		}
		std::vector<std::string> options;
		int value;
		int default_value;
		bool reset() override
		{
			bool changed = value != default_value;
			value = default_value;
			return changed;
		}
	};

	class setting_color : public setting_base
	{
	public:
		setting_color()
		{
			type = kEntryType_Color;
			name = Translatable("New Color");
			color = { 0.f, 0.f, 0.f, 1.f };
		}
		bool reset() override
		{
			bool changed = color.x != default_color.x || color.y != default_color.y || color.z != default_color.z || color.w != default_color.w;
			color = default_color;
			return changed;
		}
		ImGuiMCP::ImVec4 color;
		ImGuiMCP::ImVec4 default_color;
	};

	class setting_keymap : public setting_base
	{
	public:
		setting_keymap()
		{
			type = kEntryType_Keymap;
			name = Translatable("New Keymap");
			value = 0;
			default_value = 0;
		}
		int value;
		int default_value;
		static const char* keyid_to_str(int key_id);
	};

	class entry_button : public entry_base
	{
	public:
		entry_button()
		{
			type = kEntryType_Button;
			name = Translatable("New Button");
			id = "";
		}
		std::string id;
		bool is_setting() const override { return false; }
	};

	class mod_setting
	{
	public:
		std::string name;
		std::vector<entry_base*> entries;
		std::string ini_path;
		std::string json_path;

		std::vector<std::function<void()>> callbacks;
	};

	static inline std::vector<mod_setting*> mods;

	static inline std::unordered_set<mod_setting*> ini_dirty_mods;

public:
	static void show();

	static void init();

private:
	static entry_base* load_json_non_group(nlohmann::json& json);
	static entry_group* load_json_group(nlohmann::json& group_json);
	static entry_base* load_json_entry(nlohmann::json& json);
	static void load_json(std::filesystem::path a_path);

	static void get_all_settings(mod_setting* mod, std::vector<ModSettings::setting_base*>& r_vec);

	static void load_ini(mod_setting* mod);
	static void flush_ini(mod_setting* mod);

	static void show_saveButton();
	static void show_cancelButton();

	static void show_modSetting(mod_setting* mod);
	static void show_entry(entry_base* base, mod_setting* mod);
	static void show_entries(std::vector<entry_base*>& entries, mod_setting* mod);

	static void SendSettingsUpdateEvent(std::string& modName);
	static void send_mod_callback_event(std::string& mod_name, std::string& str_arg);
};
