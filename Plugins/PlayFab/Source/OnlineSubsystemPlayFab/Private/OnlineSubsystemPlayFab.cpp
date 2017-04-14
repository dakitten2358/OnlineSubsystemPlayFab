// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlineSubsystemPlayFab.h"
#include "OnlineSubsystemPlayFabModule.h"

// Interfaces
#include "OnlineAchievementsPlayFab.h"
#include "OnlineEntitlementsPlayFab.h"
#include "OnlineEventsPlayFab.h"
#include "OnlineExternalUIPlayFab.h"
#include "OnlineFriendsPlayFab.h"
#include "OnlineGroupsPlayFab.h"
#include "OnlineIdentityPlayFab.h"
#include "OnlineLeaderboardPlayFab.h"
#include "OnlinePresencePlayFab.h"
#include "OnlineSessionPlayFab.h"
#include "OnlineSharingPlayFab.h"
#include "OnlineStorePlayFab.h"
#include "OnlineTimePlayFab.h"
#include "OnlineUserPlayFab.h"

// PlayFab
#include "PlayFab.h"
#include "Core/PlayFabMatchmakerAPI.h"
#include "Core/PlayFabServerAPI.h"
#include "Core/PlayFabSettings.h"


// ============================
// ===== Start Interfaces =====
// ============================

IOnlineAchievementsPtr FOnlineSubsystemPlayFab::GetAchievementsInterface() const
{
	return AchievementsInterface;
}

IOnlineChatPtr FOnlineSubsystemPlayFab::GetChatInterface() const
{
	return nullptr;
}

IOnlineEntitlementsPtr FOnlineSubsystemPlayFab::GetEntitlementsInterface() const
{
	return EntitlementsInterface;
}

IOnlineEventsPtr FOnlineSubsystemPlayFab::GetEventsInterface() const
{
	return EventsInterface;
}

IOnlineExternalUIPtr FOnlineSubsystemPlayFab::GetExternalUIInterface() const
{
	return ExternalUIInterface;
}

IOnlineFriendsPtr FOnlineSubsystemPlayFab::GetFriendsInterface() const
{
	return FriendsInterface;
}

IOnlineGroupsPtr FOnlineSubsystemPlayFab::GetGroupsInterface() const
{
	return GroupsInterface;
}

IOnlineIdentityPtr FOnlineSubsystemPlayFab::GetIdentityInterface() const
{
	return IdentityInterface;
}

IOnlineLeaderboardsPtr FOnlineSubsystemPlayFab::GetLeaderboardsInterface() const
{
	return LeaderboardsInterface;
}

IOnlineMessagePtr FOnlineSubsystemPlayFab::GetMessageInterface() const
{
	return nullptr;
}

IOnlinePartyPtr FOnlineSubsystemPlayFab::GetPartyInterface() const
{
	return nullptr;
}

IOnlinePresencePtr FOnlineSubsystemPlayFab::GetPresenceInterface() const
{
	return PresenceInterface;
}

IOnlinePurchasePtr FOnlineSubsystemPlayFab::GetPurchaseInterface() const
{
	return nullptr;
}

IOnlineSessionPtr FOnlineSubsystemPlayFab::GetSessionInterface() const
{
	return SessionInterface;
}

IOnlineSharedCloudPtr FOnlineSubsystemPlayFab::GetSharedCloudInterface() const
{
	return nullptr;
}

IOnlineSharingPtr FOnlineSubsystemPlayFab::GetSharingInterface() const
{
	return SharingInterface;
}

IOnlineStorePtr FOnlineSubsystemPlayFab::GetStoreInterface() const
{
	return StoreInterface;
}

IOnlineStoreV2Ptr FOnlineSubsystemPlayFab::GetStoreV2Interface() const
{
	return nullptr;
}

IOnlineTimePtr FOnlineSubsystemPlayFab::GetTimeInterface() const
{
	return TimeInterface;
}

IOnlineTitleFilePtr FOnlineSubsystemPlayFab::GetTitleFileInterface() const
{
	return nullptr;
}

IOnlineTurnBasedPtr FOnlineSubsystemPlayFab::GetTurnBasedInterface() const
{
	return nullptr;
}

IOnlineUserCloudPtr FOnlineSubsystemPlayFab::GetUserCloudInterface() const
{
	return nullptr;
}

IOnlineUserPtr FOnlineSubsystemPlayFab::GetUserInterface() const
{
	return UserInterface;
}

IOnlineVoicePtr FOnlineSubsystemPlayFab::GetVoiceInterface() const
{
	return nullptr;
}

// ==========================
// ===== End Interfaces =====
// ==========================

bool FOnlineSubsystemPlayFab::Tick(float DeltaTime)
{
	if (!FOnlineSubsystemImpl::Tick(DeltaTime))
	{
		return false;
	}

	return true;
}

bool FOnlineSubsystemPlayFab::Init()
{
	AchievementsInterface = MakeShareable(new FOnlineAchievementsPlayFab(this));
	EntitlementsInterface = MakeShareable(new FOnlineEntitlementsPlayFab(this));
	EventsInterface = MakeShareable(new FOnlineEventsPlayFab(this));
	ExternalUIInterface = MakeShareable(new FOnlineExternalUIPlayFab(this));
	FriendsInterface = MakeShareable(new FOnlineFriendsPlayFab(this));
	GroupsInterface = MakeShareable(new FOnlineGroupsPlayFab(this));
	IdentityInterface = MakeShareable(new FOnlineIdentityPlayFab(this));
	LeaderboardsInterface = MakeShareable(new FOnlineLeaderboardsPlayFab(this));
	PresenceInterface = MakeShareable(new FOnlinePresencePlayFab(this));
	SessionInterface = MakeShareable(new FOnlineSessionPlayFab(this));
	SharingInterface = MakeShareable(new FOnlineSharingPlayFab(this));
	StoreInterface = MakeShareable(new FOnlineStorePlayFab(this));
	TimeInterface = MakeShareable(new FOnlineTimePlayFab(this));
	UserInterface = MakeShareable(new FOnlineUserPlayFab(this));

	FString cmdVal;
	if (FParse::Value(FCommandLine::Get(), TEXT("title_secret_key"), cmdVal)) {
		//cmdVal = cmdVal.Replace(TEXT("="), TEXT(""));
		PlayFabServerPtr ServerAPI = IPlayFabModuleInterface::Get().GetServerAPI();
		if (ServerAPI.IsValid())
		{
			ServerAPI->SetDevSecretKey(cmdVal);
		}
	}

	return true;
}

bool FOnlineSubsystemPlayFab::Shutdown()
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSubsystemPlayFab::Shutdown()"));

	FOnlineSubsystemImpl::Shutdown();

	AchievementsInterface = nullptr;
	EntitlementsInterface = nullptr;
	EventsInterface = nullptr;
	ExternalUIInterface = nullptr;
	FriendsInterface = nullptr;
	GroupsInterface = nullptr;
	IdentityInterface = nullptr;
	LeaderboardsInterface = nullptr;
	PresenceInterface = nullptr;
	SessionInterface = nullptr;
	SharingInterface = nullptr;
	StoreInterface = nullptr;
	TimeInterface = nullptr;
	UserInterface = nullptr;

	return true;
}

FString FOnlineSubsystemPlayFab::GetAppId() const
{
	//return PlayFab::PlayFabSettings::titleId;
	return TEXT("");
}

bool FOnlineSubsystemPlayFab::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FOnlineSubsystemImpl::Exec(InWorld, Cmd, Ar))
	{
		return true;
	}
	return false;
}

FString FOnlineSubsystemPlayFab::GetBuildVersion() const
{
	FString ProjectVersion;
	GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), ProjectVersion, GGameIni);
	return ProjectVersion;
}

bool FOnlineSubsystemPlayFab::IsEnabled()
{
	// Check the ini for disabling PlayFab
	bool bEnablePlayFab = false;
	GConfig->GetBool(TEXT("OnlineSubsystemPlayFab"), TEXT("bEnable"), bEnablePlayFab, GEngineIni);

#if !UE_BUILD_SHIPPING
	// Check the commandline for disabling PlayFab
	bEnablePlayFab = bEnablePlayFab && !FParse::Param(FCommandLine::Get(),TEXT("NOPLAYFAB"));
#endif

	return bEnablePlayFab;
}

bool FOnlineSubsystemPlayFab::IsXmppEnabled()
{
	// Check the ini for disabling PlayFab
	bool bEnableXmpp = false;
	GConfig->GetBool(TEXT("OnlineSubsystemPlayFab"), TEXT("bEnableXmpp"), bEnableXmpp, GEngineIni);

#if !UE_BUILD_SHIPPING
	// Check the commandline for disabling Xmpp
	bEnableXmpp = bEnableXmpp && !FParse::Param(FCommandLine::Get(), TEXT("NOXMPP"));
#endif

	return bEnableXmpp;
}

