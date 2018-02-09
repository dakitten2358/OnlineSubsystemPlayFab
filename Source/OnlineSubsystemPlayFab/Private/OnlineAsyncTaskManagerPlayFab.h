// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineAsyncTaskManager.h"

/**
 *	Steam version of the async task manager to register the various Steam callbacks with the engine
 */
class FOnlineAsyncTaskManagerPlayFab : public FOnlineAsyncTaskManager
{
private:

	/** Cached reference to the main online subsystem */
	class FOnlineSubsystemPlayFab* PlayFabSubsystem;

public:

	FOnlineAsyncTaskManagerPlayFab(class FOnlineSubsystemPlayFab* InSubsystem)
		: PlayFabSubsystem(InSubsystem)
	{
	}

	~FOnlineAsyncTaskManagerPlayFab()
	{
	}

	// FOnlineAsyncTaskManager
	virtual void OnlineTick() override;
};