#include "Menu.h"

#include "ModSettings.h"
#include "logger.h"

namespace
{
	void __stdcall RenderSettings()
	{
		ModSettings::show();
	}
}

void Menu::Register()
{
	if (!SKSEMenuFramework::IsInstalled()) {
		logger::warn("SKSE Menu Framework not installed; Simple Wheeler Menu UI unavailable.");
		return;
	}

	SKSEMenuFramework::SetSection("Simple Wheeler Menu");
	SKSEMenuFramework::AddSectionItem("Settings", RenderSettings);

	logger::info("Simple Wheeler Menu registered with SKSE Menu Framework.");
}
