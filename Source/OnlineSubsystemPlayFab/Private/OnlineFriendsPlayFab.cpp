// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineFriendsPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "PlayFab.h"


TSharedRef<const FUniqueNetId> FOnlineFriendPlayFab::GetUserId() const
{
	return UserIdPtr;
}

FString FOnlineFriendPlayFab::GetRealName() const
{
	return TEXT("Pinocchio");
}

FString FOnlineFriendPlayFab::GetDisplayName(const FString& Platform /*= FString()*/) const
{
	return DisplayName;
}

bool FOnlineFriendPlayFab::GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* FoundAttr = UserAttributes.Find(AttrName);
	if (FoundAttr != NULL)
	{
		OutAttrValue = *FoundAttr;
		return true;
	}
	return false;
}

EInviteStatus::Type FOnlineFriendPlayFab::GetInviteStatus() const
{
	return EInviteStatus::Unknown;
}

const FOnlineUserPresence& FOnlineFriendPlayFab::GetPresence() const
{
	return Presence;
}

bool FOnlineFriendsPlayFab::ReadFriendsList(int32 LocalUserNum, const FString& ListName, const FOnReadFriendsListComplete& Delegate /*= FOnReadFriendsListComplete()*/)
{
	//PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();
	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(LocalUserNum);
	/*if (ServerAPI.IsValid())
	{
		PlayFab::ServerModels::FGetFriendsListRequest Request;
		TSharedPtr<const FUniqueNetId> UserId = PlayFabSubsystem->GetIdentityInterface()->GetUniquePlayerId(LocalUserNum);
		if (UserId.IsValid())
		{
			Request.PlayFabId = UserId->ToString();
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("Wasn't able to get identity on server"));
			return false;
		}
		Request.IncludeFacebookFriends = false;
		Request.IncludeSteamFriends = false;
		PlayFab::UPlayFabServerAPI::FGetFriendsListDelegate SuccessDelegate_GetFriendsList;
		SuccessDelegate_GetFriendsList.CreateRaw(this, &FOnlineFriendsPlayFab::OnSuccessCallback_Server_GetFriendsList, LocalUserNum, &ListName, &Delegate);
		PlayFab::FPlayFabErrorDelegate ErrorDelegate_GetFriendsList;
		ErrorDelegate_GetFriendsList.CreateRaw(this, &FOnlineFriendsPlayFab::OnErrorCallback_GetFriendsList, LocalUserNum, &ListName, &Delegate);
		ServerAPI->GetFriendsList(Request, SuccessDelegate_GetFriendsList, ErrorDelegate_GetFriendsList);
		return true;
	}
	else */if (ClientAPI.IsValid())
	{
		PlayFab::ClientModels::FGetFriendsListRequest Request;
		Request.IncludeFacebookFriends = false;
		Request.IncludeSteamFriends = false;
		auto SuccessDelegate = PlayFab::UPlayFabClientAPI::FGetFriendsListDelegate::CreateRaw(this, &FOnlineFriendsPlayFab::OnSuccessCallback_Client_GetFriendsList, LocalUserNum, ListName, Delegate);
		auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineFriendsPlayFab::OnErrorCallback_GetFriendsList, LocalUserNum, ListName, Delegate);
		ClientAPI->GetFriendsList(Request, SuccessDelegate, ErrorDelegate);
		return true;
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("PlayFab Client Interface not available"));
	}
	Delegate.ExecuteIfBound(LocalUserNum, false, ListName, TEXT("Unknwon error occured"));
	return false;
}

bool FOnlineFriendsPlayFab::DeleteFriendsList(int32 LocalUserNum, const FString& ListName, const FOnDeleteFriendsListComplete& Delegate /*= FOnDeleteFriendsListComplete()*/)
{
	Delegate.ExecuteIfBound(LocalUserNum, false, ListName, TEXT("DeleteFriendsList() is not supported"));
	return false;
}

bool FOnlineFriendsPlayFab::SendInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnSendInviteComplete& Delegate /*= FOnSendInviteComplete()*/)
{
	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(LocalUserNum);
	if (ClientAPI.IsValid())
	{
		PlayFab::ClientModels::FAddFriendRequest Request;
		Request.FriendPlayFabId = FriendId.ToString();
		ClientAPI->AddFriend(Request);
		Delegate.ExecuteIfBound(LocalUserNum, true, FriendId, ListName, TEXT(""));
		return true;
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("PlayFab Client Interface not available"));
	}
	Delegate.ExecuteIfBound(LocalUserNum, false, FriendId, ListName, TEXT("SendInvite() is not supported"));
	return false;
}

bool FOnlineFriendsPlayFab::AcceptInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnAcceptInviteComplete& Delegate /*= FOnAcceptInviteComplete()*/)
{
	Delegate.ExecuteIfBound(LocalUserNum, false, FriendId, ListName, TEXT("AcceptInvite() is not supported"));
	return false;
}

bool FOnlineFriendsPlayFab::RejectInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	TriggerOnRejectInviteCompleteDelegates(LocalUserNum, false, FriendId, ListName, TEXT("RejectInvite() is not supported"));
	return false;
}

void FOnlineFriendsPlayFab::SetFriendAlias(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FString& Alias, const FOnSetFriendAliasComplete& Delegate)
{
	TSharedRef<const FUniqueNetId> FriendIdRef = FriendId.AsShared();
	PlayFabSubsystem->ExecuteNextTick([LocalUserNum, FriendIdRef, ListName, Delegate]()
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineFriendsPlayFab::SetFriendAlias is currently not supported"));
		Delegate.ExecuteIfBound(LocalUserNum, *FriendIdRef, ListName, FOnlineError(EOnlineErrorResult::NotImplemented));
	});
}

void FOnlineFriendsPlayFab::DeleteFriendAlias(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnDeleteFriendAliasComplete& Delegate)
{
	TSharedRef<const FUniqueNetId> FriendIdRef = FriendId.AsShared();
	PlayFabSubsystem->ExecuteNextTick([LocalUserNum, FriendIdRef, ListName, Delegate]()
	{
		UE_LOG_ONLINE_FRIEND(Warning, TEXT("FOnlineFriendsPlayFab::DeleteFriendAlias is currently not supported"));
		Delegate.ExecuteIfBound(LocalUserNum, *FriendIdRef, ListName, FOnlineError(EOnlineErrorResult::NotImplemented));
	});
}

bool FOnlineFriendsPlayFab::DeleteFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(LocalUserNum);
	if (ClientAPI.IsValid())
	{
		PlayFab::ClientModels::FRemoveFriendRequest Request;
		Request.FriendPlayFabId = FriendId.ToString();
		ClientAPI->RemoveFriend(Request);
		TriggerOnDeleteFriendCompleteDelegates(LocalUserNum, true, FriendId, ListName, TEXT(""));
		return true;
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("PlayFab Client Interface not available"));
	}
	TriggerOnDeleteFriendCompleteDelegates(LocalUserNum, false, FriendId, ListName, TEXT("DeleteFriend() failed"));
	return false;
}

bool FOnlineFriendsPlayFab::GetFriendsList(int32 LocalUserNum, const FString& ListName, TArray<TSharedRef<FOnlineFriend>>& OutFriends)
{
	TArray<TSharedRef<FOnlineFriendPlayFab>>* FriendsList = FriendsLists.Find(LocalUserNum);
	if (FriendsList != NULL)
	{
		for (TSharedRef<FOnlineFriendPlayFab> Friend : *FriendsList)
		{
			OutFriends.Add(Friend);
		}
		return true;
	}
	return false;
}

TSharedPtr<FOnlineFriend> FOnlineFriendsPlayFab::GetFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	TArray<TSharedRef<FOnlineFriendPlayFab>>* FriendsList = FriendsLists.Find(LocalUserNum);
	if (FriendsList != NULL)
	{
		for (TSharedRef<FOnlineFriendPlayFab> Friend : *FriendsList)
		{
			if (*Friend->GetUserId() == FriendId)
			{
				return Friend;
			}
		}
	}
	return nullptr;
}

bool FOnlineFriendsPlayFab::IsFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	if (GetFriend(LocalUserNum, FriendId, ListName).IsValid())
	{
		return true;
	}
	return false;
}

bool FOnlineFriendsPlayFab::QueryRecentPlayers(const FUniqueNetId& UserId, const FString& Namespace)
{
	TriggerOnQueryRecentPlayersCompleteDelegates(UserId, Namespace, false, TEXT("not implemented"));
	return false;
}

bool FOnlineFriendsPlayFab::GetRecentPlayers(const FUniqueNetId& UserId, const FString& Namespace, TArray<TSharedRef<FOnlineRecentPlayer>>& OutRecentPlayers)
{
	return false;
}

void FOnlineFriendsPlayFab::DumpRecentPlayers() const
{

}

bool FOnlineFriendsPlayFab::BlockPlayer(int32 LocalUserNum, const FUniqueNetId& PlayerId)
{
	TriggerOnBlockedPlayerCompleteDelegates(LocalUserNum, false, PlayerId, TEXT(""), TEXT("not implemented"));
	return false;
}

bool FOnlineFriendsPlayFab::UnblockPlayer(int32 LocalUserNum, const FUniqueNetId& PlayerId)
{
	TriggerOnUnblockedPlayerCompleteDelegates(LocalUserNum, false, PlayerId, TEXT(""), TEXT("not implemented"));
	return false;
}

bool FOnlineFriendsPlayFab::QueryBlockedPlayers(const FUniqueNetId& UserId)
{
	TriggerOnQueryBlockedPlayersCompleteDelegates(UserId, false, TEXT("not implemented"));
	return false;
}

bool FOnlineFriendsPlayFab::GetBlockedPlayers(const FUniqueNetId& UserId, TArray<TSharedRef<FOnlineBlockedPlayer>>& OutBlockedPlayers)
{
	return false;
}

void FOnlineFriendsPlayFab::DumpBlockedPlayers() const
{
	
}

void FOnlineFriendsPlayFab::OnSuccessCallback_Server_GetFriendsList(const PlayFab::ServerModels::FGetFriendsListResult& Result, int32 LocalUserNum, const FString ListName, const FOnReadFriendsListComplete Delegate)
{
	TArray<TSharedRef<FOnlineFriendPlayFab>>& FriendsList = FriendsLists.FindOrAdd(LocalUserNum);

	for (PlayFab::ServerModels::FFriendInfo FriendInfo : Result.Friends)
	{
		TSharedRef<FOnlineFriendPlayFab> Friend(new FOnlineFriendPlayFab(FriendInfo.FriendPlayFabId));
		FriendsList.Add(Friend);
#if false
		if (!FriendInfo.CurrentMatchmakerLobbyId.IsEmpty())
		{
			Friend->Presence.SessionId = MakeShareable(new FUniqueNetIdPlayFabId(FriendInfo.CurrentMatchmakerLobbyId));
			Friend->Presence.bIsJoinable = false;
			Friend->Presence.bIsOnline = true;
			Friend->Presence.bIsPlaying = true;
			Friend->Presence.bIsPlayingThisGame = true;
			Friend->Presence.Status.State = EOnlinePresenceState::Online;
			Friend->Presence.Status.StatusStr = "Playing Empires";
		}
		else
#endif
		{
			Friend->Presence.Status.State = EOnlinePresenceState::Offline;
			Friend->Presence.Status.StatusStr = "Not playing the game";
		}
		Friend->DisplayName = FriendInfo.TitleDisplayName;
	}

	Delegate.ExecuteIfBound(LocalUserNum, true, ListName, TEXT(""));
}

void FOnlineFriendsPlayFab::OnSuccessCallback_Client_GetFriendsList(const PlayFab::ClientModels::FGetFriendsListResult& Result, int32 LocalUserNum, const FString ListName, const FOnReadFriendsListComplete Delegate)
{
	TArray<TSharedRef<FOnlineFriendPlayFab>>& FriendsList = FriendsLists.FindOrAdd(LocalUserNum);

	for (PlayFab::ClientModels::FFriendInfo FriendInfo : Result.Friends)
	{
		TSharedRef<FOnlineFriendPlayFab> Friend(new FOnlineFriendPlayFab(FriendInfo.FriendPlayFabId));
		FriendsList.Add(Friend);

#if false
		if (!FriendInfo.CurrentMatchmakerLobbyId.IsEmpty())
		{
			Friend->Presence.SessionId = MakeShareable(new FUniqueNetIdPlayFabId(FriendInfo.CurrentMatchmakerLobbyId));
			Friend->Presence.bIsJoinable = false;
			Friend->Presence.bIsOnline = true;
			Friend->Presence.bIsPlaying = true;
			Friend->Presence.bIsPlayingThisGame = true;
			Friend->Presence.Status.State = EOnlinePresenceState::Online;
			Friend->Presence.Status.StatusStr = "Playing";
		}
		else
#endif
		{
			Friend->Presence.Status.State = EOnlinePresenceState::Offline;
			Friend->Presence.Status.StatusStr = "Not playing the game";
		}
		Friend->DisplayName = FriendInfo.TitleDisplayName;
	}

	Delegate.ExecuteIfBound(LocalUserNum, true, ListName, TEXT(""));
}

void FOnlineFriendsPlayFab::OnErrorCallback_GetFriendsList(const PlayFab::FPlayFabCppError& ErrorResult, int32 LocalUserNum, const FString ListName, const FOnReadFriendsListComplete Delegate)
{
	Delegate.ExecuteIfBound(LocalUserNum, false, ListName, ErrorResult.ErrorMessage);
}
