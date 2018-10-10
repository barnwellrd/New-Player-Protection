#pragma once

DECLARE_HOOK(APrimalStructure_FinalStructurePlacement, bool, APrimalStructure*, APlayerController*, FVector, FRotator, FRotator, APawn *, FName, bool);

void InitHooks()
{
	ArkApi::GetHooks().SetHook("APrimalStructure.FinalStructurePlacement", &Hook_APrimalStructure_FinalStructurePlacement, reinterpret_cast<LPVOID*>(&APrimalStructure_FinalStructurePlacement_original));
}

void RemoveHooks()
{
	ArkApi::GetHooks().DisableHook("APrimalStructure.FinalStructurePlacement", &Hook_APrimalStructure_FinalStructurePlacement);
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

