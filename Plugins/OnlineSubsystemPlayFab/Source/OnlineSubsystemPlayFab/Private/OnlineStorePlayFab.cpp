// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlineStorePlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFab.h"


bool FOnlineStorePlayFab::QueryForAvailablePurchases(const TArray<FString>& ProductIDs, FOnlineProductInformationReadRef& InReadObject)
{
	PlayFabClientPtr ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
	if (ClientAPI.IsValid())
	{
		InReadObject->ReadState = EOnlineAsyncTaskState::InProgress;
		PlayFab::ClientModels::FGetStoreItemsRequest Request;
		PlayFab::UPlayFabClientAPI::FGetStoreItemsDelegate SuccessDelegate = PlayFab::UPlayFabClientAPI::FGetStoreItemsDelegate::CreateRaw(this, &FOnlineStorePlayFab::OnSuccessCallback_Client_GetStoreItems, &InReadObject);
		PlayFab::FPlayFabErrorDelegate ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineStorePlayFab::OnErrorCallback, &InReadObject);
		ClientAPI->GetStoreItems(Request, SuccessDelegate, ErrorDelegate);
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("PlayFab Client Interface not available"));
	}
	return false;
}

bool FOnlineStorePlayFab::IsAllowedToMakePurchases()
{
	return false;
}

bool FOnlineStorePlayFab::BeginPurchase(const FInAppPurchaseProductRequest& ProductRequest, FOnlineInAppPurchaseTransactionRef& InReadObject)
{
	return false;
}

bool FOnlineStorePlayFab::RestorePurchases(const TArray<FInAppPurchaseProductRequest>& ConsumableProductFlags, FOnlineInAppPurchaseRestoreReadRef& InReadObject)
{
	return false;
}

void FOnlineStorePlayFab::OnSuccessCallback_Client_GetStoreItems(const PlayFab::ClientModels::FGetStoreItemsResult& Result, FOnlineProductInformationReadRef* InReadObject)
{
	FOnlineProductInformationReadRef& Obj = *InReadObject;
	for (PlayFab::ClientModels::FStoreItem StoreItem : Result.Store)
	{
		FInAppPurchaseProductInfo NewProductInfo;
		NewProductInfo.Identifier = StoreItem.ItemId;
		//NewProductInfo.DisplayName = StoreItem.;

		Obj->ProvidedProductInformation.Add(NewProductInfo);
	}
	Obj->ReadState = EOnlineAsyncTaskState::Done;
	TriggerOnQueryForAvailablePurchasesCompleteDelegates(true);
}

void FOnlineStorePlayFab::OnErrorCallback(const PlayFab::FPlayFabError& ErrorResult, FOnlineProductInformationReadRef* InReadObject)
{
	UE_LOG_ONLINE(Warning, TEXT("%s"), *ErrorResult.ErrorMessage);
	TriggerOnQueryForAvailablePurchasesCompleteDelegates(false);
}
