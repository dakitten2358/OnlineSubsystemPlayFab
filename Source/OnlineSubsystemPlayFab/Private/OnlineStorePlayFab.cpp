// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineStorePlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFab.h"


bool FOnlineStorePlayFab::QueryForAvailablePurchases(const TArray<FString>& ProductIDs, FOnlineProductInformationReadRef& InReadObject)
{
	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI();
	if (ClientAPI->IsClientLoggedIn())
	{
		InReadObject->ReadState = EOnlineAsyncTaskState::InProgress;

		PlayFab::ClientModels::FGetCatalogItemsRequest Request;
		auto SuccessDelegate = PlayFab::UPlayFabClientAPI::FGetCatalogItemsDelegate::CreateRaw(this, &FOnlineStorePlayFab::OnSuccessCallback_Client_GetCatalogItems, &InReadObject);
		auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineStorePlayFab::OnErrorCallback_Client_GetCatalogItems, &InReadObject);
		ClientAPI->GetCatalogItems(Request, SuccessDelegate, ErrorDelegate);
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
	// We'll complete this later, IOS isn't a big concern
	/*PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI();
	if (ClientAPI->IsClientLoggedIn())
	{
		for (auto ProductRequest : ConsumableProductFlags)
		{
			PlayFab::ClientModels::FRestoreIOSPurchasesRequest Request;
			Request.ReceiptData = ProductRequest.ProductIdentifier;
			ClientAPI->RestoreIOSPurchases(Request);
		}
		InReadObject->ReadState = EOnlineAsyncTaskState::InProgress;
		return true;
	}*/
	return false;
}

void FOnlineStorePlayFab::OnSuccessCallback_Client_GetCatalogItems(const PlayFab::ClientModels::FGetCatalogItemsResult& Result, FOnlineProductInformationReadRef* InReadObject)
{
	FOnlineProductInformationReadRef& Obj = *InReadObject;
	for (PlayFab::ClientModels::FCatalogItem CatalogItem : Result.Catalog)
	{
		FInAppPurchaseProductInfo NewProductInfo;
		NewProductInfo.Identifier = CatalogItem.ItemId;
		NewProductInfo.DisplayName = CatalogItem.DisplayName;
		NewProductInfo.DisplayDescription = CatalogItem.Description;

		Obj->ProvidedProductInformation.Add(NewProductInfo);
	}
	Obj->ReadState = EOnlineAsyncTaskState::Done;
	TriggerOnQueryForAvailablePurchasesCompleteDelegates(true);
}

void FOnlineStorePlayFab::OnErrorCallback_Client_GetCatalogItems(const PlayFab::FPlayFabError& ErrorResult, FOnlineProductInformationReadRef* InReadObject)
{
	UE_LOG_ONLINE(Warning, TEXT("%s"), *ErrorResult.ErrorMessage);
	TriggerOnQueryForAvailablePurchasesCompleteDelegates(false);
}
