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

	ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(0, 255, 0), "{} updated all structures to default!",
		shooter_controller->GetPlayerCharacter()->PlayerNameField().ToString().c_str());
}

inline void Info(AShooterPlayerController* player, FString* message, int mode)
{
	ArkApi::GetApiUtils().SendNotification(player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
		*NewPlayerProtection::NPPInfoMessage, NewPlayerProtection::DaysOfProtection, NewPlayerProtection::MaxLevel);
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
		auto protectionDaysInHours = std::chrono::hours(24 * NewPlayerProtection::DaysOfProtection);
		auto now = std::chrono::system_clock::now();
		auto endTime = now - protectionDaysInHours;
		auto expireTime = std::chrono::duration_cast<std::chrono::minutes>(oldestDate - endTime);
		auto daysLeft = expireTime/1440;
		auto minutesLeft= (1440 * daysLeft) - expireTime;

		//calculate level
		int levelsLeft = NewPlayerProtection::MaxLevel - highestLevel;

		//display time/level remaining message
		ArkApi::GetApiUtils().SendNotification(player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
			*NewPlayerProtection::NPPRemainingMessage, daysLeft.count(), minutesLeft.count(), levelsLeft);
	}
	else//else not new player
	{
		//display not under protection message
		ArkApi::GetApiUtils().SendNotification(player, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
			*NewPlayerProtection::NotANewPlayerMessage);
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

	if (parsed.IsValidIndex(2))
	{
		uint64 tribe_id = 0;

		try
		{
			tribe_id = std::stoull(*parsed[2]);
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

	if (parsed.IsValidIndex(2))
	{
		uint64 tribe_id = 0;

		try
		{
			tribe_id = std::stoull(*parsed[2]);
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
					//add protection & increase start date
					if (allData->tribe_id == tribe_id)
					{
						allData->isNewPlayer = 1;
						allData->startDateTime = now;
					}
				}

				for (const auto& onlineData : online_players_)
				{
					//add protection & increase start date
					if (onlineData->tribe_id == tribe_id)
					{
						onlineData->isNewPlayer = 1;
						onlineData->startDateTime = now;
					}
				}

				//display protection added message
				ArkApi::GetApiUtils().SendNotification(shooter_controller, NewPlayerProtection::MessageColor, NewPlayerProtection::MessageTextSize, NewPlayerProtection::MessageDisplayDelay, nullptr,
					*NewPlayerProtection::AdminResetTribeProtectionSuccess, tribe_id, NewPlayerProtection::DaysOfProtection);
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

	if (parsed.IsValidIndex(2))
	{
		uint64 tribe_id = 0;

		try
		{
			tribe_id = std::stoull(*parsed[2]);
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
				reply = NewPlayerProtection::AdminTribeProtectionRemoved, tribe_id;
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
			}
			else //tribe not protected
			{
				//display tribe not under protection
				reply = NewPlayerProtection::AdminTribeNotUnderProtection, tribe_id;
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
			}
		}
		else // tribe not found
		{
			//display tribe not found
			reply = NewPlayerProtection::AdminNoTribeExistsMessage, tribe_id;
			rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
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

	if (parsed.IsValidIndex(2))
	{
		uint64 tribe_id = 0;

		try
		{
			tribe_id = std::stoull(*parsed[2]);
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
					//add protection & increase start date
					if (allData->tribe_id == tribe_id)
					{
						allData->isNewPlayer = 1;
						allData->startDateTime = now;
					}
				}

				for (const auto& onlineData : online_players_)
				{
					//add protection & increase start date
					if (onlineData->tribe_id == tribe_id)
					{
						onlineData->isNewPlayer = 1;
						onlineData->startDateTime = now;
					}
				}

				//display protection added message
				reply = NewPlayerProtection::AdminResetTribeProtectionSuccess, tribe_id, NewPlayerProtection::DaysOfProtection;
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
			}
			else //tribe not under max level
			{
				//display tribe under max level message
				reply = NewPlayerProtection::AdminResetTribeProtectionLvlFailure, tribe_id;
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
			}
		}
		else // tribe not found
		{
			//display tribe not found
			reply = NewPlayerProtection::AdminNoTribeExistsMessage, tribe_id;
			rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
		}
	}
}

inline void InitCommands()
{
	ArkApi::GetCommands().AddConsoleCommand("NPP.ResetStructures", &ResetStructures);
	ArkApi::GetCommands().AddChatCommand("!npp info", &Info);
	ArkApi::GetCommands().AddChatCommand("!npp disable", &Disable);
	ArkApi::GetCommands().AddChatCommand("!npp status", &Status);
	ArkApi::GetCommands().AddConsoleCommand("NPP.RemoveProtection", &ConsoleRemoveProtection);
	ArkApi::GetCommands().AddConsoleCommand("NPP.ResetProtectionDays", &ConsoleResetProtectionDays);
	ArkApi::GetCommands().AddRconCommand("NPP.RemoveProtection", &RconRemoveProtection);
	ArkApi::GetCommands().AddRconCommand("NPP.ResetProtectionDays", &RconResetProtectionDays);
}

inline void RemoveCommands()
{
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.ResetStructures");
	ArkApi::GetCommands().RemoveChatCommand("!npp info");
	ArkApi::GetCommands().RemoveChatCommand("!npp disable");
	ArkApi::GetCommands().RemoveChatCommand("!npp status");
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.RemoveProtection");
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.ResetProtectionDays");
	ArkApi::GetCommands().RemoveRconCommand("NPP.RemoveProtection");
	ArkApi::GetCommands().RemoveRconCommand("NPP.ResetProtectionDays");
}