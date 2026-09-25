#include "Menu.h"

#include "KeyInput.h"

namespace
{
	namespace im = ImGuiMCP;

	constexpr auto kCapturePopup = "Remap Key###SimpleWheelerMenuCapture";
	constexpr im::ImVec4 kWhite{ 1.0f, 1.0f, 1.0f, 1.0f };
	constexpr im::ImVec4 kWarning{ 1.0f, 0.62f, 0.25f, 1.0f };
	constexpr im::ImVec4 kTransparent{ 0.0f, 0.0f, 0.0f, 0.0f };
	constexpr im::ImVec4 kSave{ 0.2f, 0.7f, 0.3f, 1.0f };
	constexpr im::ImVec4 kSaveHovered{ 0.3f, 0.8f, 0.4f, 1.0f };
	constexpr im::ImVec4 kSaveActive{ 0.4f, 0.9f, 0.5f, 1.0f };

	std::optional<ModSettings::Page> g_page;
	ModSettings::Entry* g_captureTarget = nullptr;
	bool g_playStation = false;

	void Describe(const ModSettings::Entry& a_entry, std::string_view a_problem = {})
	{
		if (!a_problem.empty()) {
			im::SetItemTooltip("%s\n\n%.*s", a_entry.desc.c_str(), static_cast<int>(a_problem.size()), a_problem.data());
		} else if (!a_entry.desc.empty()) {
			im::SetItemTooltip("%s", a_entry.desc.c_str());
		}
	}

	void HelpMarker(const ModSettings::Entry& a_entry)
	{
		if (a_entry.desc.empty()) {
			return;
		}
		im::SameLine();
		im::TextDisabled("(?)");
		im::SetItemTooltip("%s", a_entry.desc.c_str());
	}

	bool ResetRequested(ModSettings::Entry& a_entry)
	{
		if (im::IsKeyPressed(im::ImGuiKey_R, false) && im::IsItemHovered()) {
			return ModSettings::Reset(a_entry);
		}
		return false;
	}

	bool DrawSlider(const ModSettings::Entry& a_entry, ModSettings::Slider& a_slider)
	{
		std::array<char, 32> text{};
		std::snprintf(text.data(), text.size(), "%g", a_slider.value);
		auto index = std::clamp(static_cast<int>(std::lround((a_slider.value - a_slider.min) / a_slider.step)), 0, a_slider.steps);
		im::SetNextItemWidth(im::GetContentRegionAvail().x * 0.5f);
		if (!im::SliderInt(a_entry.name.c_str(), &index, 0, a_slider.steps, text.data(), im::ImGuiSliderFlags_AlwaysClamp | im::ImGuiSliderFlags_NoInput)) {
			return false;
		}
		a_slider.value = a_slider.min + static_cast<float>(index) * a_slider.step;
		return true;
	}

	bool DrawWidget(ModSettings::Entry& a_entry)
	{
		bool edited = false;
		if (auto* slider = std::get_if<ModSettings::Slider>(&a_entry.widget)) {
			edited = DrawSlider(a_entry, *slider);
		} else if (im::Button(a_entry.name.c_str())) {
			ModSettings::Press(std::get<ModSettings::Button>(a_entry.widget));
		}
		Describe(a_entry);
		edited = ResetRequested(a_entry) || edited;
		HelpMarker(a_entry);
		return edited;
	}

	void DrawKeymap(ModSettings::Page& a_page, ModSettings::Entry& a_entry)
	{
		auto& keymap = std::get<ModSettings::Keymap>(a_entry.widget);
		const auto problem = KeyInput::Problem(keymap.value, keymap.gamepad, g_playStation);
		im::PushID(std::addressof(a_entry));
		im::TableNextRow();
		im::TableNextColumn();
		if (im::Button("Remap")) {
			g_captureTarget = std::addressof(a_entry);
			KeyInput::Begin();
		}
		Describe(a_entry, problem);
		if (ResetRequested(a_entry)) {
			a_page.dirty = true;
		}
		im::SameLine();
		if (im::Button("Unmap")) {
			keymap.value = 0;
			a_page.dirty = true;
		}
		im::TableNextColumn();
		im::AlignTextToFramePadding();
		im::TextUnformatted(a_entry.name.c_str());
		HelpMarker(a_entry);
		im::TableNextColumn();
		im::AlignTextToFramePadding();
		const auto name = KeyInput::Name(keymap.value, g_playStation);
		auto color = kWhite;
		if (!problem.empty()) {
			color = kWarning;
		}
		im::TextColored(color, "%.*s", static_cast<int>(name.size()), name.data());
		Describe(a_entry, problem);
		im::PopID();
	}

	std::size_t DrawKeymaps(ModSettings::Page& a_page, std::span<ModSettings::Entry> a_entries)
	{
		std::size_t count = 0;
		while (count < a_entries.size() && std::holds_alternative<ModSettings::Keymap>(a_entries[count].widget)) {
			++count;
		}
		im::PushID(a_entries.data());
		if (im::BeginTable("##keymaps", 3, im::ImGuiTableFlags_SizingFixedFit | im::ImGuiTableFlags_NoSavedSettings)) {
			for (auto& entry : a_entries.first(count)) {
				DrawKeymap(a_page, entry);
			}
			im::EndTable();
		}
		im::PopID();
		return count;
	}

	void DrawEntries(ModSettings::Page& a_page, std::span<ModSettings::Entry> a_entries, int a_depth);

	void DrawGroup(ModSettings::Page& a_page, ModSettings::Entry& a_entry, int a_depth)
	{
		int flags = 0;
		if (a_depth == 0) {
			flags = im::ImGuiTreeNodeFlags_DefaultOpen;
		}
		im::PushStyleColor(im::ImGuiCol_Header, kTransparent);
		const bool open = im::CollapsingHeader(a_entry.name.c_str(), flags);
		im::PopStyleColor();
		Describe(a_entry);
		if (open) {
			DrawEntries(a_page, a_entry.children, a_depth + 1);
		}
	}

	void DrawEntries(ModSettings::Page& a_page, std::span<ModSettings::Entry> a_entries, int a_depth)
	{
		im::Indent();
		for (std::size_t index = 0; index < a_entries.size();) {
			auto& entry = a_entries[index];
			if (std::holds_alternative<ModSettings::Keymap>(entry.widget)) {
				index += DrawKeymaps(a_page, a_entries.subspan(index));
				continue;
			}
			im::PushID(std::addressof(entry));
			if (std::holds_alternative<ModSettings::Group>(entry.widget)) {
				DrawGroup(a_page, entry, a_depth);
			} else if (DrawWidget(entry)) {
				a_page.dirty = true;
			}
			im::PopID();
			++index;
		}
		im::Unindent();
	}

	void DrawCapture(ModSettings::Page& a_page)
	{
		if (!g_captureTarget) {
			return;
		}
		if (!im::IsPopupOpen(kCapturePopup)) {
			im::OpenPopup(kCapturePopup);
		}
		if (!im::BeginPopupModal(kCapturePopup, nullptr, im::ImGuiWindowFlags_AlwaysAutoResize | im::ImGuiWindowFlags_NoSavedSettings)) {
			return;
		}
		im::TextUnformatted("Press any key, mouse button, wheel, or controller button to bind.");
		std::uint32_t code = 0;
		const auto state = KeyInput::Poll(code);
		if (state == KeyInput::State::kCaptured) {
			std::get<ModSettings::Keymap>(g_captureTarget->widget).value = code;
			a_page.dirty = true;
		}
		if (state != KeyInput::State::kWaiting) {
			g_captureTarget = nullptr;
			im::CloseCurrentPopup();
		}
		im::EndPopup();
	}

	void DrawToolbar(ModSettings::Page& a_page)
	{
		const bool dirty = a_page.dirty;
		if (dirty) {
			im::PushStyleColor(im::ImGuiCol_Button, kSave);
			im::PushStyleColor(im::ImGuiCol_ButtonHovered, kSaveHovered);
			im::PushStyleColor(im::ImGuiCol_ButtonActive, kSaveActive);
		}
		if (im::Button("Save Changes") && dirty) {
			ModSettings::Save(a_page);
		}
		if (dirty) {
			im::PopStyleColor(3);
		}
		im::SameLine();
		if (im::Button("Revert") && dirty) {
			ModSettings::Revert(a_page);
		}
	}

	void __stdcall Render()
	{
		if (!g_page) {
			im::TextWrapped("Wheeler Controls.json was not found in Data/SKSE/Plugins/dmenu/customSettings. Install Wheeler - Quick Action Wheel of Skyrim.");
			return;
		}
		g_playStation = KeyInput::IsPlayStation();
		DrawToolbar(*g_page);
		im::Separator();
		im::PushStyleVar(im::ImGuiStyleVar_ItemSpacing, im::ImVec2{ 8.0f, 8.0f });
		im::PushStyleColor(im::ImGuiCol_Text, kWhite);
		for (auto& entry : g_page->entries) {
			if (std::holds_alternative<ModSettings::Group>(entry.widget)) {
				DrawEntries(*g_page, entry.children, 0);
			} else {
				DrawEntries(*g_page, std::span(std::addressof(entry), 1), 0);
			}
		}
		DrawCapture(*g_page);
		im::PopStyleColor();
		im::PopStyleVar();
	}
}

void Menu::Register(std::optional<ModSettings::Page> a_page)
{
	g_page = std::move(a_page);
	if (!SKSEMenuFramework::IsInstalled()) {
		logger::warn("SKSE Menu Framework is not installed; Simple Wheeler Menu has no UI.");
		return;
	}
	SKSEMenuFramework::SetSection("Simple Wheeler Menu");
	SKSEMenuFramework::AddSectionItem("Settings", Render);
	KeyInput::Install();
}
