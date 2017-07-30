// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineEntitlementsInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Core/PlayFabServerAPI.h"
#include "Core/PlayFabServerDataModels.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"

class FOnlineEntitlementPlayFab : public FOnlineEntitlement
{
public:
	TMap<FString, FString> Attrs;
	
	virtual bool GetAttribute(const FString& AttrName, FString& OutAttrValue) const override
	{
		const FString* Value = Attrs.Find(AttrName);
		if (Value != nullptr)
		{
			OutAttrValue = *Value;
			return true;
		}
		return false;
	}
};

/**
* Interface for retrieving user entitlements
*/
class FOnlineEntitlementsPlayFab : public IOnlineEntitlements
{
private:
	TMap<TSharedPtr<const FUniqueNetId>, TArray<TSharedPtr<FOnlineEntitlementPlayFab>>> CachedEntitlements;

	/** Reference to the main PlayFab subsystem */
	class FOnlineSubsystemPlayFab* PlayFabSubsystem;

	/** Hidden on purpose */
	FOnlineEntitlementsPlayFab() :
		PlayFabSubsystem(NULL)
	{
	}

PACKAGE_SCOPE:

	FOnlineEntitlementsPlayFab(class FOnlineSubsystemPlayFab* InPlayFabSubsystem) :
		PlayFabSubsystem(InPlayFabSubsystem)
	{
	}

public:

	virtual ~FOnlineEntitlementsPlayFab()
	{
	}

	// IOnlineEntitlements
	virtual TSharedPtr<FOnlineEntitlement> GetEntitlement(const FUniqueNetId& UserId, const FUniqueEntitlementId& EntitlementId) override;
	virtual TSharedPtr<FOnlineEntitlement> GetItemEntitlement(const FUniqueNetId& UserId, const FString& ItemId) override;
	virtual void GetAllEntitlements(const FUniqueNetId& UserId, const FString& Namespace, TArray<TSharedRef<FOnlineEntitlement>>& OutUserEntitlements) override;
	virtual bool QueryEntitlements(const FUniqueNetId& UserId, const FString& Namespace, const FPagedQuery& Page = FPagedQuery()) override;

private:
	PlayFab::UPlayFabClientAPI::FGetUserInventoryDelegate SuccessDelegate_Client_GetUserInventory;
	void OnSuccessCallback_Client_GetUserInventory(const PlayFab::ClientModels::FGetUserInventoryResult& Result, const FUniqueNetId* UserId, const FString Namespace);
	void OnSuccessCallback_Server_GetUserInventory(const PlayFab::ServerModels::FGetUserInventoryResult& Result, const FUniqueNetId* UserId, const FString Namespace);
	void OnErrorCallback_GetUserInventory(const PlayFab::FPlayFabError& ErrorResult, const FUniqueNetId* UserId, const FString Namespace);
};

typedef TSharedPtr<FOnlineEntitlementsPlayFab, ESPMode::ThreadSafe> FOnlineEntitlementsPlayFabPtr;