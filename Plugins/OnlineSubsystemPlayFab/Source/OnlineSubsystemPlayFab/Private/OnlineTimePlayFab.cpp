// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineTimePlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFab.h"


bool FOnlineTimePlayFab::QueryServerUtcTime()
{
	if (bCatchingServerTime)
	{
		UE_LOG_ONLINE(Warning, TEXT("Already iniated UTC Time query"));
		return false;
	}
	bCatchingServerTime = true;
	PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();
	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI();
	if (ServerAPI.IsValid())
	{
		ServerAPI->GetTime(PlayFab::UPlayFabServerAPI::FGetTimeDelegate::CreateRaw(this, &FOnlineTimePlayFab::OnSuccessCallback_S_GetTime), PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineTimePlayFab::OnErrorCallback_GetTime));
		return true;
	}
	else if (ClientAPI->IsClientLoggedIn())
	{
		ClientAPI->GetTime(PlayFab::UPlayFabClientAPI::FGetTimeDelegate::CreateRaw(this, &FOnlineTimePlayFab::OnSuccessCallback_C_GetTime), PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineTimePlayFab::OnErrorCallback_GetTime));
		return true;
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("PlayFab Client Interface not available"));
	}
	bCatchingServerTime = false;
	return false;
}

FString FOnlineTimePlayFab::GetLastServerUtcTime()
{
	return CachedUTC;
}

void FOnlineTimePlayFab::OnSuccessCallback_S_GetTime(const PlayFab::ServerModels::FGetTimeResult& Result)
{
	bCatchingServerTime = false;
	CachedUTC = Result.Time.ToString();
	TriggerOnQueryServerUtcTimeCompleteDelegates(true, CachedUTC, TEXT(""));
}

void FOnlineTimePlayFab::OnSuccessCallback_C_GetTime(const PlayFab::ClientModels::FGetTimeResult& Result)
{
	bCatchingServerTime = false;
	CachedUTC = Result.Time.ToString();
	TriggerOnQueryServerUtcTimeCompleteDelegates(true, CachedUTC, TEXT(""));
}

void FOnlineTimePlayFab::OnErrorCallback_GetTime(const PlayFab::FPlayFabError& ErrorResult)
{
	bCatchingServerTime = false;
	TriggerOnQueryServerUtcTimeCompleteDelegates(false, TEXT(""), ErrorResult.ErrorMessage);
}
