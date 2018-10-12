#pragma once

DECLARE_HOOK(APrimalStructure_FinalStructurePlacement, bool, APrimalStructure*, APlayerController*, FVector, FRotator, FRotator, APawn *, FName, bool);
DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*,
AShooterCharacter*, bool);

void InitHooks()
{
	ArkApi::GetHooks().SetHook("APrimalStructure.FinalStructurePlacement", &Hook_APrimalStructure_FinalStructurePlacement, reinterpret_cast<LPVOID*>(&APrimalStructure_FinalStructurePlacement_original));
	ArkApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer,
		&AShooterGameMode_HandleNewPlayer_original);
}

void RemoveHooks()
{
	ArkApi::GetHooks().DisableHook("APrimalStructure.FinalStructurePlacement", &Hook_APrimalStructure_FinalStructurePlacement);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer);
}

void setStructureInv(APrimalStructure* _this)
{
	_this->bCanBeDamaged() = false;
}

bool _cdecl Hook_APrimalStructure_FinalStructurePlacement(APrimalStructure* _this, APlayerController * PC, FVector AtLocation, FRotator AtRotation, FRotator PlayerViewRotation, APawn * AttachToPawn, FName BoneName, bool bIsFlipped)
{
	setStructureInv(_this);
	return APrimalStructure_FinalStructurePlacement_original(_this, PC, AtLocation, AtRotation, PlayerViewRotation, AttachToPawn, BoneName, bIsFlipped);
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

sqlite::database& NewPlayerProtection::GetDB()
{
	static sqlite::database db(config["NewPlayerProtection"].value("DbPathOverride", "").empty()
		? ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/NewPlayerProtection/NewPlayerProtection.db"
		: config["NewPlayerProtection"]["DbPathOverride"]);
	return db;
}

bool Hook_AShooterGameMode_HandleNewPlayer(AShooterGameMode* _this, AShooterPlayerController* new_player,
	UPrimalPlayerData* player_data, AShooterCharacter* player_character,
	bool is_from_login)
{
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(new_player);

	if (!IsPlayerExists(steam_id))
	{
		auto& db = NewPlayerProtection::GetDB();

		try
		{
			db << "INSERT INTO Players (SteamId) VALUES (?);"
				<< steam_id;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);
		}
	}

	return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);
}


