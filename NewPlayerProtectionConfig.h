#pragma once
#include <fstream>
#include <Permissions.h>

std::string NPP::GetTimestamp(std::chrono::time_point<std::chrono::system_clock> datetime) {
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

std::chrono::time_point<std::chrono::system_clock> NPP::GetDateTime(std::string timestamp) {
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

bool IsAdmin(uint64 steam_id) {
	return (NPP::IgnoreAdmins && NPP::nppAdminArray.Contains(steam_id));
}

sqlite::database& NPP::GetDB() {
	static sqlite::database db(config["General"].value("DbPathOverride", "").empty()
		? ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/NewPlayerProtection/NewPlayerProtection.db"
		: config["General"]["DbPathOverride"]);
	return db;
}

void LoadDB() {
	auto& db = NPP::GetDB();

	try {
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
	}
	catch (const sqlite::sqlite_exception& exception) {
		Log::GetLog()->error("({} {}) Unexpected DB error creating database: {}", __FILE__, __FUNCTION__, exception.what());
	}
	
	try {
		NPP::nppTribesList.clear();
		NPP::TimerProt::Get().GetAllPlayers().clear();

		std::string hours = "SELECT * FROM Players where Last_Login_DateTime > date('now', '-";
		hours.append(std::to_string(NPP::NPPPlayerDecayInHours));
		hours.append(" hour');");

		auto res = db << hours;

		res >> [](uint64 steamid, uint64 tribeid, std::string startdate, std::string lastlogindate, int level, int isnewplayer) {
			NPP::TimerProt::Get().AddPlayerFromDB(steamid, tribeid, NPP::GetDateTime(startdate), 
				NPP::GetDateTime(lastlogindate), level, isnewplayer);
		};
		if (NPP::EnableDebugging) {
			Log::GetLog()->info("Players table data loaded.");
		}
	}
	catch (const sqlite::sqlite_exception& exception) {
		Log::GetLog()->error("({} {}) Unexpected DB error loading players table: {}", __FILE__, __FUNCTION__, exception.what());
	}

	try {
		NPP::pveTribesList.clear();
		auto res = db << "SELECT TribeId FROM PVE_Tribes where Is_Protected = 1;";

		res >> [](uint64 tribeid) {
			NPP::pveTribesList.push_back(tribeid);
		};

		if (NPP::EnableDebugging) {
			Log::GetLog()->info("PVE_Tribes table data loaded.");
		}

	}
	catch (const sqlite::sqlite_exception& exception) {
		Log::GetLog()->error("({} {}) Unexpected DB error loading pve_tribes table: {}", __FILE__, __FUNCTION__, exception.what());
	}
}

void ReloadProtectedTribesArray() {
	auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();

	NPP::nppTribesList.clear();

	for (const auto& Data : all_players_) {
		if (!IsAdmin(Data->steam_id)) {
			if (std::count(NPP::nppTribesList.begin(), NPP::nppTribesList.end(), Data->tribe_id) < 1) {
				NPP::nppTribesList.push_back(Data->tribe_id);
			}
		}
	}
}

void LoadNppPermissionsArray() {
	if (NPP::EnableDebugging) {
		Log::GetLog()->warn("Executing LoadNppPermissionsArray...");
	}
	NPP::nppAdminArray.Empty();
	NPP::nppAdminArray.Append(Permissions::GetGroupMembers(NPP::NPPAdminGroup));
	if (NPP::FirstLoad) {
		if (NPP::EnableDebugging) {
			Log::GetLog()->warn("Executing LoadNppPermissionsArray. First call, so loading the DB...");
		}
		LoadDB();
		NPP::FirstLoad = false;
	}
}

inline void LoadConfig() {
	std::ifstream file(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/NewPlayerProtection/config.json");

	if (!file.is_open()) {
		return;
	}

	file >> NPP::config;
	file.close();

	NPP::next_player_update = std::chrono::system_clock::now();

	NPP::PlayerUpdateIntervalInMins = NPP::config["General"]["PlayerUpdateIntervalInMins"];
	NPP::EnableDebugging = NPP::config["General"]["EnableDebugging"];
	NPP::IgnoreAdmins = NPP::config["General"]["IgnoreAdmins"];
	NPP::AllowNewPlayersToDamageEnemyStructures = NPP::config["General"]["AllowNewPlayersToDamageEnemyStructures"];
	NPP::AllowPlayersToDisableOwnedTribeProtection = NPP::config["General"]["AllowPlayersToDisableOwnedTribeProtection"];
	NPP::AllowWildCorruptedDinoDamage = NPP::config["General"]["AllowWildCorruptedDinoDamage"];
	NPP::AllowWildDinoDamage = NPP::config["General"]["AllowWildDinoDamage"];
	
	NPP::NPPPlayerDecayInHours = NPP::config["General"]["NPPPlayerDecayInHours"];
	NPP::NPPCommandPrefix = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["NPPCommandPrefix"]).c_str());
	NPP::NPPAdminGroup = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["NPPAdminGroup"]).c_str());

	NPP::NewPlayerDoingDamageMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["NewPlayerDoingDamageMessage"]).c_str());
	NPP::NewPlayerStructureTakingDamageMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["NewPlayerStructureTakingDamageMessage"]).c_str());
	NPP::NewPlayerStructureTakingDamageFromUnknownTribemateMessage 
		= FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["NewPlayerStructureTakingDamageFromUnknownTribemateMessage"]).c_str());

	NPP::NPPRemainingMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["NPPRemainingMessage"]).c_str());
	NPP::NPPInfoMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["NPPInfoMessage"]).c_str());
	NPP::NPPInvalidCommand = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["NPPInvalidCommand"]).c_str());
	NPP::NewPlayerProtectionDisableSuccess = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["NewPlayerProtectionDisableSuccess"]).c_str());
	NPP::NotANewPlayerMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["NotANewPlayerMessage"]).c_str());
	NPP::NotTribeAdminMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["NotTribeAdminMessage"]).c_str());
	NPP::TribeIDText = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["TribeIDText"]).c_str());
	NPP::NoStructureForTribeIDText = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["NoStructureForTribeIDText"]).c_str());
	NPP::PVEDisablePlayerMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["PVEDisablePlayerMessage"]).c_str());
	NPP::PVEStatusMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["PVEStatusMessage"]).c_str());
	NPP::NotAStructureMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["NotAStructureMessage"]).c_str());
	NPP::IsAdminTribe = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["IsAdminTribe"]).c_str());
	
	NPP::AdminNoTribeExistsMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["AdminNoTribeExistsMessage"]).c_str());
	NPP::AdminTribeProtectionRemoved = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["AdminTribeProtectionRemoved"]).c_str());
	NPP::AdminTribeNotUnderProtection = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["AdminTribeNotUnderProtection"]).c_str());
	NPP::AdminResetTribeProtectionSuccess = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["AdminResetTribeProtectionSuccess"]).c_str());
	NPP::AdminResetTribeProtectionLvlFailure = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["AdminResetTribeProtectionLvlFailure"]).c_str());
	NPP::AdminPVETribeAddedSuccessMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["AdminPVETribeAddedSuccessMessage"]).c_str());
	NPP::AdminPVETribeAlreadyAddedMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["AdminPVETribeAlreadyAddedMessage"]).c_str());
	NPP::AdminPVETribeRemovedSuccessMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["AdminPVETribeRemovedSuccessMessage"]).c_str());
	NPP::AdminPVETribeAlreadyRemovedMessage = FString(ArkApi::Tools::Utf8Decode(NPP::config["General"]["AdminPVETribeAlreadyRemovedMessage"]).c_str());

	NPP::MessageIntervalInSecs = NPP::config["General"]["MessageIntervalInSecs"];
	NPP::MessageTextSize = NPP::config["General"]["MessageTextSize"];
	NPP::MessageDisplayDelay = NPP::config["General"]["MessageDisplayDelay"];
	NPP::TempConfig = NPP::config["General"]["MessageColor"];
	NPP::MessageColor = FLinearColor(NPP::TempConfig[0], NPP::TempConfig[1], NPP::TempConfig[2], NPP::TempConfig[3]);

	NPP::MaxLevel = NPP::config["General"]["NewPlayerProtection"]["NewPlayerMaxLevel"];
	NPP::HoursOfProtection = NPP::config["General"]["NewPlayerProtection"]["HoursOfProtection"];

	//Clear vector so that config reload is clean
	NPP::StructureExemptions.clear();

	//Load exception structures from config
	NPP::TempConfig = NPP::config["General"]["StructureExemptions"];

	for (nlohmann::json x : NPP::TempConfig) {
		NPP::StructureExemptions.push_back(FString(ArkApi::Tools::Utf8Decode(x).c_str()).ToString());
	}
}

inline void InitConfig() {
	LoadConfig();
	LoadDB();
}
