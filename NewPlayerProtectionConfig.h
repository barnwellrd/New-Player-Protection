#pragma once
#include <fstream>
#include <Permissions.h>

std::string NewPlayerProtection::GetTimestamp(std::chrono::time_point<std::chrono::system_clock> datetime)
{
	using namespace std;
	using namespace std::chrono;

	auto ttime_t = system_clock::to_time_t(datetime);
	auto tp_sec = system_clock::from_time_t(ttime_t);
	milliseconds ms = duration_cast<milliseconds>(datetime - tp_sec);

	std::tm * ttm = localtime(&ttime_t);

	char date_time_format[] = "%Y.%m.%d-%H.%M.%S";

	char time_str[] = "yyyy.mm.dd.HH-MM.SS.fff";

	strftime(time_str, strlen(time_str), date_time_format, ttm);

	string result(time_str);
	result.append(".");
	result.append(to_string(ms.count()));

	return result;
}

std::chrono::time_point<std::chrono::system_clock> NewPlayerProtection::GetDateTime(std::string timestamp)
{
	using namespace std;
	using namespace std::chrono;

	int yyyy, mm, dd, HH, MM, SS, fff;

	char scanf_format[] = "%4d.%2d.%2d-%2d.%2d.%2d.%3d";

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

	system_clock::time_point time_point_result = std::chrono::system_clock::from_time_t(ttime_t);

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

	db << "create table if not exists Players ("
		"SteamId integer primary key not null,"
		"TribeId integer default 0,"
		"Start_DateTime text default '',"
		"Level integer default 0,"
		"Is_New_Player integer default 0"
		");";
}

void LoadDataBase()
{
	auto& db = NewPlayerProtection::GetDB();

try
	{
		auto res = db << "SELECT * FROM Players;";

		res >> [](uint64 steamid, uint64 tribeid, std::string startdate, int level, int isnewplayer)
		{
			NewPlayerProtection::TimerProt::Get().AddPlayer(steamid, tribeid, NewPlayerProtection::GetDateTime(startdate), level, isnewplayer);
		};

	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}
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

	NewPlayerProtection::PlayerUpdateIntervalInMins = NewPlayerProtection::config["General"]["PlayerUpdateIntervalInMins"];
	NewPlayerProtection::MaxLevel = NewPlayerProtection::config["General"]["NewPlayerProtection"]["NewPlayerMaxLevel"];
	NewPlayerProtection::DaysOfProtection = NewPlayerProtection::config["General"]["NewPlayerProtection"]["DaysOfProtection"];
	NewPlayerProtection::next_player_update = std::chrono::system_clock::now();

	LoadDB();
	LoadDataBase();
}

