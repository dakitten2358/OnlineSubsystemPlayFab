// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OnlineAsyncTaskManagerPlayFab.h"

void FOnlineAsyncTaskManagerPlayFab::OnlineTick()
{
	check(PlayFabSubsystem);
	check(FPlatformTLS::GetCurrentThreadId() == OnlineThreadId || !FPlatformProcess::SupportsMultithreading());
}