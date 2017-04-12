// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlinePresencePlayFab.h"
#include "OnlineSubsystemPlayFab.h"


void FOnlinePresencePlayFab::SetPresence(const FUniqueNetId& User, const FOnlineUserPresenceStatus& Status, const FOnPresenceTaskCompleteDelegate& Delegate /*= FOnPresenceTaskCompleteDelegate()*/)
{
	Delegate.Execute(User, false);
}

void FOnlinePresencePlayFab::QueryPresence(const FUniqueNetId& User, const FOnPresenceTaskCompleteDelegate& Delegate /*= FOnPresenceTaskCompleteDelegate()*/)
{
	Delegate.Execute(User, false);
}

EOnlineCachedResult::Type FOnlinePresencePlayFab::GetCachedPresence(const FUniqueNetId& User, TSharedPtr<FOnlineUserPresence>& OutPresence)
{
	return EOnlineCachedResult::NotFound;
}

EOnlineCachedResult::Type FOnlinePresencePlayFab::GetCachedPresenceForApp(const FUniqueNetId& LocalUserId, const FUniqueNetId& User, const FString& AppId, TSharedPtr<FOnlineUserPresence>& OutPresence)
{
	return EOnlineCachedResult::NotFound;
}

