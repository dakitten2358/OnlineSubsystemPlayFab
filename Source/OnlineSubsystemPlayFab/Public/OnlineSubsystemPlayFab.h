// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystem.h"
#include "OnlineSubsystemImpl.h"
#include "OnlineSubsystemPlayFabModule.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "PlayFab.h"

#ifndef PLAYFAB_SUBSYSTEM
#define PLAYFAB_SUBSYSTEM	FName(TEXT("PLAYFAB"))
#endif

/** Forward declarations of all interface classes */
typedef TSharedPtr<class FOnlineAchievementsPlayFab, ESPMode::ThreadSafe> FOnlineAchievementsPlayFabPtr;
typedef TSharedPtr<class FOnlineChatPlayFab, ESPMode::ThreadSafe> FOnlineChatPlayFabPtr;
typedef TSharedPtr<class FOnlineEntitlementsPlayFab, ESPMode::ThreadSafe> FOnlineEntitlementsPlayFabPtr;
typedef TSharedPtr<class FOnlineEventsPlayFab, ESPMode::ThreadSafe> FOnlineEventsPlayFabPtr;
typedef TSharedPtr<class FOnlineExternalUIPlayFab, ESPMode::ThreadSafe> FOnlineExternalUIPlayFabPtr;
typedef TSharedPtr<class FOnlineFriendsPlayFab, ESPMode::ThreadSafe> FOnlineFriendsPlayFabPtr;
typedef TSharedPtr<class FOnlineGroupsPlayFab, ESPMode::ThreadSafe> FOnlineGroupsPlayFabPtr;
typedef TSharedPtr<class FOnlineIdentityPlayFab, ESPMode::ThreadSafe> FOnlineIdentityPlayFabPtr;
typedef TSharedPtr<class FOnlineLeaderboardsPlayFab, ESPMode::ThreadSafe> FOnlineLeaderboardsPlayFabPtr;
typedef TSharedPtr<class FOnlinePresencePlayFab, ESPMode::ThreadSafe> FOnlinePresencePlayFabPtr;
typedef TSharedPtr<class FOnlineSessionPlayFab, ESPMode::ThreadSafe> FOnlineSessionPlayFabPtr;
typedef TSharedPtr<class FOnlineSharingPlayFab, ESPMode::ThreadSafe> FOnlineSharingPlayFabPtr;
typedef TSharedPtr<class FOnlineStorePlayFab, ESPMode::ThreadSafe> FOnlineStorePlayFabPtr;
typedef TSharedPtr<class FOnlineTimePlayFab, ESPMode::ThreadSafe> FOnlineTimePlayFabPtr;
typedef TSharedPtr<class FOnlineUserPlayFab, ESPMode::ThreadSafe> FOnlineUserPlayFabPtr;

/**
*	OnlineSubsystemPlayFab- Implementation of the online subsystem for PlayFab services
*/
class ONLINESUBSYSTEMPLAYFAB_API FOnlineSubsystemPlayFab : public FOnlineSubsystemImpl
{
	class FOnlineFactoryPlayFab* PlayFabFactory;
	
public:
    // IOnlineSubsystem

	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
	virtual IOnlineChatPtr GetChatInterface() const override;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
	virtual IOnlineEventsPtr GetEventsInterface() const override;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlineGroupsPtr GetGroupsInterface() const override;
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
	virtual IOnlineMessagePtr GetMessageInterface() const override;
	virtual IOnlinePartyPtr GetPartyInterface() const override;
	virtual IOnlinePresencePtr GetPresenceInterface() const override;
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override;
	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
	virtual IOnlineSharingPtr GetSharingInterface() const override;
	virtual IOnlineStorePtr GetStoreInterface() const override;
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override;
	virtual IOnlineTimePtr GetTimeInterface() const override;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
	virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override;
    virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
	virtual IOnlineUserPtr GetUserInterface() const override;
	virtual IOnlineVoicePtr GetVoiceInterface() const override;

	void QueueAsyncTask(class FOnlineAsyncTask* AsyncTask);

	virtual bool Tick(float DeltaTime) override;

	// Performs checks to make sure the PlayFab Server API is valid and setup, otherwise returns nullptr
	// Prevent clients from calling server api calls without secret key
	PlayFabServerPtr GetServerAPI();

	PlayFabClientPtr GetClientAPI();
	PlayFabClientPtr GetClientAPI(int32 LocalUserNum);
	PlayFabClientPtr GetClientAPI(const FUniqueNetId& UserId);

	static FOnlineSubsystemPlayFab* GetPlayFabSubsystem(IOnlineSubsystem* Subsystem);

	// FOnlineSessionPlayFab accessors
	bool AuthenticatePlayer(const FUniqueNetId& PlayerId, FName SessionName, FString AuthTicket, bool bIsMatchmakeTicket);
	//FOnAuthenticatePlayerComplete GetAuthenticatePlayerDelegate();

    virtual bool Init() override;
    virtual bool Shutdown() override;
    virtual FString GetAppId() const override;
	virtual FText GetOnlineServiceName() const override;
    virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	/*
	 * Returns either game_build_version from command line or the ue4 project version from project settings
	 * Used as the PlayFab build Id for the ServerAPI
	 */
	inline FString GetBuildVersion() const;

    // FOnlineSubsystemPlayFab
	
	/**
	 * Destructor
	 */
	virtual ~FOnlineSubsystemPlayFab()
	{

	}

    /**
    * Is the PlayFab API available for use
    * @return true if PlayFab functionality is available, false otherwise
    */
    bool IsEnabled();

	bool IsXmppEnabled();

PACKAGE_SCOPE:

    /** Only the factory makes instances */
    FOnlineSubsystemPlayFab(FName InInstanceName)
        : FOnlineSubsystemImpl(PLAYFAB_SUBSYSTEM, InInstanceName)
		, AchievementsInterface(NULL)
		, EntitlementsInterface(NULL)
		, EventsInterface(NULL)
		, ExternalUIInterface(NULL)
		, FriendsInterface(NULL)
		, GroupsInterface(NULL)
		, IdentityInterface(NULL)
		, LeaderboardsInterface(NULL)
		, PresenceInterface(NULL)
		, SessionInterface(NULL)
		, SharingInterface(NULL)
		, StoreInterface(NULL)
		, TimeInterface(NULL)
		, UserInterface(NULL)
		//, VoiceInterface(NULL)
		, OnlineAsyncTaskThreadRunnable(NULL)
		, OnlineAsyncTaskThread(NULL)
    {}

    FOnlineSubsystemPlayFab()
		: AchievementsInterface(NULL)
		, EntitlementsInterface(NULL)
		, EventsInterface(NULL)
		, ExternalUIInterface(NULL)
		, FriendsInterface(NULL)
		, GroupsInterface(NULL)
		, IdentityInterface(NULL)
		, LeaderboardsInterface(NULL)
		, PresenceInterface(NULL)
		, SessionInterface(NULL)
		, SharingInterface(NULL)
		, StoreInterface(NULL)
		, TimeInterface(NULL)
		, UserInterface(NULL)
		//, VoiceInterface(NULL)
		, OnlineAsyncTaskThreadRunnable(NULL)
		, OnlineAsyncTaskThread(NULL)
    {}

private:
	/** Interface for achievements */
	FOnlineAchievementsPlayFabPtr AchievementsInterface;
	/** Interface to the chat services */
	FOnlineChatPlayFabPtr ChatInterface;
	/** Interface to the entitlements services */
	FOnlineEntitlementsPlayFabPtr EntitlementsInterface;
	/** Interface to the online events services */
	FOnlineEventsPlayFabPtr EventsInterface;
	/** Interface to the external UI services */
	FOnlineExternalUIPlayFabPtr ExternalUIInterface;
	/** Interface to the friends services */
	FOnlineFriendsPlayFabPtr FriendsInterface;
	/** Interface to the shared group data services */
	FOnlineGroupsPlayFabPtr GroupsInterface;
	/** Interface to the identity registration/auth services */
	FOnlineIdentityPlayFabPtr IdentityInterface;
	/** Interface to the leaderboard services */
	FOnlineLeaderboardsPlayFabPtr LeaderboardsInterface;
	/** Interface to the presence services */
	FOnlinePresencePlayFabPtr PresenceInterface;
	/** Interface to the session services */
	FOnlineSessionPlayFabPtr SessionInterface;
	/** Interface to the title sharing services */
	FOnlineSharingPlayFabPtr SharingInterface;
	/** Interface to the store services */
	FOnlineStorePlayFabPtr StoreInterface;
	/** Interface to the external time services */
	FOnlineTimePlayFabPtr TimeInterface;
	/** Interface to the online user services */
	FOnlineUserPlayFabPtr UserInterface;

	/** Online async task runnable */
	class FOnlineAsyncTaskManagerPlayFab* OnlineAsyncTaskThreadRunnable;
	/** Online async task thread */
	class FRunnableThread* OnlineAsyncTaskThread;

	bool bAttemptSetSecret;
	static TMap<int32, PlayFabClientPtr> PlayFabClientPtrs;
};

typedef TSharedPtr<FOnlineSubsystemPlayFab, ESPMode::ThreadSafe> FOnlineSubsystemPlayFabPtr;

