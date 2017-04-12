// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlineUserPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFab.h"


TSharedRef<const FUniqueNetId> FOnlineUserInfoPlayFab::GetUserId() const
{
	return UserId;
}

FString FOnlineUserInfoPlayFab::GetRealName() const
{
	return TEXT("Pinocchio");
}

FString FOnlineUserInfoPlayFab::GetDisplayName(const FString& Platform /*= FString()*/) const
{
	return !DisplayName.IsEmpty() ? DisplayName : TEXT("InvalidOnlineSubsystemName");
}

bool FOnlineUserInfoPlayFab::GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* FoundAttr = UserAttributes.Find(AttrName);
	if (FoundAttr != NULL)
	{
		OutAttrValue = *FoundAttr;
		return true;
	}
	return false;
}

bool FOnlineUserPlayFab::QueryUserInfo(int32 LocalUserNum, const TArray<TSharedRef<const FUniqueNetId>>& UserIds)
{
	if (UserIds.Num() > 1)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineUserPlayFab::QueryUserInfo: Can't accept more then 1 UserId at a time due to PlayFab API"));
		return false;
	}
	PlayFabClientPtr ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
	if (ClientAPI.IsValid())
	{
		for (TSharedRef<const FUniqueNetId> UserId : UserIds)
		{
			PlayFab::ClientModels::FGetAccountInfoRequest Request;
			Request.PlayFabId = UserId->ToString();
			SuccessDelegate_Client_GetAccountInfo = PlayFab::UPlayFabClientAPI::FGetAccountInfoDelegate::CreateRaw(this, &FOnlineUserPlayFab::OnSuccessCallback_Client_GetAccountInfo, LocalUserNum, &UserIds);
			ClientAPI->GetAccountInfo(Request, SuccessDelegate_Client_GetAccountInfo);
		}
		return true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("PlayFab Client Interface not available"));
	}
	return false;
}

bool FOnlineUserPlayFab::GetAllUserInfo(int32 LocalUserNum, TArray<TSharedRef<class FOnlineUser>>& OutUsers)
{
	for (TSharedRef<class FOnlineUser> User : CachedUsers)
	{
		OutUsers.Add(User);
	}
	return true;
}

TSharedPtr<FOnlineUser> FOnlineUserPlayFab::GetUserInfo(int32 LocalUserNum, const class FUniqueNetId& UserId)
{
	for (TSharedRef<class FOnlineUser> User : CachedUsers)
	{
		if (*(User->GetUserId()) == UserId)
		{
			return User;
		}
	}
	return nullptr;
}

bool FOnlineUserPlayFab::QueryUserIdMapping(const FUniqueNetId& UserId, const FString& DisplayNameOrEmail, const FOnQueryUserMappingComplete& Delegate /*= FOnQueryUserMappingComplete()*/)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineUserPlayFab::QueryUserIdMapping: Not yet implemented"));
	Delegate.ExecuteIfBound(false, UserId, DisplayNameOrEmail, FUniqueNetIdString(), TEXT("not implemented"));
	return false;
}

bool FOnlineUserPlayFab::QueryExternalIdMappings(const FUniqueNetId& UserId, const FExternalIdQueryOptions& QueryOptions, const TArray<FString>& ExternalIds, const FOnQueryExternalIdMappingsComplete& Delegate /*= FOnQueryExternalIdMappingsComplete()*/)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineUserPlayFab::QueryExternalIdMappings: Not yet implemented"));
	Delegate.ExecuteIfBound(false, UserId, QueryOptions, ExternalIds, TEXT("not implemented"));
	return false;
}

void FOnlineUserPlayFab::GetExternalIdMappings(const FExternalIdQueryOptions& QueryOptions, const TArray<FString>& ExternalIds, TArray<TSharedPtr<const FUniqueNetId>>& OutIds)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineUserPlayFab::GetExternalIdMappings: Not yet implemented"));
	OutIds.SetNum(ExternalIds.Num());
}

TSharedPtr<const FUniqueNetId> FOnlineUserPlayFab::GetExternalIdMapping(const FExternalIdQueryOptions& QueryOptions, const FString& ExternalId)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineUserPlayFab::GetExternalIdMapping: Not yet implemented"));
	return nullptr;
}

void FOnlineUserPlayFab::OnSuccessCallback_Client_GetAccountInfo(const PlayFab::ClientModels::FGetAccountInfoResult& Result, int32 LocalUserNum, const TArray<TSharedRef<const FUniqueNetId>>* UserIds)
{
	TSharedRef<FOnlineUserInfoPlayFab> UserInfo = MakeShareable(new FOnlineUserInfoPlayFab(Result.AccountInfo->PlayFabId));

	UserInfo->DisplayName = Result.AccountInfo->TitleInfo->DisplayName;
	CachedUsers.Add(UserInfo);

	TriggerOnQueryUserInfoCompleteDelegates(LocalUserNum, true, *UserIds, TEXT(""));
}

void FOnlineUserPlayFab::OnErrorCallback_GetAccountInfo(const PlayFab::FPlayFabError& ErrorResult, int32 LocalUserNum, const TArray<TSharedRef<const FUniqueNetId>>* UserIds)
{
	TriggerOnQueryUserInfoCompleteDelegates(LocalUserNum, false, *UserIds, ErrorResult.ErrorMessage);
}
