#pragma once

#include <Logger/Logger.h>
#include <API/UE/Containers/FString.h>
#include "hdr/sqlite_modern_cpp.h"
#include "json.hpp"
 
namespace NewPlayerProtection
{
	int PlayerUpdateIntervalInMins;
	bool NewPlayersCanDamageOtherTribesStructures;
	int MaxLevel;
	int DaysOfProtection;
	std::chrono::time_point<std::chrono::system_clock>  next_player_update;
	std::chrono::time_point<std::chrono::system_clock>  next_db_update;

	nlohmann::json config;
	sqlite::database& GetDB();

	std::string GetTimestamp(std::chrono::time_point<std::chrono::system_clock> datetime);
	std::chrono::time_point<std::chrono::system_clock> GetDateTime(std::string timestamp);

	class TimerProt
	{
		public:
			static TimerProt& Get();

			TimerProt(const TimerProt&) = delete;
			TimerProt(TimerProt&&) = delete;
			TimerProt& operator=(const TimerProt&) = delete;
			TimerProt& operator=(TimerProt&&) = delete;

			struct OnlinePlayersData
			{
				OnlinePlayersData(uint64 steam_id, uint64 tribe_id, std::chrono::time_point<std::chrono::system_clock> startDateTime, int level,
					int isNewPlayer)
					:
					steam_id(steam_id), tribe_id(tribe_id), startDateTime(startDateTime), level(level), isNewPlayer(isNewPlayer)
				{}
				uint64 steam_id;
				uint64 tribe_id;
				std::chrono::time_point<std::chrono::system_clock> startDateTime;
				int level;
				int isNewPlayer;
			};

			struct AllPlayerData
			{
				AllPlayerData(uint64 steam_id, uint64 tribe_id, std::chrono::time_point<std::chrono::system_clock> startDateTime, int level,
				int isNewPlayer) 
					: 
					steam_id(steam_id), tribe_id(tribe_id), startDateTime(startDateTime), level(level), isNewPlayer(isNewPlayer)
				{}
				uint64 steam_id;
				uint64 tribe_id;
				std::chrono::time_point<std::chrono::system_clock> startDateTime;
				int level;
				int isNewPlayer;
			};

			TimerProt();
			~TimerProt() = default;

			void UpdateTimer();

			int player_update_interval_;
			int db_update_interval_;

			std::vector<std::shared_ptr<OnlinePlayersData>> online_players_;
			std::vector<std::shared_ptr<AllPlayerData>> all_players_;

			void AddOnlinePlayer(uint64 steam_id);
			void AddNewPlayer(uint64 steam_id, uint64 tribe_id);
			void AddPlayer(uint64 steam_id, uint64 tribe_id, std::chrono::time_point<std::chrono::system_clock> startDateTime, int level, int isNewPlayer);
			void RemovePlayer(uint64 steam_id);

			void UpdateLevel(std::shared_ptr <OnlinePlayersData> data);
			void UpdateTribe(std::shared_ptr <OnlinePlayersData> data);

			std::vector<std::shared_ptr<OnlinePlayersData>> GetOnlinePlayers();
			std::vector<std::shared_ptr<AllPlayerData>> GetAllPlayers();
	};
}



