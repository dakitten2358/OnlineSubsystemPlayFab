// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlineLeaderboardPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFab.h"


bool FOnlineLeaderboardsPlayFab::ReadLeaderboards(const TArray<TSharedRef<const FUniqueNetId>>& Players, FOnlineLeaderboardReadRef& ReadObject)
{
	return false;
}

bool FOnlineLeaderboardsPlayFab::ReadLeaderboardsForFriends(int32 LocalUserNum, FOnlineLeaderboardReadRef& ReadObject)
{
	return false;
}

void FOnlineLeaderboardsPlayFab::FreeStats(FOnlineLeaderboardRead& ReadObject)
{
	
}

bool FOnlineLeaderboardsPlayFab::WriteLeaderboards(const FName& SessionName, const FUniqueNetId& Player, FOnlineLeaderboardWrite& WriteObject)
{
	return false;
}

bool FOnlineLeaderboardsPlayFab::FlushLeaderboards(const FName& SessionName)
{
	return false;
}

bool FOnlineLeaderboardsPlayFab::WriteOnlinePlayerRatings(const FName& SessionName, int32 LeaderboardId, const TArray<FOnlinePlayerScore>& PlayerScores)
{
	return false;
}
