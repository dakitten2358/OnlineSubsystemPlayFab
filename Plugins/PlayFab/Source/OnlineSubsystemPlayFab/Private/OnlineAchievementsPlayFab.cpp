// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlineAchievementsPlayFab.h"
#include "OnlineSubsystemPlayFab.h"


void FOnlineAchievementsPlayFab::WriteAchievements(const FUniqueNetId& PlayerId, FOnlineAchievementsWriteRef& WriteObject, const FOnAchievementsWrittenDelegate& Delegate /*= FOnAchievementsWrittenDelegate()*/)
{
	
}

void FOnlineAchievementsPlayFab::QueryAchievements(const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate /*= FOnQueryAchievementsCompleteDelegate()*/)
{
	
}

void FOnlineAchievementsPlayFab::QueryAchievementDescriptions(const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate /*= FOnQueryAchievementsCompleteDelegate()*/)
{
	
}

EOnlineCachedResult::Type FOnlineAchievementsPlayFab::GetCachedAchievement(const FUniqueNetId& PlayerId, const FString& AchievementId, FOnlineAchievement& OutAchievement)
{
	return EOnlineCachedResult::NotFound;
}

EOnlineCachedResult::Type FOnlineAchievementsPlayFab::GetCachedAchievements(const FUniqueNetId& PlayerId, TArray<FOnlineAchievement>& OutAchievements)
{
	return EOnlineCachedResult::NotFound;
}

EOnlineCachedResult::Type FOnlineAchievementsPlayFab::GetCachedAchievementDescription(const FString& AchievementId, FOnlineAchievementDesc& OutAchievementDesc)
{
	return EOnlineCachedResult::NotFound;
}
#if !UE_BUILD_SHIPPING
bool FOnlineAchievementsPlayFab::ResetAchievements(const FUniqueNetId& PlayerId)
{
	return false;
}
#endif // !UE_BUILD_SHIPPING
