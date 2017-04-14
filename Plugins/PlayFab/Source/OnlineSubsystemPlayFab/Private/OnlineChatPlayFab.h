// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
 
#include "OnlineChatInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "XmppMultiUserChat.h"


/**
* Info for a joined/created chat room
*/
class FChatRoomInfoPlayFab : public FChatRoomInfo
{
private:
	FXmppRoomInfo XmppRoomInfo;
	FXmppRoomConfig XmppRoomConfig;
	FChatRoomConfig RoomConfig;

	TSharedRef<const FUniqueNetId> OwnerId;

public:
	FChatRoomInfoPlayFab();
	FChatRoomInfoPlayFab(FXmppRoomInfo InXmppRoomInfo);

	virtual ~FChatRoomInfoPlayFab() {}

	virtual const FChatRoomId& GetRoomId() const override;
	virtual const TSharedRef<const FUniqueNetId>& GetOwnerId() const override;
	virtual const FString& GetSubject() const override;
	virtual bool IsPrivate() const override;
	virtual bool IsJoined() const override;
	virtual const class FChatRoomConfig& GetRoomConfig() const override;
	virtual FString ToDebugString() const override;
	virtual void SetChatInfo(const TSharedRef<class FJsonObject>& JsonInfo) override;
};

/**
* Member of a chat room
*/
class FChatRoomMemberPlayFab : public FChatRoomMember
{
private:
	FXmppChatMemberPtr XmppMember;

	TSharedRef<const FUniqueNetId> UserId;

public:
	FChatRoomMemberPlayFab(FXmppChatMemberPtr InXmppMember = nullptr);

	virtual ~FChatRoomMemberPlayFab() {}

	virtual const TSharedRef<const FUniqueNetId>& GetUserId() const override;
	virtual const FString& GetNickname() const override;
};

/**
* Chat message received from user/room
*/
class FChatMessagePlayFab : public FChatMessage
{
private:
	TSharedPtr<FXmppChatMessage> XmppMessage;

	TSharedRef<const FUniqueNetId> UserId;
	FString Nickname;

public:
	FChatMessagePlayFab(TSharedPtr<FXmppChatMessage> InXmppMessage = nullptr);

	virtual ~FChatMessagePlayFab() {}

	virtual const TSharedRef<const FUniqueNetId>& GetUserId() const override;
	virtual const FString& GetNickname() const override;
	virtual const FString& GetBody() const override;
	virtual const FDateTime& GetTimestamp() const override;
};

/**
* Interface class for user-user and user-room chat
*/
class FOnlineChatPlayFab : public IOnlineChat
{
private:

	/** Reference to the main PlayFab subsystem */
	class FOnlineSubsystemPlayFab* PlayFabSubsystem;

	/** Hidden on purpose */
	FOnlineChatPlayFab() :
		PlayFabSubsystem(NULL)
	{
	}

PACKAGE_SCOPE:

	FOnlineChatPlayFab(class FOnlineSubsystemPlayFab* InPlayFabSubsystem) :
		PlayFabSubsystem(InPlayFabSubsystem)
	{
	}

public:

	virtual ~FOnlineChatPlayFab()
	{
	}

	// IOnlineChat
	virtual bool CreateRoom(const FUniqueNetId& UserId, const FChatRoomId& RoomId, const FString& Nickname, const FChatRoomConfig& ChatRoomConfig) override;
	virtual bool ConfigureRoom(const FUniqueNetId& UserId, const FChatRoomId& RoomId, const FChatRoomConfig& ChatRoomConfig) override;
	virtual bool JoinPublicRoom(const FUniqueNetId& UserId, const FChatRoomId& RoomId, const FString& Nickname, const FChatRoomConfig& ChatRoomConfig) override;
	virtual bool JoinPrivateRoom(const FUniqueNetId& UserId, const FChatRoomId& RoomId, const FString& Nickname, const FChatRoomConfig& ChatRoomConfig) override;
	virtual bool ExitRoom(const FUniqueNetId& UserId, const FChatRoomId& RoomId) override;
	virtual bool SendRoomChat(const FUniqueNetId& UserId, const FChatRoomId& RoomId, const FString& MsgBody) override;
	virtual bool SendPrivateChat(const FUniqueNetId& UserId, const FUniqueNetId& RecipientId, const FString& MsgBody) override;
	virtual bool IsChatAllowed(const FUniqueNetId& UserId, const FUniqueNetId& RecipientId) const override;
	virtual void GetJoinedRooms(const FUniqueNetId& UserId, TArray<FChatRoomId>& OutRooms) override;
	virtual TSharedPtr<FChatRoomInfo> GetRoomInfo(const FUniqueNetId& UserId, const FChatRoomId& RoomId) override;
	virtual bool GetMembers(const FUniqueNetId& UserId, const FChatRoomId& RoomId, TArray<TSharedRef<FChatRoomMember>>& OutMembers) override;
	virtual TSharedPtr<FChatRoomMember> GetMember(const FUniqueNetId& UserId, const FChatRoomId& RoomId, const FUniqueNetId& MemberId) override;
	virtual bool GetLastMessages(const FUniqueNetId& UserId, const FChatRoomId& RoomId, int32 NumMessages, TArray<TSharedRef<FChatMessage>>& OutMessages) override;
	virtual void DumpChatState() const override;
};

typedef TSharedPtr<FOnlineChatPlayFab, ESPMode::ThreadSafe> FOnlineChatPlayFabPtr;