#pragma once

DECLARE_HOOK(APrimalStructure_FinalStructurePlacement, bool, APrimalStructure*, APlayerController*, FVector, FRotator, FRotator, APawn *, FName, bool);
DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*, AShooterCharacter*, bool);
DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);
DECLARE_HOOK(AShooterGameMode_AddNewTribe, uint64, AShooterGameMode *, AShooterPlayerState * PlayerOwner, FString * TribeName, FTribeGovernment * TribeGovernment);
DECLARE_HOOK(AddToTribe, bool, AShooterPlayerState*, FTribeData * MyNewTribe, bool bMergeTribe, bool bForce, bool bIsFromInvite, APlayerController * InviterPC);
DECLARE_HOOK(ServerRequestLeaveTribe_Implementation, void, AShooterPlayerState*);

//need hook for tribe join
//need hook for leave tribe
//need hook for level up

void InitHooks()
{
	ArkApi::GetHooks().SetHook("APrimalStructure.FinalStructurePlacement", &Hook_APrimalStructure_FinalStructurePlacement, reinterpret_cast<LPVOID*>(&APrimalStructure_FinalStructurePlacement_original));
	ArkApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer, &AShooterGameMode_HandleNewPlayer_original);
	ArkApi::GetHooks().SetHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout, &AShooterGameMode_Logout_original);
	ArkApi::GetHooks().SetHook("AShooterGameMode.AddNewTribe", &Hook_AShooterGameMode_AddNewTribe, &AShooterGameMode_AddNewTribe_original);
	ArkApi::GetHooks().SetHook("AShooterPlayerState.AddToTribe", &Hook_AddToTribe, &AddToTribe_original);
	ArkApi::GetHooks().SetHook("AShooterPlayerState.ServerRequestLeaveTribe_Implementation", &Hook_ServerRequestLeaveTribe_Implementation, &ServerRequestLeaveTribe_Implementation_original);
}

void RemoveHooks()
{
	ArkApi::GetHooks().DisableHook("APrimalStructure.FinalStructurePlacement", &Hook_APrimalStructure_FinalStructurePlacement);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.AddNewTribe", &Hook_AShooterGameMode_AddNewTribe);
	ArkApi::GetHooks().DisableHook("AShooterPlayerState.AddToTribe", &Hook_AddToTribe);
	ArkApi::GetHooks().DisableHook("AShooterPlayerState.ServerRequestLeaveTribe_Implementation", &Hook_ServerRequestLeaveTribe_Implementation);
}

bool IsPlayerExists(uint64 steam_id)
{
	auto& db = NewPlayerProtection::GetDB();
	int count = 0;

	try
	{
		db << "SELECT count(1) FROM Players WHERE SteamId = ?;" << steam_id >> count;
	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		return false;
	}
	return count != 0;
}

uint64 GetExpiredPlayersProtection()
{
	auto& db = NewPlayerProtection::GetDB();
	uint64 steam_id = 0;
	std::string prep = "SELECT SteamId FROM Players WHERE Is_New_Player = 1 AND datetime('now', '-";
	prep.append(std::to_string(NewPlayerProtection::DaysOfProtection));
	prep.append(" days') >= datetime(Start_DateTime) ORDER BY Start_DateTime LIMIT 1;");
	//Log::GetLog()->error("({} {}) prep = {}", __FILE__, __FUNCTION__, prep);

	try
	{
		db << prep >> steam_id;
	}
	catch (const sqlite::sqlite_exception& exception)
	{
		//Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}
	return steam_id;
}

uint64 GetExpiredTribesProtection(uint64 steam_id)
{
	auto& db = NewPlayerProtection::GetDB();
	uint64 tribe_id = 0;

	try
	{
		db << "SELECT TribeId FROM Players WHERE SteamId = ?;" << steam_id >> tribe_id;
	}
	catch (const sqlite::sqlite_exception& exception)
	{
		//Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}
	return tribe_id;
}

void SetPlayerLevel(int level, uint64 steam_id)
{
	auto& db = NewPlayerProtection::GetDB();

	try
	{
		db << "UPDATE Players SET Level = ? WHERE SteamId = ?;"
			<< level << steam_id;
	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}
}

void SetTribeId(uint64 tribe_id, uint64 steam_id)
{
	auto& db = NewPlayerProtection::GetDB();

	try
	{
		db << "UPDATE Players SET TribeId = ? WHERE SteamId = ?;"
			<< tribe_id << steam_id;
	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}
}

int IsPlayerProtected(APlayerController * PC)
{
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(PC);
	int isProtected = 0;
	auto& db = NewPlayerProtection::GetDB();

	try
	{
		db << "Select Is_New_Player FROM Players WHERE SteamId = ?;"
			<< steam_id >> isProtected;
	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}
	return isProtected;
}

void DisablePlayerProtection(AShooterPlayerController* _this)
{
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(_this);
	auto& db = NewPlayerProtection::GetDB();

	try
	{
		db << "UPDATE Players SET Is_New_Player = 0 WHERE SteamId = ?;"
			<< steam_id;
	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}

	AShooterPlayerState* ASPS = static_cast<AShooterPlayerState*>(_this->PlayerStateField());

	if (ASPS && ASPS->MyPlayerDataStructField())
	{
		const uint64 team_id = ASPS->TargetingTeamField();
		/**
		* \brief Finds all Structures owned by team
		*/
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
}

void DisableTribeProtection(uint64 tribe_id)
{
	auto& db = NewPlayerProtection::GetDB();

	try
	{
		db << "UPDATE Players SET Is_New_Player = 0 WHERE TribeId = ?;"
			<< tribe_id;
	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}

	const uint64 team_id = tribe_id;
	/**
	* \brief Finds all Structures owned by team
	*/
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

void UpdateProtectionByTribe()
{
	///get all tribes from db that have an unprotected player
	auto& db = NewPlayerProtection::GetDB();


	uint64 tribe_id = 0;

	do
	{
		try
		{
			db << "SELECT DISTINCT TribeId FROM Players WHERE Is_New_Player = 0 AND TribeId IN (SELECT DISTINCT TribeId FROM Players WHERE Is_New_Player = 1);" >> tribe_id;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		DisableTribeProtection(tribe_id);

	} while (tribe_id != 0);
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

	if (!IsPlayerExists(steam_id))
	{
		AActor* pc = static_cast<AActor*>(player_character);
		APlayerState* player_state = static_cast<APlayerState*>(pc);
		AShooterPlayerState* shooter_player_state = static_cast<AShooterPlayerState*>(player_state);

		auto& db = NewPlayerProtection::GetDB();

		AShooterPlayerState* ASPS = static_cast<AShooterPlayerState*>(new_player->PlayerStateField());
		const int team_id = ASPS->TargetingTeamField();

		//get player level and check level against max level to set invulnurability

		try
		{
			db << "INSERT INTO Players (SteamId, TribeId, Start_DateTime, Level, Is_New_Player) VALUES (?,?,DateTime('now'),?,?);"
				<< steam_id << team_id << 0 << 1;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);
		}
	}
	NewPlayerProtection::TimerProt::Get().AddPlayer(steam_id);
	UpdateProtectionByTribe();
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
	SetTribeId(tribeId, steamId);
	UpdateProtectionByTribe();
	return result;
}

uint64 Hook_AShooterGameMode_AddNewTribe(AShooterGameMode * _this, AShooterPlayerState * PlayerOwner, FString * TribeName, FTribeGovernment * TribeGovernment) {
	auto result = AShooterGameMode_AddNewTribe_original(_this, PlayerOwner, TribeName, TribeGovernment);
	SetTribeId(result, ArkApi::GetApiUtils().GetSteamIdFromController(PlayerOwner->GetOwnerController()));
	return result;
}

void Hook_ServerRequestLeaveTribe_Implementation(AShooterPlayerState* player) {
	ServerRequestLeaveTribe_Implementation_original(player);
	uint64 steamId = ArkApi::GetApiUtils().GetSteamIdFromController(player->GetOwnerController());
	SetTribeId(-1, steamId);
}

NewPlayerProtection::TimerProt::TimerProt()
{
	update_interval_ = NewPlayerProtection::ProtectionTimerUpdateIntervalInMin;
	ArkApi::GetCommands().AddOnTimerCallback("UpdateTimer", std::bind(&NewPlayerProtection::TimerProt::UpdateTimer, this));
}

NewPlayerProtection::TimerProt& NewPlayerProtection::TimerProt::Get()
{
	static TimerProt instance;
	return instance;
}

void NewPlayerProtection::TimerProt::AddPlayer(uint64 steam_id)
{
	const auto iter = std::find_if(
		online_players_.begin(), online_players_.end(),
		[steam_id](const std::shared_ptr<OnlinePlayersData>& data) -> bool { return data->steam_id == steam_id; });

	if (iter != online_players_.end())
		return;

	const int interval = update_interval_;
	const auto now = std::chrono::system_clock::now();
	const auto next_time = now + std::chrono::minutes(interval);
	online_players_.push_back(std::make_shared<OnlinePlayersData>(steam_id, next_time));
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

void NewPlayerProtection::TimerProt::UpdateTimer()
{
	const auto now = std::chrono::system_clock::now();

	for (const auto& data : online_players_)
	{
		const auto next_time = data->next_update_time;
		auto diff = std::chrono::duration_cast<std::chrono::minutes>(next_time - now);

		if (diff.count() <= 0)
		{
			data->next_update_time = now + std::chrono::minutes(update_interval_);
			AShooterPlayerController* player = ArkApi::GetApiUtils().FindPlayerFromSteamId(data->steam_id);

			if (ArkApi::IApiUtils::IsPlayerDead(player))
			{
				return;
			}
			
			APlayerState* player_state = player->PlayerStateField();
			AShooterPlayerState* shooter_player_state = static_cast<AShooterPlayerState*>(player_state);
			int level = shooter_player_state->MyPlayerDataStructField()->MyPersistentCharacterStatsField()->CharacterStatusComponent_HighestExtraCharacterLevelField() + 1;
			SetPlayerLevel(level, data->steam_id);

			if (level > NewPlayerProtection::MaxLevel)
			{
				DisablePlayerProtection(player);
				uint64 tribe_id = GetExpiredTribesProtection(data->steam_id);
				DisableTribeProtection(tribe_id);
			}

			uint64 steam_id = GetExpiredPlayersProtection();

			if (steam_id != 0)
			{
				uint64 tribe_id = GetExpiredTribesProtection(steam_id);
				DisableTribeProtection(tribe_id);
			}
			UpdateProtectionByTribe();
		}
	}
}


