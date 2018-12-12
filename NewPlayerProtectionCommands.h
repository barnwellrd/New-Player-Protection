#pragma once

inline void ResetStructures(APlayerController* player_controller, FString*, bool)
{
	AShooterPlayerController* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);
	if (!shooter_controller || !shooter_controller->PlayerStateField() || !shooter_controller->bIsAdmin().Get())
		return;

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(shooter_controller);

	/**
	* \brief Finds all Structures owned by team
	*/
	TArray<AActor*> AllStructures;
	UGameplayStatics::GetAllActorsOfClass(reinterpret_cast<UObject*>(ArkApi::GetApiUtils().GetWorld()), APrimalStructure::GetPrivateStaticClass(), &AllStructures);

	for (AActor* StructActor : AllStructures)
	{
		if (!StructActor) continue;
		StructActor = static_cast<APrimalStructure*>(StructActor);
		APrimalStructure* defaultstruc = static_cast<APrimalStructure*>(StructActor->ClassField()->GetDefaultObject(true));
		StructActor->bCanBeDamaged() = defaultstruc->bCanBeDamaged().Get();
	}

	ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(0, 255, 0), "Updated all structures to default!");
}

inline void Info(AShooterPlayerController* player, FString* message, int mode)
{
	ArkApi::GetApiUtils().SendNotification(player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
		*NewPlayerProtection::NPPInfoMessage, NewPlayerProtection::HoursOfProtection, NewPlayerProtection::MaxLevel);
}

inline void Disable(AShooterPlayerController* player, FString* message, int mode)
{
	if (!player || !player->PlayerStateField() || ArkApi::IApiUtils::IsPlayerDead(player) || !NewPlayerProtection::AllowPlayersToDisableOwnedTribeProtection)
		return;

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player);

	//if new player
	if (IsPlayerProtected(player))
	{
		//if tribe admin
		if (player->IsTribeAdmin())
		{
			//remove protection from all tribe members
			uint64 tribe_id = player->TargetingTeamField();

			auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();
			auto online_players_ = NewPlayerProtection::TimerProt::Get().GetOnlinePlayers();

			for (const auto& allData : all_players_)
			{
				if (allData->tribe_id == tribe_id)
				{
					allData->isNewPlayer = 0;
				}
			}

			for (const auto& onlineData : online_players_)
			{
				if (onlineData->tribe_id == tribe_id)
				{
					onlineData->isNewPlayer = 0;
				}
			}
			//display protection removed message
			ArkApi::GetApiUtils().SendNotification(player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
				*NewPlayerProtection::NewPlayerProtectionDisableSuccess);
		}
		else //else not tribe admin
		{
			//display not tribe admin message
			ArkApi::GetApiUtils().SendNotification(player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
				*NewPlayerProtection::NotTribeAdminMessage);
		}
	}
	else	//else not new player
	{
		//display not under protection message
		ArkApi::GetApiUtils().SendNotification(player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
			*NewPlayerProtection::NotANewPlayerMessage);
	}
}

inline void Status(AShooterPlayerController* player, FString* message, int mode)
{
	if (!player || !player->PlayerStateField() || ArkApi::IApiUtils::IsPlayerDead(player))
		return;

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player);

	//if new player
	if (IsPlayerProtected(player))
	{
		//loop through tribe member
		uint64 tribe_id = player->TargetingTeamField();
		std::chrono::time_point<std::chrono::system_clock> oldestDate = std::chrono::system_clock::now();;
		int highestLevel = 0;

		auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();

		//Loop through tribe
		for (const auto& allData : all_players_)
		{
			if (IsAdmin(allData->steam_id))
			{
				continue;
			}

			if (allData->tribe_id == tribe_id)
			{
				//get oldest date started
				if (allData->startDateTime < oldestDate)
				{
					oldestDate = allData->startDateTime;
				}

				//get highest level player
				if (allData->level > highestLevel)
				{
					highestLevel = allData->level;
				}
			}
		}

		//calulate time
		auto protectionDaysInHours = std::chrono::hours(NewPlayerProtection::HoursOfProtection);
		auto now = std::chrono::system_clock::now();
		auto endTime = now - protectionDaysInHours;
		auto expireTime = std::chrono::duration_cast<std::chrono::minutes>(oldestDate - endTime);
		auto daysLeft = expireTime / 1440;
		auto hoursLeft = ((expireTime - (1440 * daysLeft)) / 60);
		auto minutesLeft =  (expireTime - ((1440 * daysLeft) + (60 * hoursLeft)));

		//calculate level
		int levelsLeft = NewPlayerProtection::MaxLevel - highestLevel;

		//display time/level remaining message
		ArkApi::GetApiUtils().SendNotification(player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
			*NewPlayerProtection::NPPRemainingMessage, daysLeft.count(), hoursLeft.count(),  minutesLeft.count(), levelsLeft);
	}
	else//else not new player
	{
		//display not under protection message
		ArkApi::GetApiUtils().SendNotification(player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
			*NewPlayerProtection::NotANewPlayerMessage);
	}
}

inline void ChatCommand(AShooterPlayerController* player, FString* message, int mode)
{
	TArray<FString> parsed;
	message->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		FString input = parsed[1];
		if (input.Compare("info") == 0)
		{
			Info(player, message, mode);
		}
		else if (input.Compare("status") == 0)
		{
			Status(player, message, mode);
		}
		else if (input.Compare("disable") == 0)
		{
			Disable(player, message, mode);
		}
		else
		{
			ArkApi::GetApiUtils().SendNotification(player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
				*NewPlayerProtection::NPPInvalidCommand);
		}
	}
	else
	{
		ArkApi::GetApiUtils().SendNotification(player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
			*NewPlayerProtection::NPPInvalidCommand);
	}
}

inline void ConsoleRemoveProtection(APlayerController* player_controller, FString* cmd, bool)
{
	const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

	//if Admin
	if (!shooter_controller || !shooter_controller->PlayerStateField() || !shooter_controller->bIsAdmin().Get())
		return;

	bool found = false;
	bool isProtected = false;

	//parse command
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		uint64 tribe_id = 0;

		try
		{
			tribe_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();

		//look for tribe
		for (const auto& allData : all_players_)
		{
			if (allData->tribe_id == tribe_id)
			{
				found = true;
				if (allData->isNewPlayer == 1)
				{
					isProtected = true;
					break;
				}
			}
		}
		//if tribe found
		if (found)
		{
			//if tribe is protected
			if (isProtected)
			{
				auto online_players_ = NewPlayerProtection::TimerProt::Get().GetOnlinePlayers();

				//loop through tribe members
				for (const auto& allData : all_players_)
				{
					//remove protection
					if (allData->tribe_id == tribe_id)
					{
						allData->isNewPlayer = 0;
					}
				}

				for (const auto& onlineData : online_players_)
				{
					//remove protection
					if (onlineData->tribe_id == tribe_id)
					{
						onlineData->isNewPlayer = 0;
					}
				}

				//display protection removed message
				ArkApi::GetApiUtils().SendNotification(shooter_controller, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
					*NewPlayerProtection::AdminTribeProtectionRemoved, tribe_id);
			}
			else //tribe not protected
			{
				//display tribe not under protection
				ArkApi::GetApiUtils().SendNotification(shooter_controller, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
					*NewPlayerProtection::AdminTribeNotUnderProtection, tribe_id);
			}
		}
		else // tribe not found
		{
			//display tribe not found
			ArkApi::GetApiUtils().SendNotification(shooter_controller, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
				*NewPlayerProtection::AdminNoTribeExistsMessage, tribe_id);
		}
	}
}

inline void ConsoleResetProtectionDays(APlayerController* player, FString* cmd, bool boolean)
{
	const auto shooter_controller = static_cast<AShooterPlayerController*>(player);

	//if Admin
	if (!shooter_controller || !shooter_controller->PlayerStateField() || !shooter_controller->bIsAdmin().Get())
		return;

	bool found = false;
	bool underMaxLevel = true;

	//parse command
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		uint64 tribe_id = 0;

		try
		{
			tribe_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();

		//look for tribe
		for (const auto& allData : all_players_)
		{
			if (allData->tribe_id == tribe_id)
			{
				if (IsAdmin(allData->steam_id))
				{
					continue;
				}
				found = true;
				if (allData->level > NewPlayerProtection::MaxLevel)
				{
					underMaxLevel = false;
					break;
				}
			}
		}
		//if tribe found
		if (found)
		{
			//if tribe is under max level
			if (underMaxLevel)
			{
				auto online_players_ = NewPlayerProtection::TimerProt::Get().GetOnlinePlayers();
				auto now = std::chrono::system_clock::now();

				//loop through tribe members
				for (const auto& allData : all_players_)
				{
					if (IsAdmin(allData->steam_id))
					{
						continue;
					}

					//add protection & increase start date
					if (allData->tribe_id == tribe_id)
					{
						allData->isNewPlayer = 1;
						allData->startDateTime = now;
					}
				}

				for (const auto& onlineData : online_players_)
				{
					if (IsAdmin(onlineData->steam_id))
					{
						continue;
					}

					//add protection & increase start date
					if (onlineData->tribe_id == tribe_id)
					{
						onlineData->isNewPlayer = 1;
						onlineData->startDateTime = now;
					}
				}

				//display protection added message
				ArkApi::GetApiUtils().SendNotification(shooter_controller, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
					*NewPlayerProtection::AdminResetTribeProtectionSuccess, NewPlayerProtection::HoursOfProtection, tribe_id);
			}
			else //tribe not under max level
			{
				//display tribe under max level message
				ArkApi::GetApiUtils().SendNotification(shooter_controller, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
					*NewPlayerProtection::AdminResetTribeProtectionLvlFailure, tribe_id);
			}
		}
		else // tribe not found
		{
			//display tribe not found
			ArkApi::GetApiUtils().SendNotification(shooter_controller, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
				*NewPlayerProtection::AdminNoTribeExistsMessage, tribe_id);
		}
	}
}

inline void RconRemoveProtection(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString reply;

	bool found = false;
	bool isProtected = false;

	//parse command
	TArray<FString> parsed;
	rcon_packet->Body.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		uint64 tribe_id = 0;

		try
		{
			tribe_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();

		//look for tribe
		for (const auto& allData : all_players_)
		{
			if (allData->tribe_id == tribe_id)
			{
				found = true;
				if (allData->isNewPlayer == 1)
				{
					isProtected = true;
					break;
				}
			}
		}
		//if tribe found
		if (found)
		{
			//if tribe is protected
			if (isProtected)
			{
				auto online_players_ = NewPlayerProtection::TimerProt::Get().GetOnlinePlayers();

				//loop through tribe members
				for (const auto& allData : all_players_)
				{
					//remove protection
					if (allData->tribe_id == tribe_id)
					{
						allData->isNewPlayer = 0;
					}
				}

				for (const auto& onlineData : online_players_)
				{
					//remove protection
					if (onlineData->tribe_id == tribe_id)
					{
						onlineData->isNewPlayer = 0;
					}
				}

				//display protection removed message
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NewPlayerProtection::AdminTribeProtectionRemoved, tribe_id));
			}
			else //tribe not protected
			{
				//display tribe not under protection
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NewPlayerProtection::AdminTribeNotUnderProtection, tribe_id));
			}
		}
		else // tribe not found
		{
			//display tribe not found
			rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NewPlayerProtection::AdminNoTribeExistsMessage, tribe_id));
		}
	}
}

inline void RconResetProtectionDays(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString reply;

	bool found = false;
	bool underMaxLevel = true;

	//parse command
	TArray<FString> parsed;
	rcon_packet->Body.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		uint64 tribe_id = 0;

		try
		{
			tribe_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();

		//look for tribe
		for (const auto& allData : all_players_)
		{
			if (IsAdmin(allData->steam_id))
			{
				continue;
			}

			if (allData->tribe_id == tribe_id)
			{
				found = true;
				if (allData->level > NewPlayerProtection::MaxLevel)
				{
					underMaxLevel = false;
					break;
				}
			}
		}
		//if tribe found
		if (found)
		{
			//if tribe is under max level
			if (underMaxLevel)
			{
				auto online_players_ = NewPlayerProtection::TimerProt::Get().GetOnlinePlayers();
				auto now = std::chrono::system_clock::now();

				//loop through tribe members
				for (const auto& allData : all_players_)
				{
					if (IsAdmin(allData->steam_id))
					{
						continue;
					}

					//add protection & increase start date
					if (allData->tribe_id == tribe_id)
					{
						allData->isNewPlayer = 1;
						allData->startDateTime = now;
					}
				}

				for (const auto& onlineData : online_players_)
				{
					if (IsAdmin(onlineData->steam_id))
					{
						continue;
					}

					//add protection & increase start date
					if (onlineData->tribe_id == tribe_id)
					{
						onlineData->isNewPlayer = 1;
						onlineData->startDateTime = now;
					}
				}

				//display protection added message
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NewPlayerProtection::AdminResetTribeProtectionSuccess, NewPlayerProtection::HoursOfProtection, tribe_id));
			}
			else //tribe not under max level
			{
				//display tribe under max level message
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NewPlayerProtection::AdminResetTribeProtectionLvlFailure, tribe_id));
			}
		}
		else // tribe not found
		{
			//display tribe not found
			rcon_connection->SendMessageW(rcon_packet->Id, 0, &FString::Format(*NewPlayerProtection::AdminNoTribeExistsMessage, tribe_id));
		}
	}
}

inline void InitChatCommands()
{
	FString cmd1 = NewPlayerProtection::NPPCommandPrefix;
	cmd1 = cmd1.Append("npp");
	ArkApi::GetCommands().AddChatCommand(cmd1, &ChatCommand);
}

inline void RemoveChatCommands()
{
	FString cmd1 = NewPlayerProtection::NPPCommandPrefix;
	cmd1 = cmd1.Append("npp");
	ArkApi::GetCommands().RemoveChatCommand(cmd1);
}

inline void ResetPlayerProtection()
{
	auto all_players_ = NewPlayerProtection::TimerProt::Get().GetAllPlayers();

	//set all players to protected
	for (const auto& allData : all_players_)
	{
		allData->isNewPlayer = 1;
	}
	RemoveExpiredTribesProtection();
}

inline void ConsoleReloadConfig(APlayerController* player, FString* cmd, bool boolean)
{
	const auto shooter_controller = static_cast<AShooterPlayerController*>(player);

	//if not Admin
	if (!shooter_controller || !shooter_controller->PlayerStateField() || !shooter_controller->bIsAdmin().Get())
		return;
	RemoveChatCommands();
	ReloadConfig();
	InitChatCommands();
	ResetPlayerProtection();
}

inline void RconReloadConfig(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	RemoveChatCommands();
	ReloadConfig();
	InitChatCommands();
	ResetPlayerProtection();
}

inline void InitCommands()
{
	InitChatCommands();
	ArkApi::GetCommands().AddConsoleCommand("NPP.ResetStructures", &ResetStructures);
	ArkApi::GetCommands().AddConsoleCommand("NPP.RemoveProtection", &ConsoleRemoveProtection);
	ArkApi::GetCommands().AddConsoleCommand("NPP.ResetProtectionDays", &ConsoleResetProtectionDays);
	ArkApi::GetCommands().AddRconCommand("NPP.RemoveProtection", &RconRemoveProtection);
	ArkApi::GetCommands().AddRconCommand("NPP.ResetProtectionDays", &RconResetProtectionDays);
	ArkApi::GetCommands().AddConsoleCommand("NPP.ReloadConfig", &ConsoleReloadConfig);
	ArkApi::GetCommands().AddRconCommand("NPP.ReloadConfig", &RconReloadConfig);
}

inline void RemoveCommands()
{	
	RemoveChatCommands();
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.ResetStructures");
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.RemoveProtection");
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.ResetProtectionDays");
	ArkApi::GetCommands().RemoveRconCommand("NPP.RemoveProtection");
	ArkApi::GetCommands().RemoveRconCommand("NPP.ResetProtectionDays");
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.ReloadConfig");
	ArkApi::GetCommands().RemoveRconCommand("NPP.ReloadConfig");
}

