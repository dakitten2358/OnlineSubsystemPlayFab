// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineGroupsPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFab.h"


IGroupInfoPlayFab::IGroupInfoPlayFab(TSharedRef<const FUniqueNetId> InGroupId, TSharedRef<const FUniqueNetId> InOwnerId, FGroupDisplayInfo InDisplayInfo)
	: GroupId(InGroupId)
	, OwnerId(InOwnerId)
	, DisplayInfo(InDisplayInfo)
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

/************************/
/* FOnlineGroupsPlayFab */
/************************/
void FOnlineGroupsPlayFab::CreateGroup(const FUniqueNetId& ContextUserId, const FGroupDisplayInfo& GroupInfo, const FOnGroupsRequestCompleted& OnCompleted)
{
	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(ContextUserId);
	if (ClientAPI.IsValid())
	{
		PlayFab::ClientModels::FCreateSharedGroupRequest Request;
		TSharedPtr<FUniqueNetId> GroupId = MakeShareable(new FUniqueNetIdString(ContextUserId.ToString().Append(GroupInfo.Name.ToString())));
		Request.SharedGroupId = GroupId->ToString();

		PlayFab::UPlayFabClientAPI::FCreateSharedGroupDelegate SuccessDelegate = PlayFab::UPlayFabClientAPI::FCreateSharedGroupDelegate::CreateRaw(this, &FOnlineGroupsPlayFab::OnSuccessCallback_Client_CreateSharedGroup, GroupId, &OnCompleted);
		PlayFab::FPlayFabErrorDelegate ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineGroupsPlayFab::OnErrorCallback_Client_CreateSharedGroup, GroupId, &OnCompleted);

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
		TSharedPtr<FUniqueNetId> PGroupId = MakeShareable(new FUniqueNetIdString(ContextUserId.ToString().Append(GroupId.ToString())));
		Request.SharedGroupId = PGroupId->ToString();
		Request.GetMembers = false;
		Request.Keys.Add("Name");
		Request.Keys.Add("Description");
		Request.Keys.Add("Motto");
		Request.Keys.Add("InviteOnly");
		Request.Keys.Add("Language");
		Request.Keys.Add("OwnerId");

		PlayFab::UPlayFabClientAPI::FGetSharedGroupDataDelegate SuccessDelegate = PlayFab::UPlayFabClientAPI::FGetSharedGroupDataDelegate::CreateRaw(this, &FOnlineGroupsPlayFab::OnSuccessCallback_Client_GetSharedGroupData, PGroupId, &OnCompleted);
		PlayFab::FPlayFabErrorDelegate ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineGroupsPlayFab::OnErrorCallback_Client_GetSharedGroupData, PGroupId, &OnCompleted);

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

void FOnlineGroupsPlayFab::OnSuccessCallback_Client_CreateSharedGroup(const PlayFab::ClientModels::FCreateSharedGroupResult& Result, TSharedPtr<FUniqueNetId> GroupId, const FOnGroupsRequestCompleted* OnCompleted)
{
	FGroupsResult GroupResult = FGroupsResult(200, MakeShareable(new FUniqueNetIdString(Result.SharedGroupId)));
	OnCompleted->Execute(GroupResult);
}

void FOnlineGroupsPlayFab::OnErrorCallback_Client_CreateSharedGroup(const PlayFab::FPlayFabError& ErrorResult, TSharedPtr<FUniqueNetId> GroupId, const FOnGroupsRequestCompleted* OnCompleted)
{
	FGroupsResult GroupResult = FGroupsResult(0, GroupId);
	GroupResult.ErrorContent = ErrorResult.ErrorMessage;
	OnCompleted->Execute(GroupResult);
}

void FOnlineGroupsPlayFab::OnSuccessCallback_Client_GetSharedGroupData(const PlayFab::ClientModels::FGetSharedGroupDataResult& Result, TSharedPtr<FUniqueNetId> GroupId, const FOnGroupsRequestCompleted* OnCompleted)
{
	FGroupDisplayInfo GroupDisplayInfo;
	GroupDisplayInfo.Name = FText::FromString(Result.Data["Name"].Value);
	GroupDisplayInfo.Description = FText::FromString(Result.Data["Description"].Value);
	GroupDisplayInfo.Motto = FText::FromString(Result.Data["Motto"].Value);
	GroupDisplayInfo.InviteOnly = Result.Data["InviteOnly"].Value.ToBool();
	GroupDisplayInfo.Language = Result.Data["Language"].Value;

	TSharedPtr<FUniqueNetId> OwnerId = MakeShareable(new FUniqueNetIdString(Result.Data["OwnerId"].Value));

	TSharedPtr<IGroupInfoPlayFab> GroupInfo = MakeShareable(new IGroupInfoPlayFab(GroupId->AsShared(), OwnerId->AsShared(), GroupDisplayInfo));
	//Result.Data;
	//Result.Members;

	FGroupsResult GroupResult = FGroupsResult(200, GroupId);
	OnCompleted->Execute(GroupResult);
}

void FOnlineGroupsPlayFab::OnErrorCallback_Client_GetSharedGroupData(const PlayFab::FPlayFabError& ErrorResult, TSharedPtr<FUniqueNetId> GroupId, const FOnGroupsRequestCompleted* OnCompleted)
{
	FGroupsResult GroupResult = FGroupsResult(0, GroupId);
	GroupResult.ErrorContent = ErrorResult.ErrorMessage;
	OnCompleted->Execute(GroupResult);
}
