// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFab.h"

#include "HAL/RunnableThread.h"
#include "OnlineAsyncTaskManagerPlayFab.h"

// Interfaces
#include "OnlineAchievementsPlayFab.h"
#include "OnlineChatPlayFab.h"
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


#define LOCTEXT_NAMESPACE "FOnlineSubsystemPlayFab" 

// ============================
// ===== Start Interfaces =====
// ============================

IOnlineAchievementsPtr FOnlineSubsystemPlayFab::GetAchievementsInterface() const
{
	return AchievementsInterface;
}

IOnlineChatPtr FOnlineSubsystemPlayFab::GetChatInterface() const
{
	return ChatInterface;
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

void FOnlineSubsystemPlayFab::QueueAsyncTask(class FOnlineAsyncTask* AsyncTask)
{
	check(OnlineAsyncTaskThreadRunnable);
	OnlineAsyncTaskThreadRunnable->AddToInQueue(AsyncTask);
}

bool FOnlineSubsystemPlayFab::Tick(float DeltaTime)
{
	if (!FOnlineSubsystemImpl::Tick(DeltaTime))
	{
		return false;
	}

	if (OnlineAsyncTaskThreadRunnable)
	{
		OnlineAsyncTaskThreadRunnable->GameTick();
	}

	if (SessionInterface.IsValid())
	{
		SessionInterface->Tick(DeltaTime);
	}

	return true;
}

PlayFabServerPtr FOnlineSubsystemPlayFab::GetServerAPI()
{
	PlayFabServerPtr ServerAPI = IPlayFabModuleInterface::Get().GetServerAPI();
	if (ServerAPI.IsValid())
	{
		return ServerAPI;
	}

	return nullptr;
}

PlayFabClientPtr FOnlineSubsystemPlayFab::GetClientAPI()
{
	// Try to find a logged in client
	for (auto& Elem : PlayFabClientPtrs)
	{
		PlayFabClientPtr ClientAPI = Elem.Value;
		if (ClientAPI->IsClientLoggedIn())
		{
			return ClientAPI;
		}
	}

	// Return a valid client no matter what since we assume that all GetClientAPI returns are valid
	return GetClientAPI(0);
}

PlayFabClientPtr FOnlineSubsystemPlayFab::GetClientAPI(int32 LocalUserNum)
{
	PlayFabClientPtr* ClientAPI = PlayFabClientPtrs.Find(LocalUserNum);
	PlayFabClientPtr Result;
	if (ClientAPI)
	{
		Result = *ClientAPI;
	}
	else
	{
		Result = MakeShareable(new PlayFab::UPlayFabClientAPI());
		PlayFabClientPtrs.Add(LocalUserNum, Result);
	}

	return Result;
}

PlayFabClientPtr FOnlineSubsystemPlayFab::GetClientAPI(const FUniqueNetId& UserId)
{
	return GetClientAPI(IdentityInterface->GetPlatformUserIdFromUniqueNetId(UserId));
}

FOnlineSubsystemPlayFab* FOnlineSubsystemPlayFab::GetPlayFabSubsystem(IOnlineSubsystem* Subsystem)
{
	return static_cast<FOnlineSubsystemPlayFab*>(Subsystem);
}

bool FOnlineSubsystemPlayFab::Init()
{
	// Create the online async task thread
	OnlineAsyncTaskThreadRunnable = new FOnlineAsyncTaskManagerPlayFab(this);
	check(OnlineAsyncTaskThreadRunnable);
	OnlineAsyncTaskThread = FRunnableThread::Create(OnlineAsyncTaskThreadRunnable, *FString::Printf(TEXT("OnlineAsyncTaskThreadPlayFab %s"), *InstanceName.ToString()), 128 * 1024, TPri_Normal);
	check(OnlineAsyncTaskThread);
	UE_LOG_ONLINE(Verbose, TEXT("Created thread (ID:%d)."), OnlineAsyncTaskThread->GetThreadID());

	// Create the interfaces
	AchievementsInterface = MakeShareable(new FOnlineAchievementsPlayFab(this));
	ChatInterface = MakeShareable(new FOnlineChatPlayFab(this));
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
		UE_LOG_ONLINE(Verbose, TEXT("Secret key provided by command line: %s"), *cmdVal);
		PlayFabServerPtr ServerAPI = GetServerAPI();
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

	if (OnlineAsyncTaskThread)
	{
		// Destroy the online async task thread
		delete OnlineAsyncTaskThread;
		OnlineAsyncTaskThread = nullptr;
	}

	if (OnlineAsyncTaskThreadRunnable)
	{
		delete OnlineAsyncTaskThreadRunnable;
		OnlineAsyncTaskThreadRunnable = nullptr;
	}

	AchievementsInterface = nullptr;
	ChatInterface = nullptr;
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
	return IPlayFabModuleInterface::Get().GetTitleId();
}

FText FOnlineSubsystemPlayFab::GetOnlineServiceName() const
{
	return LOCTEXT("PlayFab", "PlayFab");
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
	// If game_build_version is provided, return that(PlayFab servers build id)
	FString cmdVal;
	if (FParse::Value(FCommandLine::Get(), TEXT("game_build_version"), cmdVal)) {
		return cmdVal;
	}

	// Otherwise return the ue4 project version
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

#undef LOCTEXT_NAMESPACE

