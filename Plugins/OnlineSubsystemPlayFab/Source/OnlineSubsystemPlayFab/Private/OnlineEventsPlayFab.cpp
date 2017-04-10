// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlineEventsPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "OnlineIdentityInterface.h"
#include "PlayFab.h"


bool FOnlineEventsPlayFab::TriggerEvent(const FUniqueNetId& PlayerId, const TCHAR* EventName, const FOnlineEventParms& Parms)
{
	// Is this a character event
	bool characterEvent = FString(EventName).StartsWith("character_", ESearchCase::CaseSensitive);
	// Is this a player event
	bool playerEvent = FString(EventName).StartsWith("player_", ESearchCase::CaseSensitive);
	// All others will default as title events

	TMap<FString, PlayFab::FMultitypeVar> PlayFabParms;
	for (auto Elem : Parms)
	{
		PlayFab::FMultitypeVar& Var = PlayFabParms.FindOrAdd(Elem.Key.ToString());
		Var = Elem.Value.ToString();
	}

	PlayFabServerPtr ServerAPI = IPlayFabModuleInterface::Get().GetServerAPI();
	PlayFabClientPtr ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
	if (ServerAPI.IsValid())
	{
		if (characterEvent)
		{
			/*PlayFab::ServerModels::FWriteServerCharacterEventRequest Request;
			Request.EventName = EventName;
			Request.PlayFabId = PlayerId.ToString();
			PlayFabSubsystem->GetIdentityInterface()->GetUserAccount(PlayerId)->GetUserAttribute("CharacterId", Request.CharacterId);
			Request.Body = PlayFabParms;
			ServerAPI->WriteCharacterEvent(Request);*/
		}
		else if (playerEvent)
		{
			PlayFab::ServerModels::FWriteServerPlayerEventRequest Request;
			Request.EventName = EventName;
			Request.PlayFabId = PlayerId.ToString();
			Request.Body = PlayFabParms;
			ServerAPI->WritePlayerEvent(Request);
		}
		else
		{
			PlayFab::ServerModels::FWriteTitleEventRequest Request;
			Request.EventName = EventName;
			Request.Body = PlayFabParms;
			ServerAPI->WriteTitleEvent(Request);
		}
		return true;
	}
	else if (ClientAPI.IsValid() && ClientAPI->IsClientLoggedIn())
	{
		if (PlayerId != *PlayFabSubsystem->GetIdentityInterface()->GetUniquePlayerId(0))
		{
			UE_LOG_ONLINE(Error, TEXT("Can't write events for another client!"));
		}
		else
		{
			if (characterEvent)
			{
				/*PlayFab::ClientModels::FWriteClientCharacterEventRequest Request;
				Request.EventName = EventName;
				PlayFabSubsystem->GetIdentityInterface()->GetUserAccount(PlayerId)->GetUserAttribute("CharacterId", Request.CharacterId);
				Request.Body = PlayFabParms;
				ClientAPI->WriteCharacterEvent(Request);*/
			}
			else if (playerEvent)
			{
				PlayFab::ClientModels::FWriteClientPlayerEventRequest Request;
				Request.EventName = EventName;
				Request.Body = PlayFabParms;
				ClientAPI->WritePlayerEvent(Request);
			}
			else
			{
				PlayFab::ClientModels::FWriteTitleEventRequest Request;
				Request.EventName = EventName;
				Request.Body = PlayFabParms;
				ClientAPI->WriteTitleEvent(Request);
			}
		}
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("PlayFab Interface not available"));
	}
	return false;
}

void FOnlineEventsPlayFab::SetPlayerSessionId(const FUniqueNetId& PlayerId, const FGuid& PlayerSessionId)
{
	
}
