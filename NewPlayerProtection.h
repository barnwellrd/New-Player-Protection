#pragma once

#include <Logger/Logger.h>
#include <API/UE/Containers/FString.h>
#include "hdr/sqlite_modern_cpp.h"
#include "json.hpp"
 
namespace NewPlayerProtection
{
	int PlayerUpdateIntervalInMins;
	bool IgnoreAdmins;
	bool AllowNewPlayersToDamageEnemyStructures;
	bool AllowPlayersToDisableOwnedTribeProtection;
	bool AllowWildCorruptedDinoDamage;
	bool AllowWildDinoDamage;

	int NPPPlayerDecayInHours;
	FString NPPCommandPrefix;
	FString NPPAdminGroup;

	FString NewPlayerDoingDamageMessage;
	FString NewPlayerStructureTakingDamageMessage;
	FString NewPlayerStructureTakingDamageFromUnknownTribemateMessage;

	FString NPPRemainingMessage;
	FString NPPInfoMessage;
	FString NPPInvalidCommand;
	FString NewPlayerProtectionDisableSuccess;
	FString NotANewPlayerMessage;
	FString NotTribeAdminMessage;
	FString TribeIDText;
	FString NoStructureForTribeIDText;
	FString PVEDisablePlayerMessage;
	FString PVEStatusMessage;
	FString NotAStructureMessage;

	FString AdminNoTribeExistsMessage;
	FString AdminTribeProtectionRemoved;
	FString AdminTribeNotUnderProtection;
	FString AdminResetTribeProtectionSuccess;
	FString AdminResetTribeProtectionLvlFailure;
	FString AdminPVETribeAddedSuccessMessage;
	FString AdminPVETribeAlreadyAddedMessage;
	FString AdminPVETribeRemovedSuccessMessage;
	FString AdminPVETribeAlreadyRemovedMessage;

	int MessageIntervalInSecs;
	float MessageTextSize;
	float MessageDisplayDelay;
	FLinearColor  MessageColor;

	int MaxLevel;
	int HoursOfProtection;

	std::vector<std::string> StructureExemptions;

	std::chrono::time_point<std::chrono::system_clock>  next_player_update;
	std::chrono::time_point<std::chrono::system_clock>  next_db_update;

	nlohmann::json config;
	nlohmann::json TempConfig;

	sqlite::database& GetDB();

	std::string GetTimestamp(std::chrono::time_point<std::chrono::system_clock> datetime);
	std::chrono::time_point<std::chrono::system_clock> GetDateTime(std::string timestamp);

	std::vector<uint64> pveTribesList;
	std::vector<uint64> removedPveTribesList;

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
				OnlinePlayersData(uint64 steam_id, uint64 tribe_id, std::chrono::time_point<std::chrono::system_clock> startDateTime, 
					std::chrono::time_point<std::chrono::system_clock> lastLoginDateTime, int level,
					int isNewPlayer, std::chrono::time_point<std::chrono::system_clock> nextMessageTime)
					:
					steam_id(steam_id), tribe_id(tribe_id), startDateTime(startDateTime), lastLoginDateTime(lastLoginDateTime),
					level(level), isNewPlayer(isNewPlayer), nextMessageTime(nextMessageTime)
				{}
				uint64 steam_id;
				uint64 tribe_id;
				std::chrono::time_point<std::chrono::system_clock> startDateTime;
				std::chrono::time_point<std::chrono::system_clock> lastLoginDateTime;
				int level;
				int isNewPlayer;
				std::chrono::time_point<std::chrono::system_clock> nextMessageTime;
			};

			struct AllPlayerData
			{
				AllPlayerData(uint64 steam_id, uint64 tribe_id, std::chrono::time_point<std::chrono::system_clock> startDateTime, 
					std::chrono::time_point<std::chrono::system_clock> lastLoginDateTime, int level, int isNewPlayer)
					: 
					steam_id(steam_id), tribe_id(tribe_id), startDateTime(startDateTime), lastLoginDateTime(lastLoginDateTime), 
					level(level), isNewPlayer(isNewPlayer)
				{}
				uint64 steam_id;
				uint64 tribe_id;
				std::chrono::time_point<std::chrono::system_clock> startDateTime;
				std::chrono::time_point<std::chrono::system_clock> lastLoginDateTime;
				int level;
				int isNewPlayer;
			};

			TimerProt();
			~TimerProt() = default;

			void UpdateTimer();

			int player_update_interval_;

			std::vector<std::shared_ptr<OnlinePlayersData>> online_players_;
			std::vector<std::shared_ptr<AllPlayerData>> all_players_;

			void AddOnlinePlayer(uint64 steam_id, uint64 team_id);
			void AddNewPlayer(uint64 steam_id, uint64 tribe_id);
			void AddPlayerFromDB(uint64 steam_id, uint64 tribe_id, std::chrono::time_point<std::chrono::system_clock> startDateTime, std::chrono::time_point<std::chrono::system_clock> lastLoginDateTime, int level, int isNewPlayer);
			void RemovePlayer(uint64 steam_id);
			bool IsNextMessageReady(uint64 steam_id);

			void UpdateLevelAndTribe(std::shared_ptr <OnlinePlayersData> data);

			std::vector<std::shared_ptr<OnlinePlayersData>> GetOnlinePlayers();
			std::vector<std::shared_ptr<AllPlayerData>> GetAllPlayers();
	};

	FString GetBlueprint(UObjectBase* object)
	{

		if (object != nullptr && object->ClassField() != nullptr)
		{
			FString path_name;
			object->ClassField()->GetDefaultObject(true)->GetFullName(&path_name, nullptr);

			if (int find_index = 0; path_name.FindChar(' ', find_index))
			{
				path_name = "Blueprint'" + path_name.Mid(find_index + 1,
					path_name.Len() - (find_index + (path_name.EndsWith(
						"_C", ESearchCase::
						CaseSensitive)
						? 3
						: 1))) + "'";

				return path_name.Replace(L"Default__", L"", ESearchCase::CaseSensitive);
			}
		}
		return FString("");
	}

}



