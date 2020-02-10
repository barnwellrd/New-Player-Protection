#include <API/ARK/Ark.h>
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
		if (ArkApi::GetApiUtils().GetStatus() == ArkApi::ServerStatus::Ready)
			LoadNppPermissionsArray();
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
