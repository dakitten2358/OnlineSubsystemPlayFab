// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlineEntitlementsPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "OnlineIdentityInterface.h"
#include "PlayFab.h"


TSharedPtr<FOnlineEntitlement> FOnlineEntitlementsPlayFab::GetEntitlement(const FUniqueNetId& UserId, const FUniqueEntitlementId& EntitlementId)
{
	for (TSharedPtr<FOnlineEntitlementPlayFab> Entitlement : CachedEntitlements.FindRef(UserId.AsShared()))
	{
		if (Entitlement->Id == EntitlementId)
		{
			return Entitlement;
		}
	}
	return nullptr;
}

TSharedPtr<FOnlineEntitlement> FOnlineEntitlementsPlayFab::GetItemEntitlement(const FUniqueNetId& UserId, const FString& ItemId)
{
	for (TSharedPtr<FOnlineEntitlementPlayFab> Entitlement : CachedEntitlements.FindRef(UserId.AsShared()))
	{
		if (Entitlement->ItemId == ItemId)
		{
			return Entitlement;
		}
	}
	return nullptr;
}

void FOnlineEntitlementsPlayFab::GetAllEntitlements(const FUniqueNetId& UserId, const FString& Namespace, TArray<TSharedRef<FOnlineEntitlement>>& OutUserEntitlements)
{
	OutUserEntitlements.Empty();

	for (TSharedPtr<FOnlineEntitlementPlayFab> Entitlement : CachedEntitlements.FindRef(UserId.AsShared()))
	{
		if (Namespace != "" && Namespace != Entitlement->Namespace)
		{
			continue;
		}
		OutUserEntitlements.Add(Entitlement.ToSharedRef());
	}
}

bool FOnlineEntitlementsPlayFab::QueryEntitlements(const FUniqueNetId& UserId, const FString& Namespace, const FPagedQuery& Page/* = FPagedQuery()*/)
{
	PlayFabServerPtr ServerAPI = IPlayFabModuleInterface::Get().GetServerAPI();
	PlayFabClientPtr ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
	if (ServerAPI.IsValid())
	{
		PlayFab::ServerModels::FGetUserInventoryRequest Request;
		Request.PlayFabId = UserId.ToString();
		PlayFab::UPlayFabServerAPI::FGetUserInventoryDelegate SuccessDelegate = PlayFab::UPlayFabServerAPI::FGetUserInventoryDelegate::CreateRaw(this, &FOnlineEntitlementsPlayFab::OnSuccessCallback_Server_GetUserInventory, &UserId, Namespace);
		PlayFab::FPlayFabErrorDelegate ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineEntitlementsPlayFab::OnErrorCallback_GetUserInventory, &UserId, Namespace);
		ServerAPI->GetUserInventory(Request, SuccessDelegate, ErrorDelegate);
		return true;
	}
	else if (ClientAPI.IsValid() && ClientAPI->IsClientLoggedIn())
	{
		if (UserId != *PlayFabSubsystem->GetIdentityInterface()->GetUniquePlayerId(0))
		{
			UE_LOG_ONLINE(Error, TEXT("Can't lookup another client's inventory!"));
		}
		else if (!SuccessDelegate_Client_GetUserInventory.IsBound())
		{
			SuccessDelegate_Client_GetUserInventory.CreateRaw(this, &FOnlineEntitlementsPlayFab::OnSuccessCallback_Client_GetUserInventory, &UserId, Namespace);
			PlayFab::FPlayFabErrorDelegate ErrorDelegate_GetUserInventory = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineEntitlementsPlayFab::OnErrorCallback_GetUserInventory, &UserId, Namespace);
			ClientAPI->GetUserInventory(SuccessDelegate_Client_GetUserInventory, ErrorDelegate_GetUserInventory);
			return true;
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("Already iniated a User Inventory query"));
		}
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("PlayFab Interface not available"));
	}
	return false;
}

void FOnlineEntitlementsPlayFab::OnSuccessCallback_Client_GetUserInventory(const PlayFab::ClientModels::FGetUserInventoryResult& Result, const FUniqueNetId* UserId, const FString Namespace)
{
	SuccessDelegate_Client_GetUserInventory.Unbind();

	for (PlayFab::ClientModels::FItemInstance Item : Result.Inventory)
	{
		FString ItemNamespace = Item.CustomData.FindRef("Namespace");
		if (Namespace != "" && ItemNamespace != Namespace)
		{
			continue;
		}
		TSharedPtr<FOnlineEntitlementPlayFab> Entitlement = MakeShareable(new FOnlineEntitlementPlayFab());
		//Entitlement->bIsConsumable
		Entitlement->Id = Item.ItemInstanceId;
		Entitlement->ItemId = Item.ItemId;
		Entitlement->RemainingCount = Item.RemainingUses;
		Entitlement->Name = Item.DisplayName;
		Entitlement->Namespace = ItemNamespace;
		Entitlement->bIsConsumable = false;

		CachedEntitlements.FindOrAdd(UserId->AsShared()).Add(Entitlement);
	}

	TriggerOnQueryEntitlementsCompleteDelegates(true, *UserId, Namespace, TEXT(""));
}

void FOnlineEntitlementsPlayFab::OnSuccessCallback_Server_GetUserInventory(const PlayFab::ServerModels::FGetUserInventoryResult& Result, const FUniqueNetId* UserId, const FString Namespace)
{
	for (PlayFab::ServerModels::FItemInstance Item : Result.Inventory)
	{
		FString ItemNamespace = Item.CustomData.FindRef("Namespace");
		if (Namespace != "" && ItemNamespace != Namespace)
		{
			continue;
		}
		TSharedPtr<FOnlineEntitlementPlayFab> Entitlement = MakeShareable(new FOnlineEntitlementPlayFab());
		//Entitlement->bIsConsumable
		Entitlement->Id = Item.ItemInstanceId;
		Entitlement->ItemId = Item.ItemId;
		Entitlement->RemainingCount = Item.RemainingUses;
		Entitlement->Name = Item.DisplayName;
		Entitlement->Namespace = ItemNamespace;
		Entitlement->bIsConsumable = false;

		CachedEntitlements.FindOrAdd(UserId->AsShared()).Add(Entitlement);
	}
	TriggerOnQueryEntitlementsCompleteDelegates(true, *UserId, Namespace, TEXT(""));
}

void FOnlineEntitlementsPlayFab::OnErrorCallback_GetUserInventory(const PlayFab::FPlayFabError& ErrorResult, const FUniqueNetId* UserId, const FString Namespace)
{
	TriggerOnQueryEntitlementsCompleteDelegates(false, *UserId, Namespace, ErrorResult.ErrorMessage);
}

