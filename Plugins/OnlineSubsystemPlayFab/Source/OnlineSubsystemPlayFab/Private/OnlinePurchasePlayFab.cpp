// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlinePurchasePlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "OnlineError.h"


bool FOnlinePurchasePlayFab::IsAllowedToPurchase(const FUniqueNetId& UserId)
{
	return false;
}

void FOnlinePurchasePlayFab::Checkout(const FUniqueNetId& UserId, const FPurchaseCheckoutRequest& CheckoutRequest, const FOnPurchaseCheckoutComplete& Delegate)
{
	if (!IsAllowedToPurchase(UserId))
	{
		return;
	}
	// PlayFab purchasing is too complex too try and put here
	// Just has too many variables it requires and has a weird handling of purchases, due to supporting many providers
}

void FOnlinePurchasePlayFab::FinalizePurchase(const FUniqueNetId& UserId, const FString& ReceiptId)
{
	if (!IsAllowedToPurchase(UserId))
	{
		return;
	}
}

void FOnlinePurchasePlayFab::RedeemCode(const FUniqueNetId& UserId, const FRedeemCodeRequest& RedeemCodeRequest, const FOnPurchaseRedeemCodeComplete& Delegate)
{
	PlayFab::ClientModels::FRedeemCouponRequest Request;
	Request.CouponCode = RedeemCodeRequest.Code;
	PlayFabSubsystem->GetClientAPI(UserId)->RedeemCoupon(Request);
}

void FOnlinePurchasePlayFab::QueryReceipts(const FUniqueNetId& UserId, bool bRestoreReceipts, const FOnQueryReceiptsComplete& Delegate)
{
	// PlayFab doesn't store receipts
	FOnlineError Error;
	Error.bSucceeded = false;
	Delegate.ExecuteIfBound(Error);
}

void FOnlinePurchasePlayFab::GetReceipts(const FUniqueNetId& UserId, TArray<FPurchaseReceipt>& OutReceipts) const
{
	// PlayFab doesn't store receipts
}

void FOnlinePurchasePlayFab::OnSuccessCallback_Client_RedeemCoupon(const PlayFab::ClientModels::FRedeemCouponResult& Result, const FOnPurchaseRedeemCodeComplete Delegate)
{
	FOnlineError Error;
	Error.bSucceeded = true;

	TSharedRef<FPurchaseReceipt> Receipt = MakeShareable(new FPurchaseReceipt);
	for (PlayFab::ClientModels::FItemInstance ItemInstance : Result.GrantedItems)
	{
		Receipt->AddReceiptOffer("", ItemInstance.ItemId, ItemInstance.RemainingUses);
	}
	Receipt->TransactionState = EPurchaseTransactionState::Purchased;

	Delegate.ExecuteIfBound(Error, Receipt);
}

void FOnlinePurchasePlayFab::OnErrorCallback_RedeemCoupon(const PlayFab::FPlayFabError& ErrorResult, const FOnPurchaseRedeemCodeComplete Delegate)
{
	FOnlineError Error;
	Error.bSucceeded = false;

	TSharedRef<FPurchaseReceipt> Receipt = MakeShareable(new FPurchaseReceipt);

	Delegate.ExecuteIfBound(Error, Receipt);
}
