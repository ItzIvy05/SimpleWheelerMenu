#include "Menu.h"

#include "KeyInput.h"

#include <FUCK_API.h>

namespace
{
	constexpr std::uint32_t kGamepadOffset = 266;
	constexpr auto kCapturePopup = "Remap Key###SimpleWheelerMenuCapture";
	constexpr ImVec4 kSave{ 0.2f, 0.7f, 0.3f, 1.0f };
	constexpr ImVec4 kSaveHovered{ 0.3f, 0.8f, 0.4f, 1.0f };
	constexpr ImVec4 kSaveActive{ 0.4f, 0.9f, 0.5f, 1.0f };

	std::optional<ModSettings::Page> g_page;
	ModSettings::Entry* g_captureTarget = nullptr;

	const char* KeyName(std::uint32_t a_code)
	{
		switch (a_code) {
		case 0:
			return "Unmapped";
		case 264:
			return "Mouse Wheel Up";
		case 265:
			return "Mouse Wheel Down";
		default:
			break;
		}
		if (a_code >= kGamepadOffset) {
			return FUCK::GetKeyName(a_code + kGamepadOffset);
		}
		return FUCK::GetKeyName(a_code);
	}

	void HelpMarker(const ModSettings::Entry& a_entry)
	{
		if (!a_entry.desc.empty()) {
			FUCK::SameLine();
			FUCK::HelpMarker(a_entry.desc.c_str());
		}
	}

	void DrawKeymap(ModSettings::Page& a_page, ModSettings::Entry& a_entry, ModSettings::Keymap& a_keymap)
	{
		if (FUCK::Button("Remap")) {
			g_captureTarget = std::addressof(a_entry);
			KeyInput::Begin();
		}
		FUCK::SameLine();
		if (FUCK::Button("Unmap")) {
			a_keymap.value = 0;
			a_page.dirty = true;
		}
		FUCK::SameLine();
		FUCK::Text("%s: %s", a_entry.name.c_str(), KeyName(a_keymap.value));
	}

	void DrawSlider(ModSettings::Page& a_page, ModSettings::Entry& a_entry, ModSettings::Slider& a_slider)
	{
		std::array<char, 32> text{};
		std::snprintf(text.data(), text.size(), "%g", a_slider.value);
		auto index = static_cast<int>(std::lround((a_slider.value - a_slider.min) / a_slider.step));
		FUCK::SetNextItemWidth(FUCK::GetContentRegionAvail().x * 0.5f);
		if (FUCK::SliderInt(a_entry.name.c_str(), &index, 0, a_slider.steps, text.data())) {
			a_slider.value = a_slider.min + static_cast<float>(index) * a_slider.step;
			a_page.dirty = true;
		}
		if (FUCK::IsKeyPressed(ImGuiKey_R, false) && FUCK::IsItemHovered() && std::exchange(a_slider.value, a_slider.fallback) != a_slider.fallback) {
			a_page.dirty = true;
		}
	}

	void DrawEntries(ModSettings::Page& a_page, std::span<ModSettings::Entry> a_entries, int a_headerFlags);

	void DrawEntry(ModSettings::Page& a_page, ModSettings::Entry& a_entry, int a_headerFlags)
	{
		FUCK::PushID(std::addressof(a_entry));
		if (auto* keymap = std::get_if<ModSettings::Keymap>(&a_entry.widget)) {
			DrawKeymap(a_page, a_entry, *keymap);
		} else if (auto* slider = std::get_if<ModSettings::Slider>(&a_entry.widget)) {
			DrawSlider(a_page, a_entry, *slider);
		} else if (auto* button = std::get_if<ModSettings::Button>(&a_entry.widget)) {
			if (FUCK::Button(a_entry.name.c_str())) {
				ModSettings::Press(*button);
			}
		} else if (FUCK::CollapsingHeader(a_entry.name.c_str(), a_headerFlags)) {
			DrawEntries(a_page, a_entry.children, 0);
		}
		HelpMarker(a_entry);
		FUCK::PopID();
	}

	void DrawEntries(ModSettings::Page& a_page, std::span<ModSettings::Entry> a_entries, int a_headerFlags)
	{
		FUCK::Indent();
		for (auto& entry : a_entries) {
			DrawEntry(a_page, entry, a_headerFlags);
		}
		FUCK::Unindent();
	}

	void DrawCapture(ModSettings::Page& a_page)
	{
		if (!g_captureTarget) {
			return;
		}
		if (!FUCK::IsPopupOpen(kCapturePopup)) {
			FUCK::OpenPopup(kCapturePopup);
		}
		if (!FUCK::BeginPopupModal(kCapturePopup)) {
			return;
		}
		FUCK::TextUnformatted("Press any key, mouse button, wheel, or controller button to bind.");
		FUCK::TextUnformatted("(Esc binds Escape.)");
		std::uint32_t code = 0;
		const auto state = KeyInput::Poll(code);
		if (state == KeyInput::State::kCaptured) {
			std::get<ModSettings::Keymap>(g_captureTarget->widget).value = code;
			a_page.dirty = true;
		}
		if (state != KeyInput::State::kWaiting) {
			g_captureTarget = nullptr;
			FUCK::CloseCurrentPopup();
		}
		FUCK::EndPopup();
	}

	void DrawToolbar(ModSettings::Page& a_page)
	{
		const bool dirty = a_page.dirty;
		if (dirty) {
			FUCK::PushStyleColor(ImGuiCol_Button, kSave);
			FUCK::PushStyleColor(ImGuiCol_ButtonHovered, kSaveHovered);
			FUCK::PushStyleColor(ImGuiCol_ButtonActive, kSaveActive);
		}
		if (FUCK::Button("Save Changes") && dirty) {
			ModSettings::Save(a_page);
		}
		if (dirty) {
			FUCK::PopStyleColor(3);
		}
		FUCK::SameLine();
		if (FUCK::Button("Revert") && dirty) {
			ModSettings::Revert(a_page);
		}
	}

	class SettingsPage : public FUCK::ITool
	{
	public:
		const char* Name() const override
		{
			return "Settings";
		}

		const char* Group() const override
		{
			return "Simple Wheeler Menu";
		}

		void Draw() override
		{
			if (!g_page) {
				FUCK::TextWrapped("Wheeler Controls.json was not found in Data/SKSE/Plugins/dmenu/customSettings. Install Wheeler - Quick Action Wheel of Skyrim.");
				return;
			}
			DrawToolbar(*g_page);
			FUCK::Separator();
			for (auto& entry : g_page->entries) {
				if (std::holds_alternative<ModSettings::Group>(entry.widget)) {
					DrawEntries(*g_page, entry.children, ImGuiTreeNodeFlags_DefaultOpen);
				} else {
					DrawEntries(*g_page, std::span(std::addressof(entry), 1), ImGuiTreeNodeFlags_DefaultOpen);
				}
			}
			DrawCapture(*g_page);
		}

		void OnClose() override
		{
			KeyInput::Cancel();
		}

		bool OnAsyncInput(const void* a_events) override
		{
			return KeyInput::Process(static_cast<const RE::InputEvent* const*>(a_events));
		}
	};

	SettingsPage g_settingsPage;
}

void Menu::Register(std::optional<ModSettings::Page> a_page)
{
	g_page = std::move(a_page);
	if (!FUCK::Connect(SKSE::GetPluginName().data())) {
		logger::warn("FLICK is not installed; Simple Wheeler Menu has no UI.");
		return;
	}
	FUCK::RegisterTool(std::addressof(g_settingsPage));
}
