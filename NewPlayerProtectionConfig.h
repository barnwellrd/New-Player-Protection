#pragma once

#include <fstream>

sqlite::database& NewPlayerProtection::GetDB()
{
	static sqlite::database db(config["General"].value("DbPathOverride", "").empty()
		? ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/NewPlayerProtection/NewPlayerProtection.db"
		: config["General"]["DbPathOverride"]);
	return db;
}

void LoadDB()
{
	auto& db = NewPlayerProtection::GetDB();

	db << "create table if not exists Players ("
		"Id integer primary key autoincrement not null,"
		"SteamId integer default 0,"
		"TribeId integer default 0,"
		"Start_DateTime text default '',"
		"Level integer default 0,"
		"Is_New_Player integer default 0"
		");";
}

inline void InitConfig()
{
	std::ifstream file(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/NewPlayerProtection/config.json");
	if (!file.is_open())
	{
		return;
	}

	file >> NewPlayerProtection::config;
	file.close();

	LoadDB();

	NewPlayerProtection::RequiresAdmin = NewPlayerProtection::config["General"]["RequireAdmin"];
	NewPlayerProtection::MaxLevel = NewPlayerProtection::config["General"]["NewPlayerProtection"]["NewPlayerMaxLevel"];
	NewPlayerProtection::DaysOfProtection = NewPlayerProtection::config["General"]["NewPlayerProtection"]["DaysOfProtection"];
	//NewPlayerProtection::JoinEstablishedTribeOveride = NewPlayerProtection::config["General"]["NewPlayerProtection"]["JoinEstablishedTribeOveride"];
	NewPlayerProtection::AllPlayerStructuresProtected = NewPlayerProtection::config["General"]["NewPlayerProtection"]["PickOnlyOneProtection"]["AllPlayerStructuresProtected"];
	//NewPlayerProtection::BedProtectionEnabled = NewPlayerProtection::config["General"]["NewPlayerProtection"]["PickOnlyOneProtection"]["OnlyFirstBedAreaIsProtected"]["Enabled"];
	//NewPlayerProtection::BedProtectedRadius = NewPlayerProtection::config["General"]["NewPlayerProtection"]["PickOnlyOneProtection"]["OnlyFirstBedAreaIsProtected"]["ProtectedRadius"];
}

