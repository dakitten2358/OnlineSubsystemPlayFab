// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineGroupsPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFab.h"


IGroupRosterPlayFab::IGroupRosterPlayFab(TSharedRef<const FUniqueNetId> InGroupId, TArray<FGroupMember*> InMembers)
	: GroupId(InGroupId)
	, Members(InMembers)
{

}

const FGroupMember* IGroupRosterPlayFab::GetEntry(const FUniqueNetId& EntryId) const
{
	for (FGroupMember* Member : Members)
	{
		FUniqueNetIdMatcher MemberMatch(*Member->GetId());
		if (MemberMatch(EntryId))
		{
			return Member;
		}
	}
	return nullptr;
}

FGroupMember* IGroupRosterPlayFab::GetEntry(const FUniqueNetId& EntryId)
{
	return GetEntry(EntryId);
}

TSharedRef<const FUniqueNetId> IGroupRosterPlayFab::GetCollectionId() const
{
	return GroupId;
}

void IGroupRosterPlayFab::CopyEntries(TArray<FGroupMember>& Out) const
{
	Out.Empty();
	for (FGroupMember* Member : Members)
	{
		Out.Add(*Member);
	}
}

IGroupInfoPlayFab::IGroupInfoPlayFab(TSharedRef<const FUniqueNetId> InGroupId, TSharedRef<const FUniqueNetId> InOwnerId, FGroupDisplayInfo InDisplayInfo, TMap<FString, PlayFab::ClientModels::FSharedGroupDataRecord> InData, TSharedRef<IGroupRosterPlayFab> InRoster)
	: GroupId(InGroupId)
	, OwnerId(InOwnerId)
	, DisplayInfo(InDisplayInfo)
	, Data(InData)
	, Roster(InRoster)
{

}

TSharedRef<const FUniqueNetId> IGroupInfoPlayFab::GetGroupId() const
{
	return GroupId;
}

const FString& IGroupInfoPlayFab::GetNamespace() const
{
	return Namespace;
}

const FGroupDisplayInfo& IGroupInfoPlayFab::GetDisplayInfo() const
{
	return DisplayInfo;
}

TSharedRef<const FUniqueNetId> IGroupInfoPlayFab::GetOwner() const
{
	return OwnerId;
}

uint32 IGroupInfoPlayFab::GetSize() const
{
	return 0;
}

const FDateTime& IGroupInfoPlayFab::GetCreatedAt() const
{
	return randomTime;
}

const FDateTime& IGroupInfoPlayFab::GetLastUpdated() const
{
	return randomTime;
}

TSharedPtr<IGroupRosterPlayFab> IGroupInfoPlayFab::GetRoster() const
{
	return Roster;
}

/************************/
/* FOnlineGroupsPlayFab */
/************************/
void FOnlineGroupsPlayFab::QueryGroupNameExist(const FUniqueNetId& ContextUserId, const FString& GroupName, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::CreateGroup(const FUniqueNetId& ContextUserId, const FGroupDisplayInfo& GroupInfo, const FOnGroupsRequestCompleted& OnCompleted)
{
	
}

void FOnlineGroupsPlayFab::FindGroups(const FUniqueNetId& ContextUserId, const FGroupSearchOptions& SearchOptions, const FOnFindGroupsCompleted& OnCompleted)
{
	UE_LOG_ONLINE(Error, TEXT("FOnlineGroupsPlayFab::FindGroups: Not currently implemented! Use QueryGroupInfo"));
	FFindGroupsResult Result;
	Result.ErrorContent = "FindGroups() Not currently implemented";
	OnCompleted.Execute(Result);
}

void FOnlineGroupsPlayFab::QueryGroupInfo(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted)
{
	
}

TSharedPtr<const IGroupInfo> FOnlineGroupsPlayFab::GetCachedGroupInfo(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId)
{
	return nullptr;
}

void FOnlineGroupsPlayFab::JoinGroup(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::LeaveGroup(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::CancelRequest(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::AcceptInvite(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::DeclineInvite(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::QueryGroupRoster(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted)
{
	
}

TSharedPtr<const IGroupRoster> FOnlineGroupsPlayFab::GetCachedGroupRoster(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId)
{
	return nullptr;
}

void FOnlineGroupsPlayFab::QueryUserMembership(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

TSharedPtr<const IUserMembership> FOnlineGroupsPlayFab::GetCachedUserMembership(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId)
{
	return nullptr;
}

void FOnlineGroupsPlayFab::QueryOutgoingApplications(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

TSharedPtr<const IApplications> FOnlineGroupsPlayFab::GetCachedApplications(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId)
{
	return nullptr;
}

void FOnlineGroupsPlayFab::QueryOutgoingInvitations(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::QueryIncomingInvitations(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

TSharedPtr<const IInvitations> FOnlineGroupsPlayFab::GetCachedInvitations(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId)
{
	return nullptr;
}

void FOnlineGroupsPlayFab::UpdateGroupInfo(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FGroupDisplayInfo& GroupInfo, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::AcceptUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::DeclineUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::InviteUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, bool bAllowBlocked, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::CancelInvite(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::RemoveUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::PromoteUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::DemoteUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::BlockUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::UnblockUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::QueryGroupInvites(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

TSharedPtr<const IGroupInvites> FOnlineGroupsPlayFab::GetCachedGroupInvites(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId)
{
	return nullptr;
}

void FOnlineGroupsPlayFab::QueryGroupRequests(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

TSharedPtr<const IGroupRequests> FOnlineGroupsPlayFab::GetCachedGroupRequests(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId)
{
	return nullptr;
}

void FOnlineGroupsPlayFab::QueryGroupBlacklist(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

TSharedPtr<const IGroupBlacklist> FOnlineGroupsPlayFab::GetCachedGroupBlacklist(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId)
{
	return nullptr;
}

void FOnlineGroupsPlayFab::QueryIncomingApplications(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::QueryConfigHeadcount(const FUniqueNetId& ContextUserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::QueryConfigMembership(const FUniqueNetId& ContextUserId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

TSharedPtr<const FGroupConfigEntryInt> FOnlineGroupsPlayFab::GetCachedConfigInt(const FString& Key)
{
	return nullptr;
}

TSharedPtr<const FGroupConfigEntryBool> FOnlineGroupsPlayFab::GetCachedConfigBool(const FString& Key)
{
	return nullptr;
}

void FOnlineGroupsPlayFab::TransferGroup(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& NewOwnerId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::DeleteGroup(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted)
{

}

void FOnlineGroupsPlayFab::SetNamespace(const FString& Ns)
{
	Namespace = Ns;
}

const FString& FOnlineGroupsPlayFab::GetNamespace() const
{
	return Namespace;
}
