#pragma once

inline void ResetStructures(APlayerController* player, FString* message, bool boolean)
{
	AShooterPlayerController* shooter_controller = static_cast<AShooterPlayerController*>(player);
	if (!shooter_controller || !shooter_controller->PlayerStateField() || !shooter_controller->bIsAdmin()())
		return;

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(shooter_controller);
	if (!IsAdmin(steam_id))
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
			/**
			* \brief Finds all Structures owned by team
			*/
			TArray<AActor*> AllStructures;
			UGameplayStatics::GetAllActorsOfClass(reinterpret_cast<UObject*>(ArkApi::GetApiUtils().GetWorld()), APrimalStructure::GetPrivateStaticClass(), &AllStructures);
			APrimalStructure* Struc;

			for (AActor* StructActor : AllStructures)
			{
				if (!StructActor || !Actor->IsA(APrimalStructure::GetPrivateStaticClass())) continue;
				StructActor = static_cast<APrimalStructure*>(StructActor);
				StructActor->bCanBeDamaged() = false;
			}

			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(0, 255, 0), "{} updated all tribe structures to default!",
				shooter_controller->GetPlayerCharacter()->PlayerNameField().ToString().c_str());
		}
	}
	else
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(255, 0, 0),
			"Please face the middle of your screen towards a tribe structure to make changes.");
	}
}

inline void InitCommands()
{
	ArkApi::GetCommands().AddConsoleCommand("NPP.ResetStructures", &ResetStructures);
}

inline void RemoveCommands()
{
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.ResetStructures");
}