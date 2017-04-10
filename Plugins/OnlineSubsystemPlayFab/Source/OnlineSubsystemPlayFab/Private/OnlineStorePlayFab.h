// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
 
#include "OnlineStoreInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"

/**
* Interface class for microtransactions
*/
class FOnlineStorePlayFab : public IOnlineStore
{
private:

	/** Reference to the main PlayFab subsystem */
	class FOnlineSubsystemPlayFab* PlayFabSubsystem;

	/** Hidden on purpose */
	FOnlineStorePlayFab() :
		PlayFabSubsystem(NULL)
	{
	}

PACKAGE_SCOPE:

	FOnlineStorePlayFab(class FOnlineSubsystemPlayFab* InPlayFabSubsystem) :
		PlayFabSubsystem(InPlayFabSubsystem)
	{
	}

public:

	virtual ~FOnlineStorePlayFab()
	{
	}

	// IOnlineStore
	virtual bool QueryForAvailablePurchases(const TArray<FString>& ProductIDs, FOnlineProductInformationReadRef& InReadObject) override;
	virtual bool IsAllowedToMakePurchases() override;
	virtual bool BeginPurchase(const FInAppPurchaseProductRequest& ProductRequest, FOnlineInAppPurchaseTransactionRef& InReadObject) override;
	virtual bool RestorePurchases(const TArray<FInAppPurchaseProductRequest>& ConsumableProductFlags, FOnlineInAppPurchaseRestoreReadRef& InReadObject) override;

private:
	void OnSuccessCallback_Client_GetStoreItems(const PlayFab::ClientModels::FGetStoreItemsResult& Result, FOnlineProductInformationReadRef* InReadObject);
	void OnErrorCallback(const PlayFab::FPlayFabError& ErrorResult, FOnlineProductInformationReadRef* InReadObject);
};

typedef TSharedPtr<FOnlineStorePlayFab, ESPMode::ThreadSafe> FOnlineStorePlayFabPtr;