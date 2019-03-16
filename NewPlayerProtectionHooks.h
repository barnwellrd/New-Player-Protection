#pragma once

DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*, AShooterCharacter*, bool);
DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);
DECLARE_HOOK(AShooterGameMode_SaveWorld, bool, AShooterGameMode*);
DECLARE_HOOK(APrimalStructure_TakeDamage, float, APrimalStructure*, float, FDamageEvent*, AController*, AActor*);

void InitHooks()
{
	ArkApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer, &AShooterGameMode_HandleNewPlayer_original);
	ArkApi::GetHooks().SetHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout, &AShooterGameMode_Logout_original);
	ArkApi::GetHooks().SetHook("AShooterGameMode.SaveWorld", &Hook_AShooterGameMode_SaveWorld, &AShooterGameMode_SaveWorld_original);
	ArkApi::GetHooks().SetHook("APrimalStructure.TakeDamage", &Hook_APrimalStructure_TakeDamage, &APrimalStructure_TakeDamage_original);
}

void RemoveHooks()
{
	ArkApi::GetHooks().DisableHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.SaveWorld", &Hook_AShooterGameMode_SaveWorld);
	ArkApi::GetHooks().DisableHook("APrimalStructure.TakeDamage", &Hook_APrimalStructure_TakeDamage);
}

bool IsAdmin(uint64 steam_id)
{
	return Permissions::IsPlayerInGroup(steam_id, "Admins");
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

bool IsTribeProtected(uint64 tribeid)
{
	int isProtected = 0;
	if (tribeid < 100000)
	{
		return isProtected;
	}
	auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();
	for (const auto& data : all_players_)
	{
		if (data->tribe_id == tribeid && data->isNewPlayer == 1 && !IsAdmin(data->steam_id))
		{
			return 1;
		}
	}
	return isProtected;
}

void RemoveExpiredTribesProtection()
{
	auto protectionDaysInHours = std::chrono::hours(NewPlayerProtection::HoursOfProtection);
	auto now = std::chrono::system_clock::now();
	auto endTime = now - protectionDaysInHours;
	auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();
	auto online_players_ = NewPlayerProtection::TimerProt::Get().GetOnlinePlayers();

	for (const auto& allData : all_players_)
	{
		//check all players for expired
		auto diff = std::chrono::duration_cast<std::chrono::seconds>(allData->startDateTime - endTime);

		if ((diff.count() <= 0 || allData->level >= NewPlayerProtection::MaxLevel || allData->isNewPlayer == 0))
		{
			allData->isNewPlayer = 0;
			//if not an admin
			if (!IsAdmin(allData->steam_id))
			{
				//update all_players protection with same tribe id
				for (const auto& moreAllData : all_players_)
				{
					if (allData->tribe_id == moreAllData->tribe_id)
					{
						if (IsAdmin(moreAllData->steam_id))
						{
							continue;
						}
						else
						{
							moreAllData->isNewPlayer = 0;
						}
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
						if (!IsAdmin(onlineData->steam_id))
						{
							onlineData->isNewPlayer = 0;
						}
					}
				}
			}
			else	//is admin, turn admin prot off
			{
				for (const auto& onlineData : online_players_)
				{
					if (allData->steam_id == onlineData->steam_id)
					{
						onlineData->isNewPlayer = 0;
					}
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
		if (data->tribe_id < 99999)
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

bool Hook_AShooterGameMode_SaveWorld(AShooterGameMode* GameMode) {
	bool result = AShooterGameMode_SaveWorld_original(GameMode);
	auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();
	for (const auto& data : all_players_)
	{
		UpdateDB(data);
	}
	return result;
}

float Hook_APrimalStructure_TakeDamage(APrimalStructure* _this, float Damage, FDamageEvent* DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (_this) // APrimalStructure != NULL
	{
		uint64 attacked_tribeid = _this->TargetingTeamField();

		if (DamageCauser) //DamageCauser != NULL
		{
			uint64 attacking_tribeid = DamageCauser->TargetingTeamField();

			if (EventInstigator && EventInstigator->IsA(APlayerController::GetPrivateStaticClass())) //EventInstigator != NULL
			{
				uint64 steam_id = ArkApi::GetApiUtils().GetSteamIdFromController(EventInstigator);
				AShooterPlayerController* player = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
				
				if (IsAdmin(steam_id))
				{
					return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
				}

				if (IsPlayerProtected(player))
				{
					if (!NewPlayerProtection::AllowNewPlayersToDamageEnemyStructures)
					{
						if (attacked_tribeid < 100000 || attacked_tribeid == attacking_tribeid)
						{
							return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
						}
						if (NewPlayerProtection::TimerProt::Get().IsNextMessageReady(steam_id))
						{
							ArkApi::GetApiUtils().SendNotification(player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr, *NewPlayerProtection::NewPlayerDoingDamageMessage);
						}
						return 0;
					}
				}
				else
				{
					if (IsTribeProtected(attacked_tribeid))
					{
						FString* name = nullptr;
						_this->GetDescriptiveName(name);
						if (NewPlayerProtection::TimerProt::Get().IsNextMessageReady(steam_id) && !name->Contains("C4"))
						{
							ArkApi::GetApiUtils().SendNotification(player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr, *NewPlayerProtection::NewPlayerStructureTakingDamageMessage);
						}
						return 0;
					}
				}
			}
			else //EventInstigator == NULL
			{
				if (attacked_tribeid == attacking_tribeid)
				{
					return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
				}
				if (IsTribeProtected(attacked_tribeid))
				{
					auto online_players_ = NewPlayerProtection::TimerProt::Get().GetOnlinePlayers();

					for (const auto& onlineData : online_players_)
					{
						if (onlineData->tribe_id == attacking_tribeid)
						{
							if (NewPlayerProtection::TimerProt::Get().IsNextMessageReady(onlineData->steam_id))
							{
								auto tribe_player = ArkApi::GetApiUtils().FindPlayerFromSteamId(onlineData->steam_id);
								if (!ArkApi::GetApiUtils().IsPlayerDead(tribe_player))
								{
									ArkApi::GetApiUtils().SendNotification(tribe_player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr, *NewPlayerProtection::NewPlayerStructureTakingDamageFromUnknownTribemateMessage);
								}
							}
						}
					}
					return 0;
				}
				if (IsTribeProtected(attacking_tribeid) && !NewPlayerProtection::AllowNewPlayersToDamageEnemyStructures)
				{
					return 0;
				}
			}
		}
		else //DamageCauser == NULL
		{
			if (IsTribeProtected(attacked_tribeid))
			{
				return 0;
			}
		}
	}	
	return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
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

void NewPlayerProtection::TimerProt::AddPlayerFromDB(uint64 steam_id, uint64 tribe_id, std::chrono::time_point<std::chrono::system_clock> startDateTime, int level, int isNewPlayer)
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
	bool isNewPlayer = !IsAdmin(steam_id);
	all_players_.push_back(std::make_shared<AllPlayerData>(steam_id, tribe_id, std::chrono::system_clock::now(), 1, isNewPlayer));
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
	std::chrono::time_point<std::chrono::system_clock> nextMessageTime = std::chrono::system_clock::now();

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

	online_players_.push_back(std::make_shared<OnlinePlayersData>(steam_id, tribeid, startDateTime, level, isNewPlayer, nextMessageTime));
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

bool NewPlayerProtection::TimerProt::IsNextMessageReady(uint64 steam_id)
{
	for (const auto& data : online_players_)
	{
		if (data->steam_id == steam_id)
		{
			const auto now_time = std::chrono::system_clock::now();
			auto diff = std::chrono::duration_cast<std::chrono::seconds>(data->nextMessageTime - now_time);

			if (diff.count() <= 0)
			{
				data->nextMessageTime = now_time + std::chrono::seconds(NewPlayerProtection::MessageIntervalInSecs);
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	return true;
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