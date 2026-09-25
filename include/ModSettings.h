#pragma once

namespace ModSettings
{
	struct Group
	{
	};

	struct Slider
	{
		float value = 0.0f;
		float fallback = 0.0f;
		float min = 0.0f;
		float step = 1.0f;
		std::int32_t steps = 1;
	};

	struct Keymap
	{
		std::uint32_t value = 0;
		std::uint32_t fallback = 0;
		bool gamepad = false;
	};

	struct Button
	{
		std::string id;
	};

	struct Entry
	{
		std::variant<Group, Slider, Keymap, Button> widget;
		std::string name;
		std::string desc;
		std::string section;
		std::string key;
		std::vector<Entry> children;
	};

	struct Page
	{
		std::string ini;
		std::vector<Entry> entries;
		bool dirty = false;
	};

	std::optional<Page> Load();
	void Save(Page& a_page);
	void Revert(Page& a_page);
	bool Reset(Entry& a_entry);
	void Press(const Button& a_button);
}
