// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
 
#include "OnlineLeaderboardInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"

/**
* Interface definition for the online services leaderboard services
*/
class FOnlineLeaderboardsPlayFab : public IOnlineLeaderboards
{
private:

	/** Reference to the main PlayFab subsystem */
	class FOnlineSubsystemPlayFab* PlayFabSubsystem;

	/** Hidden on purpose */
	FOnlineLeaderboardsPlayFab() :
		PlayFabSubsystem(NULL)
	{
	}

PACKAGE_SCOPE:

	FOnlineLeaderboardsPlayFab(class FOnlineSubsystemPlayFab* InPlayFabSubsystem) :
		PlayFabSubsystem(InPlayFabSubsystem)
	{
	}

public:

	virtual ~FOnlineLeaderboardsPlayFab()
	{
	}

	// IOnlineLeaderboards
	virtual bool ReadLeaderboards(const TArray<TSharedRef<const FUniqueNetId>>& Players, FOnlineLeaderboardReadRef& ReadObject) override;
	virtual bool ReadLeaderboardsForFriends(int32 LocalUserNum, FOnlineLeaderboardReadRef& ReadObject) override;
	virtual bool ReadLeaderboardsAroundRank(int32 Rank, uint32 Range, FOnlineLeaderboardReadRef& ReadObject) override;
	virtual bool ReadLeaderboardsAroundUser(TSharedRef<const FUniqueNetId> Player, uint32 Range, FOnlineLeaderboardReadRef& ReadObject) override;
	virtual void FreeStats(FOnlineLeaderboardRead& ReadObject) override;
	virtual bool WriteLeaderboards(const FName& SessionName, const FUniqueNetId& Player, FOnlineLeaderboardWrite& WriteObject) override;
	virtual bool FlushLeaderboards(const FName& SessionName) override;
	virtual bool WriteOnlinePlayerRatings(const FName& SessionName, int32 LeaderboardId, const TArray<FOnlinePlayerScore>& PlayerScores) override;

private:
	void OnSuccessCallback_Client_GetLeaderboardAroundPlayer(const PlayFab::ClientModels::FGetLeaderboardAroundPlayerResult& Result, FOnlineLeaderboardReadRef* ReadObject);
	void OnErrorCallback_Client_GetLeaderboardAroundPlayer(const PlayFab::FPlayFabError& ErrorResult, FOnlineLeaderboardReadRef* ReadObject);
	void OnSuccessCallback_Client_GetFriendLeaderboard(const PlayFab::ClientModels::FGetLeaderboardResult& Result, FOnlineLeaderboardReadRef* ReadObject);
	void OnErrorCallback_Client_GetFriendLeaderboard(const PlayFab::FPlayFabError& ErrorResult, FOnlineLeaderboardReadRef* ReadObject);
};

typedef TSharedPtr<FOnlineLeaderboardsPlayFab, ESPMode::ThreadSafe> FOnlineLeaderboardsPlayFabPtr;