#pragma once

#include <Permissions.h>

#include "NewPlayerProtection.h"

inline void ProtectStructure(APlayerController* player, FString* message, bool boolean )
{
	AShooterPlayerController* shooter_controller = static_cast<AShooterPlayerController*>(player);
	if (!shooter_controller || !shooter_controller->PlayerStateField() || RequiresAdmin && !shooter_controller->bIsAdmin()())
		return;

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(shooter_controller);
	if (RequiresPermission && !Permissions::IsPlayerHasPermission(steam_id, "StructureHealth"))
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0),
		                                        "You don't have permissions to use this command");
		return;
	}

	TArray<FString> Parsed;
	message->ParseIntoArray(Parsed, L" ", true);

	if (Parsed.Num() < 3 )
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0), "Incorrect Syntax: StructureHealth.ProtectStructure <AttachedRadius> <HealthMultiplier>");
		return;
	}

	int AttachedRadius = -1;
	int HealthMultiplier = 1;

	try
	{
		AttachedRadius = std::stoi(Parsed[1].ToString().c_str());
		HealthMultiplier = std::stoi(Parsed[2].ToString().c_str());
	}
	catch (...)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0), "Incorrect Syntax: StructureHealth.ProtectStructure <AttachedRadius> <HealthMultiplier>");
	}

	if (AttachedRadius < 0 || AttachedRadius > 1000)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0), "<AttachedRadius> must be between 0 and 1000!");
		return;
	}
	if (HealthMultiplier < 1)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0), "<HealthMultiplier> must be greater than 0!");
		return;
	}
	
	AActor* Actor = shooter_controller->GetPlayerCharacter()->GetAimedActor(ECC_GameTraceChannel2, nullptr, 0.0, 0.0, nullptr, nullptr,
	                                                            false, false);
	if (Actor && Actor->IsA(APrimalStructure::GetPrivateStaticClass()))
	{
		APrimalStructure* Structure = static_cast<APrimalStructure*>(Actor);
		APrimalStructure* default_structure = static_cast<APrimalStructure*>(Structure->ClassField()->GetDefaultObject(true));
		AShooterPlayerState* ASPS = static_cast<AShooterPlayerState*>(shooter_controller->PlayerStateField());

		if (ASPS && ASPS->MyPlayerDataStructField())
		{
			Structure->MaxHealthField() = (default_structure->HealthField() * HealthMultiplier);
			Structure->HealthField() = (default_structure->HealthField() * HealthMultiplier);
			Structure->UpdatedHealth(true);
			TArray<APrimalStructure*> linkedStructures = Structure->LinkedStructuresField();

			for(int cnt = 0; cnt < AttachedRadius-1; cnt++)
			{
				TArray<APrimalStructure*> tempLS;

				for (APrimalStructure* ls : linkedStructures)
				{
					tempLS.Append(ls->LinkedStructuresField());

				}
				for (APrimalStructure* templs : tempLS)
				{
					linkedStructures.AddUnique(templs);
				}
			}

			for (APrimalStructure* ls : linkedStructures)
			{
				ls->MaxHealthField() = (default_structure->HealthField() * HealthMultiplier);
				ls->HealthField() = (default_structure->HealthField() * HealthMultiplier);
				ls->UpdatedHealth(true);
			}
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(0, 255, 0), "{} updated structure health by {} times default!",
				shooter_controller->GetPlayerCharacter()->PlayerNameField().ToString().c_str(), HealthMultiplier);
		}
	}
	else
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0),
		                                        "Please face the middle of your screen towards the Structure you want to change.");
	}
}

inline void UnprotectStructure(APlayerController* player, FString* message, bool boolean)
{
	AShooterPlayerController* shooter_controller = static_cast<AShooterPlayerController*>(player);
	if (!shooter_controller || !shooter_controller->PlayerStateField() || RequiresAdmin && !shooter_controller->bIsAdmin()())
		return;

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(shooter_controller);
	if (RequiresPermission && !Permissions::IsPlayerHasPermission(steam_id, "StructureHealth"))
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0),
			"You don't have permissions to use this command");
		return;
	}

	TArray<FString> Parsed;
	message->ParseIntoArray(Parsed, L" ", true);

	if (Parsed.Num() < 2)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0), "Incorrect Syntax: StructureHealth.UnprotectStructure <AttachedRadius>");
		return;
	}

	int AttachedRadius = -1;

	try
	{
		AttachedRadius = std::stoi(Parsed[1].ToString().c_str());
	}
	catch (...)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0), "Incorrect Syntax: StructureHealth.UnprotectStructure <AttachedRadius>");
	}

	if (AttachedRadius < 0 || AttachedRadius > 1000)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0), "<AttachedRadius> must be between 0 and 1000!");
		return;
	}

	AActor* Actor = shooter_controller->GetPlayerCharacter()->GetAimedActor(ECC_GameTraceChannel2, nullptr, 0.0, 0.0, nullptr, nullptr,
		false, false);
	if (Actor && Actor->IsA(APrimalStructure::GetPrivateStaticClass()))
	{
		APrimalStructure* Structure = static_cast<APrimalStructure*>(Actor);
		APrimalStructure* default_structure = static_cast<APrimalStructure*>(Structure->ClassField()->GetDefaultObject(true));
		AShooterPlayerState* ASPS = static_cast<AShooterPlayerState*>(shooter_controller->PlayerStateField());

		if (ASPS && ASPS->MyPlayerDataStructField())
		{
			const int teamId = Structure->TargetingTeamField();

			Structure->MaxHealthField() = default_structure->HealthField();
			Structure->HealthField() = default_structure->HealthField();
			Structure->UpdatedHealth(true);
			TArray<APrimalStructure*> linkedStructures = Structure->LinkedStructuresField();

			for (int cnt = 0; cnt < AttachedRadius - 1; cnt++)
			{
				TArray<APrimalStructure*> tempLS;

				for (APrimalStructure* ls : linkedStructures)
				{
					tempLS.Append(ls->LinkedStructuresField());

				}
				for (APrimalStructure* templs : tempLS)
				{
					linkedStructures.AddUnique(templs);
				}
			}

			for (APrimalStructure* ls : linkedStructures)
			{
				ls->MaxHealthField() = default_structure->HealthField();
				ls->HealthField() = default_structure->HealthField();
				ls->UpdatedHealth(true);
			}
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(0, 255, 0), "{} updated structure health to default!",
				shooter_controller->GetPlayerCharacter()->PlayerNameField().ToString().c_str());
		}
	}
	else
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0),
			"Please face the middle of your screen towards the Structure you want to change.");
	}
}

inline void ProtectTribe(APlayerController* player, FString* message, bool boolean)
{
	AShooterPlayerController* shooter_controller = static_cast<AShooterPlayerController*>(player);
	if (!shooter_controller || !shooter_controller->PlayerStateField() || RequiresAdmin && !shooter_controller->bIsAdmin()())
		return;

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(shooter_controller);
	if (RequiresPermission && !Permissions::IsPlayerHasPermission(steam_id, "StructureHealth"))
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0),
			"You don't have permissions to use this command");
		return;
	}

	TArray<FString> Parsed;
	message->ParseIntoArray(Parsed, L" ", true);

	if (Parsed.Num() < 2)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0), "Incorrect Syntax: StructureHealth.ProtectTribe <HealthMultiplier>");
		return;
	}

	int HealthMultiplier = 1;

	try
	{
		HealthMultiplier = std::stoi(Parsed[1].ToString().c_str());
	}
	catch (...)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0), "Incorrect Syntax: StructureHealth.ProtectTribe <HealthMultiplier>");
	}

	if (HealthMultiplier < 1)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0), "<AttachedRadius> must be greater than 0!");
		return;
	}

	AActor* Actor = shooter_controller->GetPlayerCharacter()->GetAimedActor(ECC_GameTraceChannel2, nullptr, 0.0, 0.0, nullptr, nullptr,
		false, false);
	if (Actor && Actor->IsA(APrimalStructure::GetPrivateStaticClass()))
	{
		APrimalStructure* Structure = static_cast<APrimalStructure*>(Actor);
		APrimalStructure* default_structure = static_cast<APrimalStructure*>(Structure->ClassField()->GetDefaultObject(true));
		AShooterPlayerState* ASPS = static_cast<AShooterPlayerState*>(shooter_controller->PlayerStateField());

		if (ASPS && ASPS->MyPlayerDataStructField())
		{
			const int teamId = Structure->TargetingTeamField();

			/**
			* \brief Finds all Structures owned by team
			*/
			TArray<AActor*> AllStructures;
			UGameplayStatics::GetAllActorsOfClass(reinterpret_cast<UObject*>(ArkApi::GetApiUtils().GetWorld()), APrimalStructure::GetPrivateStaticClass(), &AllStructures);
			TArray<APrimalStructure*> FoundStructures;
			APrimalStructure* Struc;

			for (AActor* StructActor : AllStructures)
			{
				if (!StructActor || StructActor->TargetingTeamField() != teamId || !Actor->IsA(APrimalStructure::GetPrivateStaticClass())) continue;
				Struc = static_cast<APrimalStructure*>(StructActor);
				FoundStructures.Add(Struc);
			}

			for (APrimalStructure* st : FoundStructures)
			{
				st->MaxHealthField() = (default_structure->HealthField() * HealthMultiplier);
				st->HealthField() = (default_structure->HealthField() * HealthMultiplier);
				st->UpdatedHealth(true);
			}
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(0, 255, 0), "{} updated tribe structure health by {} times default!",
				shooter_controller->GetPlayerCharacter()->PlayerNameField().ToString().c_str(), HealthMultiplier);
		}
	}
	else
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0),
			"Please face the middle of your screen towards the Structure you want to change.");
	}
}

inline void UnprotectTribe(APlayerController* player, FString* message, bool boolean)
{
	AShooterPlayerController* shooter_controller = static_cast<AShooterPlayerController*>(player);
	if (!shooter_controller || !shooter_controller->PlayerStateField() || RequiresAdmin && !shooter_controller->bIsAdmin()())
		return;

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(shooter_controller);
	if (RequiresPermission && !Permissions::IsPlayerHasPermission(steam_id, "StructureHealth"))
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0),
			"You don't have permissions to use this command");
		return;
	}

	AActor* Actor = shooter_controller->GetPlayerCharacter()->GetAimedActor(ECC_GameTraceChannel2, nullptr, 0.0, 0.0, nullptr, nullptr,
		false, false);
	if (Actor && Actor->IsA(APrimalStructure::GetPrivateStaticClass()))
	{
		APrimalStructure* Structure = static_cast<APrimalStructure*>(Actor);
		APrimalStructure* default_structure = static_cast<APrimalStructure*>(Structure->ClassField()->GetDefaultObject(true));
		AShooterPlayerState* ASPS = static_cast<AShooterPlayerState*>(shooter_controller->PlayerStateField());

		if (ASPS && ASPS->MyPlayerDataStructField())
		{
			const int teamId = Structure->TargetingTeamField();

			/**
			* \brief Finds all Structures owned by team
			*/
			TArray<AActor*> AllStructures;
			UGameplayStatics::GetAllActorsOfClass(reinterpret_cast<UObject*>(ArkApi::GetApiUtils().GetWorld()), APrimalStructure::GetPrivateStaticClass(), &AllStructures);
			TArray<APrimalStructure*> FoundStructures;
			APrimalStructure* Struc;

			for (AActor* StructActor : AllStructures)
			{
				if (!StructActor || StructActor->TargetingTeamField() != teamId || !Actor->IsA(APrimalStructure::GetPrivateStaticClass())) continue;
				Struc = static_cast<APrimalStructure*>(StructActor);
				FoundStructures.Add(Struc);
			}

			for (APrimalStructure* st : FoundStructures)
			{
				st->MaxHealthField() = default_structure->HealthField();
				st->HealthField() = default_structure->HealthField();
				st->UpdatedHealth(true);
			}
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(0, 255, 0), "{} updated tribe structure health to default!",
				shooter_controller->GetPlayerCharacter()->PlayerNameField().ToString().c_str());
		}
	}
	else
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0),
			"Please face the middle of your screen towards the Structure you want to change.");
	}
}

inline void InvStructure(APlayerController* player, FString* message, bool boolean)
{
	AShooterPlayerController* shooter_controller = static_cast<AShooterPlayerController*>(player);
	if (!shooter_controller || !shooter_controller->PlayerStateField() || RequiresAdmin && !shooter_controller->bIsAdmin()())
		return;

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(shooter_controller);
	if (RequiresPermission && !Permissions::IsPlayerHasPermission(steam_id, "StructureHealth"))
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0),
			"You don't have permissions to use this command");
		return;
	}

	TArray<FString> Parsed;
	message->ParseIntoArray(Parsed, L" ", true);

	if (Parsed.Num() < 2)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0), "Incorrect Syntax: StructureHealth.InvStructure <Boolean>");
		return;
	}

	int Boolean = -1;

	try
	{
		Boolean = std::stoi(Parsed[1].ToString().c_str());
	}
	catch (...)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0), "Incorrect Syntax: StructureHealth.InvStructure <Boolean>");
	}

	if (Boolean < 0 || Boolean > 1)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0), "<Boolean> must be between 0 and 1!");
		return;
	}
	

	AActor* Actor = shooter_controller->GetPlayerCharacter()->GetAimedActor(ECC_GameTraceChannel2, nullptr, 0.0, 0.0, nullptr, nullptr,
		false, false);
	if (Actor && Actor->IsA(APrimalStructure::GetPrivateStaticClass()))
	{
		APrimalStructure* Structure = static_cast<APrimalStructure*>(Actor);
		APrimalStructure* default_structure = static_cast<APrimalStructure*>(Structure->ClassField()->GetDefaultObject(true));
		AShooterPlayerState* ASPS = static_cast<AShooterPlayerState*>(shooter_controller->PlayerStateField());

		if (ASPS && ASPS->MyPlayerDataStructField())
		{
			Structure->bCanBeDamaged() = false;
			Structure->UpdatedHealth(true);
			/*TArray<APrimalStructure*> linkedStructures = Structure->LinkedStructuresField();

			for (int cnt = 0; cnt < AttachedRadius - 1; cnt++)
			{
				TArray<APrimalStructure*> tempLS;

				for (APrimalStructure* ls : linkedStructures)
				{
					tempLS.Append(ls->LinkedStructuresField());

				}
				for (APrimalStructure* templs : tempLS)
				{
					linkedStructures.AddUnique(templs);
				}
			}

			for (APrimalStructure* ls : linkedStructures)
			{
				ls->MaxHealthField() = (default_structure->HealthField() * HealthMultiplier);
				ls->HealthField() = (default_structure->HealthField() * HealthMultiplier);
				ls->UpdatedHealth(true);
			}*/
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(0, 255, 0), "{} updated structure to be invincible!",
				shooter_controller->GetPlayerCharacter()->PlayerNameField().ToString().c_str());
		}
	}
	else
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0),
			"Please face the middle of your screen towards the Structure you want to make invincible.");
	}
}

inline void InitCommands()
{
	ArkApi::GetCommands().AddConsoleCommand("NewPlayerProtection.ProtectStructure", &ProtectStructure);
	ArkApi::GetCommands().AddConsoleCommand("NewPlayerProtection.UnprotectStructure", &UnprotectStructure);
	ArkApi::GetCommands().AddConsoleCommand("NewPlayerProtection.ProtectTribe", &ProtectTribe);
	ArkApi::GetCommands().AddConsoleCommand("NewPlayerProtection.UnprotectTribe", &UnprotectTribe);
	ArkApi::GetCommands().AddConsoleCommand("NewPlayerProtection.InvStructure", &InvStructure);
}

inline void RemoveCommands()
{
	ArkApi::GetCommands().RemoveConsoleCommand("NewPlayerProtection.ProtectStructure");
	ArkApi::GetCommands().RemoveConsoleCommand("NewPlayerProtection.UnprotectStructure");
	ArkApi::GetCommands().RemoveConsoleCommand("NewPlayerProtection.ProtectTribe");
	ArkApi::GetCommands().RemoveConsoleCommand("NewPlayerProtection.UnprotectTribe");
	ArkApi::GetCommands().RemoveConsoleCommand("NewPlayerProtection.InvStructure");
}
