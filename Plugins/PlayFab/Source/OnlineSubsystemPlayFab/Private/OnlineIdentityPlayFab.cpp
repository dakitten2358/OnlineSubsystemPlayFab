// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlineIdentityPlayFab.h"
#include "PlayFab.h"
#include "Xmpp.h"


FString FUserOnlineAccountPlayFab::GetRealName() const
{
	return TEXT("Pinocchio");
}

FString FUserOnlineAccountPlayFab::GetDisplayName(const FString& Platform /*= FString()*/) const
{
	return !DisplayName.IsEmpty() ? DisplayName : TEXT("InvalidOnlineSubsystemName");
}

bool FUserOnlineAccountPlayFab::GetAuthAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* FoundAttr = AdditionalAuthData.Find(AttrName);
	if (FoundAttr != NULL)
	{
		OutAttrValue = *FoundAttr;
		return true;
	}
	return false;
}

bool FUserOnlineAccountPlayFab::GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* FoundAttr = UserAttributes.Find(AttrName);
	if (FoundAttr != NULL)
	{
		OutAttrValue = *FoundAttr;
		return true;
	}
	return false;
}

bool FUserOnlineAccountPlayFab::SetUserAttribute(const FString& AttrName, const FString& AttrValue)
{
	const FString* FoundAttr = UserAttributes.Find(AttrName);
	if (FoundAttr == NULL || *FoundAttr != AttrValue)
	{
		UserAttributes.Add(AttrName, AttrValue);
		return true;
	}
	return false;
}

bool FOnlineIdentityPlayFab::Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials)
{
	FString ErrorStr;

	// valid local player index
	if (LocalUserNum < 0 || LocalUserNum >= MAX_LOCAL_PLAYERS)
	{
		ErrorStr = FString::Printf(TEXT("Invalid LocalUserNum=%d"), LocalUserNum);
	}
	else if (AccountCredentials.Id.IsEmpty())
	{
		ErrorStr = TEXT("Invalid account id, string empty");
	}
	else
	{
		PlayFabClientPtr ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
		if (ClientAPI.IsValid())
		{
			PlayFab::ClientModels::FLoginWithPlayFabRequest Request;
			Request.Username = AccountCredentials.Id;
			Request.Password = AccountCredentials.Token;

			// Request for account data when responding
			Request.InfoRequestParameters = MakeShareable(new PlayFab::ClientModels::FGetPlayerCombinedInfoRequestParams());
			Request.InfoRequestParameters->GetPlayerStatistics = true;
			Request.InfoRequestParameters->GetUserAccountInfo = true;
			Request.InfoRequestParameters->GetUserData = true;

			SuccessDelegate_Login = PlayFab::UPlayFabClientAPI::FLoginWithPlayFabDelegate::CreateRaw(this, &FOnlineIdentityPlayFab::OnSuccessCallback_Login, LocalUserNum);
			ErrorDelegate_Login = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineIdentityPlayFab::OnErrorCallback_Login, LocalUserNum);

			ClientAPI->LoginWithPlayFab(Request, SuccessDelegate_Login, ErrorDelegate_Login);
		}
		else
		{
			ErrorStr = TEXT("PlayFab Client Interface not available");
		}
	}

	if (!ErrorStr.IsEmpty())
	{
		UE_LOG_ONLINE(Warning, TEXT("Login request failed. %s"), *ErrorStr);
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, FUniqueNetIdString(), ErrorStr);
		return false;
	}
	return true;
}

bool FOnlineIdentityPlayFab::Logout(int32 LocalUserNum)
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		// remove cached user account
		UserAccounts.Remove(FUniqueNetIdString(*UserId));
		// remove cached user id
		UserIds.Remove(LocalUserNum);

		PlayFabClientPtr ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
		if (ClientAPI.IsValid())
		{
			PlayFab::ClientModels::FWriteClientPlayerEventRequest Request;
			Request.EventName = "player_logged_out";
			ClientAPI->WritePlayerEvent(Request);
		}

		// not async but should call completion delegate anyway
		TriggerOnLoginChangedDelegates(LocalUserNum);
		TriggerOnLogoutCompleteDelegates(LocalUserNum, true);

		return true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("No logged in user found for LocalUserNum=%d."),
			LocalUserNum);
		TriggerOnLogoutCompleteDelegates(LocalUserNum, false);
	}
	return false;
}

bool FOnlineIdentityPlayFab::AutoLogin(int32 LocalUserNum)
{
	FString LoginStr;
	FString PasswordStr;
	FString TypeStr;

	FParse::Value(FCommandLine::Get(), TEXT("AUTH_LOGIN="), LoginStr);
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_PASSWORD="), PasswordStr);
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_TYPE="), TypeStr);

	if (!LoginStr.IsEmpty())
	{
		if (!PasswordStr.IsEmpty())
		{
			if (!TypeStr.IsEmpty())
			{
				return Login(0, FOnlineAccountCredentials(TypeStr, LoginStr, PasswordStr));
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("AutoLogin missing AUTH_TYPE=<type>."));
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("AutoLogin missing AUTH_PASSWORD=<password>."));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("AutoLogin missing AUTH_LOGIN=<login id>."));
	}
	return false;
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityPlayFab::GetUserAccount(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccount> Result;

	FUniqueNetIdString StringUserId(UserId);
	const TSharedRef<FUserOnlineAccountPlayFab>* FoundUserAccount = UserAccounts.Find(StringUserId);
	if (FoundUserAccount != NULL)
	{
		Result = *FoundUserAccount;
	}

	return Result;
}

TArray<TSharedPtr<FUserOnlineAccount> > FOnlineIdentityPlayFab::GetAllUserAccounts() const
{
	TArray<TSharedPtr<FUserOnlineAccount> > Result;

	for (TMap<FUniqueNetIdString, TSharedRef<FUserOnlineAccountPlayFab>>::TConstIterator It(UserAccounts); It; ++It)
	{
		Result.Add(It.Value());
	}

	return Result;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityPlayFab::GetUniquePlayerId(int32 LocalUserNum) const
{
	const TSharedPtr<const FUniqueNetId>* FoundId = UserIds.Find(LocalUserNum);
	if (FoundId != NULL)
	{
		return *FoundId;
	}
	return NULL;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityPlayFab::CreateUniquePlayerId(uint8* Bytes, int32 Size)
{
	if (Bytes != NULL && Size > 0)
	{
		FString StrId(Size, (TCHAR*)Bytes);
		return MakeShareable(new FUniqueNetIdString(StrId));
	}
	return NULL;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityPlayFab::CreateUniquePlayerId(const FString& Str)
{
	return MakeShareable(new FUniqueNetIdString(Str));
}

ELoginStatus::Type FOnlineIdentityPlayFab::GetLoginStatus(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		return GetLoginStatus(*UserId);
	}
	return ELoginStatus::NotLoggedIn;
}

ELoginStatus::Type FOnlineIdentityPlayFab::GetLoginStatus(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(UserId);
	if (UserAccount.IsValid() &&
		UserAccount->GetUserId()->IsValid())
	{
		return ELoginStatus::LoggedIn;
	}
	return ELoginStatus::NotLoggedIn;
}

FString FOnlineIdentityPlayFab::GetPlayerNickname(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UniqueId = GetUniquePlayerId(LocalUserNum);
	if (UniqueId.IsValid())
	{
		return UniqueId->ToString();
	}

	return TEXT("PlayFabUser");
}

FString FOnlineIdentityPlayFab::GetPlayerNickname(const FUniqueNetId& UserId) const
{
	return UserId.ToString();
}

FString FOnlineIdentityPlayFab::GetAuthToken(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(*UserId);
		if (UserAccount.IsValid())
		{
			return UserAccount->GetAccessToken();
		}
	}
	return FString();
}

void FOnlineIdentityPlayFab::GetUserPrivilege(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, const FOnGetUserPrivilegeCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(UserId, Privilege, (uint32)EPrivilegeResults::NoFailures);
}

FPlatformUserId FOnlineIdentityPlayFab::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& UniqueNetId)
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; ++i)
	{
		auto CurrentUniqueId = GetUniquePlayerId(i);
		if (CurrentUniqueId.IsValid() && (*CurrentUniqueId == UniqueNetId))
		{
			return i;
		}
	}

	return PLATFORMUSERID_NONE;
}

FString FOnlineIdentityPlayFab::GetAuthType() const
{
	return TEXT("PlayFab");
}

void FOnlineIdentityPlayFab::OnSuccessCallback_Login(const PlayFab::ClientModels::FLoginResult& Result, int32 LocalUserNum)
{
	TSharedPtr<FUserOnlineAccountPlayFab> UserAccountPtr;
	UserAccountPtr = MakeShareable(new FUserOnlineAccountPlayFab(Result.PlayFabId, Result.SessionTicket));

	UserAccountPtr->DisplayName = Result.InfoResultPayload->AccountInfo->TitleInfo->DisplayName;
	for (auto Elem : Result.InfoResultPayload->UserData)
	{
		UserAccountPtr->UserAttributes.Add(Elem.Key, Elem.Value.Value);
	}

	UserAccounts.Add(FUniqueNetIdString(Result.PlayFabId), UserAccountPtr.ToSharedRef());
	UserIds.Add(LocalUserNum, UserAccountPtr->GetUserId());

	UE_LOG_ONLINE(Log, TEXT("FOnlineIdentityPlayFab::Login: Received PlayFabId for LocalUser: %d: %s"), LocalUserNum, *Result.PlayFabId);

	TriggerOnLoginChangedDelegates(LocalUserNum);
	TriggerOnLoginCompleteDelegates(LocalUserNum, true, UserAccountPtr->GetUserId().Get(), TEXT(""));

	if (PlayFabSubsystem->IsXmppEnabled())
	{
		TSharedRef<IXmppConnection> XmppConnection = FXmppModule::Get().CreateConnection(UserAccountPtr->GetUserId()->ToString());

		FXmppServer Server;
		Server.AppId = PlayFabSubsystem->GetAppId();
		Server.ClientResource = "test";
		Server.Domain = "localhost";
		Server.ServerAddr = "192.168.175.129";
		Server.bUseSSL = false; // This is me avoiding dealing with certificates
		Server.bUsePlainTextAuth = true; // For some reason Prosody doesn't seem to like non-plain auth?
		Server.Platform = "Windows";
		XmppConnection->SetServer(Server);

		XmppConnection->Login(UserAccountPtr->GetUserId()->ToString(), Result.SessionTicket);
	}
}

void FOnlineIdentityPlayFab::OnErrorCallback_Login(const PlayFab::FPlayFabError& ErrorResult, int32 LocalUserNum)
{
	UE_LOG_ONLINE(Error, TEXT("FOnlineIdentityPlayFab::Login: Login failed for LocalUser: %d: %s"), LocalUserNum, *ErrorResult.ErrorMessage);

	TriggerOnLoginCompleteDelegates(LocalUserNum, false, FUniqueNetIdString(TEXT("")), ErrorResult.ErrorMessage);
}

