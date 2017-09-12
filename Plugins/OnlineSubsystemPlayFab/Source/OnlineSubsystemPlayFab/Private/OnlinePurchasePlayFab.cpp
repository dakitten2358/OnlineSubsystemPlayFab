// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlinePurchasePlayFab.h"
#include "OnlineSubsystemPlayFab.h"


bool FOnlinePurchasePlayFab::IsAllowedToPurchase(const FUniqueNetId& UserId)
{
	return false;
}

void FOnlinePurchasePlayFab::Checkout(const FUniqueNetId& UserId, const FPurchaseCheckoutRequest& CheckoutRequest, const FOnPurchaseCheckoutComplete& Delegate)
{
	
}

void FOnlinePurchasePlayFab::FinalizePurchase(const FUniqueNetId& UserId, const FString& ReceiptId)
{
	
}

void FOnlinePurchasePlayFab::RedeemCode(const FUniqueNetId& UserId, const FRedeemCodeRequest& RedeemCodeRequest, const FOnPurchaseRedeemCodeComplete& Delegate)
{
	
}

void FOnlinePurchasePlayFab::QueryReceipts(const FUniqueNetId& UserId, bool bRestoreReceipts, const FOnQueryReceiptsComplete& Delegate)
{
	
}

void FOnlinePurchasePlayFab::GetReceipts(const FUniqueNetId& UserId, TArray<FPurchaseReceipt>& OutReceipts) const
{
	
}
