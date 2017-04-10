// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlineTimePlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFab.h"


bool FOnlineTimePlayFab::QueryServerUtcTime()
{
	PlayFabClientPtr ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
	if (ClientAPI.IsValid())
	{
		if (!ErrorDelegate_GetTime.IsBound())
		{
			SuccessDelegate_GetTime.CreateRaw(this, &FOnlineTimePlayFab::OnSuccessCallback_GetTime);
			ErrorDelegate_GetTime.CreateRaw(this, &FOnlineTimePlayFab::OnErrorCallback_GetTime);
			ClientAPI->GetTime(SuccessDelegate_GetTime, ErrorDelegate_GetTime);
			return true;
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("Already iniated UTC Time query"));
		}
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("PlayFab Client Interface not available"));
	}
	return false;
}

FString FOnlineTimePlayFab::GetLastServerUtcTime()
{
	return CachedUTC;
}

void FOnlineTimePlayFab::OnSuccessCallback_GetTime(const PlayFab::ClientModels::FGetTimeResult& Result)
{
	SuccessDelegate_GetTime.Unbind();
	CachedUTC = Result.Time.ToString();
	TriggerOnQueryServerUtcTimeCompleteDelegates(true, CachedUTC, TEXT(""));
}

void FOnlineTimePlayFab::OnErrorCallback_GetTime(const PlayFab::FPlayFabError& ErrorResult)
{
	ErrorDelegate_GetTime.Unbind();
	TriggerOnQueryServerUtcTimeCompleteDelegates(false, TEXT(""), ErrorResult.ErrorMessage);
}
