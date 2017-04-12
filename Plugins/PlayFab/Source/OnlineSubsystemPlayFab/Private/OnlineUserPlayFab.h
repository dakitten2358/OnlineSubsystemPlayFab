// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
 
#include "OnlineUserInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"

/**
* Info associated with an online user on the PlayFab service
*/
class FOnlineUserInfoPlayFab : public FOnlineUser
{
public:

	// FOnlineUser

	virtual TSharedRef<const FUniqueNetId> GetUserId() const override;
	virtual FString GetRealName() const override;
	virtual FString GetDisplayName(const FString& Platform = FString()) const override;
	virtual bool GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const override;

	// FOnlineUserInfoPlayFab

	/**
	* Init/default constructor
	*/
	FOnlineUserInfoPlayFab(const FString& InUserId = TEXT(""))
		: UserId(new FUniqueNetIdString(InUserId))
	{
	}

	/**
	* Destructor
	*/
	virtual ~FOnlineUserInfoPlayFab()
	{
	}

	/** User Id represented as a FUniqueNetId */
	TSharedRef<const FUniqueNetId> UserId;
	/** Additional key/value pair data related to user attribution */
	TMap<FString, FString> UserAttributes;

	FString DisplayName;
};

/**
* Interface class for achievements
*/
class FOnlineUserPlayFab : public IOnlineUser
{
private:
	TArray<TSharedRef<class FOnlineUser>> CachedUsers;

	/** Reference to the main PlayFab subsystem */
	class FOnlineSubsystemPlayFab* PlayFabSubsystem;

	/** Hidden on purpose */
	FOnlineUserPlayFab() :
		PlayFabSubsystem(NULL)
	{
	}

PACKAGE_SCOPE:

	FOnlineUserPlayFab(class FOnlineSubsystemPlayFab* InPlayFabSubsystem) :
		PlayFabSubsystem(InPlayFabSubsystem)
	{
	}

public:

	virtual ~FOnlineUserPlayFab()
	{
	}

	// IOnlineUser
	virtual bool QueryUserInfo(int32 LocalUserNum, const TArray<TSharedRef<const FUniqueNetId>>& UserIds) override;
	virtual bool GetAllUserInfo(int32 LocalUserNum, TArray<TSharedRef<class FOnlineUser>>& OutUsers) override;
	virtual TSharedPtr<FOnlineUser> GetUserInfo(int32 LocalUserNum, const class FUniqueNetId& UserId) override;
	virtual bool QueryUserIdMapping(const FUniqueNetId& UserId, const FString& DisplayNameOrEmail, const FOnQueryUserMappingComplete& Delegate = FOnQueryUserMappingComplete()) override;
	virtual bool QueryExternalIdMappings(const FUniqueNetId& UserId, const FExternalIdQueryOptions& QueryOptions, const TArray<FString>& ExternalIds, const FOnQueryExternalIdMappingsComplete& Delegate = FOnQueryExternalIdMappingsComplete()) override;
	virtual void GetExternalIdMappings(const FExternalIdQueryOptions& QueryOptions, const TArray<FString>& ExternalIds, TArray<TSharedPtr<const FUniqueNetId>>& OutIds) override;
	virtual TSharedPtr<const FUniqueNetId> GetExternalIdMapping(const FExternalIdQueryOptions& QueryOptions, const FString& ExternalId) override;

private:
	PlayFab::UPlayFabClientAPI::FGetAccountInfoDelegate SuccessDelegate_Client_GetAccountInfo;
	void OnSuccessCallback_Client_GetAccountInfo(const PlayFab::ClientModels::FGetAccountInfoResult& Result, int32 LocalUserNum, const TArray<TSharedRef<const FUniqueNetId>>* UserIds);
	void OnErrorCallback_GetAccountInfo(const PlayFab::FPlayFabError& ErrorResult, int32 LocalUserNum, const TArray<TSharedRef<const FUniqueNetId>>* UserIds);
};

typedef TSharedPtr<FOnlineUserPlayFab, ESPMode::ThreadSafe> FOnlineUserPlayFabPtr;