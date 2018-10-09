#pragma once

#include <fstream>
#include "json.hpp"

inline void InitConfig()
{
	std::ifstream file(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/NewPlayerProtection/config.json");
	if (!file.is_open())
	{
		RequiresAdmin = true;
		return;
	}

	nlohmann::json configData;
	file >> configData;
	file.close();

	RequiresAdmin = configData["NewPlayerProtection"]["RequireAdmin"];
	RequiresPermission = configData["NewPlayerProtection"].value("RequirePermission", false);
}
