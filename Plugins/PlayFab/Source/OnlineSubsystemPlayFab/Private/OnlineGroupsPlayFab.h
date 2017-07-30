// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineGroupsInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"

/**
* This struct describes metadata about a group.
*/
class IGroupInfoPlayFab : public IGroupInfo
{
protected:
	TSharedRef<const FUniqueNetId> GroupId;
	TSharedRef<const FUniqueNetId> OwnerId;
	FGroupDisplayInfo DisplayInfo;
	FString Namespace;
	FDateTime randomTime;

public:

	IGroupInfoPlayFab(TSharedRef<const FUniqueNetId> InGroupId, TSharedRef<const FUniqueNetId> InOwnerId, FGroupDisplayInfo InDisplayInfo);

	virtual ~IGroupInfoPlayFab()
	{
	}

	// IOnlineGroups
	virtual TSharedRef<const FUniqueNetId> GetGroupId() const override;
	virtual const FString& GetNamespace() const override;
	virtual const FGroupDisplayInfo& GetDisplayInfo() const override;
	virtual TSharedRef<const FUniqueNetId> GetOwner() const override;
	virtual uint32 GetSize() const override;
	virtual const FDateTime& GetCreatedAt() const override;
	virtual const FDateTime& GetLastUpdated() const override;
};

/**
* Data about the group that is used for display
*/
class FOnlineGroupsPlayFab : public IOnlineGroups
{
private:

	FString Namespace;

	/** Reference to the main PlayFab subsystem */
	class FOnlineSubsystemPlayFab* PlayFabSubsystem;

	/** Hidden on purpose */
	FOnlineGroupsPlayFab() :
		PlayFabSubsystem(NULL)
	{
	}

PACKAGE_SCOPE:

	FOnlineGroupsPlayFab(class FOnlineSubsystemPlayFab* InPlayFabSubsystem) :
		PlayFabSubsystem(InPlayFabSubsystem)
	{
	}

public:

	virtual ~FOnlineGroupsPlayFab()
	{
	}

	// IOnlineGroups
	virtual void CreateGroup(const FUniqueNetId& ContextUserId, const FGroupDisplayInfo& GroupInfo, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void FindGroups(const FUniqueNetId& ContextUserId, const FGroupSearchOptions& SearchOptions, const FOnFindGroupsCompleted& OnCompleted) override;
	virtual void QueryGroupInfo(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual TSharedPtr<const IGroupInfo> GetCachedGroupInfo(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId) override;
	virtual void JoinGroup(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void LeaveGroup(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void CancelRequest(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void AcceptInvite(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void DeclineInvite(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void QueryGroupRoster(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual TSharedPtr<const IGroupRoster> GetCachedGroupRoster(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId) override;
	virtual void QueryUserMembership(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual TSharedPtr<const IUserMembership> GetCachedUserMembership(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId) override;
	virtual void QueryOutgoingApplications(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual TSharedPtr<const IApplications> GetCachedApplications(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId) override;
	virtual void QueryOutgoingInvitations(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void QueryIncomingInvitations(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual TSharedPtr<const IInvitations> GetCachedInvitations(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId) override;
	virtual void UpdateGroupInfo(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FGroupDisplayInfo& GroupInfo, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void AcceptUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void DeclineUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void InviteUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, bool bAllowBlocked, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void CancelInvite(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void RemoveUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void PromoteUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void DemoteUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void BlockUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void UnblockUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void QueryGroupInvites(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual TSharedPtr<const IGroupInvites> GetCachedGroupInvites(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId) override;
	virtual void QueryGroupRequests(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual TSharedPtr<const IGroupRequests> GetCachedGroupRequests(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId) override;
	virtual void QueryGroupBlacklist(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual TSharedPtr<const IGroupBlacklist> GetCachedGroupBlacklist(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId) override;
	virtual void QueryIncomingApplications(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void QueryConfigHeadcount(const FUniqueNetId& ContextUserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void QueryConfigMembership(const FUniqueNetId& ContextUserId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual TSharedPtr<const FGroupConfigEntryInt> GetCachedConfigInt(const FString& Key) override;
	virtual void TransferGroup(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& NewOwnerId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void DeleteGroup(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void SetNamespace(const FString& Ns) override;
	virtual const FString& GetNamespace() const override;

private:
	void OnSuccessCallback_Client_CreateSharedGroup(const PlayFab::ClientModels::FCreateSharedGroupResult& Result, TSharedPtr<FUniqueNetId> GroupId, const FOnGroupsRequestCompleted* OnCompleted);
	void OnErrorCallback_Client_CreateSharedGroup(const PlayFab::FPlayFabError& ErrorResult, TSharedPtr<FUniqueNetId> GroupId, const FOnGroupsRequestCompleted* OnCompleted);
	void OnSuccessCallback_Client_GetSharedGroupData(const PlayFab::ClientModels::FGetSharedGroupDataResult& Result, TSharedPtr<FUniqueNetId> GroupId, const FOnGroupsRequestCompleted* OnCompleted);
	void OnErrorCallback_Client_GetSharedGroupData(const PlayFab::FPlayFabError& ErrorResult, TSharedPtr<FUniqueNetId> GroupId, const FOnGroupsRequestCompleted* OnCompleted);
};

typedef TSharedPtr<FOnlineGroupsPlayFab, ESPMode::ThreadSafe> FOnlineGroupsPlayFabPtr;