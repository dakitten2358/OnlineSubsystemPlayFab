// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineStoreV2PlayFab.h"
#include "OnlineSubsystemPlayFab.h"


void FOnlineStoreV2PlayFab::QueryCategories(const FUniqueNetId& UserId, const FOnQueryOnlineStoreCategoriesComplete& Delegate /*= FOnQueryOnlineStoreCategoriesComplete()*/)
{
	
}

void FOnlineStoreV2PlayFab::GetCategories(TArray<FOnlineStoreCategory>& OutCategories) const
{

}

void FOnlineStoreV2PlayFab::QueryOffersByFilter(const FUniqueNetId& UserId, const FOnlineStoreFilter& Filter, const FOnQueryOnlineStoreOffersComplete& Delegate /*= FOnQueryOnlineStoreOffersComplete()*/)
{
	
}

void FOnlineStoreV2PlayFab::QueryOffersById(const FUniqueNetId& UserId, const TArray<FUniqueOfferId>& OfferIds, const FOnQueryOnlineStoreOffersComplete& Delegate /*= FOnQueryOnlineStoreOffersComplete()*/)
{
	
}

void FOnlineStoreV2PlayFab::GetOffers(TArray<FOnlineStoreOfferRef>& OutOffers) const
{
	
}

TSharedPtr<FOnlineStoreOffer> FOnlineStoreV2PlayFab::GetOffer(const FUniqueOfferId& OfferId) const
{
	return nullptr;
}
