// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlineUserPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFab.h"


bool FOnlineUserPlayFab::QueryUserInfo(int32 LocalUserNum, const TArray<TSharedRef<const FUniqueNetId>>& UserIds)
{
	PlayFabClientPtr ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
	if (ClientAPI.IsValid())
	{
		return false;
		for (TSharedRef<const FUniqueNetId> UserId : UserIds)
		{
			PlayFab::ClientModels::FGetAccountInfoRequest Request;
			Request.PlayFabId = UserId->ToString();
			SuccessDelegate_Client_GetAccountInfo = PlayFab::UPlayFabClientAPI::FGetAccountInfoDelegate::CreateRaw(this, &FOnlineUserPlayFab::OnSuccessCallback_Client_GetAccountInfo, LocalUserNum, &UserIds);
			ClientAPI->GetAccountInfo(Request, SuccessDelegate_Client_GetAccountInfo);
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("PlayFab Client Interface not available"));
	}
	return false;
}

bool FOnlineUserPlayFab::GetAllUserInfo(int32 LocalUserNum, TArray<TSharedRef<class FOnlineUser>>& OutUsers)
{
	return false;
}

TSharedPtr<FOnlineUser> FOnlineUserPlayFab::GetUserInfo(int32 LocalUserNum, const class FUniqueNetId& UserId)
{
	return nullptr;
}

bool FOnlineUserPlayFab::QueryUserIdMapping(const FUniqueNetId& UserId, const FString& DisplayNameOrEmail, const FOnQueryUserMappingComplete& Delegate /*= FOnQueryUserMappingComplete()*/)
{
	Delegate.ExecuteIfBound(false, UserId, DisplayNameOrEmail, FUniqueNetIdString(), TEXT("not implemented"));
	return false;
}

bool FOnlineUserPlayFab::QueryExternalIdMappings(const FUniqueNetId& UserId, const FExternalIdQueryOptions& QueryOptions, const TArray<FString>& ExternalIds, const FOnQueryExternalIdMappingsComplete& Delegate /*= FOnQueryExternalIdMappingsComplete()*/)
{
	Delegate.ExecuteIfBound(false, UserId, QueryOptions, ExternalIds, TEXT("not implemented"));
	return false;
}

void FOnlineUserPlayFab::GetExternalIdMappings(const FExternalIdQueryOptions& QueryOptions, const TArray<FString>& ExternalIds, TArray<TSharedPtr<const FUniqueNetId>>& OutIds)
{
	OutIds.SetNum(ExternalIds.Num());
}

TSharedPtr<const FUniqueNetId> FOnlineUserPlayFab::GetExternalIdMapping(const FExternalIdQueryOptions& QueryOptions, const FString& ExternalId)
{
	return nullptr;
}

void FOnlineUserPlayFab::OnSuccessCallback_Client_GetAccountInfo(const PlayFab::ClientModels::FGetAccountInfoResult& Result, int32 LocalUserNum, const TArray<TSharedRef<const FUniqueNetId>>* UserIds)
{
	TriggerOnQueryUserInfoCompleteDelegates(LocalUserNum, true, *UserIds, TEXT(""));
}

void FOnlineUserPlayFab::OnErrorCallback_GetAccountInfo(const PlayFab::FPlayFabError& ErrorResult, int32 LocalUserNum, const TArray<TSharedRef<const FUniqueNetId>>* UserIds)
{
	TriggerOnQueryUserInfoCompleteDelegates(LocalUserNum, false, *UserIds, ErrorResult.ErrorMessage);
}
