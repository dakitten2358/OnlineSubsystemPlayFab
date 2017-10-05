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
void FOnlineGroupsPlayFab::CreateGroup(const FUniqueNetId& ContextUserId, const FGroupDisplayInfo& GroupInfo, const FOnGroupsRequestCompleted& OnCompleted)
{
	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(ContextUserId);
	if (ClientAPI.IsValid())
	{
		PlayFab::ClientModels::FCreateSharedGroupRequest Request;
		TSharedPtr<FUniqueNetId> GroupId = MakeShareable(new FUniqueNetIdPlayFabId(ContextUserId.ToString()));
		Request.SharedGroupId = GroupId->ToString();

		auto SuccessDelegate = PlayFab::UPlayFabClientAPI::FCreateSharedGroupDelegate::CreateRaw(this, &FOnlineGroupsPlayFab::OnSuccessCallback_Client_CreateSharedGroup, GroupId, OnCompleted);
		auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineGroupsPlayFab::OnErrorCallback_Client_CreateSharedGroup, GroupId, OnCompleted);

		ClientAPI->CreateSharedGroup(Request, SuccessDelegate, ErrorDelegate);
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("PlayFab Client Interface not available"));

		FGroupsResult Result;
		OnCompleted.Execute(Result);
	}
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
	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(ContextUserId);
	if (ClientAPI.IsValid())
	{
		PlayFab::ClientModels::FGetSharedGroupDataRequest Request;
		TSharedPtr<FUniqueNetId> PGroupId = MakeShareable(new FUniqueNetIdString(GroupId.ToString()));
		Request.SharedGroupId = PGroupId->ToString();
		Request.GetMembers = true;
		// Don't set Request.Keys so we get all the data about the group
		/*Request.Keys.Add("Name");
		Request.Keys.Add("Description");
		Request.Keys.Add("Motto");
		Request.Keys.Add("InviteOnly");
		Request.Keys.Add("Language");
		Request.Keys.Add("OwnerId");*/

		auto SuccessDelegate = PlayFab::UPlayFabClientAPI::FGetSharedGroupDataDelegate::CreateRaw(this, &FOnlineGroupsPlayFab::OnSuccessCallback_Client_GetSharedGroupData, PGroupId, OnCompleted);
		auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineGroupsPlayFab::OnErrorCallback_Client_GetSharedGroupData, PGroupId, OnCompleted);

		ClientAPI->GetSharedGroupData(Request, SuccessDelegate, ErrorDelegate);
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("PlayFab Client Interface not available"));

		FGroupsResult Result;
		OnCompleted.Execute(Result);
	}
}

TSharedPtr<const IGroupInfo> FOnlineGroupsPlayFab::GetCachedGroupInfo(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId)
{
	TSharedPtr<IGroupInfoPlayFab>* GroupInfo = GroupsCache.Find(GroupId.ToString());
	if (GroupInfo != nullptr)
	{
		return *GroupInfo;
	}
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
	QueryGroupInfo(ContextUserId, GroupId, OnCompleted);
}

TSharedPtr<const IGroupRoster> FOnlineGroupsPlayFab::GetCachedGroupRoster(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId)
{
	TSharedPtr<IGroupInfoPlayFab>* GroupInfo = GroupsCache.Find(GroupId.ToString());
	if (GroupInfo != nullptr)
	{
		return GroupInfo->Get()->GetRoster();
	}
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

void FOnlineGroupsPlayFab::OnSuccessCallback_Client_CreateSharedGroup(const PlayFab::ClientModels::FCreateSharedGroupResult& Result, TSharedPtr<FUniqueNetId> GroupId, const FOnGroupsRequestCompleted OnCompleted)
{
	FGroupsResult GroupResult = FGroupsResult(200, MakeShareable(new FUniqueNetIdString(Result.SharedGroupId)));
	OnCompleted.ExecuteIfBound(GroupResult);
}

void FOnlineGroupsPlayFab::OnErrorCallback_Client_CreateSharedGroup(const PlayFab::FPlayFabError& ErrorResult, TSharedPtr<FUniqueNetId> GroupId, const FOnGroupsRequestCompleted OnCompleted)
{
	FGroupsResult GroupResult = FGroupsResult(0, GroupId);
	GroupResult.ErrorContent = ErrorResult.ErrorMessage;
	OnCompleted.ExecuteIfBound(GroupResult);
}

void FOnlineGroupsPlayFab::OnSuccessCallback_Client_GetSharedGroupData(const PlayFab::ClientModels::FGetSharedGroupDataResult& Result, TSharedPtr<FUniqueNetId> GroupId, const FOnGroupsRequestCompleted OnCompleted)
{
	FGroupDisplayInfo GroupDisplayInfo;
	GroupDisplayInfo.Name = FText::FromString(Result.Data["Name"].Value);
	GroupDisplayInfo.Description = FText::FromString(Result.Data["Description"].Value);
	GroupDisplayInfo.Motto = FText::FromString(Result.Data["Motto"].Value);
	GroupDisplayInfo.InviteOnly = Result.Data["InviteOnly"].Value.ToBool();
	GroupDisplayInfo.Language = Result.Data["Language"].Value;

	TSharedPtr<FUniqueNetId> OwnerId = MakeShareable(new FUniqueNetIdPlayFabId(Result.Data["OwnerId"].Value));

	TArray<FGroupMember*> Members;
	for (FString MemberId : Result.Members)
	{
		FGroupMember Member;
		Member.AccountId = MakeShareable(new FUniqueNetIdPlayFabId(MemberId));
		Member.bAdmin = false;
		Member.bIsOwner = false;
		Members.Add(&Member);
	}
	TSharedRef<IGroupRosterPlayFab> Roster = MakeShareable(new IGroupRosterPlayFab(GroupId.ToSharedRef(), Members));

	TSharedPtr<IGroupInfoPlayFab> GroupInfo = MakeShareable(new IGroupInfoPlayFab(GroupId->AsShared(), OwnerId->AsShared(), GroupDisplayInfo, Result.Data, Roster));

	GroupsCache.Add(GroupId->ToString(), GroupInfo);

	FGroupsResult GroupResult = FGroupsResult(200, GroupId);
	OnCompleted.ExecuteIfBound(GroupResult);
}

void FOnlineGroupsPlayFab::OnErrorCallback_Client_GetSharedGroupData(const PlayFab::FPlayFabError& ErrorResult, TSharedPtr<FUniqueNetId> GroupId, const FOnGroupsRequestCompleted OnCompleted)
{
	FGroupsResult GroupResult = FGroupsResult(0, GroupId);
	GroupResult.ErrorContent = ErrorResult.ErrorMessage;
	OnCompleted.ExecuteIfBound(GroupResult);
}
