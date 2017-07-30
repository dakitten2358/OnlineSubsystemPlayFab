// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
 
#include "CoreMinimal.h"
#include "OnlineSharingInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"

/**
* Interface class for achievements
*/
class FOnlineSharingPlayFab : public IOnlineSharing
{
private:

	TArray<FOnlineStatusUpdate> CachedNewsFeeds;

	/** Reference to the main PlayFab subsystem */
	class FOnlineSubsystemPlayFab* PlayFabSubsystem;

	/** Hidden on purpose */
	FOnlineSharingPlayFab() :
		PlayFabSubsystem(NULL)
	{
	}

PACKAGE_SCOPE:

	FOnlineSharingPlayFab(class FOnlineSubsystemPlayFab* InPlayFabSubsystem) :
		PlayFabSubsystem(InPlayFabSubsystem)
	{
	}

public:

	virtual ~FOnlineSharingPlayFab()
	{
	}

	// IOnlineSharing
	virtual void RequestCurrentPermissions(int32 LocalUserNum, FOnRequestCurrentPermissionsComplete& CompletionDelegate) override;
	virtual void GetCurrentPermissions(int32 LocalUserNum, TArray<FSharingPermission>& OutPermissions) override;
	virtual bool RequestNewReadPermissions(int32 LocalUserNum, EOnlineSharingCategory NewPermissions) override;
	virtual bool RequestNewPublishPermissions(int32 LocalUserNum, EOnlineSharingCategory NewPermissions, EOnlineStatusUpdatePrivacy Privacy) override;
	virtual bool ReadNewsFeed(int32 LocalUserNum, int32 NumPostsToRead = 50) override;
	virtual EOnlineCachedResult::Type GetCachedNewsFeed(int32 LocalUserNum, int32 NewsFeedIdx, FOnlineStatusUpdate& OutNewsFeed) override;
	virtual EOnlineCachedResult::Type GetCachedNewsFeeds(int32 LocalUserNum, TArray<FOnlineStatusUpdate>& OutNewsFeeds) override;
	virtual bool ShareStatusUpdate(int32 LocalUserNum, const FOnlineStatusUpdate& StatusUpdate) override;

private:
	PlayFab::UPlayFabClientAPI::FGetTitleNewsDelegate SuccessDelegate_Client_GetTitleNews;

	void OnSuccessCallback_Client_GetTitleNews(const PlayFab::ClientModels::FGetTitleNewsResult& Result, int32 LocalUserNum);
	void OnErrorCallback_GetTitleNews(const PlayFab::FPlayFabError& ErrorResult, int32 LocalUserNum);
};

typedef TSharedPtr<FOnlineSharingPlayFab, ESPMode::ThreadSafe> FOnlineSharingPlayFabPtr;