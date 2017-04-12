// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
 
#include "OnlineTimeInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"

/**
* Interface for querying server time from an online service
*/
class FOnlineTimePlayFab : public IOnlineTime
{
private:

	FString CachedUTC;

	/** Reference to the main PlayFab subsystem */
	class FOnlineSubsystemPlayFab* PlayFabSubsystem;

	/** Hidden on purpose */
	FOnlineTimePlayFab() :
		PlayFabSubsystem(NULL)
	{
	}

PACKAGE_SCOPE:

	FOnlineTimePlayFab(class FOnlineSubsystemPlayFab* InPlayFabSubsystem) :
		PlayFabSubsystem(InPlayFabSubsystem)
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
	PlayFab::UPlayFabClientAPI::FGetTimeDelegate SuccessDelegate_GetTime;
	void OnSuccessCallback_GetTime(const PlayFab::ClientModels::FGetTimeResult& Result);
	PlayFab::FPlayFabErrorDelegate ErrorDelegate_GetTime;
	void OnErrorCallback_GetTime(const PlayFab::FPlayFabError& ErrorResult);
};

typedef TSharedPtr<FOnlineTimePlayFab, ESPMode::ThreadSafe> FOnlineTimePlayFabPtr;