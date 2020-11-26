// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
 
#include "CoreMinimal.h"
#include "Interfaces/OnlineStoreInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"

/**
* Interface class for microtransactions
*/
PRAGMA_DISABLE_DEPRECATION_WARNINGS
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
	void OnSuccessCallback_Client_GetCatalogItems(const PlayFab::ClientModels::FGetCatalogItemsResult& Result, FOnlineProductInformationReadRef* InReadObject);
	void OnErrorCallback_Client_GetCatalogItems(const PlayFab::FPlayFabCppError& ErrorResult, FOnlineProductInformationReadRef* InReadObject);
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS

typedef TSharedPtr<FOnlineStorePlayFab, ESPMode::ThreadSafe> FOnlineStorePlayFabPtr;