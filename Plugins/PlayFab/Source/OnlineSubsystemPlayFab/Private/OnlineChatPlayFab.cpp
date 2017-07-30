// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineChatPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "OnlineIdentityInterface.h"
#include "OnlinePresenceInterface.h"
#include "PlayFab.h"
#include "Xmpp.h"


FChatRoomInfoPlayFab::FChatRoomInfoPlayFab()
	: XmppRoomInfo(FXmppRoomInfo())
	, OwnerId(MakeShareable(new FUniqueNetIdString(XmppRoomInfo.OwnerId)))
{

}

FChatRoomInfoPlayFab::FChatRoomInfoPlayFab(FXmppRoomInfo InXmppRoomInfo)
	: XmppRoomInfo(InXmppRoomInfo)
	, OwnerId(MakeShareable(new FUniqueNetIdString(XmppRoomInfo.OwnerId)))
{
	RoomConfig.bPasswordRequired = XmppRoomInfo.bIsPrivate;
}

const FChatRoomId& FChatRoomInfoPlayFab::GetRoomId() const
{
	return XmppRoomInfo.Id;
}

const TSharedRef<const FUniqueNetId>& FChatRoomInfoPlayFab::GetOwnerId() const
{
	return OwnerId;
}

const FString& FChatRoomInfoPlayFab::GetSubject() const
{
	return XmppRoomInfo.Subject;
}

bool FChatRoomInfoPlayFab::IsPrivate() const
{
	return XmppRoomInfo.bIsPrivate;
}

bool FChatRoomInfoPlayFab::IsJoined() const
{
	return false;
}

const class FChatRoomConfig& FChatRoomInfoPlayFab::GetRoomConfig() const
{
	return RoomConfig;
}

FString FChatRoomInfoPlayFab::ToDebugString() const
{
	return XmppRoomInfo.ToDebugString();
}

void FChatRoomInfoPlayFab::SetChatInfo(const TSharedRef<class FJsonObject>& JsonInfo)
{
	
}


/*************/
FChatRoomMemberPlayFab::FChatRoomMemberPlayFab(FXmppChatMemberPtr InXmppMember /*= nullptr*/)
	: XmppMember(InXmppMember)
	, UserId(MakeShareable(new FUniqueNetIdString(XmppMember.IsValid() ? XmppMember->MemberJid.Id : "")))
{
	if (!XmppMember.IsValid())
	{
		XmppMember = MakeShareable(new FXmppChatMember());
	}
}

const TSharedRef<const FUniqueNetId>& FChatRoomMemberPlayFab::GetUserId() const
{
	return UserId;
}

const FString& FChatRoomMemberPlayFab::GetNickname() const
{
	return XmppMember->Nickname;
}

/*************/
FChatMessagePlayFab::FChatMessagePlayFab(TSharedPtr<FXmppChatMessage> InXmppMessage /*= nullptr*/)
	: XmppMessage(InXmppMessage)
	, UserId(MakeShareable(new FUniqueNetIdString(XmppMessage.IsValid() ? XmppMessage->FromJid.Id : "")))
{
	if (!XmppMessage.IsValid())
	{
		XmppMessage = MakeShareable(new FXmppChatMessage());
	}
}

const TSharedRef<const FUniqueNetId>& FChatMessagePlayFab::GetUserId() const
{
	return UserId;
}

const FString& FChatMessagePlayFab::GetNickname() const
{
	return Nickname;
}

const FString& FChatMessagePlayFab::GetBody() const
{
	return XmppMessage->Body;
}

const FDateTime& FChatMessagePlayFab::GetTimestamp() const
{
	return XmppMessage->Timestamp;
}

/*************/
bool FOnlineChatPlayFab::CreateRoom(const FUniqueNetId& UserId, const FChatRoomId& RoomId, const FString& Nickname, const FChatRoomConfig& ChatRoomConfig)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return false;
	}
	FXmppRoomConfig XmppRoomConfig;
	XmppRoomConfig.bIsPersistent = false;
	XmppRoomConfig.bIsPrivate = ChatRoomConfig.bPasswordRequired;
	XmppRoomConfig.Password = ChatRoomConfig.Password;
	return XmppConnection->MultiUserChat()->CreateRoom(RoomId, Nickname, XmppRoomConfig);
}

bool FOnlineChatPlayFab::ConfigureRoom(const FUniqueNetId& UserId, const FChatRoomId& RoomId, const FChatRoomConfig& ChatRoomConfig)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return false;
	}
	FXmppRoomConfig XmppRoomConfig;
	XmppRoomConfig.bIsPersistent = false;
	XmppRoomConfig.bIsPrivate = ChatRoomConfig.bPasswordRequired;
	XmppRoomConfig.Password = ChatRoomConfig.Password;
	return XmppConnection->MultiUserChat()->ConfigureRoom(RoomId, XmppRoomConfig);
}

bool FOnlineChatPlayFab::JoinPublicRoom(const FUniqueNetId& UserId, const FChatRoomId& RoomId, const FString& Nickname, const FChatRoomConfig& ChatRoomConfig)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return false;
	}
	return XmppConnection->MultiUserChat()->JoinPublicRoom(RoomId, Nickname);
}

bool FOnlineChatPlayFab::JoinPrivateRoom(const FUniqueNetId& UserId, const FChatRoomId& RoomId, const FString& Nickname, const FChatRoomConfig& ChatRoomConfig)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return false;
	}
	return XmppConnection->MultiUserChat()->JoinPrivateRoom(RoomId, Nickname, ChatRoomConfig.Password);
}

bool FOnlineChatPlayFab::ExitRoom(const FUniqueNetId& UserId, const FChatRoomId& RoomId)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return false;
	}
	return XmppConnection->MultiUserChat()->ExitRoom(RoomId);
}

bool FOnlineChatPlayFab::SendRoomChat(const FUniqueNetId& UserId, const FChatRoomId& RoomId, const FString& MsgBody)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return false;
	}
	return XmppConnection->MultiUserChat()->SendChat(RoomId, MsgBody, "");
}

bool FOnlineChatPlayFab::SendPrivateChat(const FUniqueNetId& UserId, const FUniqueNetId& RecipientId, const FString& MsgBody)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return false;
	}
	FXmppChatMessage Message;
	Message.Body = MsgBody;
	Message.FromJid = XmppConnection->GetUserJid();
	Message.ToJid = FXmppUserJid(RecipientId.ToString());
	Message.Timestamp = FDateTime::Now();
	return XmppConnection->PrivateChat()->SendChat(UserId.ToString(), Message);
}

bool FOnlineChatPlayFab::IsChatAllowed(const FUniqueNetId& UserId, const FUniqueNetId& RecipientId) const
{
	/*TSharedPtr<FOnlineUserPresence> Presence;
	if (PlayFabSubsystem->GetPresenceInterface()->GetCachedPresence(RecipientId, Presence) == EOnlineCachedResult::Success)
	{
		
	}*/
	return true; // Dunno what else to do yet.
}

void FOnlineChatPlayFab::GetJoinedRooms(const FUniqueNetId& UserId, TArray<FChatRoomId>& OutRooms)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return;
	}
	TArray<FString> OutXmppRooms;
	XmppConnection->MultiUserChat()->GetJoinedRooms(OutXmppRooms);
	for (FString XmppRoomId : OutXmppRooms)
	{
		OutRooms.Add(FChatRoomId(XmppRoomId));
	}
}

TSharedPtr<FChatRoomInfo> FOnlineChatPlayFab::GetRoomInfo(const FUniqueNetId& UserId, const FChatRoomId& RoomId)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return nullptr;
	}
	FXmppRoomInfo XmppRoomInfo;
	XmppConnection->MultiUserChat()->GetRoomInfo(RoomId, XmppRoomInfo);

	return MakeShareable(new FChatRoomInfoPlayFab(XmppRoomInfo));
}

bool FOnlineChatPlayFab::GetMembers(const FUniqueNetId& UserId, const FChatRoomId& RoomId, TArray<TSharedRef<FChatRoomMember>>& OutMembers)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return false;
	}
	TArray<FXmppChatMemberRef> OutXmppMembers;
	bool bReturn = XmppConnection->MultiUserChat()->GetMembers(RoomId, OutXmppMembers);
	for (FXmppChatMemberRef XmppMember : OutXmppMembers)
	{
		OutMembers.Add(MakeShareable(new FChatRoomMemberPlayFab(XmppMember)));
	}
	return bReturn;
}

TSharedPtr<FChatRoomMember> FOnlineChatPlayFab::GetMember(const FUniqueNetId& UserId, const FChatRoomId& RoomId, const FUniqueNetId& MemberId)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return nullptr;
	}
	FXmppUserJid XmppMember(MemberId.ToString());
	FXmppChatMemberPtr XmppChatMember = XmppConnection->MultiUserChat()->GetMember(RoomId, XmppMember);
	return MakeShareable(new FChatRoomMemberPlayFab(XmppChatMember));
}

bool FOnlineChatPlayFab::GetLastMessages(const FUniqueNetId& UserId, const FChatRoomId& RoomId, int32 NumMessages, TArray<TSharedRef<FChatMessage>>& OutMessages)
{
	TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return false;
	}
	TArray<TSharedRef<FXmppChatMessage>> OutXmppMessages;
	bool bReturn = XmppConnection->MultiUserChat()->GetLastMessages(RoomId, NumMessages, OutXmppMessages);
	for (TSharedRef<FXmppChatMessage> XmppMessage : OutXmppMessages)
	{
		OutMessages.Add(MakeShareable(new FChatMessagePlayFab(XmppMessage)));
	}
	return bReturn;
}

void FOnlineChatPlayFab::DumpChatState() const
{
	/*TSharedPtr<IXmppConnection> XmppConnection = FXmppModule::Get().GetConnection(UserId.ToString());
	if (!XmppConnection.IsValid())
	{
		return;
	}
	XmppConnection->MultiUserChat()->DumpMultiUserChatState();*/
}

