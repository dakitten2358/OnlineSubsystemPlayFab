// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineLeaderboardPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFab.h"


bool FOnlineLeaderboardsPlayFab::ReadLeaderboards(const TArray<TSharedRef<const FUniqueNetId>>& Players, FOnlineLeaderboardReadRef& ReadObject)
{
	if (Players.Num() > 1)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineLeaderboardsPlayFab::ReadLeaderboards: Can't accept more then 1 player at a time due to PlayFab API"));
		return false;
	}

	if (true)
	{
		ReadObject->ReadState = EOnlineAsyncTaskState::InProgress;

		for (TSharedRef<const FUniqueNetId> UserId : Players)
		{
			PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(UserId.Get());
			PlayFab::ClientModels::FGetLeaderboardAroundPlayerRequest Request;
			Request.StatisticName = ReadObject->LeaderboardName.ToString();
			Request.MaxResultsCount = 1;
			Request.PlayFabId = UserId->ToString();
			//Request.ProfileConstraints->

			PlayFab::UPlayFabClientAPI::FGetLeaderboardAroundPlayerDelegate SuccessDelegate = PlayFab::UPlayFabClientAPI::FGetLeaderboardAroundPlayerDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnSuccessCallback_Client_GetLeaderboardAroundPlayer, &ReadObject);
			PlayFab::FPlayFabErrorDelegate ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnErrorCallback_Client_GetLeaderboardAroundPlayer, &ReadObject);
			ClientAPI->GetLeaderboardAroundPlayer(Request, SuccessDelegate, ErrorDelegate);
		}
		return true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("PlayFab Client Interface not available"));
	}
	return false;
}

bool FOnlineLeaderboardsPlayFab::ReadLeaderboardsForFriends(int32 LocalUserNum, FOnlineLeaderboardReadRef& ReadObject)
{
	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(LocalUserNum);
	if (ClientAPI.IsValid())
	{
		ReadObject->ReadState = EOnlineAsyncTaskState::InProgress;

		PlayFab::ClientModels::FGetFriendLeaderboardRequest Request;
		Request.StatisticName = ReadObject->LeaderboardName.ToString();
		Request.MaxResultsCount = 10;
		//Request.ProfileConstraints->

		PlayFab::UPlayFabClientAPI::FGetFriendLeaderboardDelegate SuccessDelegate = PlayFab::UPlayFabClientAPI::FGetFriendLeaderboardDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnSuccessCallback_Client_GetFriendLeaderboard, &ReadObject);
		PlayFab::FPlayFabErrorDelegate ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnErrorCallback_Client_GetFriendLeaderboard, &ReadObject);
		ClientAPI->GetFriendLeaderboard(Request, SuccessDelegate, ErrorDelegate);
		return true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("PlayFab Client Interface not available"));
	}
	return false;
}

bool FOnlineLeaderboardsPlayFab::ReadLeaderboardsAroundRank(int32 Rank, uint32 Range, FOnlineLeaderboardReadRef& ReadObject)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineLeaderboardsPlayFab::ReadLeaderboardsAroundRank: Not currently implemented"));
	return false;
}


bool FOnlineLeaderboardsPlayFab::ReadLeaderboardsAroundUser(TSharedRef<const FUniqueNetId> Player, uint32 Range, FOnlineLeaderboardReadRef& ReadObject)
{
	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(Player.Get());
	if (ClientAPI.IsValid())
	{
		ReadObject->ReadState = EOnlineAsyncTaskState::InProgress;

		PlayFab::ClientModels::FGetLeaderboardAroundPlayerRequest Request;
		Request.StatisticName = ReadObject->LeaderboardName.ToString();
		Request.MaxResultsCount = Range;
		Request.PlayFabId = Player->ToString();

		PlayFab::UPlayFabClientAPI::FGetLeaderboardAroundPlayerDelegate SuccessDelegate = PlayFab::UPlayFabClientAPI::FGetLeaderboardAroundPlayerDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnSuccessCallback_Client_GetLeaderboardAroundPlayer, &ReadObject);
		PlayFab::FPlayFabErrorDelegate ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnErrorCallback_Client_GetLeaderboardAroundPlayer, &ReadObject);
		ClientAPI->GetLeaderboardAroundPlayer(Request, SuccessDelegate, ErrorDelegate);

		return true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("PlayFab Client Interface not available"));
	}
	return false;
}

void FOnlineLeaderboardsPlayFab::FreeStats(FOnlineLeaderboardRead& ReadObject)
{
	// No idea what this is for yet :/
}

bool FOnlineLeaderboardsPlayFab::WriteLeaderboards(const FName& SessionName, const FUniqueNetId& Player, FOnlineLeaderboardWrite& WriteObject)
{
	TSharedPtr<const FUniqueNetId> LoggedInPlayerId = PlayFabSubsystem->GetIdentityInterface()->GetUniquePlayerId(0);
	if (!(LoggedInPlayerId.IsValid() && Player == *LoggedInPlayerId))
	{
		UE_LOG_ONLINE(Error, TEXT("Can only write to leaderboards for logged in player id"));
		return false;
	}

	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(Player);
	if (ClientAPI.IsValid())
	{
		PlayFab::ClientModels::FUpdatePlayerStatisticsRequest Request;

		for (auto Row : WriteObject.Properties)
		{
			auto* StatData = &Row.Value;
			long long Score;

			switch (StatData->GetType())
			{
			case EOnlineKeyValuePairDataType::Int32:
			{
				int32 Value;
				StatData->GetValue(Value);
				Score = Value;
				break;
			}
			case EOnlineKeyValuePairDataType::UInt32:
			{
				uint32 Value;
				StatData->GetValue(Value);
				Score = Value;
				break;
			}
			case EOnlineKeyValuePairDataType::Int64:
			{
				int64 Value;
				StatData->GetValue(Value);
				Score = Value;
				break;
			}
			case EOnlineKeyValuePairDataType::UInt64:
			case EOnlineKeyValuePairDataType::Double:
			case EOnlineKeyValuePairDataType::Float:
			default:
			{
				UE_LOG_ONLINE(Error, TEXT("Invalid Stat type to save to the leaderboard: %s"), EOnlineKeyValuePairDataType::ToString(StatData->GetType()));
				return false;
			}
			}

			PlayFab::ClientModels::FStatisticUpdate Statistic;
			Statistic.StatisticName = Row.Key.ToString();
			Statistic.Value = Score;
			Request.Statistics.Add(Statistic);
		}

		ClientAPI->UpdatePlayerStatistics(Request);
		return true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("PlayFab Client Interface not available"));
	}

	return false;
}

bool FOnlineLeaderboardsPlayFab::FlushLeaderboards(const FName& SessionName)
{
	TriggerOnLeaderboardFlushCompleteDelegates(SessionName, true);
	return true;
}

bool FOnlineLeaderboardsPlayFab::WriteOnlinePlayerRatings(const FName& SessionName, int32 LeaderboardId, const TArray<FOnlinePlayerScore>& PlayerScores)
{
	return false;
}

void FOnlineLeaderboardsPlayFab::OnSuccessCallback_Client_GetLeaderboardAroundPlayer(const PlayFab::ClientModels::FGetLeaderboardAroundPlayerResult& Result, FOnlineLeaderboardReadRef* ReadObject)
{
	for (auto LeaderboardRow : Result.Leaderboard)
	{
		TSharedPtr<FUniqueNetId> UserId = MakeShareable(new FUniqueNetIdString(LeaderboardRow.PlayFabId));
		const FString DisplayName(LeaderboardRow.DisplayName);
		FOnlineStatsRow Row(DisplayName, UserId->AsShared());
		Row.Rank = LeaderboardRow.Position;
		ReadObject->Get().Rows.Add(Row);
	}

	ReadObject->Get().ReadState = EOnlineAsyncTaskState::Done;
	TriggerOnLeaderboardReadCompleteDelegates(true);
}

void FOnlineLeaderboardsPlayFab::OnErrorCallback_Client_GetLeaderboardAroundPlayer(const PlayFab::FPlayFabError& ErrorResult, FOnlineLeaderboardReadRef* ReadObject)
{
	UE_LOG_ONLINE(Error, TEXT("FOnlineLeaderboardsPlayFab::GetLeaderboardAroundPlayer: Failed grabbing leaderboard around player: %s"), *ErrorResult.ErrorMessage);

	ReadObject->Get().ReadState = EOnlineAsyncTaskState::Failed;
	TriggerOnLeaderboardReadCompleteDelegates(false);
}

void FOnlineLeaderboardsPlayFab::OnSuccessCallback_Client_GetFriendLeaderboard(const PlayFab::ClientModels::FGetLeaderboardResult& Result, FOnlineLeaderboardReadRef* ReadObject)
{
	for (auto LeaderboardRow : Result.Leaderboard)
	{
		TSharedPtr<FUniqueNetId> UserId = MakeShareable(new FUniqueNetIdString(LeaderboardRow.PlayFabId));
		const FString DisplayName(LeaderboardRow.DisplayName);
		FOnlineStatsRow Row(DisplayName, UserId->AsShared());
		Row.Rank = LeaderboardRow.Position;
		ReadObject->Get().Rows.Add(Row);
	}

	ReadObject->Get().ReadState = EOnlineAsyncTaskState::Done;
	TriggerOnLeaderboardReadCompleteDelegates(true);
}

void FOnlineLeaderboardsPlayFab::OnErrorCallback_Client_GetFriendLeaderboard(const PlayFab::FPlayFabError& ErrorResult, FOnlineLeaderboardReadRef* ReadObject)
{
	UE_LOG_ONLINE(Error, TEXT("FOnlineLeaderboardsPlayFab::GetFriendLeaderboard: Failed grabbing leaderboard around player: %s"), *ErrorResult.ErrorMessage);

	ReadObject->Get().ReadState = EOnlineAsyncTaskState::Failed;
	TriggerOnLeaderboardReadCompleteDelegates(false);
}
