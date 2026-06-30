#include "Menu.h"
#include "ModSettings.h"
#include "logger.h"

void OnMessage(SKSE::MessagingInterface::Message* message)
{
	switch (message->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		try {
			ModSettings::init();
			Menu::Register();
		} catch (const std::exception& e) {
			logger::error("Simple Wheeler Menu startup failed: {}", e.what());
		}
		break;
	default:
		break;
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
	SetupLog();
	logger::info("Plugin loaded");
	SKSE::Init(skse);
	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
	return true;
}
