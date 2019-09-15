#pragma once
#include <fstream>
#include <Permissions.h>

std::string NewPlayerProtection::GetTimestamp(std::chrono::time_point<std::chrono::system_clock> datetime)
{
	auto ttime_t = std::chrono::system_clock::to_time_t(datetime);
	auto tp_sec = std::chrono::system_clock::from_time_t(ttime_t);
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(datetime - tp_sec);

	std::tm * ttm = localtime(&ttime_t);

	char date_time_format[] = "%Y-%m-%d %H:%M:%S";

	char time_str[] = "yyyy-mm-dd HH:MM:SS:fff";

	strftime(time_str, strlen(time_str), date_time_format, ttm);

	std::string result(time_str);
	result.append(".");
	result.append(std::to_string(ms.count()));

	return result;
}

std::chrono::time_point<std::chrono::system_clock> NewPlayerProtection::GetDateTime(std::string timestamp)
{
	int yyyy;
	int mm;
	int dd;
	int HH;
	int MM;
	int SS;
	int fff;

	char scanf_format[] = "%4d-%2d-%2d %2d:%2d:%2d.%3d";

	sscanf(timestamp.c_str(), scanf_format, &yyyy, &mm, &dd, &HH, &MM, &SS, &fff);

	tm ttm = tm();
	ttm.tm_year = yyyy - 1900; // Year since 1900
	ttm.tm_mon = mm - 1; // Month since January 
	ttm.tm_mday = dd; // Day of the month [1-31]
	ttm.tm_hour = HH; // Hour of the day [00-23]
	ttm.tm_min = MM;
	ttm.tm_sec = SS;
	ttm.tm_isdst = -1;

	time_t ttime_t = mktime(&ttm);

	std::chrono::system_clock::time_point time_point_result = std::chrono::system_clock::from_time_t(ttime_t);

	time_point_result += std::chrono::milliseconds(fff);
	return time_point_result;
}

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
	try
	{
		// create players table
		db << "create table if not exists Players ("
			"SteamId integer primary key not null,"
			"TribeId integer default 0,"
			"Start_DateTime text default '',"
			"Last_Login_DateTime text default '',"
			"Level integer default 0,"
			"Is_New_Player integer default 0"
			");";

		// create pve_tribes table
		db << "create table if not exists PVE_Tribes ("
			"TribeId integer primary key not null,"
			"Is_Protected integer default 0"
			");";

		// set pragmas for db
		db << "PRAGMA journal_mode = WAL;";
		//db << "PRAGMA synchronous = OFF;";
	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error creating database: {}", __FILE__, __FUNCTION__, exception.what());
	}
	

	try
	{
		std::string hours = "'-";
		hours.append(std::to_string(NewPlayerProtection::NPPPlayerDecayInHours));
		hours.append(" hour'");

		auto res = db << "SELECT * FROM Players where Last_Login_DateTime > date('now', ? );"
			<< hours;

		res >> [](uint64 steamid, uint64 tribeid, std::string startdate, std::string lastlogindate, int level, int isnewplayer)
		{
			NewPlayerProtection::TimerProt::Get().AddPlayerFromDB(steamid, tribeid, NewPlayerProtection::GetDateTime(startdate), NewPlayerProtection::GetDateTime(lastlogindate),level, isnewplayer);
		};

		Log::GetLog()->info("Players table data loaded.");
	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error loading players table: {}", __FILE__, __FUNCTION__, exception.what());
	}

	try
	{
		auto res = db << "SELECT TribeId FROM PVE_Tribes where Is_Protected = 1;";

		res >> [](uint64 tribeid)
		{
			NewPlayerProtection::pveTribesList.push_back(tribeid);
		};

		Log::GetLog()->info("PVE_Tribes table data loaded.");

	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error loading pve_tribes table: {}", __FILE__, __FUNCTION__, exception.what());
	}
}

inline void LoadConfig()
{
	std::ifstream file(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/NewPlayerProtection/config.json");

	if (!file.is_open())
	{
		return;
	}

	file >> NewPlayerProtection::config;
	file.close();

	NewPlayerProtection::PlayerUpdateIntervalInMins = NewPlayerProtection::config["General"]["PlayerUpdateIntervalInMins"];
	NewPlayerProtection::IgnoreAdmins = NewPlayerProtection::config["General"]["IgnoreAdmins"];
	NewPlayerProtection::AllowNewPlayersToDamageEnemyStructures = NewPlayerProtection::config["General"]["AllowNewPlayersToDamageEnemyStructures"];
	NewPlayerProtection::NPPPlayerDecayInHours = NewPlayerProtection::config["General"]["NPPPlayerDecayInHours"];
	NewPlayerProtection::NPPCommandPrefix = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["NPPCommandPrefix"]).c_str());
	NewPlayerProtection::MaxLevel = NewPlayerProtection::config["General"]["NewPlayerProtection"]["NewPlayerMaxLevel"];
	NewPlayerProtection::HoursOfProtection = NewPlayerProtection::config["General"]["NewPlayerProtection"]["HoursOfProtection"];
	NewPlayerProtection::next_player_update = std::chrono::system_clock::now();
	NewPlayerProtection::AllowPlayersToDisableOwnedTribeProtection = NewPlayerProtection::config["General"]["AllowPlayersToDisableOwnedTribeProtection"];
	NewPlayerProtection::NewPlayerDoingDamageMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["NewPlayerDoingDamageMessage"]).c_str());
	NewPlayerProtection::NewPlayerStructureTakingDamageMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["NewPlayerStructureTakingDamageMessage"]).c_str());
	NewPlayerProtection::NewPlayerStructureTakingDamageFromUnknownTribemateMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["NewPlayerStructureTakingDamageFromUnknownTribemateMessage"]).c_str());
	NewPlayerProtection::NPPInvalidCommand = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["NPPInvalidCommand"]).c_str());
	NewPlayerProtection::NewPlayerProtectionDisableSuccess = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["NewPlayerProtectionDisableSuccess"]).c_str());
	NewPlayerProtection::NotANewPlayerMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["NotANewPlayerMessage"]).c_str());
	NewPlayerProtection::NotTribeAdminMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["NotTribeAdminMessage"]).c_str());
	NewPlayerProtection::NPPRemainingMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["NPPRemainingMessage"]).c_str());
	NewPlayerProtection::AdminNoTribeExistsMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["AdminNoTribeExistsMessage"]).c_str());
	NewPlayerProtection::AdminTribeProtectionRemoved = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["AdminTribeProtectionRemoved"]).c_str());
	NewPlayerProtection::AdminTribeNotUnderProtection = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["AdminTribeNotUnderProtection"]).c_str());
	NewPlayerProtection::AdminResetTribeProtectionSuccess = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["AdminResetTribeProtectionSuccess"]).c_str());
	NewPlayerProtection::AdminResetTribeProtectionLvlFailure = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["AdminResetTribeProtectionLvlFailure"]).c_str());
	NewPlayerProtection::NPPInfoMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["NPPInfoMessage"]).c_str());
	NewPlayerProtection::TribeIDText = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["TribeIDText"]).c_str());
	NewPlayerProtection::NoStructureForTribeIDText = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["NoStructureForTribeIDText"]).c_str());
	NewPlayerProtection::PVEDisablePlayerMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["PVEDisablePlayerMessage"]).c_str());
	NewPlayerProtection::PVEStatusMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["PVEStatusMessage"]).c_str());
	NewPlayerProtection::AdminPVETribeAddedSuccessMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["AdminPVETribeAddedSuccessMessage"]).c_str());
	NewPlayerProtection::AdminPVETribeAlreadyAddedMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["AdminPVETribeAlreadyAddedMessage"]).c_str());
	NewPlayerProtection::AdminPVETribeRemovedSuccessMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["AdminPVETribeRemovedSuccessMessage"]).c_str());
	NewPlayerProtection::AdminPVETribeAlreadyRemovedMessage = FString(ArkApi::Tools::Utf8Decode(NewPlayerProtection::config["General"]["AdminPVETribeAlreadyRemovedMessage"]).c_str());

	NewPlayerProtection::MessageIntervalInSecs = NewPlayerProtection::config["General"]["MessageIntervalInSecs"];
	NewPlayerProtection::MessageTextSize = NewPlayerProtection::config["General"]["MessageTextSize"];
	NewPlayerProtection::MessageDisplayDelay = NewPlayerProtection::config["General"]["MessageDisplayDelay"];
	NewPlayerProtection::TempConfig = NewPlayerProtection::config["General"]["MessageColor"];
	NewPlayerProtection::MessageColor = FLinearColor(NewPlayerProtection::TempConfig[0], NewPlayerProtection::TempConfig[1], NewPlayerProtection::TempConfig[2], NewPlayerProtection::TempConfig[3]);

}

inline void InitConfig()
{
	LoadConfig();
	LoadDB();
}


