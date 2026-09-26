#include "ModSettings.h"

#include "SimpleIni.h"

#include <nlohmann/json.hpp>

namespace ModSettings
{
	namespace
	{
		using json = nlohmann::json;
		using Widget = decltype(Entry::widget);

		constexpr auto kName = "Wheeler Controls";
		constexpr auto kConfigPath = "Data\\SKSE\\Plugins\\dmenu\\customSettings\\Wheeler Controls.json";
		constexpr int kMaxWords = 30;

		const json& Find(const json& a_json, std::string_view a_key)
		{
			static const json empty = json::object();
			const auto it = a_json.find(a_key);
			if (it == a_json.end()) {
				return empty;
			}
			return *it;
		}

		std::string Summarize(const std::string& a_desc)
		{
			std::istringstream stream{ a_desc };
			std::string summary;
			std::string word;
			for (int count = 0; count < kMaxWords && stream >> word; ++count) {
				if (!summary.empty()) {
					summary += ' ';
				}
				summary += word;
				if (word.ends_with('.') || word.ends_with('!') || word.ends_with('?')) {
					break;
				}
			}
			return summary;
		}

		Slider ParseSlider(const json& a_json)
		{
			const auto& style = Find(a_json, "style");
			Slider slider;
			slider.min = style.value("min", 0.0f);
			slider.step = style.value("step", 1.0f);
			if (slider.step <= 0.0f) {
				slider.step = 1.0f;
			}
			slider.steps = std::max(1, static_cast<std::int32_t>((style.value("max", slider.min) - slider.min) / slider.step + 0.001f));
			slider.fallback = a_json.value("default", slider.min);
			slider.value = slider.fallback;
			return slider;
		}

		std::optional<Widget> ParseWidget(const json& a_json)
		{
			const auto type = a_json.value("type", ""s);
			if (type == "group") {
				return Group{};
			}
			if (type == "slider") {
				return ParseSlider(a_json);
			}
			if (type == "keymap") {
				const auto fallback = a_json.value("default", 0u);
				return Keymap{ fallback, fallback };
			}
			if (type == "button") {
				return Button{ a_json.value("id", ""s) };
			}
			return std::nullopt;
		}

		void ParseEntries(const json& a_array, std::vector<Entry>& a_entries)
		{
			for (const auto& item : a_array) {
				try {
					const auto& text = Find(item, "text");
					const auto& ini = Find(item, "ini");
					auto widget = ParseWidget(item);
					if (!widget) {
						continue;
					}
					auto& entry = a_entries.emplace_back(std::move(*widget), text.value("name", ""s), Summarize(text.value("desc", ""s)), ini.value("section", ""s), ini.value("id", ""s));
					ParseEntries(Find(item, "entries"), entry.children);
				} catch (const json::exception& e) {
					logger::warn("Skipping malformed setting: {}", e.what());
				}
			}
		}

		void Read(const CSimpleIniA& a_ini, std::vector<Entry>& a_entries)
		{
			for (auto& entry : a_entries) {
				const auto* section = entry.section.c_str();
				const auto* key = entry.key.c_str();
				if (auto* slider = std::get_if<Slider>(&entry.widget)) {
					slider->value = static_cast<float>(a_ini.GetDoubleValue(section, key, slider->fallback));
				} else if (auto* keymap = std::get_if<Keymap>(&entry.widget)) {
					keymap->value = static_cast<std::uint32_t>(std::max(a_ini.GetLongValue(section, key, static_cast<long>(keymap->fallback)), 0L));
				}
				Read(a_ini, entry.children);
			}
		}

		void Write(CSimpleIniA& a_ini, const std::vector<Entry>& a_entries)
		{
			for (const auto& entry : a_entries) {
				const auto* section = entry.section.c_str();
				const auto* key = entry.key.c_str();
				if (const auto* slider = std::get_if<Slider>(&entry.widget)) {
					a_ini.SetDoubleValue(section, key, slider->value);
				} else if (const auto* keymap = std::get_if<Keymap>(&entry.widget)) {
					a_ini.SetLongValue(section, key, static_cast<long>(keymap->value));
				}
				Write(a_ini, entry.children);
			}
		}

		void Send(const std::string& a_event, const std::string& a_arg)
		{
			SKSE::ModCallbackEvent event{ a_event, a_arg, 0.0f, nullptr };
			SKSE::GetModCallbackEventSource()->SendEvent(std::addressof(event));
		}

		void Queue(std::string a_event, std::string a_arg)
		{
			if (const auto* tasks = SKSE::GetTaskInterface()) {
				tasks->AddTask(std::bind_front(Send, std::move(a_event), std::move(a_arg)));
			}
		}
	}

	std::optional<Page> Load()
	{
		try {
			std::ifstream file{ kConfigPath };
			const auto root = json::parse(file, nullptr, true, true);
			Page page{ root.value("ini", "Data\\SKSE\\Plugins\\wheeler\\Controls.ini"s) };
			ParseEntries(Find(root, "data"), page.entries);
			Revert(page);
			return page;
		} catch (const json::exception& e) {
			logger::error("Could not load {}: {}", kConfigPath, e.what());
			return std::nullopt;
		}
	}

	void Save(Page& a_page)
	{
		CSimpleIniA ini{ true };
		ini.LoadFile(a_page.ini.c_str());
		Write(ini, a_page.entries);
		if (ini.SaveFile(a_page.ini.c_str()) < 0) {
			logger::error("Could not write {}", a_page.ini);
			return;
		}
		a_page.dirty = false;
		Queue("dmenu_updateSettings", kName);
	}

	void Revert(Page& a_page)
	{
		CSimpleIniA ini{ true };
		ini.LoadFile(a_page.ini.c_str());
		Read(ini, a_page.entries);
		a_page.dirty = false;
	}

	void Press(const Button& a_button)
	{
		Queue("dmenu_buttonCallback", a_button.id);
	}
}
