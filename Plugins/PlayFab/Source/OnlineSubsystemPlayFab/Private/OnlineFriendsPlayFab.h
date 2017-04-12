// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
 
#include "OnlineFriendsInterface.h"
#include "OnlinePresenceInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Core/PlayFabServerAPI.h"
#include "Core/PlayFabServerDataModels.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"

class FOnlineFriendPlayFab :
	public FOnlineFriend
{
public:

	// FOnlineUser

	virtual TSharedRef<const FUniqueNetId> GetUserId() const override;
	virtual FString GetRealName() const override;
	virtual FString GetDisplayName(const FString& Platform = FString()) const override;
	virtual bool GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const override;

	// FOnlineFriend

	virtual EInviteStatus::Type GetInviteStatus() const override;
	virtual const FOnlineUserPresence& GetPresence() const override;

	// FOnlineFriendMcp

	/**
	* Init/default constructor
	*/
	FOnlineFriendPlayFab(const FString& InUserId = TEXT(""))
		: UserIdPtr(new FUniqueNetIdString(InUserId))
		, UserId(InUserId)
	{}
	//FOnlineFriendSteam(const CSteamID& InUserId = CSteamID());

	/** User Id represented as a FUniqueNetId */
	TSharedRef<const FUniqueNetId> UserIdPtr;
	/** Id associated with the user account provided by the online service */
	FString UserId;
	/** Additional key/value pair data related to user attribution */
	TMap<FString, FString> UserAttributes;
	/** @temp presence info */
	FOnlineUserPresence Presence;

	FString DisplayName;
};

/**
* Interface definition for the online services friends services
* Friends services are anything related to the maintenance of friends and friends lists
*/
class FOnlineFriendsPlayFab : public IOnlineFriends
{
private:

	/** Reference to the main PlayFab subsystem */
	class FOnlineSubsystemPlayFab* PlayFabSubsystem;

	/** Hidden on purpose */
	FOnlineFriendsPlayFab() :
		PlayFabSubsystem(NULL)
	{
	}

	/** map of local user idx to friends */
	TMap<int32, TArray<TSharedRef<FOnlineFriendPlayFab>>> FriendsLists;

PACKAGE_SCOPE:

	FOnlineFriendsPlayFab(class FOnlineSubsystemPlayFab* InPlayFabSubsystem) :
		PlayFabSubsystem(InPlayFabSubsystem)
	{
	}

public:

	virtual ~FOnlineFriendsPlayFab()
	{
	}

	// IOnlineFriends
	virtual bool ReadFriendsList(int32 LocalUserNum, const FString& ListName, const FOnReadFriendsListComplete& Delegate = FOnReadFriendsListComplete()) override;
	virtual bool DeleteFriendsList(int32 LocalUserNum, const FString& ListName, const FOnDeleteFriendsListComplete& Delegate = FOnDeleteFriendsListComplete()) override;
	virtual bool SendInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnSendInviteComplete& Delegate = FOnSendInviteComplete()) override;
	virtual bool AcceptInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnAcceptInviteComplete& Delegate = FOnAcceptInviteComplete()) override;
	virtual bool RejectInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) override;
	virtual bool DeleteFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) override;
	virtual bool GetFriendsList(int32 LocalUserNum, const FString& ListName, TArray<TSharedRef<FOnlineFriend>>& OutFriends) override;
	virtual TSharedPtr<FOnlineFriend> GetFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) override;
	virtual bool IsFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) override;
	virtual bool QueryRecentPlayers(const FUniqueNetId& UserId, const FString& Namespace) override;
	virtual bool GetRecentPlayers(const FUniqueNetId& UserId, const FString& Namespace, TArray<TSharedRef<FOnlineRecentPlayer>>& OutRecentPlayers) override;
	virtual bool BlockPlayer(int32 LocalUserNum, const FUniqueNetId& PlayerId) override;
	virtual bool UnblockPlayer(int32 LocalUserNum, const FUniqueNetId& PlayerId) override;
	virtual bool QueryBlockedPlayers(const FUniqueNetId& UserId) override;
	virtual bool GetBlockedPlayers(const FUniqueNetId& UserId, TArray<TSharedRef<FOnlineBlockedPlayer>>& OutBlockedPlayers) override;
	virtual void DumpBlockedPlayers() const override;

private:
	void OnSuccessCallback_Server_GetFriendsList(const PlayFab::ServerModels::FGetFriendsListResult& Result, int32 LocalUserNum, const FString* ListName, const FOnReadFriendsListComplete* Delegate);
	void OnSuccessCallback_Client_GetFriendsList(const PlayFab::ClientModels::FGetFriendsListResult& Result, int32 LocalUserNum, const FString* ListName, const FOnReadFriendsListComplete* Delegate);
	void OnErrorCallback_GetFriendsList(const PlayFab::FPlayFabError& ErrorResult, int32 LocalUserNum, const FString* ListName, const FOnReadFriendsListComplete* Delegate);
};

typedef TSharedPtr<FOnlineFriendsPlayFab, ESPMode::ThreadSafe> FOnlineFriendsPlayFabPtr;