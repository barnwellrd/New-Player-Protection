#pragma once

inline void Info(AShooterPlayerController* player) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize, 
		NPP::MessageDisplayDelay, nullptr, *NPP::NPPInfoMessage, NPP::HoursOfProtection, 
		NPP::MaxLevel);

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void Disable(AShooterPlayerController* player) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	if (!player || !player->PlayerStateField() || ArkApi::IApiUtils::IsPlayerDead(player) || !NPP::AllowPlayersToDisableOwnedTribeProtection) {
		if (NPP::EnableDebugging) {
			Log::GetLog()->info("Exiting {}.", __FUNCTION__);
		}

		return;
	}

	// if not PVE player
	if (!IsPVETribe(player->TargetingTeamField())) {
		//if new player
		if (IsPlayerProtected(player)) {
			AShooterPlayerState* State = static_cast<AShooterPlayerState*>(player->PlayerStateField());

			//if tribe admin
			if (State->IsTribeAdmin()) {
				//remove protection from all tribe members
				uint64 tribe_id = player->TargetingTeamField();
				uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player);

				auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();
				auto online_players_ = NPP::TimerProt::Get().GetOnlinePlayers();

				//remove from protected tribes list if found
				if (std::count(NPP::nppTribesList.begin(), NPP::nppTribesList.end(), tribe_id) > 0) {
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

				for (const auto& allData : all_players_) {
					if (allData->tribe_id == tribe_id) {
						allData->isNewPlayer = 0;
					}
				}

				for (const auto& onlineData : online_players_) {
					if (onlineData->tribe_id == tribe_id) {
						onlineData->isNewPlayer = 0;
					}
				}
				//display protection removed message
				ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize,
					NPP::MessageDisplayDelay, nullptr, *NPP::NewPlayerProtectionDisableSuccess);

				if (NPP::EnableDebugging) {
					Log::GetLog()->info("Player: {} of Tribe: {} disabled own tribes NPP Protection.", steam_id, tribe_id);
				}
			}
			//else not tribe admin
			else {
				//display not tribe admin message
				ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize,
					NPP::MessageDisplayDelay, nullptr, *NPP::NotTribeAdminMessage);
			}
		}
		//else not new player 
		else {
			//display not under protection message
			ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize, 
				NPP::MessageDisplayDelay, nullptr, *NPP::NotANewPlayerMessage);
		}
	}
	// else PVE player  
	else {
		//display PVE protection message
		ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize, 
			NPP::MessageDisplayDelay, nullptr, *NPP::PVEDisablePlayerMessage);
	}

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void Status(AShooterPlayerController* player) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	if (!player || !player->PlayerStateField() || ArkApi::IApiUtils::IsPlayerDead(player)) {
		if (NPP::EnableDebugging) {
			Log::GetLog()->info("Exiting {}.", __FUNCTION__);
		}

		return;
	}

	uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player);

	// if not PVE player
	if (!IsPVETribe(player->TargetingTeamField())) {
		//if new player or admin
		if (IsPlayerProtected(player) || IsAdmin(steam_id)) {
			//loop through tribe member
			uint64 tribe_id = player->TargetingTeamField();
			std::chrono::time_point<std::chrono::system_clock> oldestDate = std::chrono::system_clock::now() + std::chrono::hours(999999);
			int highestLevel = 0;

			auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();

			//Loop through tribe
			for (const auto& allData : all_players_) {

				if (IsAdmin(allData->steam_id)) {
					continue;
				}

				if (allData->tribe_id == tribe_id) {
					//get oldest date started
					if (allData->startDateTime < oldestDate) {
						oldestDate = allData->startDateTime;
					}

					//get highest level player
					if (allData->level > highestLevel) {
						highestLevel = allData->level;
					}
				}
			}

			//calulate time
			auto protectionInHours = std::chrono::hours(NPP::HoursOfProtection);
			auto now = std::chrono::system_clock::now();
			auto expireTime = now - protectionInHours;
			auto expireTimeinMin = std::chrono::duration_cast<std::chrono::minutes>(oldestDate - expireTime);
			auto daysLeft = expireTimeinMin / 1440;
			auto hoursLeft = ((expireTimeinMin - (1440 * daysLeft)) / 60);
			auto minutesLeft =  (expireTimeinMin - ((1440 * daysLeft) + (60 * hoursLeft)));

			//calculate level
			int levelsLeft = NPP::MaxLevel - highestLevel;

			// player is admin checking protection of an all admin tribe
			if (IsAdmin(steam_id) && ((std::chrono::system_clock::now() + std::chrono::hours(999995)) < oldestDate)) {
				ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize,
					NPP::MessageDisplayDelay, nullptr, *NPP::IsAdminTribe);
			}
			else {
				//display time/level remaining message
				ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize,
					NPP::MessageDisplayDelay, nullptr, *NPP::NPPRemainingMessage, daysLeft.count(), hoursLeft.count(),
					minutesLeft.count(), levelsLeft);
			}
		}
		//else not new player
		else {
			//display not under protection message
			ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize, 
				NPP::MessageDisplayDelay, nullptr, *NPP::NotANewPlayerMessage);
		}
	}
	// is pve tribe 
	else {
		// pve status notification
		ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize, 
			NPP::MessageDisplayDelay, nullptr, *NPP::PVEStatusMessage);
	}

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void GetTribeID(AShooterPlayerController* player) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	if (!player || !player->PlayerStateField() || ArkApi::IApiUtils::IsPlayerDead(player)) {
		if (NPP::EnableDebugging) {
			Log::GetLog()->info("Exiting {}.", __FUNCTION__);
		}

		return;
	}

	AActor* Actor = player->GetPlayerCharacter()->GetAimedActor(ECollisionChannel::ECC_GameTraceChannel2, nullptr, 0.0, 0.0, nullptr, nullptr,
		false, false, false);

	if (Actor && Actor->IsA(APrimalStructure::GetPrivateStaticClass())) {
		APrimalStructure* Structure = static_cast<APrimalStructure*>(Actor);
		const int teamId = Structure->TargetingTeamField();
		ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize, 20.0f, nullptr,
			*NPP::TribeIDText, teamId);	
	}
	else {
		ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize, 
			NPP::MessageDisplayDelay, nullptr, *NPP::NoStructureForTribeIDText);
	}

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

// "!npp getpath"
//Get the Plugin's blueprint path of target structure
//May vary wildly from spawn Blueprint, so use this command's output plugin paths
inline void GetTargetPath(AShooterPlayerController* player) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	//if player is dead or doesn't exist, break
	if (!player || !player->PlayerStateField() || ArkApi::IApiUtils::IsPlayerDead(player)) {
		if (NPP::EnableDebugging) {
			Log::GetLog()->info("Exiting {}.", __FUNCTION__);
		}

		return;
	}

	//get aimed target
	AActor* Actor = player->GetPlayerCharacter()->GetAimedActor(ECollisionChannel::ECC_GameTraceChannel2, nullptr, 0.0, 0.0, nullptr, nullptr,
		false, false, false);

	//check if target is a dino or structure
	if (Actor && Actor->IsA(APrimalStructure::GetPrivateStaticClass())) {

		ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize, 20.0f, nullptr,
			"{}", NPP::GetBlueprint(Actor).ToString());
		Log::GetLog()->info("Blueprint Path From Command: {}", NPP::GetBlueprint(Actor).ToString());
	}
	//target not a dino or structure
	else {
		ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize, 
			NPP::MessageDisplayDelay, nullptr, *NPP::NotAStructureMessage);
	}

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void ChatCommand(AShooterPlayerController* player, FString* message, int mode) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	TArray<FString> parsed;
	message->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1)) {
		FString input = parsed[1];
		if (input.Compare("info") == 0) {
			Info(player);
		}
		else if (input.Compare("status") == 0) {
			Status(player);
		}
		else if (input.Compare("disable") == 0) {
			Disable(player);
		}
		else if (input.Compare("tribeid") == 0) {
			GetTribeID(player);
		}
		else if (input.Compare("path") == 0) {
			GetTargetPath(player);
		}
		else {
			ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize, 
				NPP::MessageDisplayDelay, nullptr, *NPP::NPPInvalidCommand);
		}
	}
	else {
		ArkApi::GetApiUtils().SendNotification(player, NPP::MessageColor, NPP::MessageTextSize, 
			NPP::MessageDisplayDelay, nullptr, *NPP::NPPInvalidCommand);
	}

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void ConsoleRemoveProtection(APlayerController* player_controller, FString* cmd, bool) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

	//if Admin
	if (!shooter_controller || !shooter_controller->PlayerStateField() || !shooter_controller->bIsAdmin().Get()) {
		if (NPP::EnableDebugging) {
			Log::GetLog()->info("Exiting {}.", __FUNCTION__);
		}

		return;
	}

	uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(shooter_controller);

	bool found = false;
	bool isProtected = false;

	//parse command
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1)) {
		uint64 tribe_id = 0;

		try {
			tribe_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception) {
			Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			if (NPP::EnableDebugging) {
				Log::GetLog()->info("Exiting {}.", __FUNCTION__);
			}

			return;
		}

		auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();

		//look for tribe
		for (const auto& allData : all_players_) {
			if (allData->tribe_id == tribe_id) {
				found = true;

				if (allData->isNewPlayer == 1) {
					isProtected = true;
					break;
				}
			}
		}
		//if tribe found
		if (found) {
			//if tribe is protected
			if (isProtected) {
				auto online_players_ = NPP::TimerProt::Get().GetOnlinePlayers();

				//remove from protected tribes list if found
				if (std::count(NPP::nppTribesList.begin(), NPP::nppTribesList.end(), tribe_id) > 0) {
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

				//loop through tribe members
				for (const auto& allData : all_players_) {
					//remove protection
					if (allData->tribe_id == tribe_id) {
						allData->isNewPlayer = 0;
					}
				}

				for (const auto& onlineData : online_players_) {
					//remove protection
					if (onlineData->tribe_id == tribe_id) {
						onlineData->isNewPlayer = 0;
					}
				}

				//display protection removed message
				ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
					NPP::MessageDisplayDelay, nullptr, *NPP::AdminTribeProtectionRemoved, tribe_id);

				if (NPP::EnableDebugging) {
					Log::GetLog()->info("Admin: {} removed NPP Protection of Tribe: {}.", steam_id, tribe_id);
				}
			}
			//tribe not protected 
			else {
				//display tribe not under protection
				ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
					NPP::MessageDisplayDelay, nullptr, *NPP::AdminTribeNotUnderProtection, tribe_id);
			}
		}
		// tribe not found 
		else {
			//display tribe not found
			ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
				NPP::MessageDisplayDelay, nullptr, *NPP::AdminNoTribeExistsMessage, tribe_id);
		}
	}

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void ConsoleResetProtection(APlayerController* player, FString* cmd, bool boolean) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	const auto shooter_controller = static_cast<AShooterPlayerController*>(player);

	//if Admin
	if (!shooter_controller || !shooter_controller->PlayerStateField() || !shooter_controller->bIsAdmin().Get()) {
		if (NPP::EnableDebugging) {
			Log::GetLog()->info("Exiting {}.", __FUNCTION__);
		}

		return;
	}

	uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(shooter_controller);

	bool found = false;
	bool underMaxLevel = true;

	//parse command
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1)) {
		uint64 tribe_id = 0;

		try {
			tribe_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception) {
			Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			if (NPP::EnableDebugging) {
				Log::GetLog()->info("Exiting {}.", __FUNCTION__);
			}

			return;
		}

		auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();

		//look for tribe
		for (const auto& allData : all_players_) {
			if (allData->tribe_id == tribe_id) {

				if (IsAdmin(allData->steam_id)) {
					continue;
				}

				found = true;

				if (allData->level >= NPP::MaxLevel) {
					underMaxLevel = false;
					break;
				}
			}
		}
		//if tribe found
		if (found) {
			//if tribe is under max level
			if (underMaxLevel) {
				auto online_players_ = NPP::TimerProt::Get().GetOnlinePlayers();
				auto now = std::chrono::system_clock::now();

				//loop through tribe members
				for (const auto& allData : all_players_) {
					
					if (IsAdmin(allData->steam_id)) {
						continue;
					}

					//Add to protected tribes list if found
					if (std::count(NPP::nppTribesList.begin(), NPP::nppTribesList.end(), tribe_id) < 1) {
						NPP::nppTribesList.push_back(tribe_id);
					}

					//add protection & increase start date
					if (allData->tribe_id == tribe_id) {
						allData->isNewPlayer = 1;
						allData->startDateTime = now;
					}
				}

				for (const auto& onlineData : online_players_) {

					if (IsAdmin(onlineData->steam_id)) {
						continue;
					}

					//add protection & increase start date
					if (onlineData->tribe_id == tribe_id) {
						onlineData->isNewPlayer = 1;
						onlineData->startDateTime = now;
					}
				}

				//display protection added message
				ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
					NPP::MessageDisplayDelay, nullptr, *NPP::AdminResetTribeProtectionSuccess, 
					NPP::HoursOfProtection, tribe_id);

				if (NPP::EnableDebugging) {
					Log::GetLog()->info("Admin: {} reset the NPP Protection of Tribe: {}.", steam_id, tribe_id);
				}
			}
			//tribe not under max level 
			else {
				//display tribe under max level message
				ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
					NPP::MessageDisplayDelay, nullptr, *NPP::AdminResetTribeProtectionLvlFailure, tribe_id);
			}
		}
		// tribe not found 
		else {
			//display tribe not found
			ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
				NPP::MessageDisplayDelay, nullptr, *NPP::AdminNoTribeExistsMessage, tribe_id);
		}
	}

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void ConsoleAddProtection(APlayerController* player, FString* cmd, bool boolean) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	const auto shooter_controller = static_cast<AShooterPlayerController*>(player);

	//if Admin
	if (!shooter_controller || !shooter_controller->PlayerStateField() || !shooter_controller->bIsAdmin().Get()) {
		if (NPP::EnableDebugging) {
			Log::GetLog()->info("Exiting {}.", __FUNCTION__);
		}

		return;
	}

	uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(shooter_controller);


	bool found = false;
	bool underMaxLevel = true;

	//parse command
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1) && parsed.IsValidIndex(2)) {
		uint64 tribe_id = 0;
		uint64 hours = 0;

		try {
			tribe_id = std::stoull(*parsed[1]);
			
			if (parsed[2].Contains("-")) {
				Log::GetLog()->warn("({} {}) Parsing error: Hours cannot be negative.", __FILE__, __FUNCTION__);
				if (NPP::EnableDebugging) {
					Log::GetLog()->info("Exiting {}.", __FUNCTION__);
				}

				return;
			}

			hours = std::stoull(*parsed[2]);
		}
		catch (const std::exception& exception) {
			Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			if (NPP::EnableDebugging) {
				Log::GetLog()->info("Exiting {}.", __FUNCTION__);
			}

			return;
		}

		auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();

		//look for tribe
		for (const auto& allData : all_players_) {
			if (allData->tribe_id == tribe_id) {

				if (IsAdmin(allData->steam_id)) {
					continue;
				}

				found = true;

				if (allData->level >= NPP::MaxLevel) {
					underMaxLevel = false;
					break;
				}
			}
		}
		//if tribe found
		if (found) {
			//if tribe is under max level
			if (underMaxLevel) {
				auto online_players_ = NPP::TimerProt::Get().GetOnlinePlayers();
				auto now = std::chrono::system_clock::now();

				//loop through tribe members
				for (const auto& allData : all_players_) {
					
					if (IsAdmin(allData->steam_id)) {
						continue;
					}

					//Add to protected tribes list if found
					if (std::count(NPP::nppTribesList.begin(), NPP::nppTribesList.end(), tribe_id) < 1) {
						NPP::nppTribesList.push_back(tribe_id);
					}

					//add protection & increase start date
					if (allData->tribe_id == tribe_id) {
						allData->isNewPlayer = 1;
						allData->startDateTime = now += std::chrono::hours(hours - NPP::HoursOfProtection);
					}
				}

				for (const auto& onlineData : online_players_) {

					if (IsAdmin(onlineData->steam_id)) {
						continue;
					}

					//add protection & increase start date
					if (onlineData->tribe_id == tribe_id) {
						onlineData->isNewPlayer = 1;
						onlineData->startDateTime = now += std::chrono::hours(hours - NPP::HoursOfProtection);
					}
				}

				//display protection added message
				ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
					NPP::MessageDisplayDelay, nullptr, *NPP::AdminResetTribeProtectionSuccess, hours, tribe_id);

				if (NPP::EnableDebugging) {
					Log::GetLog()->info("Admin: {} added {} hours of NPP Protection to Tribe: {}.", steam_id, hours, tribe_id);
				}
			}
			//tribe not under max level 
			else {
				//display tribe under max level message
				ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
					NPP::MessageDisplayDelay, nullptr, *NPP::AdminResetTribeProtectionLvlFailure, tribe_id);
			}
		}
		// tribe not found 
		else {
			//display tribe not found
			ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
				NPP::MessageDisplayDelay, nullptr, *NPP::AdminNoTribeExistsMessage, tribe_id);
		}
	}

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void RconRemoveProtection(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	FString reply;

	bool found = false;
	bool isProtected = false;

	//parse command
	TArray<FString> parsed;
	rcon_packet->Body.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1)) {
		uint64 tribe_id = 0;

		try {
			tribe_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception) {
			Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			if (NPP::EnableDebugging) {
				Log::GetLog()->info("Exiting {}.", __FUNCTION__);
			}

			return;
		}

		auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();

		//look for tribe
		for (const auto& allData : all_players_) {
			if (allData->tribe_id == tribe_id) {
				found = true;

				if (allData->isNewPlayer == 1) {
					isProtected = true;
					break;
				}
			}
		}
		//if tribe found
		if (found) {
			//if tribe is protected
			if (isProtected) {
				auto online_players_ = NPP::TimerProt::Get().GetOnlinePlayers();

				//remove from protected tribes list if found
				if (std::count(NPP::nppTribesList.begin(), NPP::nppTribesList.end(), tribe_id) > 0) {
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

				//loop through tribe members
				for (const auto& allData : all_players_) {
					//remove protection
					if (allData->tribe_id == tribe_id) {
						allData->isNewPlayer = 0;
					}
				}

				for (const auto& onlineData : online_players_) {
					//remove protection
					if (onlineData->tribe_id == tribe_id) {
						onlineData->isNewPlayer = 0;
					}
				}

				//display protection removed message
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminTribeProtectionRemoved, tribe_id));

				if (NPP::EnableDebugging) {
					Log::GetLog()->info("RCON removed NPP Protection of Tribe: {}.", tribe_id);
				}
			}
			//tribe not protected 
			else {
				//display tribe not under protection
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminTribeNotUnderProtection, tribe_id));
			}
		}
		// tribe not found 
		else {
			//display tribe not found
			rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminNoTribeExistsMessage, tribe_id));
		}
	}

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void RconResetProtection(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	FString reply;

	bool found = false;
	bool underMaxLevel = true;

	//parse command
	TArray<FString> parsed;
	rcon_packet->Body.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1)) {
		uint64 tribe_id = 0;

		try {
			tribe_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception) {
			Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			if (NPP::EnableDebugging) {
				Log::GetLog()->info("Exiting {}.", __FUNCTION__);
			}

			return;
		}

		auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();

		//look for tribe
		for (const auto& allData : all_players_) {

			if (IsAdmin(allData->steam_id)) {
				continue;
			}

			if (allData->tribe_id == tribe_id) {
				found = true;

				if (allData->level >= NPP::MaxLevel) {
					underMaxLevel = false;
					break;
				}
			}
		}
		//if tribe found
		if (found) {
			//if tribe is under max level
			if (underMaxLevel) {
				auto online_players_ = NPP::TimerProt::Get().GetOnlinePlayers();
				auto now = std::chrono::system_clock::now();

				//loop through tribe members
				for (const auto& allData : all_players_) {

					if (IsAdmin(allData->steam_id)) {
						continue;
					}

					//Add to protected tribes list if found
					if (std::count(NPP::nppTribesList.begin(), NPP::nppTribesList.end(), tribe_id) < 1) {
						NPP::nppTribesList.push_back(tribe_id);
					}

					//add protection & increase start date
					if (allData->tribe_id == tribe_id) {
						allData->isNewPlayer = 1;
						allData->startDateTime = now;
					}
				}

				for (const auto& onlineData : online_players_) {

					if (IsAdmin(onlineData->steam_id)) {
						continue;
					}

					//add protection & increase start date
					if (onlineData->tribe_id == tribe_id) {
						onlineData->isNewPlayer = 1;
						onlineData->startDateTime = now;
					}
				}

				//display protection added message
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminResetTribeProtectionSuccess, 
					NPP::HoursOfProtection, tribe_id));

				if (NPP::EnableDebugging) {
					Log::GetLog()->info("RCON reset NPP Protection of Tribe: {}.", tribe_id);
				}
			}
			//tribe not under max level 
			else {
				//display tribe under max level message
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminResetTribeProtectionLvlFailure, tribe_id));
			}
		}
		// tribe not found 
		else {
			//display tribe not found
			rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminNoTribeExistsMessage, tribe_id));
		}
	}

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void RconAddProtection(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	FString reply;

	bool found = false;
	bool underMaxLevel = true;

	//parse command
	TArray<FString> parsed;
	rcon_packet->Body.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1) && parsed.IsValidIndex(2)) {
		uint64 tribe_id = 0;
		uint64 hours = 0;

		try {
			tribe_id = std::stoull(*parsed[1]);
			hours = std::stoi(*parsed[2]);
			if (hours < 0) {
				Log::GetLog()->warn("({} {}) Parsing error: Hours cannot be negative.", __FILE__, __FUNCTION__);
				if (NPP::EnableDebugging) {
					Log::GetLog()->info("Exiting {}.", __FUNCTION__);
				}

				return;
			}
		}
		catch (const std::exception& exception) {
			Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			if (NPP::EnableDebugging) {
				Log::GetLog()->info("Exiting {}.", __FUNCTION__);
			}

			return;
		}

		auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();

		//look for tribe
		for (const auto& allData : all_players_) {
			if (allData->tribe_id == tribe_id) {

				if (IsAdmin(allData->steam_id)) {
					continue;
				}

				//Add to protected tribes list if found
				if (std::count(NPP::nppTribesList.begin(), NPP::nppTribesList.end(), tribe_id) < 1) {
					NPP::nppTribesList.push_back(tribe_id);
				}

				found = true;

				if (allData->level >= NPP::MaxLevel) {
					underMaxLevel = false;
					break;
				}
			}
		}
		//if tribe found
		if (found) {
			//if tribe is under max level
			if (underMaxLevel) {
				auto online_players_ = NPP::TimerProt::Get().GetOnlinePlayers();
				auto now = std::chrono::system_clock::now();

				//loop through tribe members
				for (const auto& allData : all_players_) {

					if (IsAdmin(allData->steam_id)) {
						continue;
					}

					//add protection & increase start date
					if (allData->tribe_id == tribe_id) {
						allData->isNewPlayer = 1;
						allData->startDateTime = now += std::chrono::hours(hours - NPP::HoursOfProtection);
					}
				}

				for (const auto& onlineData : online_players_) {

					if (IsAdmin(onlineData->steam_id)) {
						continue;
					}

					//add protection & increase start date
					if (onlineData->tribe_id == tribe_id) {
						onlineData->isNewPlayer = 1;
						onlineData->startDateTime = now += std::chrono::hours(hours - NPP::HoursOfProtection);
					}
				}

				//display protection added message
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminResetTribeProtectionSuccess, hours, tribe_id));

				if (NPP::EnableDebugging) {
					Log::GetLog()->info("RCON added {} hours of NPP Protection to Tribe: {}.", hours, tribe_id);
				}
			}
			//tribe not under max level 
			else {
				//display tribe under max level message
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminResetTribeProtectionLvlFailure, tribe_id));
			}
		}
		// tribe not found 
		else {
			//display tribe not found
			rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminNoTribeExistsMessage, tribe_id));
		}
	}

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void ConsoleSetPVE(APlayerController* player, FString* cmd, bool boolean) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	const auto shooter_controller = static_cast<AShooterPlayerController*>(player);

	//if Admin
	if (!shooter_controller || !shooter_controller->PlayerStateField() || !shooter_controller->bIsAdmin().Get()) {
		if (NPP::EnableDebugging) {
			Log::GetLog()->info("Exiting {}.", __FUNCTION__);
		}

		return;
	}

	uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player);

	bool found = false;

	//parse command
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1) && parsed.IsValidIndex(2)) {
		uint64 tribe_id = 0;
		uint64 setToPve = 0;

		try {
			tribe_id = std::stoull(*parsed[1]);
			setToPve = std::stoull(*parsed[2]);
			if (setToPve < 0 || setToPve > 1) {
				Log::GetLog()->warn("({} {}) Parsing error: setToPve can only be a 1 or 0.", __FILE__, __FUNCTION__);
				if (NPP::EnableDebugging) {
					Log::GetLog()->info("Exiting {}.", __FUNCTION__);
				}

				return;
			}
		}
		catch (const std::exception& exception) {
			Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			if (NPP::EnableDebugging) {
				Log::GetLog()->info("Exiting {}.", __FUNCTION__);
			}

			return;
		}

		auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();

		//look for tribe
		for (const auto& allData : all_players_) {
			if (allData->tribe_id == tribe_id) {

				if (IsAdmin(allData->steam_id)) {
					continue;
				}

				found = true;
			}
		}
		//if tribe found
		if (found) {
			if (setToPve == 1) {
				if (std::count(NPP::pveTribesList.begin(), NPP::pveTribesList.end(), tribe_id) < 1) {
					NPP::pveTribesList.push_back(tribe_id);

					if (std::count(NPP::removedPveTribesList.begin(), NPP::removedPveTribesList.end(), tribe_id) > 0) {
						const auto iter = std::find_if(
							NPP::removedPveTribesList.begin(), NPP::removedPveTribesList.end(),
							[tribe_id](const uint64 data) {
								return data == tribe_id;
							});

						if (iter != NPP::removedPveTribesList.end()) {
							NPP::removedPveTribesList.erase(std::remove(NPP::removedPveTribesList.begin(), 
								NPP::removedPveTribesList.end(), *iter), NPP::removedPveTribesList.end());
						}
					}

					//display pve tribe added message
					ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
						NPP::MessageDisplayDelay, nullptr, *NPP::AdminPVETribeAddedSuccessMessage, tribe_id);

					if (NPP::EnableDebugging) {
						Log::GetLog()->info("Admin: {} enabled PVE status of Tribe: {}.", steam_id, tribe_id);
					}

				}
				else {
					//display pve tribe already set message
					ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
						NPP::MessageDisplayDelay, nullptr, *NPP::AdminPVETribeAlreadyAddedMessage, tribe_id);
				}
			}
			else {
				if (std::count(NPP::removedPveTribesList.begin(), NPP::removedPveTribesList.end(), tribe_id) < 1) {
					NPP::removedPveTribesList.push_back(tribe_id);

					if (std::count(NPP::pveTribesList.begin(), NPP::pveTribesList.end(), tribe_id) > 0) {
						const auto iter = std::find_if(
							NPP::pveTribesList.begin(), NPP::pveTribesList.end(),
							[tribe_id](const uint64 data) {
								return data == tribe_id;
							});

						if (iter != NPP::pveTribesList.end()) {
							NPP::pveTribesList.erase(std::remove(NPP::pveTribesList.begin(), 
								NPP::pveTribesList.end(), *iter), NPP::pveTribesList.end());
						}
					}

					//display tribe removed message
					ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
						NPP::MessageDisplayDelay, nullptr, *NPP::AdminPVETribeRemovedSuccessMessage, tribe_id);

					if (NPP::EnableDebugging) {
						Log::GetLog()->info("Admin: {} disabled PVE status of Tribe: {}.", steam_id, tribe_id);
					}
				}
				else {
					//display tribe already removed message
					ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
						NPP::MessageDisplayDelay, nullptr, *NPP::AdminPVETribeAlreadyRemovedMessage, tribe_id);
				}
			}
		}
		// tribe not found 
		else {
			//display tribe not found
			ArkApi::GetApiUtils().SendNotification(shooter_controller, NPP::MessageColor, NPP::MessageTextSize, 
				NPP::MessageDisplayDelay, nullptr, *NPP::AdminNoTribeExistsMessage, tribe_id);
		}
	}

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void RconSetPVE(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	FString reply;

	bool found = false;

	//parse command
	TArray<FString> parsed;
	rcon_packet->Body.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1) && parsed.IsValidIndex(2)) {
		uint64 tribe_id = 0;
		uint64 setToPve = 0;

		try {
			tribe_id = std::stoull(*parsed[1]);
			setToPve = std::stoull(*parsed[2]);
			if (setToPve != 0 && setToPve != 1) {
				Log::GetLog()->warn("({} {}) Parsing error: setToPve can only be a 1 or 0.", __FILE__, __FUNCTION__);
				if (NPP::EnableDebugging) {
					Log::GetLog()->info("Exiting {}.", __FUNCTION__);
				}

				return;
			}
		}
		catch (const std::exception& exception) {
			Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			if (NPP::EnableDebugging) {
				Log::GetLog()->info("Exiting {}.", __FUNCTION__);
			}

			return;
		}

		auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();

		//look for tribe
		for (const auto& allData : all_players_) {
			if (allData->tribe_id == tribe_id) {

				if (IsAdmin(allData->steam_id)) {
					continue;
				}

				found = true;
			}
		}
		//if tribe found
		if (found || std::count(NPP::pveTribesList.begin(), NPP::pveTribesList.end(), tribe_id) > 0 
			|| std::count(NPP::removedPveTribesList.begin(), NPP::removedPveTribesList.end(), tribe_id) > 0) {

			if (setToPve == 1) {
				if (std::count(NPP::pveTribesList.begin(), NPP::pveTribesList.end(), tribe_id) < 1) {
					NPP::pveTribesList.push_back(tribe_id);

					if (std::count(NPP::removedPveTribesList.begin(), NPP::removedPveTribesList.end(), tribe_id) > 0) {
						const auto iter = std::find_if(
							NPP::removedPveTribesList.begin(), NPP::removedPveTribesList.end(),
							[tribe_id](const uint64 data) {
							return data == tribe_id;
						});

						if (iter != NPP::removedPveTribesList.end()) {
							NPP::removedPveTribesList.erase(std::remove(NPP::removedPveTribesList.begin(), 
								NPP::removedPveTribesList.end(), *iter), NPP::removedPveTribesList.end());
						}
					}

					//display pve tribe added message
					rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminPVETribeAddedSuccessMessage, tribe_id));

					if (NPP::EnableDebugging) {
						Log::GetLog()->info("RCON enabled PVE status of Tribe: {}.", tribe_id);
					}
				}
				else {
					//display pve tribe already set message
					rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminPVETribeAlreadyAddedMessage, tribe_id));
				}
			}
			else {
				if (std::count(NPP::removedPveTribesList.begin(), NPP::removedPveTribesList.end(), tribe_id) < 1) {
					NPP::removedPveTribesList.push_back(tribe_id);

					if (std::count(NPP::pveTribesList.begin(), NPP::pveTribesList.end(), tribe_id) > 0) {
						const auto iter = std::find_if(
							NPP::pveTribesList.begin(), NPP::pveTribesList.end(),
							[tribe_id](const uint64 data) {
							return data == tribe_id;
						});

						if (iter != NPP::pveTribesList.end()) {
							NPP::pveTribesList.erase(std::remove(NPP::pveTribesList.begin(), 
								NPP::pveTribesList.end(), *iter), NPP::pveTribesList.end());
						}

						//display tribe removed message
						rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminPVETribeRemovedSuccessMessage, tribe_id));

						if (NPP::EnableDebugging) {
							Log::GetLog()->info("RCON disabled PVE status of Tribe: {}.", tribe_id);
						}
					}
					else {
						//display tribe already removed message
						rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminPVETribeAlreadyRemovedMessage, tribe_id));
					}
				}
			}
		}
		// tribe not found 
		else {
			//display tribe not found
			rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NPP::AdminNoTribeExistsMessage, tribe_id));
		}
	}

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void InitChatCommands() {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	FString cmd1 = NPP::NPPCommandPrefix;
	cmd1 = cmd1.Append("npp");
	ArkApi::GetCommands().AddChatCommand(cmd1, &ChatCommand);

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void RemoveChatCommands() {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	FString cmd1 = NPP::NPPCommandPrefix;
	cmd1 = cmd1.Append("npp");
	ArkApi::GetCommands().RemoveChatCommand(cmd1);

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void ResetPlayerProtection() {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	auto all_players_ = NPP::TimerProt::Get().GetAllPlayers();
	auto all_online_players_ = NPP::TimerProt::Get().GetOnlinePlayers();

	//set all players to protected
	for (const auto& allData : all_players_) {
		allData->isNewPlayer = 1;
	}

	for (const auto& Data : all_online_players_) {
		Data->isNewPlayer = 1;
	}

	ReloadProtectedTribesArray();
	RemoveExpiredTribesProtection();

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

void ReloadConfig() {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	RemoveChatCommands();
	LoadConfig();
	InitChatCommands();
	ResetPlayerProtection();

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void ConsoleReloadConfig(APlayerController* player, FString* cmd, bool boolean) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	const auto shooter_controller = static_cast<AShooterPlayerController*>(player);

	//if not Admin
	if (!shooter_controller || !shooter_controller->PlayerStateField() || !shooter_controller->bIsAdmin().Get()) {
		if (NPP::EnableDebugging) {
			Log::GetLog()->info("Exiting {}.", __FUNCTION__);
		}

		return;
	}

	ReloadConfig();

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void RconReloadConfig(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*) {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	ReloadConfig();

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void InitCommands() {
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	InitChatCommands();
	ArkApi::GetCommands().AddConsoleCommand("NPP.RemoveProtection", &ConsoleRemoveProtection);
	ArkApi::GetCommands().AddRconCommand("NPP.RemoveProtection",	&RconRemoveProtection);
	ArkApi::GetCommands().AddConsoleCommand("NPP.ResetProtection",	&ConsoleResetProtection);
	ArkApi::GetCommands().AddRconCommand("NPP.ResetProtection",		&RconResetProtection);
	ArkApi::GetCommands().AddConsoleCommand("NPP.AddProtection",	&ConsoleAddProtection);
	ArkApi::GetCommands().AddRconCommand("NPP.AddProtection",		&RconAddProtection);
	ArkApi::GetCommands().AddConsoleCommand("NPP.ReloadConfig",		&ConsoleReloadConfig);
	ArkApi::GetCommands().AddRconCommand("NPP.ReloadConfig",		&RconReloadConfig);
	ArkApi::GetCommands().AddConsoleCommand("NPP.SetPVE",			&ConsoleSetPVE);
	ArkApi::GetCommands().AddRconCommand("NPP.SetPVE",				&RconSetPVE);

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}

inline void RemoveCommands() {	
	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Entering {}.", __FUNCTION__);
	}

	RemoveChatCommands();
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.RemoveProtection");
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.ResetProtection");
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.AddProtection");
	ArkApi::GetCommands().RemoveRconCommand("NPP.RemoveProtection");
	ArkApi::GetCommands().RemoveRconCommand("NPP.ResetProtection");
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.ReloadConfig");
	ArkApi::GetCommands().RemoveRconCommand("NPP.ReloadConfig");
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.SetPVE");
	ArkApi::GetCommands().RemoveRconCommand("NPP.SetPVE");

	if (NPP::EnableDebugging) {
		Log::GetLog()->info("Exiting {}.", __FUNCTION__);
	}
}
