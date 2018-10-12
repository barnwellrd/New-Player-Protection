#pragma once

#include <fstream>

void LoadDB()
{

	auto& db = NewPlayerProtection::GetDB();

	db << "create table if not exists Players ("
		"Id integer primary key autoincrement not null,"
		"SteamId integer default 0,"
		"Kits text default '{}',"
		"Points integer default 0"
		");";
}

inline void InitConfig()
{
	std::ifstream file(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/NewPlayerProtection/config.json");
	if (!file.is_open())
	{
		RequiresAdmin = true;
		return;
	}

	file >> NewPlayerProtection::config;
	file.close();

	LoadDB();
	RequiresAdmin = NewPlayerProtection::config["NewPlayerProtection"]["RequireAdmin"];
	RequiresPermission = NewPlayerProtection::config["NewPlayerProtection"].value("RequirePermission", false);
}

