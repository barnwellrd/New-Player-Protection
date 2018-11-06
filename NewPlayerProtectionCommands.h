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

	/**
	* \brief Finds all Structures owned by team
	*/
	TArray<AActor*> AllStructures;
	UGameplayStatics::GetAllActorsOfClass(reinterpret_cast<UObject*>(ArkApi::GetApiUtils().GetWorld()), APrimalStructure::GetPrivateStaticClass(), &AllStructures);

	for (AActor* StructActor : AllStructures)
	{
		if (!StructActor) continue;
		StructActor = static_cast<APrimalStructure*>(StructActor);
		StructActor->bCanBeDamaged() = true;
	}

	ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FLinearColor(0, 255, 0), "{} updated all tribe structures to default!",
		shooter_controller->GetPlayerCharacter()->PlayerNameField().ToString().c_str());
}

inline void InitCommands()
{
	ArkApi::GetCommands().AddConsoleCommand("NPP.ResetStructures", &ResetStructures);
}

inline void RemoveCommands()
{
	ArkApi::GetCommands().RemoveConsoleCommand("NPP.ResetStructures");
}