// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlineExternalUIPlayFab.h"
#include "OnlineSessionPlayFab.h"
#include "OnlineSubsystemPlayFab.h"


bool FOnlineExternalUIPlayFab::ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate /*= FOnLoginUIClosedDelegate()*/)
{
	return false;
}

bool FOnlineExternalUIPlayFab::ShowFriendsUI(int32 LocalUserNum)
{
	return false;
}

bool FOnlineExternalUIPlayFab::ShowInviteUI(int32 LocalUserNum, FName SessionMame)
{
	return false;
}

bool FOnlineExternalUIPlayFab::ShowAchievementsUI(int32 LocalUserNum)
{
	return false;
}

bool FOnlineExternalUIPlayFab::ShowLeaderboardUI(const FString& LeaderboardName)
{
	return false;
}

bool FOnlineExternalUIPlayFab::ShowWebURL(const FString& Url, const FShowWebUrlParams& ShowParams, const FOnShowWebUrlClosedDelegate& Delegate)
{
	return false;
}

bool FOnlineExternalUIPlayFab::CloseWebURL()
{
	return false;
}

bool FOnlineExternalUIPlayFab::ShowProfileUI(const FUniqueNetId& Requestor, const FUniqueNetId& Requestee, const FOnProfileUIClosedDelegate& Delegate)
{
	return false;
}

bool FOnlineExternalUIPlayFab::ShowAccountUpgradeUI(const FUniqueNetId& UniqueId)
{
	return false;
}

bool FOnlineExternalUIPlayFab::ShowStoreUI(int32 LocalUserNum, const FShowStoreParams& ShowParams, const FOnShowStoreUIClosedDelegate& Delegate)
{
	return false;
}

bool FOnlineExternalUIPlayFab::ShowSendMessageUI(int32 LocalUserNum, const FShowSendMessageParams& ShowParams, const FOnShowSendMessageUIClosedDelegate& Delegate)
{
	return false;
}

