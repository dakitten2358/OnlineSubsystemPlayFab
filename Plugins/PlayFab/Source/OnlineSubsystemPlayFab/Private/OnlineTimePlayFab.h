// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
 
#include "OnlineTimeInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabServerAPI.h"
#include "Core/PlayFabServerDataModels.h"

/**
* Interface for querying server time from an online service
*/
class FOnlineTimePlayFab : public IOnlineTime
{
private:
	bool bCatchingServerTime;

	FString CachedUTC;

	/** Reference to the main PlayFab subsystem */
	class FOnlineSubsystemPlayFab* PlayFabSubsystem;

	/** Hidden on purpose */
	FOnlineTimePlayFab()
		: PlayFabSubsystem(NULL)
		, bCatchingServerTime(false)
	{
	}

PACKAGE_SCOPE:

	FOnlineTimePlayFab(class FOnlineSubsystemPlayFab* InPlayFabSubsystem)
		: PlayFabSubsystem(InPlayFabSubsystem)
		, bCatchingServerTime(false)
	{
	}

public:

	virtual ~FOnlineTimePlayFab()
	{
	}

	// IOnlineTime
	virtual bool QueryServerUtcTime() override;
	virtual FString GetLastServerUtcTime() override;

private:
	void OnSuccessCallback_S_GetTime(const PlayFab::ServerModels::FGetTimeResult& Result);
	void OnSuccessCallback_C_GetTime(const PlayFab::ClientModels::FGetTimeResult& Result);
	void OnErrorCallback_GetTime(const PlayFab::FPlayFabError& ErrorResult);
};

typedef TSharedPtr<FOnlineTimePlayFab, ESPMode::ThreadSafe> FOnlineTimePlayFabPtr;