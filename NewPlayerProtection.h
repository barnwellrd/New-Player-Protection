#pragma once

#include <Logger/Logger.h>
#include <API/UE/Containers/FString.h>
#include "hdr/sqlite_modern_cpp.h"
#include "json.hpp"
 
namespace NewPlayerProtection
{
	bool RequiresAdmin;
	int MaxLevel;
	int DaysOfProtection;
	//bool JoinEstablishedTribeOveride;
	bool AllPlayerStructuresProtected;
	//bool BedProtectionEnabled;
	//int BedProtectedRadius;

	nlohmann::json config;
	sqlite::database& GetDB();
	FString GetText(const std::string& str);

	class TimerProt
	{
		public:
			static TimerProt& Get();

			TimerProt(const TimerProt&) = delete;
			TimerProt(TimerProt&&) = delete;
			TimerProt& operator=(const TimerProt&) = delete;
			TimerProt& operator=(TimerProt&&) = delete;

			void AddPlayer(uint64 steam_id);
			void RemovePlayer(uint64 steam_id);

		private:
			struct OnlinePlayersData
			{
				OnlinePlayersData(uint64 steam_id,
					const std::chrono::time_point<std::chrono::system_clock>& next_update_time)
					: steam_id(steam_id),
					next_update_time(next_update_time)
				{
				}

				uint64 steam_id;
				std::chrono::time_point<std::chrono::system_clock> next_update_time;
			};

			TimerProt();
			~TimerProt() = default;

			void UpdateTimer();

			int update_interval_;
			std::vector<std::shared_ptr<OnlinePlayersData>> online_players_;
	};
}



