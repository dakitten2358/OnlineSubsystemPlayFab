// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

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
	// We can accept more then one, we just have to push this to an async task to deal with it most effectively
	if (UserIds.Num() > 1)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineUserPlayFab::QueryUserInfo: Can't accept more then 1 UserId at a time due to PlayFab API"));
		return false;
	}
	PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();
	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(LocalUserNum);
	if (ServerAPI.IsValid())
	{
		for (TSharedRef<const FUniqueNetId> UserId : UserIds)
		{
			PlayFab::ServerModels::FGetUserAccountInfoRequest Request;
			Request.PlayFabId = UserId->ToString();
			PlayFab::UPlayFabServerAPI::FGetUserAccountInfoDelegate SuccessDelegate = PlayFab::UPlayFabServerAPI::FGetUserAccountInfoDelegate::CreateRaw(this, &FOnlineUserPlayFab::OnSuccessCallback_Server_GetAccountInfo, LocalUserNum, &UserIds);
			PlayFab::FPlayFabErrorDelegate ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineUserPlayFab::OnErrorCallback_GetAccountInfo, LocalUserNum, &UserIds);
			ServerAPI->GetUserAccountInfo(Request, SuccessDelegate);
		}
		return true;
	}
	else if (ClientAPI.IsValid())
	{
		for (TSharedRef<const FUniqueNetId> UserId : UserIds)
		{
			PlayFab::ClientModels::FGetAccountInfoRequest Request;
			Request.PlayFabId = UserId->ToString();
			SuccessDelegate_Client_GetAccountInfo = PlayFab::UPlayFabClientAPI::FGetAccountInfoDelegate::CreateRaw(this, &FOnlineUserPlayFab::OnSuccessCallback_Client_GetAccountInfo, LocalUserNum, &UserIds);
			auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineUserPlayFab::OnErrorCallback_GetAccountInfo, LocalUserNum, &UserIds);
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
	PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();
	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI();
	// Can't just yet, PlayFab doesn't have the method for PlayFabUser/Email on the Server API(probably never will)
	if (ServerAPI.IsValid() && !ClientAPI->IsClientLoggedIn())
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineUserPlayFab::QueryUserIdMapping: PlayFab ServerAPI can't query from Display Name/Email/etc"));
		return false;
	}

	bool bIsEmail = DisplayNameOrEmail.Contains("@", ESearchCase::IgnoreCase);

	PlayFab::ClientModels::FGetAccountInfoRequest Request;
	if (bIsEmail)
		Request.Email = DisplayNameOrEmail;
	else
		Request.TitleDisplayName = DisplayNameOrEmail;

	TSharedRef<const FUniqueNetId> PlayerId = MakeShareable(new FUniqueNetIdPlayFabId(UserId));
	auto SuccessDelegate = PlayFab::UPlayFabClientAPI::FGetAccountInfoDelegate::CreateRaw(this, &FOnlineUserPlayFab::OnSuccessCallback_Client_Query_GetAccountInfo, PlayerId, DisplayNameOrEmail, Delegate);
	auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineUserPlayFab::OnErrorCallback_Query_GetAccountInfo, PlayerId, DisplayNameOrEmail, Delegate);
	PlayFabSubsystem->GetClientAPI(UserId)->GetAccountInfo(Request, SuccessDelegate, ErrorDelegate);
	return true;
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

void FOnlineUserPlayFab::OnSuccessCallback_Server_GetAccountInfo(const PlayFab::ServerModels::FGetUserAccountInfoResult& Result, int32 LocalUserNum, const TArray<TSharedRef<const FUniqueNetId>>* UserIds)
{
	TSharedRef<FOnlineUserInfoPlayFab> UserInfo = MakeShareable(new FOnlineUserInfoPlayFab(Result.UserInfo->PlayFabId));

	UserInfo->DisplayName = Result.UserInfo->TitleInfo->DisplayName;
	CachedUsers.Add(UserInfo);

	TriggerOnQueryUserInfoCompleteDelegates(LocalUserNum, true, *UserIds, TEXT(""));
}

void FOnlineUserPlayFab::OnErrorCallback_GetAccountInfo(const PlayFab::FPlayFabCppError& ErrorResult, int32 LocalUserNum, const TArray<TSharedRef<const FUniqueNetId>>* UserIds)
{
	TriggerOnQueryUserInfoCompleteDelegates(LocalUserNum, false, *UserIds, ErrorResult.ErrorMessage);
}

void FOnlineUserPlayFab::OnSuccessCallback_Client_Query_GetAccountInfo(const PlayFab::ClientModels::FGetAccountInfoResult& Result, TSharedRef<const FUniqueNetId> UserId, const FString DisplayNameOrEmail, const FOnQueryUserMappingComplete Delegate)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineUserPlayFab::OnSuccessCallback_Client_Query_GetAccountInfo: Not yet implemented"));
	Delegate.ExecuteIfBound(true, UserId.Get(), DisplayNameOrEmail, FUniqueNetIdPlayFabId(Result.AccountInfo->PlayFabId), TEXT(""));
}

void FOnlineUserPlayFab::OnErrorCallback_Query_GetAccountInfo(const PlayFab::FPlayFabCppError& ErrorResult, TSharedRef<const FUniqueNetId> UserId, const FString DisplayNameOrEmail, const FOnQueryUserMappingComplete Delegate)
{
	Delegate.ExecuteIfBound(false, UserId.Get(), DisplayNameOrEmail, FUniqueNetIdPlayFabId(), ErrorResult.GenerateErrorReport());
}
