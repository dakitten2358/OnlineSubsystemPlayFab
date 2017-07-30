// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlinePresencePlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "Xmpp.h"


void FOnlinePresencePlayFab::SetPresence(const FUniqueNetId& UserId, const FOnlineUserPresenceStatus& Status, const FOnPresenceTaskCompleteDelegate& Delegate /*= FOnPresenceTaskCompleteDelegate()*/)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		Delegate.Execute(UserId, false);
		return;
	}
	FXmppUserPresence XmppPresence = XmppConnection->Presence()->GetPresence();
	//Status.
	switch (Status.State)
	{
	case EOnlinePresenceState::Away:
		XmppPresence.Status = EXmppPresenceStatus::Away;
		break;
	case EOnlinePresenceState::Chat:
		XmppPresence.Status = EXmppPresenceStatus::Chat;
		break;
	case EOnlinePresenceState::DoNotDisturb:
		XmppPresence.Status = EXmppPresenceStatus::DoNotDisturb;
		break;
	case EOnlinePresenceState::ExtendedAway:
		XmppPresence.Status = EXmppPresenceStatus::ExtendedAway;
		break;
	case EOnlinePresenceState::Offline:
		XmppPresence.Status = EXmppPresenceStatus::Offline;
		break;
	case EOnlinePresenceState::Online:
		XmppPresence.Status = EXmppPresenceStatus::Online;
		break;
	default:
		XmppPresence.Status = EXmppPresenceStatus::Online;
		break;
	}
	XmppConnection->Presence()->UpdatePresence(XmppPresence);
}

void FOnlinePresencePlayFab::QueryPresence(const FUniqueNetId& UserId, const FOnPresenceTaskCompleteDelegate& Delegate /*= FOnPresenceTaskCompleteDelegate()*/)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		Delegate.Execute(UserId, false);
		return;
	}
	XmppConnection->Presence()->QueryPresence(UserId.ToString());
	Delegate.Execute(UserId, false); // Jingle XMPP query presence not implemented
}

EOnlineCachedResult::Type FOnlinePresencePlayFab::GetCachedPresence(const FUniqueNetId& UserId, TSharedPtr<FOnlineUserPresence>& OutPresence)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return EOnlineCachedResult::NotFound;
	}
	OutPresence = MakeShareable(new FOnlineUserPresence());
	const FXmppUserPresence XmppPresence = XmppConnection->Presence()->GetPresence();
	OutPresence->bIsOnline = true;
	switch (XmppPresence.Status)
	{
	case EXmppPresenceStatus::Away:
		OutPresence->Status.State = EOnlinePresenceState::Away;
		break;
	case EXmppPresenceStatus::Chat:
		OutPresence->Status.State = EOnlinePresenceState::Chat;
		break;
	case EXmppPresenceStatus::DoNotDisturb:
		OutPresence->Status.State = EOnlinePresenceState::DoNotDisturb;
		break;
	case EXmppPresenceStatus::ExtendedAway:
		OutPresence->Status.State = EOnlinePresenceState::ExtendedAway;
		break;
	case EXmppPresenceStatus::Offline:
		OutPresence->Status.State = EOnlinePresenceState::Offline;
		OutPresence->bIsOnline = false;
		break;
	case EXmppPresenceStatus::Online:
		OutPresence->Status.State = EOnlinePresenceState::Online;
		break;
	default:
		OutPresence->Status.State = EOnlinePresenceState::Online;
		break;
	}
	if (PlayFabSubsystem->GetAppId() == XmppPresence.AppId)
	{
		OutPresence->bIsPlayingThisGame = true;
	}
	return EOnlineCachedResult::Success;
}

EOnlineCachedResult::Type FOnlinePresencePlayFab::GetCachedPresenceForApp(const FUniqueNetId& LocalUserId, const FUniqueNetId& UserId, const FString& AppId, TSharedPtr<FOnlineUserPresence>& OutPresence)
{
	return EOnlineCachedResult::NotFound;
}

