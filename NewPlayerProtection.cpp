#include <API/ARK/Ark.h>
#include <Timer.h>
#include "NewPlayerProtection.h"
#include "NewPlayerProtectionConfig.h"
#include "NewPlayerProtectionHooks.h"
#include "NewPlayerProtectionCommands.h"

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "Permissions.lib")

void Init() {
	Log::Get().Init("NewPlayerProtection");

	InitConfig();
	InitHooks();
	InitCommands();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		Init();

		// Timer to init Permission members array after Permission.dll has been loaded
		API::Timer::Get().DelayExecute(&LoadNppPermissionsArray, 10);
		break;
	case DLL_PROCESS_DETACH:
		RemoveHooks();
		RemoveCommands();
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
