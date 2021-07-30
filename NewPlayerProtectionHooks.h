#pragma once

DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*, AShooterCharacter*, bool);
DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);
DECLARE_HOOK(AShooterGameMode_SaveWorld, bool, AShooterGameMode*);
DECLARE_HOOK(APrimalStructure_TakeDamage, float, APrimalStructure*, float, FDamageEvent*, AController*, AActor*);

void InitHooks() {
	ArkApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer, 
		&AShooterGameMode_HandleNewPlayer_original);
	ArkApi::GetHooks().SetHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout, &AShooterGameMode_Logout_original);
	ArkApi::GetHooks().SetHook("AShooterGameMode.SaveWorld", &Hook_AShooterGameMode_SaveWorld, &AShooterGameMode_SaveWorld_original);
	ArkApi::GetHooks().SetHook("APrimalStructure.TakeDamage", &Hook_APrimalStructure_TakeDamage, &APrimalStructure_TakeDamage_original);
}

void RemoveHooks() {
	ArkApi::GetHooks().DisableHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.SaveWorld", &Hook_AShooterGameMode_SaveWorld);
	ArkApi::GetHooks().DisableHook("APrimalStructure.TakeDamage", &Hook_APrimalStructure_TakeDamage);
}

bool IsPlayerExists(uint64 steam_id) {
	int exists = 0;
	auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();

	const auto iter = std::find_if(
		all_players_.begin(), all_players_.end(),
		[steam_id](const std::shared_ptr<NPP::TimerProt::AllPlayerData>& data) {
			return data->steam_id == steam_id;
		});

	if (iter != all_players_.end()) {
		exists = 1;
	}
	return exists;
}

bool IsPVETribe(uint64 tribeid) {
	return (std::count(NPP::pveTribesList.begin(), NPP::pveTribesList.end(), tribeid) > 0);
}

bool IsTribeProtected(uint64 tribeid) {
	if (tribeid > 100000) {
		//check a vector of tribes that lists protected tribes
		return IsPVETribe(tribeid) ? true : (std::count(NPP::nppTribesList.begin(), NPP::nppTribesList.end(), tribeid) > 0);
	}
	return false;
}

bool IsExemptStructure(AActor* actor) {
	if (NPP::StructureExemptions.size() > 0) {
		APrimalStructure* structure = static_cast<APrimalStructure*>(actor);
		FString stuctPath;
		stuctPath = NPP::GetBlueprint(structure);

		if (std::count(NPP::StructureExemptions.begin(), NPP::StructureExemptions.end(), stuctPath.ToString())) {
			return true;
		}
	}
	return false;
}

void RemoveExpiredTribesProtection() {
	auto protectionInHours = std::chrono::hours(NPP::HoursOfProtection);
	auto now = std::chrono::system_clock::now();
	auto expireTime = now - protectionInHours;
	auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();
	auto online_players_ = NPP::TimerProt::Get().GetOnlinePlayers();

	for (const auto& allData : all_players_) {
		//check all players for expired protection
		auto diff = std::chrono::duration_cast<std::chrono::seconds>(allData->startDateTime - expireTime);

		if (diff.count() <= 0 || allData->level >= NPP::MaxLevel || allData->isNewPlayer == 0) {
			//if not an admin
			if (!IsAdmin(allData->steam_id)) {
				allData->isNewPlayer = 0;

				//update all_players protection with same tribe id
				for (const auto& moreAllData : all_players_) {
					if (allData->tribe_id == moreAllData->tribe_id) {
						if (!IsAdmin(moreAllData->steam_id)) {
							moreAllData->isNewPlayer = 0;
							uint64 tribe_id = moreAllData->tribe_id;

							//remove from protected tribes list if found
							if (std::count(NPP::nppTribesList.begin(), NPP::nppTribesList.end(), moreAllData->tribe_id) > 0) {
								const auto iter = std::find_if(
									NPP::nppTribesList.begin(), NPP::nppTribesList.end(),
									[tribe_id](const uint64 data) {
									return data == tribe_id;
								});

								if (iter != NPP::nppTribesList.end()) {
									NPP::nppTribesList.erase(std::remove(NPP::nppTribesList.begin(),
										NPP::nppTribesList.end(), *iter), NPP::nppTribesList.end());
								}
							}
						}
					}
				}

				//update online players with same steam id and tribe id
				for (const auto& onlineData : online_players_) {
					if (allData->steam_id == onlineData->steam_id || allData->tribe_id == onlineData->tribe_id) {
						if (!IsAdmin(onlineData->steam_id)) {
							onlineData->isNewPlayer = 0;
						}
					}
				}
			}
		}
	}
}

bool IsPlayerProtected(APlayerController * PC) {
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(PC);
	// If not admin, return if in a protected tribe, if admin, return unprotected
	return !IsAdmin(steam_id) ? IsTribeProtected(PC->TargetingTeamField()) : false;
}

void UpdatePlayerDB(std::shared_ptr<NPP::TimerProt::AllPlayerData> data) {
	auto& db = NPP::GetDB();

	try {
		db << "INSERT OR REPLACE INTO Players(SteamId, TribeId, Start_DateTime, Last_Login_DateTime, Level, Is_New_Player) VALUES(?,?,?,?,?,?);"
			<< data->steam_id << data->tribe_id << NPP::GetTimestamp(data->startDateTime) 
			<< NPP::GetTimestamp(data->lastLoginDateTime) << data->level << data->isNewPlayer;
	}
	catch (const sqlite::sqlite_exception& exception) {
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}
}

void UpdatePVETribeDB(uint64 tribe_id, bool stillProtected) {
	auto& db = NPP::GetDB();

	try {
		db << "INSERT OR REPLACE INTO PVE_Tribes(TribeId, Is_Protected) VALUES(?,?);"
			<< tribe_id << stillProtected;

		if (!stillProtected) {
			// remove tribe from removed pve tribe array
			const auto iter = std::find_if(
				NPP::removedPveTribesList.begin(), NPP::removedPveTribesList.end(),
				[tribe_id](const uint64& data) {
				return data == tribe_id;
			});

			if (iter != NPP::removedPveTribesList.end()) {
				NPP::removedPveTribesList.erase(std::remove(NPP::removedPveTribesList.begin(), NPP::removedPveTribesList.end(), *iter), NPP::removedPveTribesList.end());
			}

			NPP::pveTribesList.clear();

			try {
				auto res = db << "SELECT TribeId FROM PVE_Tribes where Is_Protected = 1;";

				res >> [](uint64 tribeid) {
					NPP::pveTribesList.push_back(tribeid);
				};
			}
			catch (const sqlite::sqlite_exception& exception) {
				Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			}
		}
	}
	catch (const sqlite::sqlite_exception& exception) {
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}
}

bool Hook_AShooterGameMode_HandleNewPlayer(AShooterGameMode* _this, AShooterPlayerController* new_player, UPrimalPlayerData* player_data, 
		AShooterCharacter* player_character, bool is_from_login) {

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(new_player);

	uint64 team_id = new_player->TargetingTeamField();

	if (team_id != 0) {

		if (!IsPlayerExists(steam_id)) {
			NPP::TimerProt::Get().AddNewPlayer(steam_id, team_id);
		}

		NPP::TimerProt::Get().AddOnlinePlayer(steam_id, team_id);
	}

	return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);
}

void Hook_AShooterGameMode_Logout(AShooterGameMode* _this, AController* exiting) {
	// Remove player from the online list
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(exiting);
	NPP::TimerProt::TimerProt::Get().RemovePlayer(steam_id);
	AShooterGameMode_Logout_original(_this, exiting);
}

bool Hook_AShooterGameMode_SaveWorld(AShooterGameMode* GameMode) {
	bool result = AShooterGameMode_SaveWorld_original(GameMode);

	auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();
	auto& db = NPP::GetDB();

	db << "BEGIN TRANSACTION;";

	for (const auto& data : all_players_) {
		UpdatePlayerDB(data);
	}

	for (const auto& tribe_id : NPP::pveTribesList) {
		UpdatePVETribeDB(tribe_id, 1);
	}

	for (const auto& tribe_id : NPP::removedPveTribesList) {
		UpdatePVETribeDB(tribe_id, 0);
	}
	NPP::removedPveTribesList.clear();

	db << "END TRANSACTION;";
	db << "PRAGMA optimize;";

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("NPP database updated during world save.");
	}

	return result;
}

float Hook_APrimalStructure_TakeDamage(APrimalStructure* _this, float Damage, FDamageEvent* DamageEvent, AController* EventInstigator, AActor* DamageCauser) {
	// APrimalStructure != NULL
	if (_this) {
		if (!IsExemptStructure(_this)) {
			uint64 attacked_tribeid = _this->TargetingTeamField();
			if (NPP::EnableDebugging) {
				Log::GetLog()->warn("Has DamageCauser: {}.", DamageCauser ? "true" : "false");
				if (DamageCauser) {
					FName Dname = DamageCauser->NameField();
					FString name = "";
					Dname.ToString(&name);
					Log::GetLog()->warn("DamageCauser Name: {}.", name.ToString());
					if (DamageCauser->IsA(APrimalDinoCharacter::GetPrivateStaticClass())) {
						Log::GetLog()->warn("DamageCauser is a APrimalDinoCharacter.");
					}
					if (DamageCauser->IsA(AShooterPlayerController::GetPrivateStaticClass())) {
						Log::GetLog()->warn("DamageCauser is a AShooterPlayerController.");
					}
				}

				Log::GetLog()->warn("Has EventInstigator: {}.", EventInstigator ? "true" : "false");
				if (EventInstigator) {
					FName Dname = EventInstigator->NameField();
					FString name = "";
					Dname.ToString(&name);
					Log::GetLog()->warn("EventInstigator Name: {}.", name.ToString());
					if (EventInstigator->IsA(APrimalDinoCharacter::GetPrivateStaticClass())) {
						Log::GetLog()->warn("EventInstigator is a APrimalDinoCharacter.");
					}
					if (EventInstigator->IsA(AShooterPlayerController::GetPrivateStaticClass())) {
						Log::GetLog()->warn("EventInstigator is a AShooterPlayerController.");
					}
				}
			}

			//DamageCauser != NULL
			if (DamageCauser) {
				uint64 attacking_tribeid = DamageCauser->TargetingTeamField();

				//EventInstigator != NULL 
				if (EventInstigator) {

					//EventInstigator is a dino
					if (EventInstigator->IsA(APrimalDinoCharacter::GetPrivateStaticClass())) {
						if (NPP::AllowWildCorruptedDinoDamage && EventInstigator->TargetingTeamField() < 10000) {
							FString dinoName;
							EventInstigator->NameField().ToString(&dinoName);
								if (dinoName.Contains("Corrupt")) {
									return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
								}
						}

						if (NPP::AllowWildDinoDamage && EventInstigator->TargetingTeamField() < 10000) {
							return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
						}

						// Dino is in a tribe
						if (EventInstigator->TargetingTeamField() > 10000 && 
								(IsTribeProtected(EventInstigator->TargetingTeamField()) || IsTribeProtected(_this->TargetingTeamField()))) {
							return 0;
						}
					}

					//DamageCauser is a dino
					if (DamageCauser->IsA(APrimalDinoCharacter::GetPrivateStaticClass())) {
						if (NPP::AllowWildCorruptedDinoDamage && DamageCauser->TargetingTeamField() < 10000) {
							FString dinoName;
							DamageCauser->NameField().ToString(&dinoName);
							if (dinoName.Contains("Corrupt")) {
								return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
							}
						}

						if (NPP::AllowWildDinoDamage && DamageCauser->TargetingTeamField() < 10000) {
							return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
						}

						// Dino is in a tribe
						if (DamageCauser->TargetingTeamField() > 10000 &&
								(IsTribeProtected(DamageCauser->TargetingTeamField()) || IsTribeProtected(_this->TargetingTeamField()))) {
							return 0;
						}
					}
					
					//EventInstigator == AShooterPlayerController 
					if (EventInstigator->IsA(AShooterPlayerController::GetPrivateStaticClass())) {
						uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(EventInstigator);
						AShooterPlayerController* player = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);

						if (IsAdmin(steam_id)) {
							return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
						}

						if (IsPlayerProtected(player)) {
							if (!NPP::AllowNewPlayersToDamageEnemyStructures) {
								if (attacked_tribeid < 100000 || attacked_tribeid == attacking_tribeid) {
									return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
								}
								if (NPP::TimerProt::Get().IsNextMessageReady(steam_id)) {
									ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize, 
										NPP::MessageDisplayDelay, nullptr, *NPP::NewPlayerDoingDamageMessage);

									if (NPP::EnableDebugging) {
										Log::GetLog()->info("NPP Player / Tribe: {} / {} tried to damage a structure of Tribe: {}.", steam_id, attacking_tribeid,
											attacked_tribeid);
									}
								}
								return 0;
							}
						}
						else {
							if (IsTribeProtected(attacked_tribeid) && attacked_tribeid != attacking_tribeid) {
								if (NPP::TimerProt::Get().IsNextMessageReady(steam_id)) {
									ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize,
										NPP::MessageDisplayDelay, nullptr, *NPP::NewPlayerStructureTakingDamageMessage);

									if (NPP::EnableDebugging) {
										Log::GetLog()->info("Unprotected Player / Tribe: {} / {} tried to damage a structure of NPP Protected Tribe: {}.",
											steam_id, attacking_tribeid, attacked_tribeid);
									}
								}
								return 0;
							}
						}
					}
				}
				//EventInstigator == NULL
				else {
					if (attacked_tribeid == attacking_tribeid) {
						return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
					}
					if (IsTribeProtected(attacked_tribeid)) {
						if (DamageCauser->IsA(APrimalDinoCharacter::GetPrivateStaticClass())) {
							if (NPP::AllowWildCorruptedDinoDamage && DamageCauser->TargetingTeamField() < 10000) {
								FString dinoName;
								DamageCauser->NameField().ToString(&dinoName);
								if (dinoName.Contains("Corrupt")) {
									return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
								}
							}

							if (NPP::AllowWildDinoDamage && DamageCauser->TargetingTeamField() < 10000) {
								return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
							}
						}

						auto online_players_ = NPP::TimerProt::Get().GetOnlinePlayers();

						for (const auto& onlineData : online_players_) {
							if (onlineData->tribe_id == attacking_tribeid && NPP::TimerProt::Get().IsNextMessageReady(onlineData->steam_id)) {
								auto tribe_player = ArkApi::GetApiUtils().FindPlayerFromSteamId(onlineData->steam_id);
								if (!ArkApi::IApiUtils::IsPlayerDead(tribe_player)) {
									ArkApi::GetApiUtils().SendNotification(tribe_player, NPP::MessageColor, 
										NPP::MessageTextSize, NPP::MessageDisplayDelay, nullptr, 
										*NPP::NewPlayerStructureTakingDamageFromUnknownTribemateMessage);
								}
							}
						}
						return 0;
					}
					if (IsTribeProtected(attacking_tribeid) && !NPP::AllowNewPlayersToDamageEnemyStructures) {
						return 0;
					}
				}
			}
			//DamageCauser == NULL
			else {
				if (IsTribeProtected(attacked_tribeid)) {
					return 0;
				}
			}
		}
	}	
	return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
}

NPP::TimerProt::TimerProt() {
	player_update_interval_ = NPP::PlayerUpdateIntervalInMins;
	ArkApi::GetCommands().AddOnTimerCallback("UpdateTimer", std::bind(&NPP::TimerProt::UpdateTimer, this));
}

NPP::TimerProt& NPP::TimerProt::Get() {
	static TimerProt instance;
	return instance;
}

void NPP::TimerProt::AddPlayerFromDB(uint64 steam_id, uint64 tribe_id, std::chrono::time_point<std::chrono::system_clock> startDateTime, 
	std::chrono::time_point<std::chrono::system_clock> lastLoginDateTime, int level, int isNewPlayer) {

	const auto iter = std::find_if(
		all_players_.begin(), all_players_.end(),
		[steam_id](const std::shared_ptr<AllPlayerData>& data) {
			return data->steam_id == steam_id;
		});

	if (iter != all_players_.end())
		return;

	all_players_.push_back(std::make_shared<AllPlayerData>(steam_id, tribe_id, startDateTime, lastLoginDateTime, level, isNewPlayer));
	if (isNewPlayer == 1) {
		if (!IsAdmin(steam_id)) {
			if (std::count(NPP::nppTribesList.begin(), NPP::nppTribesList.end(), tribe_id) < 1) {
				NPP::nppTribesList.push_back(tribe_id);
			}
		}
	}
}

void NPP::TimerProt::AddNewPlayer(uint64 steam_id, uint64 tribe_id) {
	const auto iter = std::find_if(
		all_players_.begin(), all_players_.end(),
		[steam_id](const std::shared_ptr<AllPlayerData>& data) { 
			return data->steam_id == steam_id;
		});

	if (iter != all_players_.end())
		return;
	all_players_.push_back(std::make_shared<AllPlayerData>(steam_id, tribe_id, std::chrono::system_clock::now(), std::chrono::system_clock::now(), 1, 1));
	if (!IsAdmin(steam_id)) {
		if (std::count(NPP::nppTribesList.begin(), NPP::nppTribesList.end(), tribe_id) < 1) {
			NPP::nppTribesList.push_back(tribe_id);
		}
	}
}

void NPP::TimerProt::AddOnlinePlayer(uint64 steam_id, uint64 team_id) {
	const auto iter = std::find_if(
		online_players_.begin(), online_players_.end(),
		[steam_id](const std::shared_ptr<OnlinePlayersData>& data) { 
			return data->steam_id == steam_id; 
		});

	if (iter != online_players_.end())
		return;

	std::chrono::time_point<std::chrono::system_clock> startDateTime = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> lastLoginDateTime = std::chrono::system_clock::now();
	int level = 1;
	int isNewPlayer = 1;
	std::chrono::time_point<std::chrono::system_clock> nextMessageTime = std::chrono::system_clock::now();

	for (const auto& alldata : all_players_) {
		if (alldata->steam_id == steam_id) {
			team_id = alldata->tribe_id;
			startDateTime = alldata->startDateTime;
			alldata->lastLoginDateTime = lastLoginDateTime;
			level = alldata->level;
			isNewPlayer = alldata->isNewPlayer;
			break;
		}
	}

	online_players_.push_back(std::make_shared<OnlinePlayersData>(steam_id, team_id, startDateTime, lastLoginDateTime, level, isNewPlayer, nextMessageTime));
}

void NPP::TimerProt::RemovePlayer(uint64 steam_id) {
	const auto iter = std::find_if(
		online_players_.begin(), online_players_.end(),
		[steam_id](const std::shared_ptr<OnlinePlayersData>& data) { 
			return data->steam_id == steam_id; 
		});

	if (iter != online_players_.end()) {
		online_players_.erase(std::remove(online_players_.begin(), online_players_.end(), *iter), online_players_.end());
	}
}

bool NPP::TimerProt::IsNextMessageReady(uint64 steam_id) {
	for (const auto& data : online_players_) {
		if (data->steam_id == steam_id) {
			const auto now_time = std::chrono::system_clock::now();
			auto diff = std::chrono::duration_cast<std::chrono::seconds>(data->nextMessageTime - now_time);

			if (diff.count() <= 0) {
				data->nextMessageTime = now_time + std::chrono::seconds(NPP::MessageIntervalInSecs);
				return true;
			}
			else {
				return false;
			}
		}
	}
	return true;
}

void NPP::TimerProt::UpdateLevelAndTribe() {

	for (const auto& data : online_players_) {

		AShooterPlayerController* player = ArkApi::GetApiUtils().FindPlayerFromSteamId(data->steam_id);

		if (ArkApi::IApiUtils::IsPlayerDead(player)) {
			return;
		}

		APlayerState* player_state = player->PlayerStateField();
		AShooterPlayerState* shooter_player_state = static_cast<AShooterPlayerState*>(player_state);
		uint64 tribe_id = shooter_player_state->TargetingTeamField();
		int level = shooter_player_state->MyPlayerDataStructField()->MyPersistentCharacterStatsField()
			->CharacterStatusComponent_HighestExtraCharacterLevelField() + 1;

		data->level = level;
		data->tribe_id = tribe_id;

		for (const auto& alldata : all_players_) {
			if (alldata->steam_id == data->steam_id) {
				alldata->level = level;
				alldata->tribe_id = tribe_id;
				break;
			}
		}
	}

	NPP::nppTribesList.clear();

	for (const auto& data : all_players_) {
		if (!IsAdmin(data->steam_id) && data->isNewPlayer == 1) {
			if (std::count(NPP::nppTribesList.begin(), NPP::nppTribesList.end(), data->tribe_id) < 1) {
				NPP::nppTribesList.push_back(data->tribe_id);
			}
		}
	}
}

std::vector<std::shared_ptr<NPP::TimerProt::OnlinePlayersData>> NPP::TimerProt::GetOnlinePlayers() {
	return online_players_;
}

std::vector<std::shared_ptr<NPP::TimerProt::AllPlayerData>> NPP::TimerProt::GetAllPlayers() {
	return all_players_;
}

void NPP::TimerProt::UpdateTimer() {
	const auto now_time = std::chrono::system_clock::now();

	auto diff = std::chrono::duration_cast<std::chrono::seconds>(NPP::next_player_update - now_time);

	if (diff.count() <= 0) {
		if (NPP::EnableDebugging) {
			Log::GetLog()->warn("Running NPP update tasks...");
		}
		LoadNppPermissionsArray();
		auto player_interval = std::chrono::minutes(player_update_interval_);
		NPP::next_player_update = now_time + player_interval;
		NPP::TimerProt::UpdateLevelAndTribe();
		RemoveExpiredTribesProtection();

		if (NPP::EnableDebugging) {
			auto all_players_ = NPP::TimerProt::GetAllPlayers();
			auto online_players_ = NPP::TimerProt::GetOnlinePlayers();

			Log::GetLog()->info(" ");
			Log::GetLog()->info("----------------------------------------------------------------------");

			Log::GetLog()->info("all_players_:");
			for (std::shared_ptr<AllPlayerData>& data : all_players_) {
				Log::GetLog()->info("steam_id:          {}", data->steam_id);
				Log::GetLog()->info("tribe_id:          {}", data->tribe_id);
				Log::GetLog()->info("level:             {}", data->level);
				Log::GetLog()->info("isNewPlayer:       {}", data->isNewPlayer);
				Log::GetLog()->info("startDateTime:     {}", GetTimestamp(data->startDateTime));
				Log::GetLog()->info("lastLoginDateTime: {}", GetTimestamp(data->lastLoginDateTime));
				Log::GetLog()->info("----------------------------------------------");
			}
		
			Log::GetLog()->info("----------------------------------------------------------------------");
			Log::GetLog()->info(" ");
			Log::GetLog()->info("----------------------------------------------------------------------");

			Log::GetLog()->info("online_players_:");
			for (std::shared_ptr<OnlinePlayersData>& data : online_players_) {
				Log::GetLog()->info("steam_id:          {}", data->steam_id);
				Log::GetLog()->info("tribe_id:          {}", data->tribe_id);
				Log::GetLog()->info("level:             {}", data->level);
				Log::GetLog()->info("isNewPlayer:       {}", data->isNewPlayer);
				Log::GetLog()->info("startDateTime:     {}", GetTimestamp(data->startDateTime));
				Log::GetLog()->info("lastLoginDateTime: {}", GetTimestamp(data->lastLoginDateTime));
				Log::GetLog()->info("----------------------------------------------");
			}

			Log::GetLog()->info("----------------------------------------------------------------------");
			Log::GetLog()->info(" ");
			Log::GetLog()->info("----------------------------------------------------------------------");

			Log::GetLog()->info("nppTribesList:");
			for (uint64 data : nppTribesList) {
				Log::GetLog()->info(data);
			}

			Log::GetLog()->info("----------------------------------------------------------------------");
			Log::GetLog()->info(" ");
			Log::GetLog()->info("----------------------------------------------------------------------");

			Log::GetLog()->info("pveTribesList:");
			for (uint64 data : pveTribesList) {
				Log::GetLog()->info(data);
			}

			Log::GetLog()->info("----------------------------------------------------------------------");
			Log::GetLog()->info(" ");
			Log::GetLog()->info("----------------------------------------------------------------------");

			Log::GetLog()->info("StructureExemptions:");
			for (std::string data : StructureExemptions) {
				Log::GetLog()->info(data);
			}

			Log::GetLog()->info("----------------------------------------------------------------------");
			Log::GetLog()->info(" ");
			Log::GetLog()->info("----------------------------------------------------------------------");

			Log::GetLog()->info("nppAdminArray:");
			for (uint64 data : nppAdminArray) {
				Log::GetLog()->info(data);
			}

			Log::GetLog()->info("----------------------------------------------------------------------");
			Log::GetLog()->info(" ");

			Log::GetLog()->info("PlayerUpdateIntervalInMins timer finished: NPP Protections updated.");
		}
	}
}
