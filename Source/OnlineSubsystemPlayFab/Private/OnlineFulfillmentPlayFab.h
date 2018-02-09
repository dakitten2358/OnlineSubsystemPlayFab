// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#if 0
#include "CoreMinimal.h"
#include "OnlineFulfillmentInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"

/**
* Interface class for events
*/
class FOnlineFulfillmentPlayFab : public IOnlineFulfillment
{
private:
	/** Reference to the main PlayFab subsystem */
	class FOnlineSubsystemPlayFab* PlayFabSubsystem;

	/** Hidden on purpose */
	FOnlineFulfillmentPlayFab() :
		PlayFabSubsystem(NULL)
	{
	}

PACKAGE_SCOPE:

	FOnlineFulfillmentPlayFab(class FOnlineSubsystemPlayFab* InPlayFabSubsystem) :
		PlayFabSubsystem(InPlayFabSubsystem)
	{
	}

public:

	virtual ~FOnlineFulfillmentPlayFab()
	{
	}

	// IOnlineFulfillment
	virtual void RedeemCode(const FUniqueNetId& UserId, const FRedeemCodeRequest& RedeemCodeRequest, const FOnPurchaseRedeemCodeComplete& Delegate) override;
};

typedef TSharedPtr<FOnlineFulfillmentPlayFab, ESPMode::ThreadSafe> FOnlineFulfillmentPlayFabPtr;
#endif