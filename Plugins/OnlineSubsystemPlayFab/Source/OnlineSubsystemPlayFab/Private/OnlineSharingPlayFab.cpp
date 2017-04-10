// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlineSharingPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFab.h"


void FOnlineSharingPlayFab::RequestCurrentPermissions(int32 LocalUserNum, FOnRequestCurrentPermissionsComplete& CompletionDelegate)
{
	TArray<FSharingPermission> Perms;
	CompletionDelegate.Execute(LocalUserNum, false, Perms);
}

void FOnlineSharingPlayFab::GetCurrentPermissions(int32 LocalUserNum, TArray<FSharingPermission>& OutPermissions)
{
	
}

bool FOnlineSharingPlayFab::RequestNewReadPermissions(int32 LocalUserNum, EOnlineSharingCategory NewPermissions)
{
	TriggerOnReadNewsFeedCompleteDelegates(LocalUserNum, false);
	return false;
}

bool FOnlineSharingPlayFab::RequestNewPublishPermissions(int32 LocalUserNum, EOnlineSharingCategory NewPermissions, EOnlineStatusUpdatePrivacy Privacy)
{
	TriggerOnRequestNewPublishPermissionsCompleteDelegates(LocalUserNum, false);
	return false;
}

bool FOnlineSharingPlayFab::ReadNewsFeed(int32 LocalUserNum, int32 NumPostsToRead /*= 50*/)
{
	PlayFabClientPtr ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
	if (ClientAPI.IsValid())
	{
		if (!SuccessDelegate_Client_GetTitleNews.IsBound())
		{
			PlayFab::ClientModels::FGetTitleNewsRequest Request;
			Request.Count = NumPostsToRead;
			SuccessDelegate_Client_GetTitleNews.CreateRaw(this, &FOnlineSharingPlayFab::OnSuccessCallback_Client_GetTitleNews, LocalUserNum);
			PlayFab::FPlayFabErrorDelegate ErrorDelegate_GetTitleNews = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSharingPlayFab::OnErrorCallback_GetTitleNews, LocalUserNum);
			ClientAPI->GetTitleNews(Request, SuccessDelegate_Client_GetTitleNews, ErrorDelegate_GetTitleNews);
			return true;
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("Already reading news feed"));
		}
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("PlayFab Interface not available"));
	}
	TriggerOnReadNewsFeedCompleteDelegates(LocalUserNum, false);
	return false;
}

EOnlineCachedResult::Type FOnlineSharingPlayFab::GetCachedNewsFeed(int32 LocalUserNum, int32 NewsFeedIdx, FOnlineStatusUpdate& OutNewsFeed)
{
	if (CachedNewsFeeds.IsValidIndex(NewsFeedIdx))
	{
		OutNewsFeed = CachedNewsFeeds[NewsFeedIdx];
		return EOnlineCachedResult::Success;
	}
	return EOnlineCachedResult::NotFound;
}

EOnlineCachedResult::Type FOnlineSharingPlayFab::GetCachedNewsFeeds(int32 LocalUserNum, TArray<FOnlineStatusUpdate>& OutNewsFeeds)
{
	if (CachedNewsFeeds.Num() < 0)
	{
		OutNewsFeeds.Append(CachedNewsFeeds);
		return EOnlineCachedResult::Success;
	}
	return EOnlineCachedResult::NotFound;
}

bool FOnlineSharingPlayFab::ShareStatusUpdate(int32 LocalUserNum, const FOnlineStatusUpdate& StatusUpdate)
{
	TriggerOnSharePostCompleteDelegates(LocalUserNum, false);
	return false;
}

void FOnlineSharingPlayFab::OnSuccessCallback_Client_GetTitleNews(const PlayFab::ClientModels::FGetTitleNewsResult& Result, int32 LocalUserNum)
{
	SuccessDelegate_Client_GetTitleNews.Unbind();

	for (auto News : Result.News)
	{
		FOnlineStatusUpdate NewsFeed;
		NewsFeed.Message = News.Body;
		CachedNewsFeeds.Add(NewsFeed);
	}

	TriggerOnReadNewsFeedCompleteDelegates(LocalUserNum, true);
}

void FOnlineSharingPlayFab::OnErrorCallback_GetTitleNews(const PlayFab::FPlayFabError& ErrorResult, int32 LocalUserNum)
{
	SuccessDelegate_Client_GetTitleNews.Unbind();

	TriggerOnReadNewsFeedCompleteDelegates(LocalUserNum, false);
}
