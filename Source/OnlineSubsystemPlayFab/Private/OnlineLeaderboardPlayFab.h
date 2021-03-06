// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineLeaderboardInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabServerAPI.h"
#include "Core/PlayFabServerDataModels.h"

/**
* Interface definition for the online services leaderboard services
*/
class FOnlineLeaderboardsPlayFab : public IOnlineLeaderboards
{
private:
	struct FLeaderboardStatistics
	{
		TMap<FString, PlayFab::ServerModels::FStatisticValue> Values;
	};

	TMap<TSharedRef<const FUniqueNetId>, FLeaderboardStatistics> CachedStatistics;

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
	void OnSuccessCallback_Server_GetPlayerStatistics(const PlayFab::ServerModels::FGetPlayerStatisticsResult& Result, TSharedRef<const FUniqueNetId> PlayerId, FOnlineLeaderboardReadRef ReadObject);
	void OnErrorCallback_GetPlayerStatistics(const PlayFab::FPlayFabCppError& ErrorResult, TSharedRef<const FUniqueNetId> PlayerId, FOnlineLeaderboardReadRef ReadObject);

	void OnSuccessCallback_Server_UpdatePlayerStatistics(const PlayFab::ServerModels::FUpdatePlayerStatisticsResult& Result, FName SessionName);
	void OnErrorCallback_UpdatePlayerStatistics(const PlayFab::FPlayFabCppError& ErrorResult, FName SessionName);

	void OnSuccessCallback_Client_GetLeaderboardAroundPlayer(const PlayFab::ClientModels::FGetLeaderboardAroundPlayerResult& Result, FOnlineLeaderboardReadRef ReadObject);
	void OnErrorCallback_Client_GetLeaderboardAroundPlayer(const PlayFab::FPlayFabCppError& ErrorResult, FOnlineLeaderboardReadRef ReadObject);
	void OnSuccessCallback_Client_GetFriendLeaderboard(const PlayFab::ClientModels::FGetLeaderboardResult& Result, FOnlineLeaderboardReadRef ReadObject);
	void OnErrorCallback_Client_GetFriendLeaderboard(const PlayFab::FPlayFabCppError& ErrorResult, FOnlineLeaderboardReadRef ReadObject);
};

typedef TSharedPtr<FOnlineLeaderboardsPlayFab, ESPMode::ThreadSafe> FOnlineLeaderboardsPlayFabPtr;