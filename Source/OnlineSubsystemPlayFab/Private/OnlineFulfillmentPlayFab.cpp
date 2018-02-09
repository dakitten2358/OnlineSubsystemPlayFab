// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#if 0
#include "OnlineFulfillmentPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFab.h"


void FOnlineFulfillmentPlayFab::RedeemCode(const FUniqueNetId& UserId, const FRedeemCodeRequest& RedeemCodeRequest, const FOnPurchaseRedeemCodeComplete& Delegate)
{
	if (UserId.IsValid())
	{
		PlayFab::ClientModels::FRedeemCouponRequest Request;
		Request.CouponCode = RedeemCodeRequest.Code;
		PlayFabSubsystem->GetClientAPI(UserId)->RedeemCoupon(Request);
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("RedeemCode requires valid UserId!"));
	}
}
#endif