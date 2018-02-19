// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineLeaderboardPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFab.h"


bool FOnlineLeaderboardsPlayFab::ReadLeaderboards(const TArray<TSharedRef<const FUniqueNetId>>& Players, FOnlineLeaderboardReadRef& ReadObject)
{
	ReadObject->ReadState = EOnlineAsyncTaskState::Failed;
	if (Players.Num() > 1)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineLeaderboardsPlayFab::ReadLeaderboards: Can't accept more then 1 player at a time due to PlayFab API"));
		return false;
	}

	PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();
	if (ServerAPI.IsValid())
	{
		ReadObject->ReadState = EOnlineAsyncTaskState::InProgress;

		for (TSharedRef<const FUniqueNetId> UserId : Players)
		{
			PlayFab::ServerModels::FGetPlayerStatisticsRequest Request;
			Request.PlayFabId = UserId->ToString();
			for (auto Column : ReadObject->ColumnMetadata)
			{
				Request.StatisticNames.Add(Column.ColumnName.ToString());
			}

			PlayFab::UPlayFabServerAPI::FGetPlayerStatisticsDelegate SuccessDelegate = PlayFab::UPlayFabServerAPI::FGetPlayerStatisticsDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnSuccessCallback_Server_GetPlayerStatistics, UserId, ReadObject);
			PlayFab::FPlayFabErrorDelegate ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnErrorCallback_GetPlayerStatistics, UserId, ReadObject);
			ServerAPI->GetPlayerStatistics(Request);
		}
		return true;
	}
	else
	{
		ReadObject->ReadState = EOnlineAsyncTaskState::InProgress;

		for (TSharedRef<const FUniqueNetId> UserId : Players)
		{
			PlayFab::ClientModels::FGetLeaderboardAroundPlayerRequest Request;
			Request.PlayFabId = UserId->ToString();
			Request.StatisticName = ReadObject->ColumnMetadata[0].ColumnName.ToString();

			auto SuccessDelegate = PlayFab::UPlayFabClientAPI::FGetLeaderboardAroundPlayerDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnSuccessCallback_Client_GetLeaderboardAroundPlayer, ReadObject);
			auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnErrorCallback_Client_GetLeaderboardAroundPlayer, ReadObject);
			PlayFabSubsystem->GetClientAPI()->GetLeaderboardAroundPlayer(Request, SuccessDelegate, ErrorDelegate);
		}
		return true;
	}
	UE_LOG_ONLINE(Warning, TEXT("Unknown error during ReadLeaderboards"));
	return false;
}

bool FOnlineLeaderboardsPlayFab::ReadLeaderboardsForFriends(int32 LocalUserNum, FOnlineLeaderboardReadRef& ReadObject)
{
	if (ReadObject->ColumnMetadata.Num() != 1)
	{
		UE_LOG_ONLINE(Error, TEXT("ReadLeaderboardsAroundRank can only get/requires one column!"));
	}
	ReadObject->ReadState = EOnlineAsyncTaskState::InProgress;

	PlayFab::ClientModels::FGetFriendLeaderboardRequest Request;
	Request.StatisticName = ReadObject->ColumnMetadata[0].ColumnName.ToString();
	Request.MaxResultsCount = 10;
	//Request.ProfileConstraints->

	auto SuccessDelegate = PlayFab::UPlayFabClientAPI::FGetFriendLeaderboardDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnSuccessCallback_Client_GetFriendLeaderboard, ReadObject);
	auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnErrorCallback_Client_GetFriendLeaderboard, ReadObject);
	PlayFabSubsystem->GetClientAPI(LocalUserNum)->GetFriendLeaderboard(Request, SuccessDelegate, ErrorDelegate);
	return true;
}

bool FOnlineLeaderboardsPlayFab::ReadLeaderboardsAroundRank(int32 Rank, uint32 Range, FOnlineLeaderboardReadRef& ReadObject)
{
	if (ReadObject->ColumnMetadata.Num() != 1)
	{
		UE_LOG_ONLINE(Error, TEXT("ReadLeaderboardsAroundRank can only get/requires one column!"));
	}
	ReadObject->ReadState = EOnlineAsyncTaskState::InProgress;

	PlayFab::ClientModels::FGetLeaderboardRequest Request;
	Request.StatisticName = ReadObject->ColumnMetadata[0].ColumnName.ToString();
	Request.MaxResultsCount = Range;
	Request.StartPosition = Rank;
	PlayFabSubsystem->GetClientAPI()->GetLeaderboard(Request);
	return true;
	//return false;
}

bool FOnlineLeaderboardsPlayFab::ReadLeaderboardsAroundUser(TSharedRef<const FUniqueNetId> Player, uint32 Range, FOnlineLeaderboardReadRef& ReadObject)
{
	ReadObject->ReadState = EOnlineAsyncTaskState::InProgress;

	PlayFab::ClientModels::FGetLeaderboardAroundPlayerRequest Request;
	Request.StatisticName = ReadObject->LeaderboardName.ToString();
	Request.MaxResultsCount = Range;
	Request.PlayFabId = Player->ToString();

	auto SuccessDelegate = PlayFab::UPlayFabClientAPI::FGetLeaderboardAroundPlayerDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnSuccessCallback_Client_GetLeaderboardAroundPlayer, ReadObject);
	PlayFab::FPlayFabErrorDelegate ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnErrorCallback_Client_GetLeaderboardAroundPlayer, ReadObject);
	PlayFabSubsystem->GetClientAPI()->GetLeaderboardAroundPlayer(Request, SuccessDelegate, ErrorDelegate);

	return true;
}

void FOnlineLeaderboardsPlayFab::FreeStats(FOnlineLeaderboardRead& ReadObject)
{
	// No idea what this is for yet, at all.
}

bool FOnlineLeaderboardsPlayFab::WriteLeaderboards(const FName& SessionName, const FUniqueNetId& Player, FOnlineLeaderboardWrite& WriteObject)
{
	PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();
	if (!ServerAPI.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("Can only write to leaderboards where PlayFab ServerAPI is valid"));
		return false;
	}

	FLeaderboardStatistics* PlayerCachedStatistics = nullptr;
	FUniqueNetIdMatcher PlayerMatch(Player);
	for (auto& Elem : CachedStatistics)
	{
		if (PlayerMatch(Elem.Key))
		{
			PlayerCachedStatistics = &Elem.Value;
		}
	}
	if (PlayerCachedStatistics == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("You must get the player's leaderboard via ReadLeaderboards before setting it!"));
		return false;
	}
	for (auto& Row : WriteObject.Properties)
	{
		const FName& StatName = Row.Key;
		const FVariantData& Stat = Row.Value;
		PlayFab::ServerModels::FStatisticValue* CachedStatistic = PlayerCachedStatistics->Values.Find(StatName.ToString());
		if (CachedStatistic != nullptr)
		{
			switch (Stat.GetType())
			{
			case EOnlineKeyValuePairDataType::Int32:
			{
				int32 Value;
				Stat.GetValue(Value);
				CachedStatistic->Value = Value;
				break;
			}
			case EOnlineKeyValuePairDataType::UInt32:
			{
				uint32 Value;
				Stat.GetValue(Value);
				CachedStatistic->Value = Value;
				break;
			}
			case EOnlineKeyValuePairDataType::Int64:
			{
				int64 Value;
				Stat.GetValue(Value);
				CachedStatistic->Value = Value;
				break;
			}
			case EOnlineKeyValuePairDataType::UInt64:
			{
				uint64 Value;
				Stat.GetValue(Value);
				CachedStatistic->Value = Value;
				break;
			}
			case EOnlineKeyValuePairDataType::Double:
			case EOnlineKeyValuePairDataType::Float:
			// All other values(string, bool, blob, etc) can't be handled via PlayFab
			default:
			{
				UE_LOG_ONLINE(Error, TEXT("Invalid Stat type to save to the leaderboard: %s"), EOnlineKeyValuePairDataType::ToString(Stat.GetType()));
				return false;
			}
			}
		}
	}

	return true;
}

bool FOnlineLeaderboardsPlayFab::FlushLeaderboards(const FName& SessionName)
{
	PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();
	if (!ServerAPI.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("Can only flush leaderboards where PlayFab ServerAPI is valid"));
		return false;
	}
	for (auto Elem : CachedStatistics)
	{
		TSharedRef<const FUniqueNetId> PlayerId = Elem.Key;
		FLeaderboardStatistics Statistics = Elem.Value;
		PlayFab::ServerModels::FUpdatePlayerStatisticsRequest Request;
		Request.PlayFabId = PlayerId->ToString();
		for (auto Statistic : Statistics.Values)
		{
			PlayFab::ServerModels::FStatisticUpdate Update;
			Update.StatisticName = Statistic.Value.StatisticName;
			Update.Value = Statistic.Value.Value;
			Update.Version = Statistic.Value.Version;
			Request.Statistics.Add(Update);
		}
		Request.ForceUpdate = true;

		auto SuccessDelegate = PlayFab::UPlayFabServerAPI::FUpdatePlayerStatisticsDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnSuccessCallback_Server_UpdatePlayerStatistics, SessionName);
		auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineLeaderboardsPlayFab::OnErrorCallback_UpdatePlayerStatistics, SessionName);
		ServerAPI->UpdatePlayerStatistics(Request);
	}
	TriggerOnLeaderboardFlushCompleteDelegates(SessionName, true);
	return true;
}

bool FOnlineLeaderboardsPlayFab::WriteOnlinePlayerRatings(const FName& SessionName, int32 LeaderboardId, const TArray<FOnlinePlayerScore>& PlayerScores)
{
	return false;
}

void FOnlineLeaderboardsPlayFab::OnSuccessCallback_Server_GetPlayerStatistics(const PlayFab::ServerModels::FGetPlayerStatisticsResult& Result, TSharedRef<const FUniqueNetId> PlayerId, FOnlineLeaderboardReadRef ReadObject)
{
	FLeaderboardStatistics& PlayerCachedStatistics = CachedStatistics.FindOrAdd(PlayerId);
	for (auto Statistic : Result.Statistics)
	{
		FOnlineStatsRow Row(Statistic.StatisticName, PlayerId);
		Row.Columns.Add(FName(*Statistic.StatisticName), Statistic.Value);
		ReadObject->Rows.Add(Row);

		// Update or add to cached values
		PlayFab::ServerModels::FStatisticValue& CachedStatistic = PlayerCachedStatistics.Values.FindOrAdd(Statistic.StatisticName);
		CachedStatistic = Statistic;
	}

	ReadObject->ReadState = EOnlineAsyncTaskState::Done;
	TriggerOnLeaderboardReadCompleteDelegates(true);
}

void FOnlineLeaderboardsPlayFab::OnErrorCallback_GetPlayerStatistics(const PlayFab::FPlayFabError& ErrorResult, TSharedRef<const FUniqueNetId> PlayerId, FOnlineLeaderboardReadRef ReadObject)
{
	ReadObject->ReadState = EOnlineAsyncTaskState::Failed;
	TriggerOnLeaderboardReadCompleteDelegates(false);
}

void FOnlineLeaderboardsPlayFab::OnSuccessCallback_Server_UpdatePlayerStatistics(const PlayFab::ServerModels::FUpdatePlayerStatisticsResult& Result, FName SessionName)
{
	TriggerOnLeaderboardFlushCompleteDelegates(SessionName, true);
}

void FOnlineLeaderboardsPlayFab::OnErrorCallback_UpdatePlayerStatistics(const PlayFab::FPlayFabError& ErrorResult, FName SessionName)
{
	TriggerOnLeaderboardFlushCompleteDelegates(SessionName, false);
}

void FOnlineLeaderboardsPlayFab::OnSuccessCallback_Client_GetLeaderboardAroundPlayer(const PlayFab::ClientModels::FGetLeaderboardAroundPlayerResult& Result, FOnlineLeaderboardReadRef ReadObject)
{
	for (auto LeaderboardRow : Result.Leaderboard)
	{
		TSharedPtr<FUniqueNetId> UserId = MakeShareable(new FUniqueNetIdPlayFabId(LeaderboardRow.PlayFabId));
		const FString DisplayName(LeaderboardRow.DisplayName);
		FOnlineStatsRow Row(DisplayName, UserId->AsShared());
		Row.Rank = LeaderboardRow.Position;
		ReadObject->Rows.Add(Row);
	}

	ReadObject->ReadState = EOnlineAsyncTaskState::Done;
	TriggerOnLeaderboardReadCompleteDelegates(true);
}

void FOnlineLeaderboardsPlayFab::OnErrorCallback_Client_GetLeaderboardAroundPlayer(const PlayFab::FPlayFabError& ErrorResult, FOnlineLeaderboardReadRef ReadObject)
{
	UE_LOG_ONLINE(Error, TEXT("FOnlineLeaderboardsPlayFab::GetLeaderboardAroundPlayer: Failed grabbing leaderboard around player: %s"), *ErrorResult.ErrorMessage);

	ReadObject->ReadState = EOnlineAsyncTaskState::Failed;
	TriggerOnLeaderboardReadCompleteDelegates(false);
}

void FOnlineLeaderboardsPlayFab::OnSuccessCallback_Client_GetFriendLeaderboard(const PlayFab::ClientModels::FGetLeaderboardResult& Result, FOnlineLeaderboardReadRef ReadObject)
{
	for (auto LeaderboardRow : Result.Leaderboard)
	{
		TSharedPtr<FUniqueNetId> UserId = MakeShareable(new FUniqueNetIdPlayFabId(LeaderboardRow.PlayFabId));
		const FString DisplayName(LeaderboardRow.DisplayName);
		FOnlineStatsRow Row(DisplayName, UserId->AsShared());
		Row.Rank = LeaderboardRow.Position;
		ReadObject->Rows.Add(Row);
	}

	ReadObject->ReadState = EOnlineAsyncTaskState::Done;
	TriggerOnLeaderboardReadCompleteDelegates(true);
}

void FOnlineLeaderboardsPlayFab::OnErrorCallback_Client_GetFriendLeaderboard(const PlayFab::FPlayFabError& ErrorResult, FOnlineLeaderboardReadRef ReadObject)
{
	UE_LOG_ONLINE(Error, TEXT("FOnlineLeaderboardsPlayFab::GetFriendLeaderboard: Failed grabbing leaderboard around player: %s"), *ErrorResult.ErrorMessage);

	ReadObject->ReadState = EOnlineAsyncTaskState::Failed;
	TriggerOnLeaderboardReadCompleteDelegates(false);
}
