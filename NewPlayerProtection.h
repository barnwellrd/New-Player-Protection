#pragma once

#include <Logger/Logger.h>
#include <API/UE/Containers/FString.h>
#include "hdr/sqlite_modern_cpp.h"
#include "json.hpp"
 
namespace NewPlayerProtection
{
	int PlayerUpdateIntervalInMins;
	bool AllowNewPlayersToDamageEnemyStructures;
	bool AllowPlayersToDisableOwnedTribeProtection;
	FString NPPCommandPrefix;
	FString NewPlayerDoingDamageMessage;
	FString NewPlayerStructureTakingDamageMessage;
	FString NewPlayerStructureTakingDamageFromUnknownTribemateMessage;
	FString NPPInvalidCommand;
	FString NewPlayerProtectionDisableSuccess;
	FString NotANewPlayerMessage;
	FString NotTribeAdminMessage;
	FString NPPRemainingMessage;
	FString AdminNoTribeExistsMessage;
	FString AdminTribeProtectionRemoved;
	FString AdminTribeNotUnderProtection;
	FString AdminResetTribeProtectionSuccess;
	FString AdminResetTribeProtectionLvlFailure;
	FString NPPInfoMessage;
	int MessageIntervalInSecs;
	float MessageTextSize;
	float MessageDisplayDelay;
	FLinearColor  MessageColor;
	int MaxLevel;
	int HoursOfProtection;
	std::chrono::time_point<std::chrono::system_clock>  next_player_update;
	std::chrono::time_point<std::chrono::system_clock>  next_db_update;

	nlohmann::json config, TempConfig;
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
					int isNewPlayer, std::chrono::time_point<std::chrono::system_clock> nextMessageTime)
					:
					steam_id(steam_id), tribe_id(tribe_id), startDateTime(startDateTime), level(level), isNewPlayer(isNewPlayer), nextMessageTime(nextMessageTime)
				{}
				uint64 steam_id;
				uint64 tribe_id;
				std::chrono::time_point<std::chrono::system_clock> startDateTime;
				int level;
				int isNewPlayer;
				std::chrono::time_point<std::chrono::system_clock> nextMessageTime;

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
			void AddPlayerFromDB(uint64 steam_id, uint64 tribe_id, std::chrono::time_point<std::chrono::system_clock> startDateTime, int level, int isNewPlayer);
			void RemovePlayer(uint64 steam_id);
			bool IsNextMessageReady(uint64 steam_id);

			void UpdateLevel(std::shared_ptr <OnlinePlayersData> data);
			void UpdateTribe(std::shared_ptr <OnlinePlayersData> data);

			std::vector<std::shared_ptr<OnlinePlayersData>> GetOnlinePlayers();
			std::vector<std::shared_ptr<AllPlayerData>> GetAllPlayers();
	};
}



