#include <API/ARK/Ark.h>

#include "NewPlayerProtection.h"
#include "NewPlayerProtectionCommands.h"
#include "NewPlayerProtectionConfig.h"

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "Permissions.lib")
void Init()
{
	Log::Get().Init("NewPlayerProtection");
	InitConfig();
	InitCommands();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Init();
		break;
	case DLL_PROCESS_DETACH:
		RemoveCommands();
		break;
	}
	return TRUE;
}
