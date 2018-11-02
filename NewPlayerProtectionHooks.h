#pragma once

DECLARE_HOOK(APrimalStructure_FinalStructurePlacement, bool, APrimalStructure*, APlayerController*, FVector, FRotator, FRotator, APawn *, FName, bool);
DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*, AShooterCharacter*, bool);
DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);
DECLARE_HOOK(AShooterGameMode_AddNewTribe, uint64, AShooterGameMode *, AShooterPlayerState * PlayerOwner, FString * TribeName, FTribeGovernment * TribeGovernment);
DECLARE_HOOK(AddToTribe, bool, AShooterPlayerState*, FTribeData * MyNewTribe, bool bMergeTribe, bool bForce, bool bIsFromInvite, APlayerController * InviterPC);
DECLARE_HOOK(ServerRequestLeaveTribe_Implementation, void, AShooterPlayerState*);
DECLARE_HOOK(AShooterGameMode_SaveWorld, bool, AShooterGameMode*);

void InitHooks()
{
	ArkApi::GetHooks().SetHook("APrimalStructure.FinalStructurePlacement", &Hook_APrimalStructure_FinalStructurePlacement, reinterpret_cast<LPVOID*>(&APrimalStructure_FinalStructurePlacement_original));
	ArkApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer, &AShooterGameMode_HandleNewPlayer_original);
	ArkApi::GetHooks().SetHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout, &AShooterGameMode_Logout_original);
	ArkApi::GetHooks().SetHook("AShooterGameMode.AddNewTribe", &Hook_AShooterGameMode_AddNewTribe, &AShooterGameMode_AddNewTribe_original);
	ArkApi::GetHooks().SetHook("AShooterPlayerState.AddToTribe", &Hook_AddToTribe, &AddToTribe_original);
	ArkApi::GetHooks().SetHook("AShooterPlayerState.ServerRequestLeaveTribe_Implementation", &Hook_ServerRequestLeaveTribe_Implementation, &ServerRequestLeaveTribe_Implementation_original);
	ArkApi::GetHooks().SetHook("AShooterGameMode.SaveWorld", &Hook_AShooterGameMode_SaveWorld, &AShooterGameMode_SaveWorld_original);
}

void RemoveHooks()
{
	ArkApi::GetHooks().DisableHook("APrimalStructure.FinalStructurePlacement", &Hook_APrimalStructure_FinalStructurePlacement);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.AddNewTribe", &Hook_AShooterGameMode_AddNewTribe);
	ArkApi::GetHooks().DisableHook("AShooterPlayerState.AddToTribe", &Hook_AddToTribe);
	ArkApi::GetHooks().DisableHook("AShooterPlayerState.ServerRequestLeaveTribe_Implementation", &Hook_ServerRequestLeaveTribe_Implementation);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.SaveWorld", &Hook_AShooterGameMode_SaveWorld);
}

bool IsPlayerExists(uint64 steam_id)
{
	int exists = 0;
	auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();
	for (const auto& data : all_players_)
	{
		if (data->steam_id == steam_id)
		{
			return 1;
		}
	}
	return exists;
}

void DisableTribeProtection(uint64 tribe_id)
{
	const uint64 team_id = tribe_id;

	//brief Finds all Structures owned by team
	TArray<AActor*> AllStructures;
	UGameplayStatics::GetAllActorsOfClass(reinterpret_cast<UObject*>(ArkApi::GetApiUtils().GetWorld()), APrimalStructure::GetPrivateStaticClass(), &AllStructures);
	TArray<APrimalStructure*> FoundStructures;
	APrimalStructure* Struc;

	for (AActor* StructActor : AllStructures)
	{
		if (!StructActor || (uint64)StructActor->TargetingTeamField() != team_id) continue;
		Struc = static_cast<APrimalStructure*>(StructActor);
		FoundStructures.Add(Struc);
	}

	for (APrimalStructure* st : FoundStructures)
	{
		st->bCanBeDamaged() = true;;
	}
}

void RemoveExpiredTribesProtection()
{
	auto protectionDaysInHours = std::chrono::hours(24  * NewPlayerProtection::DaysOfProtection);
	auto now = std::chrono::system_clock::now();
	auto endTime = now - protectionDaysInHours;
	auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();
	auto online_players_ = NewPlayerProtection::TimerProt::Get().GetOnlinePlayers();

	for (const auto& allData : all_players_)
	{
		//check all players for expired
		auto diff = std::chrono::duration_cast<std::chrono::seconds>(allData->startDateTime - endTime);

		if (diff.count() <= 0 || allData->level >= NewPlayerProtection::MaxLevel)
		{
			allData->isNewPlayer = 0;

			DisableTribeProtection(allData->tribe_id);

			//update all_players protection with same tribe id
			for (const auto& moreAllData : all_players_)
			{
				if (allData->tribe_id == moreAllData->tribe_id)
				{
					moreAllData->isNewPlayer = 0;
				}
			}

			//update online players with same steam id and tribe id
			for (const auto& onlineData : online_players_)
			{
				if (allData->steam_id == onlineData->steam_id)
				{
					onlineData->isNewPlayer = 0;
				}
				if (allData->tribe_id == onlineData->tribe_id)
				{
					onlineData->isNewPlayer = 0;
				}
			}
		}
	}
}

int IsPlayerProtected(APlayerController * PC)
{
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(PC);
	int isProtected = 0;
	auto online_players_ = NewPlayerProtection::TimerProt::Get().GetOnlinePlayers();
	for (const auto& data : online_players_)
	{
		if (data->steam_id == steam_id)
		{
			return data->isNewPlayer;
		}
	}
	return isProtected;
}

uint64 GetMaxUnknownTribeId()
{
	uint64 tribeid = 0;
	auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();
	std::vector<uint64> tribe_ids;

	for (const auto& data : all_players_)
	{
		if (data->tribe_id < 999999)
		{
			if (tribeid < data->tribe_id)
				tribeid = data->tribe_id;
		}
	}
	return tribeid+1;
}

void UpdateDB(std::shared_ptr<NewPlayerProtection::TimerProt::AllPlayerData> data)
{
	auto& db = NewPlayerProtection::GetDB();

	try
	{
		db << "INSERT OR REPLACE INTO Players(SteamId, TribeId, Start_DateTime, Level, Is_New_Player) VALUES(?,?,?,?,?);"
			<< data->steam_id << data->tribe_id << NewPlayerProtection::GetTimestamp(data->startDateTime) << data->level << data->isNewPlayer;

	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}
}

bool _cdecl Hook_APrimalStructure_FinalStructurePlacement(APrimalStructure* _this, APlayerController * PC, FVector AtLocation, FRotator AtRotation, FRotator PlayerViewRotation, APawn * AttachToPawn, FName BoneName, bool bIsFlipped)
{
	if (IsPlayerProtected(PC))
	{
		_this->bCanBeDamaged() = false;
	}
	return APrimalStructure_FinalStructurePlacement_original(_this, PC, AtLocation, AtRotation, PlayerViewRotation, AttachToPawn, BoneName, bIsFlipped);
}

bool Hook_AShooterGameMode_HandleNewPlayer(AShooterGameMode* _this, AShooterPlayerController* new_player, UPrimalPlayerData* player_data, AShooterCharacter* player_character, bool is_from_login)
{
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(new_player);
	uint64 team_id = 0;

	if (!IsPlayerExists(steam_id))
	{
		AActor* pc = static_cast<AActor*>(player_character);
		APlayerState* player_state = static_cast<APlayerState*>(pc);
		AShooterPlayerState* shooter_player_state = static_cast<AShooterPlayerState*>(player_state);

		auto& db = NewPlayerProtection::GetDB();

		AShooterPlayerState* ASPS = static_cast<AShooterPlayerState*>(new_player->PlayerStateField());
		team_id = ASPS->TargetingTeamField();

		if (team_id == 0)
		{
			team_id = GetMaxUnknownTribeId();

			if (team_id == 0)
				team_id = 100000;
		}

		NewPlayerProtection::TimerProt::Get().AddNewPlayer(steam_id, team_id);
	}

	NewPlayerProtection::TimerProt::Get().AddOnlinePlayer(steam_id);
	return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);
}

void Hook_AShooterGameMode_Logout(AShooterGameMode* _this, AController* exiting)
{
	// Remove player from the online list
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(exiting);
	NewPlayerProtection::TimerProt::TimerProt::Get().RemovePlayer(steam_id);
	AShooterGameMode_Logout_original(_this, exiting);
}

bool Hook_AddToTribe(AShooterPlayerState* player, FTribeData * MyNewTribe, bool bMergeTribe, bool bForce, bool bIsFromInvite, APlayerController * InviterPC) {
	bool result = AddToTribe_original(player, MyNewTribe, bMergeTribe, bForce, bIsFromInvite, InviterPC);		
	uint64 tribeId = MyNewTribe->TribeIDField();
	uint64 steamId = ArkApi::GetApiUtils().GetSteamIdFromController(player->GetOwnerController());

	auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();
	for (const auto& data : all_players_)
	{
		if (data->steam_id == steamId)
		{
			data->tribe_id = tribeId;
			break;
		}
	}

	auto online_players_ = NewPlayerProtection::TimerProt::Get().GetOnlinePlayers();
	for (const auto& data : online_players_)
	{
		if (data->steam_id == steamId)
		{
			data->tribe_id = tribeId;
			break;
		}
	}
	return result;
}

uint64 Hook_AShooterGameMode_AddNewTribe(AShooterGameMode * _this, AShooterPlayerState * PlayerOwner, FString * TribeName, FTribeGovernment * TribeGovernment) {
	auto result = AShooterGameMode_AddNewTribe_original(_this, PlayerOwner, TribeName, TribeGovernment);

	uint64 steamId = ArkApi::GetApiUtils().GetSteamIdFromController(PlayerOwner->GetOwnerController());
	auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();

	for (const auto& data : all_players_)
	{
		if (data->steam_id == steamId)
		{
			data->tribe_id = result;
			break;
		}
	}

	auto online_players_ = NewPlayerProtection::TimerProt::Get().GetOnlinePlayers();

	for (const auto& data : online_players_)
	{
		if (data->steam_id == steamId)
		{
			data->tribe_id = result;
			break;
		}
	}
	return result;
}

void Hook_ServerRequestLeaveTribe_Implementation(AShooterPlayerState* player) {
	ServerRequestLeaveTribe_Implementation_original(player);

	uint64 steamId = ArkApi::GetApiUtils().GetSteamIdFromController(player->GetOwnerController());

	uint64 tribe_id = player->TargetingTeamField();

	if(tribe_id == 0)
		tribe_id = GetMaxUnknownTribeId();

	auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();

	for (const auto& data : all_players_)
	{
		if (data->steam_id == steamId)
		{
			data->tribe_id = tribe_id;
			break;
		}
	}

	auto online_players_ = NewPlayerProtection::TimerProt::Get().GetOnlinePlayers();

	for (const auto& data : online_players_)
	{
		if (data->steam_id == steamId)
		{
			data->tribe_id = tribe_id;
			break;
		}
	}
}

bool Hook_AShooterGameMode_SaveWorld(AShooterGameMode* GameMode) {
	bool result = AShooterGameMode_SaveWorld_original(GameMode);
	auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();
	for (const auto& data : all_players_)
	{
		UpdateDB(data);
	}
	return result;
}

NewPlayerProtection::TimerProt::TimerProt()
{
	player_update_interval_ = NewPlayerProtection::PlayerUpdateIntervalInMins;
	ArkApi::GetCommands().AddOnTimerCallback("UpdateTimer", std::bind(&NewPlayerProtection::TimerProt::UpdateTimer, this));
}

NewPlayerProtection::TimerProt& NewPlayerProtection::TimerProt::Get()
{
	static TimerProt instance;
	return instance;
}

void NewPlayerProtection::TimerProt::AddPlayer(uint64 steam_id, uint64 tribe_id, std::chrono::time_point<std::chrono::system_clock> startDateTime, int level, int isNewPlayer)
{
	const auto iter = std::find_if(
		all_players_.begin(), all_players_.end(),
		[steam_id](const std::shared_ptr<AllPlayerData>& data) -> bool { return data->steam_id == steam_id; });

	if (iter != all_players_.end())
		return;

	all_players_.push_back(std::make_shared<AllPlayerData>(steam_id, tribe_id, startDateTime, level, isNewPlayer));
}

void NewPlayerProtection::TimerProt::AddNewPlayer(uint64 steam_id, uint64 tribe_id)
{
	const auto iter = std::find_if(
		all_players_.begin(), all_players_.end(),
		[steam_id](const std::shared_ptr<AllPlayerData>& data) -> bool { return data->steam_id == steam_id; });

	if (iter != all_players_.end())
		return;

	all_players_.push_back(std::make_shared<AllPlayerData>(steam_id, tribe_id, std::chrono::system_clock::now(), 1, 1));
}

void NewPlayerProtection::TimerProt::AddOnlinePlayer(uint64 steam_id)
{
	const auto iter = std::find_if(
		online_players_.begin(), online_players_.end(),
		[steam_id](const std::shared_ptr<OnlinePlayersData>& data) -> bool { return data->steam_id == steam_id; });

	if (iter != online_players_.end())
		return;

	uint64 tribeid;
	std::chrono::time_point<std::chrono::system_clock> startDateTime;
	int level;
	int isNewPlayer;

	for (const auto& alldata : all_players_)
	{
		if (alldata->steam_id == steam_id)
		{
			tribeid = alldata->tribe_id;
			startDateTime = alldata->startDateTime;
			level = alldata->level;
			isNewPlayer = alldata->isNewPlayer;
			alldata->level = level;
			break;
		}
	}

	online_players_.push_back(std::make_shared<OnlinePlayersData>(steam_id, tribeid, startDateTime, level, isNewPlayer));
}

void NewPlayerProtection::TimerProt::RemovePlayer(uint64 steam_id)
{
	const auto iter = std::find_if(
		online_players_.begin(), online_players_.end(),
		[steam_id](const std::shared_ptr<OnlinePlayersData>& data) -> bool { return data->steam_id == steam_id; });

	if (iter != online_players_.end())
	{
		online_players_.erase(std::remove(online_players_.begin(), online_players_.end(), *iter), online_players_.end());
	}
}

void NewPlayerProtection::TimerProt::UpdateLevel(std::shared_ptr<OnlinePlayersData> data)
{
	AShooterPlayerController* player = ArkApi::GetApiUtils().FindPlayerFromSteamId(data->steam_id);

	if (ArkApi::IApiUtils::IsPlayerDead(player))
	{
		return;
	}

	APlayerState* player_state = player->PlayerStateField();
	AShooterPlayerState* shooter_player_state = static_cast<AShooterPlayerState*>(player_state);
	int level = shooter_player_state->MyPlayerDataStructField()->MyPersistentCharacterStatsField()->CharacterStatusComponent_HighestExtraCharacterLevelField() + 1;
	data->level = level;

	for (const auto& alldata : all_players_)
	{
		if (alldata->steam_id == data->steam_id)
		{
			alldata->level = level;
			break;
		}
	}
}

void NewPlayerProtection::TimerProt::UpdateTribe(std::shared_ptr<OnlinePlayersData> data)
{
	AShooterPlayerController* player = ArkApi::GetApiUtils().FindPlayerFromSteamId(data->steam_id);

	if (ArkApi::IApiUtils::IsPlayerDead(player))
	{
		return;
	}

	APlayerState* player_state = player->PlayerStateField();
	AShooterPlayerState* shooter_player_state = static_cast<AShooterPlayerState*>(player_state);
	uint64 tribe_id = shooter_player_state->TargetingTeamField();
	data->tribe_id = tribe_id;


	for (const auto& alldata : all_players_)
	{
		if (alldata->steam_id == data->steam_id)
		{
			alldata->tribe_id = tribe_id;
			break;
		}
	}
}

std::vector<std::shared_ptr<NewPlayerProtection::TimerProt::OnlinePlayersData>> NewPlayerProtection::TimerProt::GetOnlinePlayers()
{
	return online_players_;
}

std::vector<std::shared_ptr<NewPlayerProtection::TimerProt::AllPlayerData>> NewPlayerProtection::TimerProt::GetAllPlayers()
{
	return all_players_;
}

void NewPlayerProtection::TimerProt::UpdateTimer()
{
	const auto now_time = std::chrono::system_clock::now();

	auto diff = std::chrono::duration_cast<std::chrono::seconds>(NewPlayerProtection::next_player_update - now_time);

	if (diff.count() <= 0)
	{
		auto player_interval = std::chrono::minutes(player_update_interval_);
		NewPlayerProtection::next_player_update = now_time + player_interval;

		for (const auto& data : online_players_)
		{
			AShooterPlayerController* player = ArkApi::GetApiUtils().FindPlayerFromSteamId(data->steam_id);

			NewPlayerProtection::TimerProt::UpdateTribe(data);
			NewPlayerProtection::TimerProt::UpdateLevel(data);
		}
		RemoveExpiredTribesProtection();
	}
}


