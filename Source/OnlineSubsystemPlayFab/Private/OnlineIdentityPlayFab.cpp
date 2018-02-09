// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineIdentityPlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "OnlineChatPlayFab.h"
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
	/*if (GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn)
	{
		ErrorStr = TEXT("Already logged in! Logout first");
	}
	else */if (bAttemptingLogin)
	{
		ErrorStr = TEXT("Already attempting login!");
	}
	else if (LocalUserNum < 0 || LocalUserNum >= MAX_LOCAL_PLAYERS)
	{
		ErrorStr = FString::Printf(TEXT("Invalid LocalUserNum=%d"), LocalUserNum);
	}
	else if (AccountCredentials.Id.IsEmpty())
	{
		ErrorStr = TEXT("Invalid account id, string empty");
	}
	else
	{
		PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(LocalUserNum);
		if (ClientAPI.IsValid())
		{
			bAttemptingLogin = true;

			// Request for account data when responding
			TSharedPtr<PlayFab::ClientModels::FGetPlayerCombinedInfoRequestParams> InfoRequestParameters = MakeShareable(new PlayFab::ClientModels::FGetPlayerCombinedInfoRequestParams());
			InfoRequestParameters->GetPlayerStatistics = true;
			InfoRequestParameters->GetUserAccountInfo = true;
			InfoRequestParameters->GetUserData = true;

			if (AccountCredentials.Type == "playfab")
			{
				PlayFab::ClientModels::FLoginWithPlayFabRequest Request;
				Request.Username = AccountCredentials.Id;
				Request.Password = AccountCredentials.Token;
				Request.InfoRequestParameters = InfoRequestParameters;

				auto SuccessDelegate_Login = PlayFab::UPlayFabClientAPI::FLoginWithPlayFabDelegate::CreateRaw(this, &FOnlineIdentityPlayFab::OnSuccessCallback_Login, LocalUserNum);
				auto ErrorDelegate_Login = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineIdentityPlayFab::OnErrorCallback_Login, LocalUserNum);

				ClientAPI->LoginWithPlayFab(Request, SuccessDelegate_Login, ErrorDelegate_Login);
			}
			else if (AccountCredentials.Type == "email")
			{
				PlayFab::ClientModels::FLoginWithEmailAddressRequest Request;
				Request.Email = AccountCredentials.Id;
				Request.Password = AccountCredentials.Token;
				Request.InfoRequestParameters = InfoRequestParameters;

				auto SuccessDelegate_Login = PlayFab::UPlayFabClientAPI::FLoginWithEmailAddressDelegate::CreateRaw(this, &FOnlineIdentityPlayFab::OnSuccessCallback_Login, LocalUserNum);
				auto ErrorDelegate_Login = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineIdentityPlayFab::OnErrorCallback_Login, LocalUserNum);

				ClientAPI->LoginWithEmailAddress(Request, SuccessDelegate_Login, ErrorDelegate_Login);
			}
			else if (AccountCredentials.Type == "customid")
			{
				PlayFab::ClientModels::FLoginWithCustomIDRequest Request;
				Request.CustomId = AccountCredentials.Token;
				Request.CreateAccount = false;
				Request.InfoRequestParameters = InfoRequestParameters;

				auto SuccessDelegate_Login = PlayFab::UPlayFabClientAPI::FLoginWithCustomIDDelegate::CreateRaw(this, &FOnlineIdentityPlayFab::OnSuccessCallback_Login, LocalUserNum);
				auto ErrorDelegate_Login = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineIdentityPlayFab::OnErrorCallback_Login, LocalUserNum);

				ClientAPI->LoginWithCustomID(Request, SuccessDelegate_Login, ErrorDelegate_Login);
			}
			else if (AccountCredentials.Type == "steam")
			{
				PlayFab::ClientModels::FLoginWithSteamRequest Request;
				Request.SteamTicket = AccountCredentials.Token;
				Request.CreateAccount = false;
				Request.InfoRequestParameters = InfoRequestParameters;

				auto SuccessDelegate_Login = PlayFab::UPlayFabClientAPI::FLoginWithSteamDelegate::CreateRaw(this, &FOnlineIdentityPlayFab::OnSuccessCallback_Login, LocalUserNum);
				auto ErrorDelegate_Login = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineIdentityPlayFab::OnErrorCallback_Login, LocalUserNum);

				ClientAPI->LoginWithSteam(Request, SuccessDelegate_Login, ErrorDelegate_Login);
			}
			else
			{
				bAttemptingLogin = false;
				ErrorStr = TEXT("Unknown AccountCredentials Type");
			}
		}
		else
		{
			ErrorStr = TEXT("PlayFab Client Interface not available");
		}
	}

	if (!ErrorStr.IsEmpty())
	{
		UE_LOG_ONLINE(Warning, TEXT("Login request failed. %s"), *ErrorStr);
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, FUniqueNetIdPlayFabId(), ErrorStr);
		return false;
	}
	return true;
}

bool FOnlineIdentityPlayFab::Logout(int32 LocalUserNum)
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		if (PlayFabSubsystem->IsXmppEnabled())
		{
			TSharedRef<IXmppConnection> XmppConnection = FXmppModule::Get().CreateConnection(UserId->ToString());
			static_cast<FOnlineChatPlayFab*>(PlayFabSubsystem->GetChatInterface().Get())->XmppClearDelegates(XmppConnection);
			XmppConnection->Logout();
			FXmppModule::Get().RemoveConnection(UserId->ToString());
		}

		// remove cached user account
		UserAccounts.Remove(FUniqueNetIdPlayFabId(*UserId));
		// remove cached user id
		UserIds.Remove(LocalUserNum);

		PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(LocalUserNum);
		if (ClientAPI.IsValid())
		{
			PlayFab::ClientModels::FWriteClientPlayerEventRequest Request;
			Request.EventName = "player_logged_out";
			ClientAPI->WritePlayerEvent(Request);
		}

		// not async but should call completion delegate anyway
		TriggerOnLoginChangedDelegates(LocalUserNum);
		TriggerOnLoginStatusChangedDelegates(LocalUserNum, ELoginStatus::LoggedIn, ELoginStatus::NotLoggedIn, *UserId.Get());
		TriggerOnLogoutCompleteDelegates(LocalUserNum, true);

		return true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("No logged in user found for LocalUserNum=%d."), LocalUserNum);
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

	// FPlatformMisc::GetMachineId()

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

	FUniqueNetIdPlayFabId StringUserId(UserId);
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

	for (TMap<FUniqueNetIdPlayFabId, TSharedRef<FUserOnlineAccountPlayFab>>::TConstIterator It(UserAccounts); It; ++It)
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
		return MakeShareable(new FUniqueNetIdPlayFabId(StrId));
	}
	return NULL;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityPlayFab::CreateUniquePlayerId(const FString& Str)
{
	return MakeShareable(new FUniqueNetIdPlayFabId(Str));
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

FPlatformUserId FOnlineIdentityPlayFab::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& UniqueNetId) const
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

void FOnlineIdentityPlayFab::RevokeAuthToken(const FUniqueNetId& UserId, const FOnRevokeAuthTokenCompleteDelegate& Delegate)
{
	//Delegate.ExecuteIfBound(UserId, )
}

void FOnlineIdentityPlayFab::OnSuccessCallback_Login(const PlayFab::ClientModels::FLoginResult& Result, int32 LocalUserNum)
{
	bAttemptingLogin = false;

	TSharedPtr<FUserOnlineAccountPlayFab> UserAccountPtr;
	UserAccountPtr = MakeShareable(new FUserOnlineAccountPlayFab(Result.PlayFabId, Result.SessionTicket));

	UserAccountPtr->DisplayName = Result.InfoResultPayload->AccountInfo->TitleInfo->DisplayName;
	for (auto Elem : Result.InfoResultPayload->UserData)
	{
		UserAccountPtr->UserAttributes.Add(Elem.Key, Elem.Value.Value);
	}

	UserAccounts.Add(FUniqueNetIdPlayFabId(Result.PlayFabId), UserAccountPtr.ToSharedRef());
	UserIds.Add(LocalUserNum, UserAccountPtr->GetUserId());

	UE_LOG_ONLINE(Log, TEXT("FOnlineIdentityPlayFab::Login: Received PlayFabId for LocalUser: %d: %s"), LocalUserNum, *Result.PlayFabId);

	TriggerOnLoginChangedDelegates(LocalUserNum);
	TriggerOnLoginStatusChangedDelegates(LocalUserNum, ELoginStatus::NotLoggedIn, ELoginStatus::LoggedIn, UserAccountPtr->GetUserId().Get());
	TriggerOnLoginCompleteDelegates(LocalUserNum, true, UserAccountPtr->GetUserId().Get(), TEXT(""));

	if (PlayFabSubsystem->IsXmppEnabled())
	{
		TSharedRef<IXmppConnection> XmppConnection = FXmppModule::Get().CreateConnection(UserAccountPtr->GetUserId()->ToString());

		FXmppServer ServerRequest;
		ServerRequest.AppId = PlayFabSubsystem->GetAppId();
		ServerRequest.ClientResource = "UE4";
		ServerRequest.Domain = PlayFabSubsystem->GetAppId();
		int32 XmppPort = 5222;
		GConfig->GetInt(TEXT("OnlineSubsystemPlayFab"), TEXT("XmppPort"), XmppPort, GEngineIni);
		ServerRequest.ServerPort = XmppPort;
		FString XmppHost;
		GConfig->GetString(TEXT("OnlineSubsystemPlayFab"), TEXT("XmppHost"), XmppHost, GEngineIni);
		ServerRequest.ServerAddr = XmppHost;
		bool bXmppSSL = false;
		GConfig->GetBool(TEXT("OnlineSubsystemPlayFab"), TEXT("bXmppSSL"), bXmppSSL, GEngineIni);
		ServerRequest.bUseSSL = bXmppSSL; // This is me avoiding dealing with certificates
		ServerRequest.bUsePlainTextAuth = true; // Prosody mod_auth_external only accepts sasl plain_auth!
		ServerRequest.Platform = FPlatformProperties::IniPlatformName();
		XmppConnection->SetServer(ServerRequest);

		XmppConnection->Login(UserAccountPtr->GetUserId()->ToString(), Result.SessionTicket);
		IOnlineChat* OnlineChat = PlayFabSubsystem->GetChatInterface().Get();
		FOnlineChatPlayFab* PlayFabChat = static_cast<FOnlineChatPlayFab*>(OnlineChat);
		PlayFabChat->XmppSetupDelegates(XmppConnection);
	}
}

void FOnlineIdentityPlayFab::OnErrorCallback_Login(const PlayFab::FPlayFabError& ErrorResult, int32 LocalUserNum)
{
	bAttemptingLogin = false;

	UE_LOG_ONLINE(Error, TEXT("FOnlineIdentityPlayFab::Login: Login failed for LocalUser: %d: %s"), LocalUserNum, *ErrorResult.ErrorMessage);

	TriggerOnLoginCompleteDelegates(LocalUserNum, false, FUniqueNetIdPlayFabId(TEXT("")), ErrorResult.ErrorMessage);
}
