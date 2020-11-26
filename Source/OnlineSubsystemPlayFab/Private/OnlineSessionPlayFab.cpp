// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSessionPlayFab.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemPlayFab.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSubsystemPlayFabSettings.h"
#include "SocketSubsystem.h"
#include "NboSerializerPlayFab.h"
#include "LANBeacon.h"
#include "Json.h"

// PlayFab
#include "PlayFab.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabServerAPI.h"
#include "Core/PlayFabServerDataModels.h"
#include "Core/PlayFabMatchmakerAPI.h"
#include "Core/PlayFabMatchmakerDataModels.h"


FString FOnlineAsyncTaskPingServer::ToString() const
{
	return FString::Printf(TEXT("FOnlineAsyncTaskPingServer bWasSuccessful: %d"), bWasSuccessful);
}

bool FOnlineAsyncTaskPingServer::IsDone() const
{
	return bIsComplete;
}

bool FOnlineAsyncTaskPingServer::WasSuccessful() const
{
	return bWasSuccessful;
}

void FOnlineAsyncTaskPingServer::Tick()
{
	if (bInit)
	{
		bInit = true;
		for (FString TargetHost : TargetHosts)
		{
			auto Delegate = FIcmpEchoResultDelegate::CreateRaw(this, &FOnlineAsyncTaskPingServer::ServerPingResult, TargetHost);
			FIcmp::IcmpEcho(TargetHost, 2.0f, Delegate); // the ICMP module will only handle one at a time
		}
	}
}

void FOnlineAsyncTaskPingServer::ServerPingResult(FIcmpEchoResult Result, FString Host)
{
	
}

FOnlineSessionInfoPlayFab::FOnlineSessionInfoPlayFab(EPlayFabSession::Type InSessionType)
	: SessionType(InSessionType)
	, HostAddr(NULL)
	, SessionId(TEXT("INVALID"))
{
}

FOnlineSessionInfoPlayFab::FOnlineSessionInfoPlayFab(EPlayFabSession::Type InSessionType, const FUniqueNetIdLobbyId& InSessionId, FString InMatchmakeTicket /* = "" */)
	: SessionType(InSessionType)
	, HostAddr(NULL)
	, SessionId(InSessionId)
	, MatchmakeTicket(InMatchmakeTicket)
{

}

void FOnlineSessionInfoPlayFab::Init(const FOnlineSubsystemPlayFab& Subsystem)
{
	HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

	bool bIsValid = false;
	FString cmdVal;
	if (FParse::Value(FCommandLine::Get(), TEXT("server_host_domain"), cmdVal))
	{
		HostAddr->SetIp(*cmdVal, bIsValid);
	}
	if (!bIsValid)
	{
		bool bCanBindAll = false;
		TSharedRef<FInternetAddr> LocalAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);

		uint32 HostIp = 0;
		LocalAddr->GetIp(HostIp); // will return in host order
								  // if this address is on loopback interface, advertise it as 127.0.0.1
		if ((HostIp & 0xff000000) == 0x7f000000)
		{
			HostAddr->SetIp(0x7f000001);	// 127.0.0.1
		}
	}

	HostAddr->SetPort(FURL::UrlConfig.DefaultPort);
}

void FOnlineSessionInfoPlayFab::InitLAN(const FOnlineSubsystemPlayFab& Subsystem)
{
	// Read the IP from the system
	bool bCanBindAll;
	HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);

	// The below is a workaround for systems that set hostname to a distinct address from 127.0.0.1 on a loopback interface.
	// See e.g. https://www.debian.org/doc/manuals/debian-reference/ch05.en.html#_the_hostname_resolution
	// and http://serverfault.com/questions/363095/why-does-my-hostname-appear-with-the-address-127-0-1-1-rather-than-127-0-0-1-in
	// Since we bind to 0.0.0.0, we won't answer on 127.0.1.1, so we need to advertise ourselves as 127.0.0.1 for any other loopback address we may have.
	uint32 HostIp = 0;
	HostAddr->GetIp(HostIp); // will return in host order
							 // if this address is on loopback interface, advertise it as 127.0.0.1
	if ((HostIp & 0xff000000) == 0x7f000000)
	{
		HostAddr->SetIp(0x7f000001);	// 127.0.0.1
	}

	// Now set the port that was configured
	HostAddr->SetPort(GetPortFromNetDriver(Subsystem.GetInstanceName()));

	SessionId = FUniqueNetIdLobbyId(FGuid::NewGuid().ToString());
}

void FOnlineSessionPlayFab::OnSuccessCallback_Client_GetCurrentGames(const PlayFab::ClientModels::FCurrentGamesResult& Result)
{
	FOnlineSessionSettings NewServer;
	if (CurrentSessionSearch.IsValid())
	{
		for (PlayFab::ClientModels::FGameInfo GameInfo : Result.Games)
		{
			// Add space in the search results array
			FOnlineSessionSearchResult* NewResult = new (CurrentSessionSearch->SearchResults) FOnlineSessionSearchResult();
			// this is not a correct ping, but better than nothing
			NewResult->PingInMs = static_cast<int32>((FPlatformTime::Seconds() - SessionSearchStartInSeconds) * 1000);

			FOnlineSession* NewSession = &NewResult->Session;
			//GameInfo.Tags
			/** Owner of the session */
			// Right now we're only doing dedi servers, so... no owner(for now)
			//NewSession->OwningUserId = MakeShareable(new FUniqueNetIdPlayFabId(""));
			//NewSession->OwningUserName =

			/** Available Slots */
			NewSession->NumOpenPrivateConnections = 0;
			NewSession->NumOpenPublicConnections = GameInfo.MaxPlayers - GameInfo.PlayerUserIds.Num();

			FOnlineSessionInfoPlayFab* PlayFabSessionInfo = new FOnlineSessionInfoPlayFab(EPlayFabSession::AdvertisedSessionClient, FUniqueNetIdLobbyId(GameInfo.LobbyID));
			PlayFabSessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
			bool bIsValid;
			PlayFabSessionInfo->HostAddr->SetIp(*GameInfo.ServerPublicDNSName, bIsValid);
			PlayFabSessionInfo->HostAddr->SetPort((!GameInfo.ServerPort.isNull() && GameInfo.ServerPort.mValue !=0) ? GameInfo.ServerPort.mValue : 7777);
			NewSession->SessionInfo = MakeShareable(PlayFabSessionInfo);

			FOnlineSessionSettings& SessionSettings = NewSession->SessionSettings;

			// Clear out any old settings
			SessionSettings.Settings.Empty();

			TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(GameInfo.GameServerData);
			TSharedPtr<FJsonObject> JsonObj;
			if (FJsonSerializer::Deserialize(Reader, JsonObj))
			{
				JsonObj->TryGetStringField(SETTING_SERVERNAME.ToString(), NewSession->OwningUserName);

				// Read all the data
				SessionSettings.bAllowInvites = JsonObj->GetBoolField("bAllowInvites");
				SessionSettings.bAllowJoinInProgress = JsonObj->GetBoolField("bAllowJoinInProgress");
				SessionSettings.bAllowJoinViaPresence = JsonObj->GetBoolField("bAllowJoinViaPresence");
				SessionSettings.bAllowJoinViaPresenceFriendsOnly = JsonObj->GetBoolField("bAllowJoinViaPresenceFriendsOnly");
				SessionSettings.bAntiCheatProtected = JsonObj->GetBoolField("bAntiCheatProtected");
				SessionSettings.bIsDedicated = JsonObj->GetBoolField("bIsDedicated");
				SessionSettings.bIsLANMatch = false;
				SessionSettings.bShouldAdvertise = JsonObj->GetBoolField("bShouldAdvertise");
				SessionSettings.bUsesPresence = JsonObj->GetBoolField("bUsesPresence");
				SessionSettings.bUsesStats = JsonObj->GetBoolField("bUsesStats");
				SessionSettings.NumPrivateConnections = 0;
				SessionSettings.NumPublicConnections = GameInfo.MaxPlayers - GameInfo.PlayerUserIds.Num();

				// BuildId
				SessionSettings.BuildUniqueId = JsonObj->GetNumberField("BuildUniqueId");

				TArray<TSharedPtr<FJsonValue>> Settings = JsonObj->GetArrayField("Settings");
				for (TSharedPtr<FJsonValue> SettingVal : Settings)
				{
					TSharedPtr<FJsonObject> Setting = SettingVal->AsObject();
					SessionSettings.Settings.FindOrAdd(FName(*Setting->GetStringField("Key"))).Data.FromJson(Setting->GetObjectField("Value").ToSharedRef());
				}
			}

			/*for (auto Setting : Tags)
			{
				FVariantData Data;
				if (Data.FromString(Setting.Value))
				{
					SessionSettings.Settings.FindOrAdd(FName(*Setting.Key)).Data = Data;
				}
			}*/

		}

		CurrentSessionSearch->SortSearchResults();
		CurrentSessionSearch = nullptr;
		TriggerOnFindSessionsCompleteDelegates(true);
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Couldn't find CurrentSessionSearch"));
	}

	TriggerOnFindSessionsCompleteDelegates(false);

	if (CurrentSessionSearch.IsValid())
	{
		CurrentSessionSearch = NULL;
	}
}

void FOnlineSessionPlayFab::OnSuccessCallback_Client_Matchmake(const PlayFab::ClientModels::FMatchmakeResult& Result, FName SessionName)
{
	if (SessionName != CurrentMatchmakeName)
	{
		TriggerOnMatchmakingCompleteDelegates(SessionName, false);
		return;
	}

	FOnlineSessionSearchResult NewResult;
	FOnlineSession NewSession;

	FOnlineSessionInfoPlayFab* PlayFabSessionInfo = new FOnlineSessionInfoPlayFab(EPlayFabSession::AdvertisedSessionClient, FUniqueNetIdLobbyId(Result.LobbyID), Result.Ticket);
	PlayFabSessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool bIsValid;
	PlayFabSessionInfo->HostAddr->SetIp(*Result.ServerIPV4Address, bIsValid);
	PlayFabSessionInfo->HostAddr->SetPort((!Result.ServerPort.isNull() && Result.ServerPort.mValue != 0) ? Result.ServerPort.mValue : 7777);

	NewSession.SessionInfo = MakeShareable(PlayFabSessionInfo);
	NewResult.Session = NewSession;

	CurrentMatchmakeSearch->SearchState = EOnlineAsyncTaskState::Done;
	CurrentMatchmakeSearch->SearchResults.Add(NewResult);
	CurrentMatchmakeSearch = nullptr; // null out our pointer so we can start new matchmaking/find sessions
	TriggerOnMatchmakingCompleteDelegates(SessionName, true);
}

void FOnlineSessionPlayFab::OnErrorCallback_Client(const PlayFab::FPlayFabCppError& ErrorResult, FName FunctionName)
{
	OnErrorCallback_Client(ErrorResult, FunctionName, NAME_None);
}

void FOnlineSessionPlayFab::OnErrorCallback_Client(const PlayFab::FPlayFabCppError& ErrorResult, FName FunctionName, FName SessionName)
{
	UE_LOG_ONLINE(Error, TEXT("PlayFabClient: Function \"%s\" error: %s"), *FunctionName.ToString(), *ErrorResult.GenerateErrorReport());

	if (FunctionName == "GetCurrentGames")
	{
		if (CurrentSessionSearch.IsValid())
		{
			CurrentSessionSearch = NULL;
		}
		TriggerOnFindSessionsCompleteDelegates(false);
	}
	if (FunctionName == "Matchmake")
	{
		CurrentMatchmakeSearch = nullptr;
		TriggerOnMatchmakingCompleteDelegates(SessionName, false);
	}
}

void FOnlineSessionPlayFab::OnSuccessCallback_Server_RegisterGame(const PlayFab::ServerModels::FRegisterGameResponse& Result, FName SessionName)
{
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// Setup the host session info
		UE_LOG_ONLINE(Log, TEXT("Received new LobbyId: %s"), *Result.LobbyId);
		FOnlineSessionInfoPlayFab* NewSessionInfo = new FOnlineSessionInfoPlayFab(EPlayFabSession::AdvertisedSessionHost, FUniqueNetIdLobbyId(*Result.LobbyId));
		NewSessionInfo->Init(*PlayFabSubsystem);
		Session->SessionInfo = MakeShareable(NewSessionInfo);

		Session->SessionState = EOnlineSessionState::Pending;
		RegisterLocalPlayers(Session);

		TimerDelegate_PlayFabHeartbeat = FTimerDelegate::CreateRaw(this, &FOnlineSessionPlayFab::PlayFab_Server_Heartbeat, SessionName);
		UWorld* World = GetWorldForOnline(PlayFabSubsystem->GetInstanceName());
		if (World != nullptr)
		{
			float HeartBeatInterval = 60;
			if (!GConfig->GetFloat(TEXT("OnlineSubsystemPlayFab"), TEXT("HeartBeatInterval"), HeartBeatInterval, GEngineIni))
			{
				HeartBeatInterval = 60.f;
			}
			World->GetTimerManager().SetTimer(TimerHandle_PlayFabHeartbeat, TimerDelegate_PlayFabHeartbeat, HeartBeatInterval, true);
		}

		TriggerOnCreateSessionCompleteDelegates(SessionName, true);

		// Update the server data
		UpdateSession(SessionName, Session->SessionSettings, true);
	}
	else
	{
		TriggerOnCreateSessionCompleteDelegates(SessionName, false);
	}
}

void FOnlineSessionPlayFab::OnSuccessCallback_Server_DeregisterGame(const PlayFab::ServerModels::FDeregisterGameResponse& Result, FName SessionName)
{
	RemoveNamedSession(SessionName);
	TriggerOnDestroySessionCompleteDelegates(SessionName, true);
}

void FOnlineSessionPlayFab::OnSuccessCallback_Server_InstanceState(const PlayFab::ServerModels::FSetGameServerInstanceStateResult& Result, FName SessionName)
{
	if (GetNamedSession(SessionName)->SessionState == EOnlineSessionState::InProgress)
		TriggerOnStartSessionCompleteDelegates(SessionName, true);
	if (GetNamedSession(SessionName)->SessionState == EOnlineSessionState::Ending)
		TriggerOnEndSessionCompleteDelegates(SessionName, true);
}

void FOnlineSessionPlayFab::OnSuccessCallback_Server_InstanceData(const PlayFab::ServerModels::FSetGameServerInstanceDataResult& Result, FName SessionName)
{
	TriggerOnUpdateSessionCompleteDelegates(SessionName, true);
}

void FOnlineSessionPlayFab::OnSuccessCallback_Server_AuthenticateSessionTicket(const PlayFab::ServerModels::FAuthenticateSessionTicketResult& Result, FName SessionName)
{
	TriggerOnAuthenticatePlayerCompleteDelegates(FUniqueNetIdPlayFabId(Result.UserInfo->PlayFabId), true);
}

void FOnlineSessionPlayFab::OnSuccessCallback_Server_RedeemMatchmakerTicket(const PlayFab::ServerModels::FRedeemMatchmakerTicketResult& Result, FName SessionName)
{
	TriggerOnAuthenticatePlayerCompleteDelegates(FUniqueNetIdPlayFabId(Result.UserInfo->PlayFabId), Result.TicketIsValid);
}

void FOnlineSessionPlayFab::OnErrorCallback_Server(const PlayFab::FPlayFabCppError& ErrorResult, FName FunctionName, FName SessionName)
{
	OnErrorCallback_Server(ErrorResult, FunctionName, SessionName, MakeShareable(new FUniqueNetIdPlayFabId()));
}

void FOnlineSessionPlayFab::OnErrorCallback_Server(const PlayFab::FPlayFabCppError& ErrorResult, FName FunctionName, FName SessionName, TSharedRef<FUniqueNetId> PlayerId)
{
	UE_LOG_ONLINE(Error, TEXT("PlayFabServer: Function \"%s\" error: %s"), *FunctionName.ToString(), *ErrorResult.ErrorMessage);

	if (FunctionName == "RegisterGame")
	{
		RemoveNamedSession(SessionName);
		TriggerOnCreateSessionCompleteDelegates(SessionName, false);
	}
	else if (FunctionName == "DeregisterGame")
	{
		TriggerOnDestroySessionCompleteDelegates(SessionName, false);
	}
	else if (FunctionName == "SetGameServerInstanceState")
	{
		if (GetNamedSession(SessionName)->SessionState == EOnlineSessionState::InProgress)
			TriggerOnStartSessionCompleteDelegates(SessionName, false);
		if (GetNamedSession(SessionName)->SessionState == EOnlineSessionState::Ending)
			TriggerOnEndSessionCompleteDelegates(SessionName, false);
	}
	else if (FunctionName == "SetGameServerInstanceData")
	{
		TriggerOnUpdateSessionCompleteDelegates(SessionName, false);
	}
	else if (FunctionName == "AuthenticateSessionTicket")
	{
		TriggerOnAuthenticatePlayerCompleteDelegates(PlayerId.Get(), false);
	}
	else if (FunctionName == "RedeemMatchmakerTicket")
	{
		TriggerOnAuthenticatePlayerCompleteDelegates(PlayerId.Get(), false);
	}
}

void FOnlineSessionPlayFab::PlayFab_Server_Heartbeat(FName SessionName)
{
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());

		PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();
		if (ServerAPI.IsValid())
		{
			PlayFab::ServerModels::FRefreshGameServerInstanceHeartbeatRequest Request;
			Request.LobbyId = SessionInfo->SessionId.ToString();

			// Only specify an errordelegate, it's a passive function so if should never failed unless our game went invalid
			auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, FName("RefreshGameServerInstanceHeartbeat"), SessionName);
			ServerAPI->RefreshGameServerInstanceHeartbeat(Request, nullptr, ErrorDelegate);
		}
	}
}

void FOnlineSessionPlayFab::PlayerJoined(const FUniqueNetId& PlayerId, FName SessionName)
{
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());

		bool bUsesCustomMatchmaker = false;
		GConfig->GetBool(TEXT("OnlineSubsystemPlayFab"), TEXT("bCustomMatchmaker"), bUsesCustomMatchmaker, GEngineIni);

		if (bUsesCustomMatchmaker)
		{
			PlayFabMatchmakerPtr MatchmakerAPI = IPlayFabModuleInterface::Get().GetMatchmakerAPI();
			if (MatchmakerAPI.IsValid())
			{
				PlayFab::MatchmakerModels::FPlayerJoinedRequest Request;
				Request.LobbyId = SessionInfo->SessionId.ToString();
				Request.PlayFabId = PlayerId.ToString();

				MatchmakerAPI->PlayerJoined(Request);
			}
		}
		else
		{
			// PlayerJoined event is called when authenticated when not using a custom matchmaker
		}
	}
}

void FOnlineSessionPlayFab::PlayerLeft(const FUniqueNetId& PlayerId, FName SessionName)
{
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());

		bool bUsesCustomMatchmaker = false;
		GConfig->GetBool(TEXT("OnlineSubsystemPlayFab"), TEXT("bCustomMatchmaker"), bUsesCustomMatchmaker, GEngineIni);

		if (bUsesCustomMatchmaker)
		{
			PlayFabMatchmakerPtr MatchmakerAPI = IPlayFabModuleInterface::Get().GetMatchmakerAPI();
			if (MatchmakerAPI.IsValid())
			{
				PlayFab::MatchmakerModels::FPlayerLeftRequest Request;
				Request.LobbyId = SessionInfo->SessionId.ToString();
				Request.PlayFabId = PlayerId.ToString();

				MatchmakerAPI->PlayerLeft(Request);
			}
		}
		else
		{
			PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();
			if (ServerAPI.IsValid())
			{
				PlayFab::ServerModels::FNotifyMatchmakerPlayerLeftRequest Request;
				Request.LobbyId = SessionInfo->SessionId.ToString();
				Request.PlayFabId = PlayerId.ToString();

				ServerAPI->NotifyMatchmakerPlayerLeft(Request);
			}
		}
	}
}

bool FOnlineSessionPlayFab::AuthenticatePlayer(const FUniqueNetId& PlayerId, FName SessionName, FString AuthTicket, bool bIsMatchmakeTicket)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	uint32 Result = E_FAIL;

	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		if (AuthTicket.IsEmpty())
		{
			// No ticket, no admittance
			TriggerOnAuthenticatePlayerCompleteDelegates(PlayerId, false);
		}
		FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());

		bool bUsesCustomMatchmaker = false;
		GConfig->GetBool(TEXT("OnlineSubsystemPlayFab"), TEXT("CustomMatchmaker"), bUsesCustomMatchmaker, GEngineIni);

		if (bUsesCustomMatchmaker)
		{
			// Right now, matchmaker API doesn't actually do shit without a direct connection...
			Result = ERROR_SUCCESS;
			TriggerOnAuthenticatePlayerCompleteDelegates(PlayerId, true);
		}
		else
		{
			PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();
			if (ServerAPI.IsValid())
			{
				TSharedRef<FUniqueNetId> PlayerIdRef = MakeShareable(new FUniqueNetIdPlayFabId(PlayerId));

				// Honestly, no need to authenticate both. If there is a matchmaker ticket, they are valid
				// only up to 2 minutes or so, versus session tickets being 24 hrs.
				// so it's technically more reliable to authenticate via the matchmake ticket.
				if (bIsMatchmakeTicket)
				{
					PlayFab::ServerModels::FRedeemMatchmakerTicketRequest Request;
					Request.LobbyId = SessionInfo->SessionId.ToString();
					Request.Ticket = AuthTicket;

					auto SuccessDelegate = PlayFab::UPlayFabServerAPI::FRedeemMatchmakerTicketDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnSuccessCallback_Server_RedeemMatchmakerTicket, FName(*Session->SessionName.ToString()));
					auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, FName("RedeemMatchmakerTicket"), FName(*Session->SessionName.ToString()), PlayerIdRef);
					ServerAPI->RedeemMatchmakerTicket(Request, SuccessDelegate, ErrorDelegate);
				}
				else
				{
					PlayFab::ServerModels::FAuthenticateSessionTicketRequest Request;
					Request.SessionTicket = AuthTicket;

					auto SuccessDelegate = PlayFab::UPlayFabServerAPI::FAuthenticateSessionTicketDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnSuccessCallback_Server_AuthenticateSessionTicket, FName(*Session->SessionName.ToString()));
					auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, FName("AuthenticateSessionTicket"), FName(*Session->SessionName.ToString()), PlayerIdRef);
					ServerAPI->AuthenticateSessionTicket(Request, SuccessDelegate, ErrorDelegate);
				}
				Result = ERROR_IO_PENDING;
			}
		}
	}
	if (Result != ERROR_IO_PENDING)
	{
		TriggerOnAuthenticatePlayerCompleteDelegates(PlayerId, (Result == ERROR_SUCCESS) ? true : false);
	}
	return (Result == ERROR_SUCCESS || Result == ERROR_IO_PENDING) ? true : false;
}

bool FOnlineSessionPlayFab::CreateSession(int32 HostingPlayerNum, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	uint32 Result = E_FAIL;

	// Check for an existing session
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session == NULL)
	{
		// Create a new session and deep copy the game settings
		Session = AddNamedSession(SessionName, NewSessionSettings);
		check(Session);
		Session->SessionState = EOnlineSessionState::Creating;
		Session->NumOpenPrivateConnections = NewSessionSettings.NumPrivateConnections;
		Session->NumOpenPublicConnections = NewSessionSettings.NumPublicConnections;	// always start with full public connections, local player will register later

		Session->HostingPlayerNum = HostingPlayerNum;

		check(PlayFabSubsystem);
		IOnlineIdentityPtr Identity = PlayFabSubsystem->GetIdentityInterface();
		if (Identity.IsValid())
		{
			Session->OwningUserId = Identity->GetUniquePlayerId(HostingPlayerNum);
			Session->OwningUserName = Identity->GetPlayerNickname(HostingPlayerNum);
		}

		// if did not get a valid one, use just something
		if (!Session->OwningUserId.IsValid())
		{
			Session->OwningUserId = MakeShareable(new FUniqueNetIdPlayFabId(FString::Printf(TEXT("%d"), HostingPlayerNum)));
			Session->OwningUserName = FString(TEXT(""));
		}

		// Unique identifier of this build for compatibility
		Session->SessionSettings.BuildUniqueId = GetBuildUniqueId();

		if (!Session->SessionSettings.bIsLANMatch)
		{
			Result = CreateInternetSession(HostingPlayerNum, Session);
		}
		else
		{
			Result = CreateLANSession(HostingPlayerNum, Session);
		}

		if (Result != ERROR_IO_PENDING)
		{
			// Set the game state as pending (not started)
			Session->SessionState = EOnlineSessionState::Pending;

			if (Result != ERROR_SUCCESS)
			{
				// Clean up the session info so we don't get into a confused state
				RemoveNamedSession(SessionName);
			}
			else
			{
				RegisterLocalPlayers(Session);
			}
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("CreateSession: Cannot create session '%s': session already exists."), *SessionName.ToString());
	}

	if (Result != ERROR_IO_PENDING)
	{
		TriggerOnCreateSessionCompleteDelegates(SessionName, (Result == ERROR_SUCCESS) ? true : false);
	}

	return Result == ERROR_IO_PENDING || Result == ERROR_SUCCESS;
}

bool FOnlineSessionPlayFab::CreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	// Shouldn't need proper HostingPlayerNum, why the hell would we create multiple session on one game instance?
	return CreateSession(0, SessionName, NewSessionSettings);
}

uint32 FOnlineSessionPlayFab::CreateInternetSession(int32 HostingPlayerNum, class FNamedOnlineSession* Session)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	uint32 Result = E_FAIL;

	// Only allowed one published session with PlayFab
	FNamedOnlineSession* MasterSession = GetGameServerSession();
	if (MasterSession == NULL)
	{
		if (Session->SessionSettings.bIsDedicated)
		{
			PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();

			if (ServerAPI.IsValid())
			{
				Session->SessionSettings.Get(SETTING_SERVERNAME, Session->OwningUserName);

				FString cmdVal;
				if (FParse::Value(FCommandLine::Get(), TEXT("game_id"), cmdVal))
				{
					// Server is already registered with PlayFab, update data
					FOnlineSessionInfoPlayFab* NewSessionInfo = new FOnlineSessionInfoPlayFab(EPlayFabSession::AdvertisedSessionPlayFab, FUniqueNetIdLobbyId(*cmdVal));
					NewSessionInfo->Init(*PlayFabSubsystem);
					Session->SessionInfo = MakeShareable(NewSessionInfo);
					Session->SessionState = EOnlineSessionState::Pending;
					RegisterLocalPlayers(Session);
					UpdateSession(FName(*Session->SessionName.ToString()), Session->SessionSettings, true);
					TriggerOnCreateSessionCompleteDelegates(FName(*Session->SessionName.ToString()), true);
					return ERROR_SUCCESS;
				}

				// Server isn't registered with PlayFab, let's register it
				PlayFab::ServerModels::FRegisterGameRequest Request;

				// Add tags when session is created, after creation we call Update Session to set all server data as well
				FString MapName;
				if (Session->SessionSettings.Get(SETTING_MAPNAME, MapName) && !MapName.IsEmpty())
					Request.Tags.Add(SETTING_MAPNAME.ToString(), MapName);

				Request.Build = PlayFabSubsystem->GetBuildVersion();
				if (Session->SessionSettings.Settings.Find(SETTING_GAMENAME))
				{
					Request.GameMode = Session->SessionSettings.Settings[SETTING_GAMENAME].Data.ToString();
				}
				else if (Session->SessionSettings.Settings.Find(SETTING_GAMEMODE))
				{
					Request.GameMode = Session->SessionSettings.Settings[SETTING_GAMEMODE].Data.ToString();
				}
				else
				{
					UE_LOG_ONLINE(Error, TEXT("CreateInternetSession: Can not register game without either SETTING_GAMENAME or SETTING_GAMEMODE set."));
					return E_FAIL;
				}

				const FOnlineSessionSetting* RegionSetting = Session->SessionSettings.Settings.Find(SETTING_REGION);
				if (RegionSetting != nullptr)
				{
					Request.pfRegion = PlayFab::ServerModels::readRegionFromValue(RegionSetting->ToString());
				}

				bool bCanBindAll;
				TSharedPtr<class FInternetAddr> HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);
				if (FParse::Value(FCommandLine::Get(), TEXT("server_host_domain"), cmdVal))
				{
					bool bIsValid;
					HostAddr->SetIp(*cmdVal, bIsValid);
				}
				HostAddr->SetPort(FURL::UrlConfig.DefaultPort);

				Request.ServerIPV4Address = HostAddr->ToString(false);
				int32 port = HostAddr->GetPort();
				port = port != 0 ? port : 7777;
				Request.ServerPort = FString::FromInt(port);

				auto SuccessDelegate = PlayFab::UPlayFabServerAPI::FRegisterGameDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnSuccessCallback_Server_RegisterGame, FName(*Session->SessionName.ToString()));
				auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, FName("RegisterGame"), FName(*Session->SessionName.ToString()));
				ServerAPI->RegisterGame(Request, SuccessDelegate, ErrorDelegate);

				Result = ERROR_IO_PENDING;
			}
			else
			{
				UE_LOG_ONLINE(Verbose, TEXT("CreateInternetSession: Failed to initialize game server with PlayFab."));
			}
		}
		else
		{
			UE_LOG_ONLINE(Verbose, TEXT("CreateInternetSession: Can't register a non-dedicated server through PlayFab services!"));
		}
	}
	else
	{
		UE_LOG_ONLINE(Verbose, TEXT("CreateInternetSession: Advertised session %s already exists, unable to create another."), *Session->SessionName.ToString());
	}

	return Result;
}

uint32 FOnlineSessionPlayFab::CreateLANSession(int32 HostingPlayerNum, class FNamedOnlineSession* Session)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	check(Session);
	uint32 Result = ERROR_SUCCESS;

	// Setup the host session info
	FOnlineSessionInfoPlayFab* NewSessionInfo = new FOnlineSessionInfoPlayFab(EPlayFabSession::LANSession);
	NewSessionInfo->InitLAN(*PlayFabSubsystem);
	Session->SessionInfo = MakeShareable(NewSessionInfo);

	// Don't create a the beacon if advertising is off
	if (Session->SessionSettings.bShouldAdvertise)
	{
		if (!LANSession)
		{
			LANSession = new FLANSession();
		}

		FOnValidQueryPacketDelegate QueryPacketDelegate = FOnValidQueryPacketDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnValidQueryPacketReceived);
		if (!LANSession->Host(QueryPacketDelegate))
		{
			Result = E_FAIL;
		}
	}

	return Result;
}

TSharedPtr<const FUniqueNetId> FOnlineSessionPlayFab::CreateSessionIdFromString(const FString& SessionIdStr)
{
	ensureMsgf(false, TEXT("NYI"));
	TSharedPtr<const FUniqueNetId> SessionId;
	return SessionId;
}


bool FOnlineSessionPlayFab::StartSession(FName SessionName)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	uint32 Result = E_FAIL;
	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// Can't start a match multiple times
		if (Session->SessionState == EOnlineSessionState::Pending || Session->SessionState == EOnlineSessionState::Ended)
		{
			if (!Session->SessionSettings.bIsLANMatch)
			{
				Session->SessionState = EOnlineSessionState::InProgress;
				FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());

				//Result = ERROR_SUCCESS;
				PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();

				if (ServerAPI.IsValid())
				{
					PlayFab::ServerModels::FSetGameServerInstanceStateRequest Request;
					Request.LobbyId = SessionInfo->SessionId.ToString();
					Request.State = PlayFab::ServerModels::GameInstanceState::GameInstanceStateOpen;

					auto SuccessDelegate = PlayFab::UPlayFabServerAPI::FSetGameServerInstanceStateDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnSuccessCallback_Server_InstanceState, FName(*Session->SessionName.ToString()));
					auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, FName("SetGameServerInstanceState"), FName(*Session->SessionName.ToString()));
					ServerAPI->SetGameServerInstanceState(Request, SuccessDelegate, ErrorDelegate);
					Result = ERROR_IO_PENDING;
				}
			}
			else
			{
				// If this lan match has join in progress disabled, shut down the beacon
				if (!Session->SessionSettings.bAllowJoinInProgress)
				{
					LANSession->StopLANSession();
				}
				Result = ERROR_SUCCESS;
				Session->SessionState = EOnlineSessionState::InProgress;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("StartSession: Can't start an online session (%s) in state %s"), *SessionName.ToString(), EOnlineSessionState::ToString(Session->SessionState));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("StartSession: Can't start an online game for session (%s) that hasn't been created"), *SessionName.ToString());
	}

	if (Result != ERROR_IO_PENDING)
	{
		// Just trigger the delegate
		TriggerOnStartSessionCompleteDelegates(SessionName, (Result == ERROR_SUCCESS) ? true : false);
	}

	return Result == ERROR_SUCCESS || Result == ERROR_IO_PENDING;
}

bool FOnlineSessionPlayFab::UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData)
{
	bool bWasSuccessful = true;

	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// Worry about properly owning the session another time
		/*TSharedPtr<const FUniqueNetId> UniqueId = PlayFabSubsystem->GetIdentityInterface()->GetUniquePlayerId(0);
		if (!UniqueId.IsValid() || *Session->OwningUserId != *UniqueId)
		{
			UE_LOG_ONLINE(Warning, TEXT("Need to own session (%s) before updating.  Current Owner: %s"), *SessionName.ToString(), *Session->OwningUserName);
			return false;
		}*/
		if (!Session->SessionSettings.bIsLANMatch)
		{
			bool bUsesPresence = Session->SessionSettings.bUsesPresence;
			if (bUsesPresence != UpdatedSessionSettings.bUsesPresence)
			{
				UE_LOG_ONLINE(Warning, TEXT("Can't change presence settings on existing session %s, ignoring."), *SessionName.ToString());
				UpdatedSessionSettings.bUsesPresence = bUsesPresence;
			}

			Session->SessionSettings = UpdatedSessionSettings;

			if (bShouldRefreshOnlineData)
			{
				UpdateInternetSession(Session);
			}
		}
		else
		{
			// TODO: Care about LAN servers
			Session->SessionSettings = UpdatedSessionSettings;
			TriggerOnUpdateSessionCompleteDelegates(SessionName, bWasSuccessful);
		}
	}

	return bWasSuccessful;
}

uint32 FOnlineSessionPlayFab::UpdateInternetSession(FNamedOnlineSession* Session)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	uint32 Result = E_FAIL;

	FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());

	if (SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionPlayFab || SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionHost)
	{
		// Game server update
		PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();

		if (ServerAPI.IsValid())
		{
			PlayFab::ServerModels::FSetGameServerInstanceTagsRequest Request;
			PlayFab::ServerModels::FSetGameServerInstanceDataRequest DataRequest;

			Request.LobbyId = SessionInfo->SessionId.ToString();
			DataRequest.LobbyId = SessionInfo->SessionId.ToString();

			// Only add tags when applicable
			if (Session->SessionSettings.bAllowInvites)
				Request.Tags.Add("bAllowInvites", TEXT("true"));
			if (Session->SessionSettings.bAllowJoinInProgress)
				Request.Tags.Add("bAllowJoinInProgress", TEXT("true"));
			if (Session->SessionSettings.bAllowJoinViaPresence)
				Request.Tags.Add("bAllowJoinViaPresence", TEXT("true"));
			if (Session->SessionSettings.bAllowJoinViaPresenceFriendsOnly)
				Request.Tags.Add("bAllowJoinViaPresenceFriendsOnly", TEXT("true"));
			if (Session->SessionSettings.bAntiCheatProtected)
				Request.Tags.Add("bAntiCheatProtected", TEXT("true"));
			if (Session->SessionSettings.bIsDedicated)
				Request.Tags.Add("bIsDedicated", TEXT("true"));
			if (Session->SessionSettings.bShouldAdvertise)
				Request.Tags.Add("bShouldAdvertise", TEXT("true"));
			if (Session->SessionSettings.bUsesPresence)
				Request.Tags.Add("bUsesPresence", TEXT("true"));
			if (Session->SessionSettings.bUsesStats)
				Request.Tags.Add("bUsesStats", TEXT("true"));

			Request.Tags.Add("bEmpty", (Session->RegisteredPlayers.Num() == 0) ? TEXT("true") : TEXT("false"));

			TSharedPtr< FJsonObject > InstanceDataJsonObj = MakeShareable(new FJsonObject);

			InstanceDataJsonObj->SetBoolField("bAllowInvites", Session->SessionSettings.bAllowInvites);
			InstanceDataJsonObj->SetBoolField("bAllowJoinInProgress", Session->SessionSettings.bAllowJoinInProgress);
			InstanceDataJsonObj->SetBoolField("bAllowJoinViaPresence", Session->SessionSettings.bAllowJoinViaPresence);
			InstanceDataJsonObj->SetBoolField("bAllowJoinViaPresenceFriendsOnly", Session->SessionSettings.bAllowJoinViaPresenceFriendsOnly);
			InstanceDataJsonObj->SetBoolField("bAntiCheatProtected", Session->SessionSettings.bAntiCheatProtected);
			InstanceDataJsonObj->SetBoolField("bIsDedicated", Session->SessionSettings.bIsDedicated);
			InstanceDataJsonObj->SetBoolField("bShouldAdvertise", Session->SessionSettings.bShouldAdvertise);
			InstanceDataJsonObj->SetBoolField("bUsesPresence", Session->SessionSettings.bUsesPresence);
			InstanceDataJsonObj->SetBoolField("bUsesStats", Session->SessionSettings.bUsesStats);

			InstanceDataJsonObj->SetNumberField("BuildUniqueId", Session->SessionSettings.BuildUniqueId);

			TArray<TSharedPtr<FJsonValue>> SettingsJsonArray;

			for (FSessionSettings::TConstIterator It(Session->SessionSettings.Settings); It; ++It)
			{
				FName Key = It.Key();
				const FOnlineSessionSetting& Setting = It.Value();

				switch (Setting.AdvertisementType)
				{
				case EOnlineDataAdvertisementType::ViaOnlineServiceAndPing:
				case EOnlineDataAdvertisementType::ViaOnlineService: // ViaOnlineService will be server instance data
				{
					TSharedPtr< FJsonObject > JsonObj = MakeShareable(new FJsonObject);
					JsonObj->SetStringField("Key", Key.ToString());
					JsonObj->SetObjectField("Value", Setting.Data.ToJson());
					TSharedPtr< FJsonValue > JsonVal = MakeShareable(new FJsonValueObject(JsonObj));
					SettingsJsonArray.Add(JsonVal);

					// If not both, then break
					if (Setting.AdvertisementType != EOnlineDataAdvertisementType::ViaOnlineServiceAndPing)
						break;
				}
				case EOnlineDataAdvertisementType::ViaPingOnly: // ViaPing will be server instance tags(tags can be used to filter server list)
				{
					Request.Tags.Add(Key.ToString(), Setting.Data.ToString());

					// If not both, then break
					if (Setting.AdvertisementType != EOnlineDataAdvertisementType::ViaOnlineServiceAndPing)
						break;
				}
				case EOnlineDataAdvertisementType::DontAdvertise:
				default:
					break;
				}
			}
			InstanceDataJsonObj->SetArrayField("Settings", SettingsJsonArray);

			FString OutputString;
			TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutputString);
			FJsonSerializer::Serialize(InstanceDataJsonObj.ToSharedRef(), Writer);
			DataRequest.GameServerData = OutputString;

			// We must send them ALL again, as this will overwrite all tags(any not written will be nil)
			auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, FName("SetGameServerInstanceTags"), FName(*Session->SessionName.ToString()));
			ServerAPI->SetGameServerInstanceTags(Request, NULL, ErrorDelegate);
			auto DataSuccessDelegate = PlayFab::UPlayFabServerAPI::FSetGameServerInstanceDataDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnSuccessCallback_Server_InstanceData, FName(*Session->SessionName.ToString()));
			auto DataErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, FName("SetGameServerInstanceData"), FName(*Session->SessionName.ToString()));
			ServerAPI->SetGameServerInstanceData(DataRequest, DataSuccessDelegate, DataErrorDelegate);
		}
	}

	return Result;
}

bool FOnlineSessionPlayFab::EndSession(FName SessionName)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	uint32 Result = E_FAIL;

	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// Can't end a match that isn't in progress
		if (Session->SessionState == EOnlineSessionState::InProgress)
		{
			Session->SessionState = EOnlineSessionState::Ending;

			if (!Session->SessionSettings.bIsLANMatch)
			{
				FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());

				//Result = ERROR_SUCCESS;
				PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();

				if (ServerAPI.IsValid())
				{
					PlayFab::ServerModels::FSetGameServerInstanceStateRequest Request;
					Request.LobbyId = SessionInfo->SessionId.ToString();
					Request.State = PlayFab::ServerModels::GameInstanceState::GameInstanceStateClosed;

					auto SuccessDelegate = PlayFab::UPlayFabServerAPI::FSetGameServerInstanceStateDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnSuccessCallback_Server_InstanceState, FName(*Session->SessionName.ToString()));
					auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, FName("SetGameServerInstanceState"), FName(*Session->SessionName.ToString()));
					ServerAPI->SetGameServerInstanceState(Request, SuccessDelegate, ErrorDelegate);
					Result = ERROR_IO_PENDING;
				}
			}
			else
			{
				// If the session should be advertised and the lan beacon was destroyed, recreate
				if (Session->SessionSettings.bShouldAdvertise && LANSession->LanBeacon == NULL && PlayFabSubsystem->IsServer())
				{
					// Recreate the beacon
					Result = CreateLANSession(Session->HostingPlayerNum, Session);
				}
				else
				{
					Result = ERROR_SUCCESS;
				}
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("EndSession: Can't end session (%s) in state %s"),
				*SessionName.ToString(),
				EOnlineSessionState::ToString(Session->SessionState));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("EndSession: Can't end an online game for session (%s) that hasn't been created"),
			*SessionName.ToString());
	}

	if (Result != ERROR_IO_PENDING)
	{
		if (Session)
		{
			Session->SessionState = EOnlineSessionState::Ended;
		}

		TriggerOnEndSessionCompleteDelegates(SessionName, (Result == ERROR_SUCCESS) ? true : false);
	}

	return Result == ERROR_SUCCESS || Result == ERROR_IO_PENDING;
}

bool FOnlineSessionPlayFab::DestroySession(FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	uint32 Result = E_FAIL;
	// Find the session in question
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		if (Session->SessionState != EOnlineSessionState::Destroying)
		{
			if (!Session->SessionSettings.bIsLANMatch)
			{
				Result = DestroyInternetSession(Session, CompletionDelegate);
			}
			else
			{
				if (LANSession)
				{
					// Tear down the LAN beacon
					LANSession->StopLANSession();
					delete LANSession;
					LANSession = NULL;
				}

				Result = ERROR_SUCCESS;
			}

			if (Result != ERROR_IO_PENDING)
			{
				// The session info is no longer needed
				RemoveNamedSession(Session->SessionName);

				CompletionDelegate.ExecuteIfBound(SessionName, (Result == ERROR_SUCCESS) ? true : false);
				TriggerOnDestroySessionCompleteDelegates(SessionName, (Result == ERROR_SUCCESS) ? true : false);
			}
		}
		else
		{
			// Purposefully skip the delegate call as one should already be in flight
			UE_LOG_ONLINE(Warning, TEXT("DestroySession: Already in process of destroying session (%s)"), *SessionName.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("DestroySession: Can't destroy a null online session (%s)"), *SessionName.ToString());
		CompletionDelegate.ExecuteIfBound(SessionName, false);
		TriggerOnDestroySessionCompleteDelegates(SessionName, false);
	}

	return Result == ERROR_SUCCESS || Result == ERROR_IO_PENDING;
}

uint32 FOnlineSessionPlayFab::DestroyInternetSession(FNamedOnlineSession* Session, const FOnDestroySessionCompleteDelegate& CompletionDelegate)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	Session->SessionState = EOnlineSessionState::Destroying;

	if (Session->SessionInfo.IsValid())
	{
		FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());

		if (SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionPlayFab)
		{
			// PlayFab will know when the process is closed, no action should be required
		}
		else if (SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionHost)
		{
			// External Servers need to remove the session from matchmaker
			PlayFabServerPtr ServerAPI = PlayFabSubsystem->GetServerAPI();
			if (ServerAPI.IsValid())
			{
				PlayFab::ServerModels::FDeregisterGameRequest Request;
				Request.LobbyId = SessionInfo->SessionId.ToString();

				auto SuccessDelegate = PlayFab::UPlayFabServerAPI::FDeregisterGameDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnSuccessCallback_Server_DeregisterGame, FName(*Session->SessionName.ToString()));
				auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, FName("DeregisterGame"), FName(*Session->SessionName.ToString()));
				ServerAPI->DeregisterGame(Request, SuccessDelegate, ErrorDelegate);
#if UE_EDITOR
				// Have to remove instantly with editor since it'll keep the session valid throughout PIE
				RemoveNamedSession(Session->SessionName);
				return ERROR_SUCCESS;
#else
				return ERROR_IO_PENDING;
#endif
			}
		}
		else if (SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionClient)
		{
			// Client just need to remove the session
			RemoveNamedSession(Session->SessionName);
		}
	}

	return ERROR_SUCCESS;
}

bool FOnlineSessionPlayFab::IsPlayerInSession(FName SessionName, const FUniqueNetId& UniqueId)
{
	return IsPlayerInSessionImpl(this, SessionName, UniqueId);
}

bool FOnlineSessionPlayFab::StartMatchmaking(const TArray< TSharedRef<const FUniqueNetId> >& LocalPlayers, FName SessionName, const FOnlineSessionSettings& NewSessionSettings, TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	if (LocalPlayers.Num() > 1)
	{
		UE_LOG_ONLINE(Warning, TEXT("PlayFab currently does not support more than one player for matchmaking"));
		return false;
	}

	if (CurrentMatchmakeSearch.IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("PlayFab already matchmaking! Use CancelMatchmaking."));
		return false;
	}

	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(LocalPlayers[0].Get());
	if (ClientAPI.IsValid())
	{
		CurrentMatchmakeName = SessionName;
		CurrentMatchmakeSearch = SearchSettings;
		CurrentMatchmakeSearch->SearchState = EOnlineAsyncTaskState::InProgress;

		PlayFab::ClientModels::FMatchmakeRequest Request;

		FString OutValue;
		bool OutBool;
		// Lobby Id or...
		if (SearchSettings->QuerySettings.Get(SETTING_MATCHING_LOBBYID, OutValue))
		{
			Request.LobbyId = OutValue;
		}
		else
		{
			// BuildId, Gamemode, and Region
			Request.BuildVersion = PlayFabSubsystem->GetBuildVersion();

			if (SearchSettings->QuerySettings.Get(SETTING_GAMENAME, OutValue))
			{
				Request.GameMode = OutValue;
			}
			else if (SearchSettings->QuerySettings.Get(SETTING_GAMEMODE, OutValue))
			{
				Request.GameMode = OutValue;
			}
			else
			{
				UE_LOG_ONLINE(Error, TEXT("PlayFab Matchmaking requires either a LobbyId or Region and GameMode!"));
				TriggerOnMatchmakingCompleteDelegates(SessionName, false);
				return false;
			}

			if (SearchSettings->QuerySettings.Get(SETTING_REGION, OutValue))
			{
				Request.pfRegion = PlayFab::ClientModels::readRegionFromValue(OutValue);
			}
			else
			{
				UE_LOG_ONLINE(Error, TEXT("PlayFab Matchmaking requires either a LobbyId or Region and GameMode!"));
				TriggerOnMatchmakingCompleteDelegates(SessionName, false);
				return false;
			}
		}

		if (SearchSettings->QuerySettings.Get(SETTING_MATCHING_CHARACTERID, OutValue))
		{
			Request.CharacterId = OutValue;
		}

		if (SearchSettings->QuerySettings.Get(SETTING_MATCHING_STATISTICNAME, OutValue))
		{
			Request.StatisticName = OutValue;
		}

		if (SearchSettings->QuerySettings.Get(SETTING_MATCHING_STARTNEW, OutBool))
		{
			Request.StartNewIfNoneFound = OutBool;
		}

		// TODO: Add filters

		auto SuccessDelegate = PlayFab::UPlayFabClientAPI::FMatchmakeDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnSuccessCallback_Client_Matchmake, SessionName);
		auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Client, FName("Matchmake"), SessionName);
		ClientAPI->Matchmake(Request, SuccessDelegate, ErrorDelegate);
		return true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("PlayFab Client Interface not available"));
	}
	//UE_LOG_ONLINE(Warning, TEXT("StartMatchmaking is not supported on this platform. Use FindSessions or FindSessionById."));
	TriggerOnMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionPlayFab::CancelMatchmaking(int32 SearchingPlayerNum, FName SessionName)
{
	if (CurrentMatchmakeSearch.IsValid())
	{
		CurrentMatchmakeName = "";
		CurrentMatchmakeSearch.Reset();

		TriggerOnCancelMatchmakingCompleteDelegates(SessionName, true);
		return true;
	}
	//UE_LOG_ONLINE(Warning, TEXT("CancelMatchmaking is not supported on this platform. Use CancelFindSessions."));
	UE_LOG_ONLINE(Warning, TEXT("Not currently Matchmaking."));
	TriggerOnCancelMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionPlayFab::CancelMatchmaking(const FUniqueNetId& SearchingPlayerId, FName SessionName)
{
	// The matchmaking user doesn't matter, use 0
	return CancelMatchmaking(0, SessionName);
}

bool FOnlineSessionPlayFab::FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	uint32 Return = E_FAIL;

	if (CurrentMatchmakeSearch.IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("PlayFab already matchmaking! Use CancelMatchmaking."));
		return false;
	}

	// Don't start another search while one is in progress
	if (!CurrentSessionSearch.IsValid() && SearchSettings->SearchState != EOnlineAsyncTaskState::InProgress)
	{
		// Free up previous results
		SearchSettings->SearchResults.Empty();

		// Copy the search pointer so we can keep it around
		CurrentSessionSearch = SearchSettings;
		SearchSettings->SearchState = EOnlineAsyncTaskState::Failed; // Default to failed

		if (SearchSettings->bIsLanQuery == false)
		{
			// remember the time at which we started search, as this will be used for a "good enough" ping estimation
			SessionSearchStartInSeconds = FPlatformTime::Seconds();

			Return = FindInternetSession(SearchingPlayerNum, SearchSettings);
		}
		else
		{
			// remember the time at which we started search, as this will be used for a "good enough" ping estimation
			SessionSearchStartInSeconds = FPlatformTime::Seconds();

			Return = FindLANSession();
		}

		if (Return == ERROR_IO_PENDING)
		{
			SearchSettings->SearchState = EOnlineAsyncTaskState::InProgress;
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("FindSessions: Ignoring game search request while one is pending"));
		Return = ERROR_IO_PENDING;
	}

	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

bool FOnlineSessionPlayFab::FindSessions(const FUniqueNetId& SearchingPlayerId, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	// This function doesn't use the SearchingPlayerNum parameter, so passing in anything is fine.
	return FindSessions(0, SearchSettings);
}

// TODO: Add FindSessionById
bool FOnlineSessionPlayFab::FindSessionById(const FUniqueNetId& SearchingUserId, const FUniqueNetId& SessionId, const FUniqueNetId& FriendId, const FOnSingleSessionResultCompleteDelegate& CompletionDelegates)
{
	FOnlineSessionSearchResult EmptyResult;
	CompletionDelegates.ExecuteIfBound(0, false, EmptyResult);
	return true;
}

uint32 FOnlineSessionPlayFab::FindInternetSession(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(SearchingPlayerNum);
	if (ClientAPI.IsValid())
	{
		PlayFab::ClientModels::FCurrentGamesRequest Request;
		Request.BuildVersion = PlayFabSubsystem->GetBuildVersion();

		FString OutValue;
		if (SearchSettings->QuerySettings.Get(SETTING_GAMENAME, OutValue))
		{
			Request.GameMode = OutValue;
		}
		else if (SearchSettings->QuerySettings.Get(SETTING_GAMEMODE, OutValue))
		{
			Request.GameMode = OutValue;
		}

		if (SearchSettings->QuerySettings.Get(SETTING_REGION, OutValue))
		{
			Request.pfRegion = PlayFab::ClientModels::readRegionFromValue(OutValue);
		}

		if (SearchSettings->QuerySettings.Get(SETTING_MATCHING_STATISTICNAME, OutValue))
		{
			Request.StatisticName = OutValue;
		}

		PlayFab::ClientModels::FContainer_Dictionary_String_String ExcludesDictionary;
		PlayFab::ClientModels::FContainer_Dictionary_String_String IncludesDictionary;

		// Copy the params so we can remove the values as we use them
		FOnlineSearchSettings TempSearchSettings = SearchSettings->QuerySettings;

		// for each SEARCH_ define, we must do separately, since their tag keys are named differently

		// The first 4 are added directly via the FOnlineSessionSearch Constructor thus need to be handled differently
		FString MapName;
		if (TempSearchSettings.Get(SETTING_MAPNAME, MapName) && !MapName.IsEmpty())
		{
			IncludesDictionary.Data.Add(SETTING_MAPNAME.ToString(), MapName);
		}
		TempSearchSettings.SearchParams.Remove(SETTING_MAPNAME);

		bool DedicatedOnly;
		if (TempSearchSettings.Get(SEARCH_DEDICATED_ONLY, DedicatedOnly) && DedicatedOnly)
		{
			IncludesDictionary.Data.Add("bIsDedicated", TEXT("true"));
		}
		TempSearchSettings.SearchParams.Remove(SEARCH_DEDICATED_ONLY);

		bool SecureOnly;
		if (TempSearchSettings.Get(SEARCH_SECURE_SERVERS_ONLY, SecureOnly) && SecureOnly)
		{
			IncludesDictionary.Data.Add("bAntiCheatProtected", TEXT("true"));
		}
		TempSearchSettings.SearchParams.Remove(SEARCH_SECURE_SERVERS_ONLY);

		bool EmptyOnly;
		if (TempSearchSettings.Get(SEARCH_EMPTY_SERVERS_ONLY, EmptyOnly) && EmptyOnly)
		{
			IncludesDictionary.Data.Add("bEmpty", TEXT("true"));
		}
		TempSearchSettings.SearchParams.Remove(SEARCH_EMPTY_SERVERS_ONLY);

		bool NonEmptyOnly;
		if (TempSearchSettings.Get(SEARCH_NONEMPTY_SERVERS_ONLY, NonEmptyOnly) && NonEmptyOnly)
		{
			ExcludesDictionary.Data.Add("bEmpty", TEXT("true"));
		}
		TempSearchSettings.SearchParams.Remove(SEARCH_NONEMPTY_SERVERS_ONLY);

		// TODO: Add the following search defines
		/*
		SEARCH_MINSLOTSAVAILABLE
		*/

		// All other settings should have a tag key to match the define
		for (FSearchParams::TConstIterator It(SearchSettings->QuerySettings.SearchParams); It; ++It)
		{
			const FName Key = It.Key();
			const FOnlineSessionSearchParam& SearchParam = It.Value();

			const FString KeyStr = Key.ToString();
			if (SearchParam.ComparisonOp == EOnlineComparisonOp::Equals)
			{
				IncludesDictionary.Data.Add(KeyStr, SearchParam.Data.ToString());
			}
			else if (SearchParam.ComparisonOp == EOnlineComparisonOp::NotEquals)
			{
				ExcludesDictionary.Data.Add(KeyStr, SearchParam.Data.ToString());
			}
			/*
			else if (SearchParam.ComparisonOp == EOnlineComparisonOp::In)
			{
				IncludesDictionary.Data.Add(KeyStr, "");
			}
			else if (SearchParam.ComparisonOp == EOnlineComparisonOp::NotIn)
			{
				ExcludesDictionary.Data.Add(KeyStr, "");
			}
			*/
		}
		if (ExcludesDictionary.Data.Num() > 0 || IncludesDictionary.Data.Num() > 0)
		{
			Request.TagFilter = MakeShareable(new PlayFab::ClientModels::FCollectionFilter);
			if (ExcludesDictionary.Data.Num() > 0)
			{
				Request.TagFilter->Excludes.Add(ExcludesDictionary);
			}
			else if (IncludesDictionary.Data.Num() > 0)
			{
				Request.TagFilter->Includes.Add(IncludesDictionary);
			}
		}

		auto SuccessDelegate = PlayFab::UPlayFabClientAPI::FGetCurrentGamesDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnSuccessCallback_Client_GetCurrentGames);
		auto ErrorDelegate = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Client, FName("GetCurrentGames"));
		ClientAPI->GetCurrentGames(Request, SuccessDelegate, ErrorDelegate);
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("PlayFab Client Interface not available"));
		return E_FAIL;
	}

	return ERROR_IO_PENDING;
}

uint32 FOnlineSessionPlayFab::FindLANSession()
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	uint32 Return = ERROR_IO_PENDING;

	if (!LANSession)
	{
		LANSession = new FLANSession();
	}

	// Recreate the unique identifier for this client
	GenerateNonce((uint8*)&LANSession->LanNonce, 8);

	FOnValidResponsePacketDelegate ResponseDelegate = FOnValidResponsePacketDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnValidResponsePacketReceived);
	FOnSearchingTimeoutDelegate TimeoutDelegate = FOnSearchingTimeoutDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnLANSearchTimeout);

	FNboSerializeToBufferPlayFab Packet(LAN_BEACON_MAX_PACKET_SIZE);
	LANSession->CreateClientQueryPacket(Packet, LANSession->LanNonce);
	if (Packet.HasOverflow() || LANSession->Search(Packet, ResponseDelegate, TimeoutDelegate) == false)
	{
		Return = E_FAIL;
		delete LANSession;
		LANSession = NULL;

		CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Failed;

		// Just trigger the delegate as having failed
		TriggerOnFindSessionsCompleteDelegates(false);
	}

	return Return;
}

bool FOnlineSessionPlayFab::CancelFindSessions()
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	uint32 Return = E_FAIL;
	if (CurrentSessionSearch.IsValid() && CurrentSessionSearch->SearchState == EOnlineAsyncTaskState::InProgress)
	{
		// Make sure it's the right type
		if (!CurrentSessionSearch->bIsLanQuery)
		{
			// We can't stop the PlayFab search currently...
			Return = ERROR_SUCCESS;
			// Just clear it out
			CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Failed;
			CurrentSessionSearch = nullptr;
		}
		else
		{
			Return = ERROR_SUCCESS;
			LANSession->StopLANSession();
			CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Failed;
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Can't cancel a search that isn't in progress"));
	}

	if (Return == ERROR_SUCCESS)
	{
		TriggerOnCancelFindSessionsCompleteDelegates(true);
	}
	else if (Return != ERROR_IO_PENDING)
	{
		TriggerOnCancelFindSessionsCompleteDelegates(false);
	}

	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

bool FOnlineSessionPlayFab::JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;


	uint32 Return = E_FAIL;
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	// Don't join a session if already in one or hosting one
	if (Session == NULL)
	{
		// Create a named session from the search result data
		Session = AddNamedSession(SessionName, DesiredSession.Session);
		Session->HostingPlayerNum = PlayerNum;

		// Create Internet or LAN match
		if (!Session->SessionSettings.bIsLANMatch)
		{
			if (DesiredSession.Session.SessionInfo.IsValid())
			{
				const FOnlineSessionInfoPlayFab* SearchSessionInfo = (const FOnlineSessionInfoPlayFab*)DesiredSession.Session.SessionInfo.Get();

				FOnlineSessionInfoPlayFab* NewSessionInfo = new FOnlineSessionInfoPlayFab(EPlayFabSession::AdvertisedSessionClient, SearchSessionInfo->SessionId);
				Session->SessionInfo = MakeShareable(NewSessionInfo);

				Return = JoinInternetSession(PlayerNum, Session, &DesiredSession.Session);
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("JoinSession: Invalid session info on search result"), *SessionName.ToString());
			}
		}
		else
		{
			FOnlineSessionInfoPlayFab* NewSessionInfo = new FOnlineSessionInfoPlayFab(EPlayFabSession::LANSession);
			Session->SessionInfo = MakeShareable(NewSessionInfo);

			Return = JoinLANSession(PlayerNum, Session, &DesiredSession.Session);
		}

		if (Return != ERROR_IO_PENDING)
		{
			if (Return != ERROR_SUCCESS)
			{
				// Clean up the session info so we don't get into a confused state
				RemoveNamedSession(SessionName);
			}
			else
			{
				RegisterLocalPlayers(Session);
			}
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("JoinSession: Session (%s) already exists, can't join twice"), *SessionName.ToString());
	}

	if (Return != ERROR_IO_PENDING)
	{
		// Just trigger the delegate as having failed
		TriggerOnJoinSessionCompleteDelegates(SessionName, Return == ERROR_SUCCESS ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::UnknownError);
	}

	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

bool FOnlineSessionPlayFab::JoinSession(const FUniqueNetId& PlayerId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	// Assuming player 0 should be OK here
	return JoinSession(0, SessionName, DesiredSession);
}

uint32 FOnlineSessionPlayFab::JoinInternetSession(int32 PlayerNum, FNamedOnlineSession* Session, const FOnlineSession* SearchSession)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;


	uint32 Result = E_FAIL;
	Session->SessionState = EOnlineSessionState::Pending;

	if (Session->SessionInfo.IsValid())
	{
		FOnlineSessionInfoPlayFab* PlayFabSessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());
		if (PlayFabSessionInfo->SessionType == EPlayFabSession::AdvertisedSessionClient && PlayFabSessionInfo->SessionId.IsValid())
		{
			// Copy the session info over
			const FOnlineSessionInfoPlayFab* SearchSessionInfo = (const FOnlineSessionInfoPlayFab*)SearchSession->SessionInfo.Get();
			PlayFabSessionInfo->HostAddr = SearchSessionInfo->HostAddr;

			Result = ERROR_SUCCESS;
		}
	}

	return Result;
}

uint32 FOnlineSessionPlayFab::JoinLANSession(int32 PlayerNum, FNamedOnlineSession* Session, const FOnlineSession* SearchSession)
{
	constexpr uint32 E_FAIL = 0x80004005;
	constexpr uint32 ERROR_SUCCESS = 0;
	constexpr uint32 ERROR_IO_PENDING = 0x3E5;

	check(Session != nullptr);

	uint32 Result = E_FAIL;
	Session->SessionState = EOnlineSessionState::Pending;

	if (Session->SessionInfo.IsValid() && SearchSession != nullptr && SearchSession->SessionInfo.IsValid())
	{
		// Copy the session info over
		const FOnlineSessionInfoPlayFab* SearchSessionInfo = (const FOnlineSessionInfoPlayFab*)SearchSession->SessionInfo.Get();
		FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)Session->SessionInfo.Get();
		SessionInfo->SessionId = SearchSessionInfo->SessionId;

		uint32 IpAddr;
		SearchSessionInfo->HostAddr->GetIp(IpAddr);
		SessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
		SessionInfo->HostAddr->SetIp(IpAddr);
		SessionInfo->HostAddr->SetPort(SearchSessionInfo->HostAddr->GetPort());
		Result = ERROR_SUCCESS;
	}

	return Result;
}

bool FOnlineSessionPlayFab::FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend)
{
	TArray<FOnlineSessionSearchResult> EmptySearchResults;
	TriggerOnFindFriendSessionCompleteDelegates(LocalUserNum, false, EmptySearchResults);
	return false;
};

// TODO: Add implementation of FindFriendSession
bool FOnlineSessionPlayFab::FindFriendSession(const FUniqueNetId& LocalUserId, const FUniqueNetId& Friend)
{
	return FindFriendSession(0, Friend);
}

bool FOnlineSessionPlayFab::FindFriendSession(const FUniqueNetId& LocalUserId, const TArray<TSharedRef<const FUniqueNetId>>& FriendList)
{
	TArray<FOnlineSessionSearchResult> EmptySearchResult;
	TriggerOnFindFriendSessionCompleteDelegates(0, false, EmptySearchResult);
	return false;
}

// TODO: Add implementation of SendSessionInviteToFriend
bool FOnlineSessionPlayFab::SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend)
{
	return false;
};

bool FOnlineSessionPlayFab::SendSessionInviteToFriend(const FUniqueNetId& LocalUserId, FName SessionName, const FUniqueNetId& Friend)
{
	return SendSessionInviteToFriend(0, SessionName, Friend);
}

// TODO: Add implementation of SendSessionInviteToFriends
bool FOnlineSessionPlayFab::SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends)
{
	return false;
};

bool FOnlineSessionPlayFab::SendSessionInviteToFriends(const FUniqueNetId& LocalUserId, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends)
{
	return SendSessionInviteToFriends(0, SessionName, Friends);
}

// TODO: Add implementation of PingSearchResults
bool FOnlineSessionPlayFab::PingSearchResults(const FOnlineSessionSearchResult& SearchResult)
{
	TriggerOnPingSearchResultsCompleteDelegates(false);
	return false;
}

/** Get a resolved connection string from a session info */
static bool GetConnectStringFromSessionInfo(TSharedPtr<FOnlineSessionInfoPlayFab>& SessionInfo, FString& ConnectInfo, int32 PortOverride = 0)
{
	bool bSuccess = false;
	if (SessionInfo.IsValid())
	{
		FString AuthToken;
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get(FName("PlayFab"));
		if (OnlineSubsystem != nullptr)
		{
			IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface();
			if (Identity.IsValid())
			{
				AuthToken = Identity->GetAuthToken(0);
			}
		}

		if (SessionInfo->HostAddr.IsValid() && SessionInfo->HostAddr->IsValid())
		{
			int32 HostPort = SessionInfo->HostAddr->GetPort();
			if (PortOverride > 0)
			{
				HostPort = PortOverride;
			}

			ConnectInfo = FString::Printf(TEXT("%s:%d"), *SessionInfo->HostAddr->ToString(false), HostPort);

			// These URL tokens will have to be authenticated in the GameMode...
			// Why doesn't the Session get the info back to authenticate!?
			if (!AuthToken.IsEmpty())
			{
				ConnectInfo = FString::Printf(TEXT("%s?AuthToken=%s"), *ConnectInfo, *AuthToken);
			}
			if (!SessionInfo->MatchmakeTicket.IsEmpty())
			{
				ConnectInfo = FString::Printf(TEXT("%s?MatchmakeTicket=%s"), *ConnectInfo, *SessionInfo->MatchmakeTicket);
			}
			bSuccess = true;
		}
	}

	return bSuccess;
}

bool FOnlineSessionPlayFab::GetResolvedConnectString(FName SessionName, FString& ConnectInfo, FName PortType /*= GamePort*/)
{
	bool bSuccess = false;
	// Find the session
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session != NULL)
	{
		TSharedPtr<FOnlineSessionInfoPlayFab> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoPlayFab>(Session->SessionInfo);
		if (PortType == BeaconPort)
		{
			int32 BeaconListenPort = GetBeaconPortFromSessionSettings(Session->SessionSettings);
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo, BeaconListenPort);
		}
		else if (PortType == GamePort)
		{
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo);
		}

		if (!bSuccess)
		{
			UE_LOG_ONLINE(Warning, TEXT("Invalid session info for session %s in GetResolvedConnectString()"), *SessionName.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Unknown session name (%s) specified to GetResolvedConnectString()"), *SessionName.ToString());
	}

	return bSuccess;
}

bool FOnlineSessionPlayFab::GetResolvedConnectString(const class FOnlineSessionSearchResult& SearchResult, FName PortType, FString& ConnectInfo)
{
	bool bSuccess = false;
	if (SearchResult.Session.SessionInfo.IsValid())
	{
		TSharedPtr<FOnlineSessionInfoPlayFab> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoPlayFab>(SearchResult.Session.SessionInfo);

		if (PortType == BeaconPort)
		{
			int32 BeaconListenPort = GetBeaconPortFromSessionSettings(SearchResult.Session.SessionSettings);
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo, BeaconListenPort);
		}

		else if (PortType == GamePort)
		{
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo);
		}
	}

	if (!bSuccess || ConnectInfo.IsEmpty())
	{
		UE_LOG_ONLINE(Warning, TEXT("Invalid session info in search result to GetResolvedConnectString()"));
	}

	return bSuccess;
}

FOnlineSessionSettings* FOnlineSessionPlayFab::GetSessionSettings(FName SessionName)
{
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		return &Session->SessionSettings;
	}
	return NULL;
}

void FOnlineSessionPlayFab::RegisterLocalPlayers(FNamedOnlineSession* Session)
{
	if (!PlayFabSubsystem->IsDedicated())
	{
		IOnlineVoicePtr VoiceInt = PlayFabSubsystem->GetVoiceInterface();
		if (VoiceInt.IsValid())
		{
			for (int32 Index = 0; Index < MAX_LOCAL_PLAYERS; Index++)
			{
				// Register the local player as a local talker
				VoiceInt->RegisterLocalTalker(Index);
			}
		}
	}
}

void FOnlineSessionPlayFab::RegisterVoice(const FUniqueNetId& PlayerId)
{
	IOnlineVoicePtr VoiceInt = PlayFabSubsystem->GetVoiceInterface();
	if (VoiceInt.IsValid())
	{
		if (!PlayFabSubsystem->IsLocalPlayer(PlayerId))
		{
			VoiceInt->RegisterRemoteTalker(PlayerId);
		}
		else
		{
			// This is a local player. In case their PlayerState came last during replication, reprocess muting
			VoiceInt->ProcessMuteChangeNotification();
		}
	}
}

void FOnlineSessionPlayFab::UnregisterVoice(const FUniqueNetId& PlayerId)
{
	IOnlineVoicePtr VoiceInt = PlayFabSubsystem->GetVoiceInterface();
	if (VoiceInt.IsValid())
	{
		if (!PlayFabSubsystem->IsLocalPlayer(PlayerId))
		{
			if (VoiceInt.IsValid())
			{
				VoiceInt->UnregisterRemoteTalker(PlayerId);
			}
		}
	}
}

bool FOnlineSessionPlayFab::RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited)
{
	TArray< TSharedRef<const FUniqueNetId> > Players;
	Players.Add(MakeShareable(new FUniqueNetIdPlayFabId(PlayerId)));
	return RegisterPlayers(SessionName, Players, bWasInvited);
}

bool FOnlineSessionPlayFab::RegisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players, bool bWasInvited)
{
	bool bSuccess = false;
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		if (Session->SessionInfo.IsValid())
		{
			FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());

			for (int32 PlayerIdx = 0; PlayerIdx < Players.Num(); PlayerIdx++)
			{
				const TSharedRef<const FUniqueNetId>& PlayerId = Players[PlayerIdx];

				FUniqueNetIdMatcher PlayerMatch(*PlayerId);
				if (Session->RegisteredPlayers.IndexOfByPredicate(PlayerMatch) == INDEX_NONE)
				{
					if (SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionPlayFab || SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionHost)
					{
						PlayerJoined(PlayerId.Get(), SessionName);
					}

					Session->RegisteredPlayers.Add(PlayerId);
					RegisterVoice(*PlayerId);

					// update number of open connections
					if (Session->NumOpenPublicConnections > 0)
					{
						Session->NumOpenPublicConnections--;
					}
					else if (Session->NumOpenPrivateConnections > 0)
					{
						Session->NumOpenPrivateConnections--;
					}
				}
				else
				{
					RegisterVoice(*PlayerId);
					UE_LOG_ONLINE(Log, TEXT("RegisterPlayers: Player %s already registered in session %s"), *PlayerId->ToDebugString(), *SessionName.ToString());
				}
			}
			bSuccess = true;
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("RegisterPlayers: No session info to join for session (%s)"), *SessionName.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("RegisterPlayers: No game present to join for session (%s)"), *SessionName.ToString());
	}

	TriggerOnRegisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}

bool FOnlineSessionPlayFab::UnregisterPlayer(FName SessionName, const FUniqueNetId& PlayerId)
{
	TArray< TSharedRef<const FUniqueNetId> > Players;
	Players.Add(MakeShareable(new FUniqueNetIdPlayFabId(PlayerId)));
	return UnregisterPlayers(SessionName, Players);
}

bool FOnlineSessionPlayFab::UnregisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players)
{
	bool bSuccess = false;

	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		if (Session->SessionInfo.IsValid())
		{
			FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());

			for (int32 PlayerIdx = 0; PlayerIdx < Players.Num(); PlayerIdx++)
			{
				const TSharedRef<const FUniqueNetId>& PlayerId = Players[PlayerIdx];

				FUniqueNetIdMatcher PlayerMatch(*PlayerId);
				int32 RegistrantIndex = Session->RegisteredPlayers.IndexOfByPredicate(PlayerMatch);
				if (RegistrantIndex != INDEX_NONE)
				{
					if (SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionPlayFab || SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionHost)
					{
						PlayerLeft(PlayerId.Get(), SessionName);
					}

					Session->RegisteredPlayers.RemoveAtSwap(RegistrantIndex);
					UnregisterVoice(*PlayerId);

					// update number of open connections
					if (Session->NumOpenPublicConnections < Session->SessionSettings.NumPublicConnections)
					{
						Session->NumOpenPublicConnections++;
					}
					else if (Session->NumOpenPrivateConnections < Session->SessionSettings.NumPrivateConnections)
					{
						Session->NumOpenPrivateConnections++;
					}
				}
				else
				{
					UE_LOG_ONLINE(Warning, TEXT("UnregisterPlayer: Player %s is not part of session (%s)"), *PlayerId->ToDebugString(), *SessionName.ToString());
				}
			}
			bSuccess = true;
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("UnregisterPlayer: No session info to leave for session (%s)"), *SessionName.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("UnregisterPlayer: No game present to leave for session (%s)"), *SessionName.ToString());
	}

	TriggerOnUnregisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}

void FOnlineSessionPlayFab::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_Session_Interface);
	//TickLanTasks(DeltaTime);
}

void FOnlineSessionPlayFab::TickLanTasks(float DeltaTime)
{
	if (LANSession != NULL && LANSession->GetBeaconState() > ELanBeaconState::NotUsingLanBeacon)
	{
		LANSession->Tick(DeltaTime);
	}
}

void FOnlineSessionPlayFab::AppendSessionToPacket(FNboSerializeToBufferPlayFab& Packet, FOnlineSession* Session)
{
	/** Owner of the session */
	Packet << *StaticCastSharedPtr<const FUniqueNetIdPlayFabId>(Session->OwningUserId)
		<< Session->OwningUserName
		<< Session->NumOpenPrivateConnections
		<< Session->NumOpenPublicConnections;

	// Try to get the actual port the netdriver is using
	SetPortFromNetDriver(*PlayFabSubsystem, Session->SessionInfo);

	// Write host info (host addr, session id, and key)
	Packet << *StaticCastSharedPtr<FOnlineSessionInfoPlayFab>(Session->SessionInfo);

	// Now append per game settings
	AppendSessionSettingsToPacket(Packet, &Session->SessionSettings);
}

void FOnlineSessionPlayFab::AppendSessionSettingsToPacket(FNboSerializeToBufferPlayFab& Packet, FOnlineSessionSettings* SessionSettings)
{
#if DEBUG_LAN_BEACON
	UE_LOG_ONLINE(Verbose, TEXT("Sending session settings to client"));
#endif

	// Members of the session settings class
	Packet << SessionSettings->NumPublicConnections
		<< SessionSettings->NumPrivateConnections
		<< (uint8)SessionSettings->bShouldAdvertise
		<< (uint8)SessionSettings->bIsLANMatch
		<< (uint8)SessionSettings->bIsDedicated
		<< (uint8)SessionSettings->bUsesStats
		<< (uint8)SessionSettings->bAllowJoinInProgress
		<< (uint8)SessionSettings->bAllowInvites
		<< (uint8)SessionSettings->bUsesPresence
		<< (uint8)SessionSettings->bAllowJoinViaPresence
		<< (uint8)SessionSettings->bAllowJoinViaPresenceFriendsOnly
		<< (uint8)SessionSettings->bAntiCheatProtected
		<< SessionSettings->BuildUniqueId;

	// First count number of advertised keys
	int32 NumAdvertisedProperties = 0;
	for (FSessionSettings::TConstIterator It(SessionSettings->Settings); It; ++It)
	{
		const FOnlineSessionSetting& Setting = It.Value();
		if (Setting.AdvertisementType >= EOnlineDataAdvertisementType::ViaOnlineService)
		{
			NumAdvertisedProperties++;
		}
	}

	// Add count of advertised keys and the data
	Packet << (int32)NumAdvertisedProperties;
	for (FSessionSettings::TConstIterator It(SessionSettings->Settings); It; ++It)
	{
		const FOnlineSessionSetting& Setting = It.Value();
		if (Setting.AdvertisementType >= EOnlineDataAdvertisementType::ViaOnlineService)
		{
			Packet << It.Key();
			Packet << Setting;
#if DEBUG_LAN_BEACON
			UE_LOG_ONLINE(Verbose, TEXT("%s"), *Setting.ToString());
#endif
		}
	}
}

void FOnlineSessionPlayFab::OnValidQueryPacketReceived(uint8* PacketData, int32 PacketLength, uint64 ClientNonce)
{
	// Iterate through all registered sessions and respond for each one that can be joinable
	FScopeLock ScopeLock(&SessionLock);
	for (int32 SessionIndex = 0; SessionIndex < Sessions.Num(); SessionIndex++)
	{
		FNamedOnlineSession* Session = &Sessions[SessionIndex];

		if (Session && Session->SessionSettings.bIsLANMatch && Session->NumOpenPublicConnections > 0)
		{
			FNboSerializeToBufferPlayFab Packet(LAN_BEACON_MAX_PACKET_SIZE);
			// Create the basic header before appending additional information
			LANSession->CreateHostResponsePacket(Packet, ClientNonce);

			// Add all the session details
			AppendSessionToPacket(Packet, Session);

			// Broadcast this response so the client can see us
			if (!Packet.HasOverflow())
			{
				LANSession->BroadcastPacket(Packet, Packet.GetByteCount());
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("LAN broadcast packet overflow, cannot broadcast on LAN"));
			}
		}
	}
}

void FOnlineSessionPlayFab::ReadSessionFromPacket(FNboSerializeFromBufferPlayFab& Packet, FOnlineSession* Session)
{
#if DEBUG_LAN_BEACON
	UE_LOG_ONLINE(Verbose, TEXT("Reading session information from server"));
#endif

	/** Owner of the session */
	FUniqueNetIdPlayFabId* UniqueId = new FUniqueNetIdPlayFabId;
	Packet >> *UniqueId
		>> Session->OwningUserName
		>> Session->NumOpenPrivateConnections
		>> Session->NumOpenPublicConnections;

	Session->OwningUserId = MakeShareable(UniqueId);

	// Allocate and read the connection data
	FOnlineSessionInfoPlayFab* PlayFabSessionInfo = new FOnlineSessionInfoPlayFab(EPlayFabSession::LANSession);
	PlayFabSessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	Packet >> *PlayFabSessionInfo;
	Session->SessionInfo = MakeShareable(PlayFabSessionInfo);

	// Read any per object data using the server object
	ReadSettingsFromPacket(Packet, Session->SessionSettings);
}

void FOnlineSessionPlayFab::ReadSettingsFromPacket(FNboSerializeFromBufferPlayFab& Packet, FOnlineSessionSettings& SessionSettings)
{
#if DEBUG_LAN_BEACON
	UE_LOG_ONLINE(Verbose, TEXT("Reading game settings from server"));
#endif

	// Clear out any old settings
	SessionSettings.Settings.Empty();

	// Members of the session settings class
	Packet >> SessionSettings.NumPublicConnections
		>> SessionSettings.NumPrivateConnections;
	uint8 Read = 0;
	// Read all the bools as bytes
	Packet >> Read;
	SessionSettings.bShouldAdvertise = !!Read;
	Packet >> Read;
	SessionSettings.bIsLANMatch = !!Read;
	Packet >> Read;
	SessionSettings.bIsDedicated = !!Read;
	Packet >> Read;
	SessionSettings.bUsesStats = !!Read;
	Packet >> Read;
	SessionSettings.bAllowJoinInProgress = !!Read;
	Packet >> Read;
	SessionSettings.bAllowInvites = !!Read;
	Packet >> Read;
	SessionSettings.bUsesPresence = !!Read;
	Packet >> Read;
	SessionSettings.bAllowJoinViaPresence = !!Read;
	Packet >> Read;
	SessionSettings.bAllowJoinViaPresenceFriendsOnly = !!Read;
	Packet >> Read;
	SessionSettings.bAntiCheatProtected = !!Read;

	// BuildId
	Packet >> SessionSettings.BuildUniqueId;

	// Now read the contexts and properties from the settings class
	int32 NumAdvertisedProperties = 0;
	// First, read the number of advertised properties involved, so we can presize the array
	Packet >> NumAdvertisedProperties;
	if (Packet.HasOverflow() == false)
	{
		FName Key;
		// Now read each context individually
		for (int32 Index = 0;
			Index < NumAdvertisedProperties && Packet.HasOverflow() == false;
			Index++)
		{
			FOnlineSessionSetting Setting;
			Packet >> Key;
			Packet >> Setting;
			SessionSettings.Set(Key, Setting);

#if DEBUG_LAN_BEACON
			UE_LOG_ONLINE(Verbose, TEXT("%s"), *Setting->ToString());
#endif
		}
	}

	// If there was an overflow, treat the string settings/properties as broken
	if (Packet.HasOverflow())
	{
		SessionSettings.Settings.Empty();
		UE_LOG_ONLINE(Verbose, TEXT("Packet overflow detected in ReadGameSettingsFromPacket()"));
	}
}

void FOnlineSessionPlayFab::OnValidResponsePacketReceived(uint8* PacketData, int32 PacketLength)
{
	// Create an object that we'll copy the data to
	FOnlineSessionSettings NewServer;
	if (CurrentSessionSearch.IsValid())
	{
		// Add space in the search results array
		FOnlineSessionSearchResult* NewResult = new (CurrentSessionSearch->SearchResults) FOnlineSessionSearchResult();
		// this is not a correct ping, but better than nothing
		NewResult->PingInMs = static_cast<int32>((FPlatformTime::Seconds() - SessionSearchStartInSeconds) * 1000);

		FOnlineSession* NewSession = &NewResult->Session;

		// Prepare to read data from the packet
		FNboSerializeFromBufferPlayFab Packet(PacketData, PacketLength);

		ReadSessionFromPacket(Packet, NewSession);

		// NOTE: we don't notify until the timeout happens
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to create new online game settings object"));
	}
}

void FOnlineSessionPlayFab::OnLANSearchTimeout()
{
	if (CurrentSessionSearch.IsValid())
	{
		if (CurrentSessionSearch->SearchResults.Num() > 0)
		{
			// Allow game code to sort the servers
			CurrentSessionSearch->SortSearchResults();
		}
		CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Done;

		CurrentSessionSearch = NULL;
	}

	// Trigger the delegate as complete
	TriggerOnFindSessionsCompleteDelegates(true);
}

int32 FOnlineSessionPlayFab::GetNumSessions()
{
	FScopeLock ScopeLock(&SessionLock);
	return Sessions.Num();
}

void FOnlineSessionPlayFab::DumpSessionState()
{
	FScopeLock ScopeLock(&SessionLock);

	for (int32 SessionIdx = 0; SessionIdx < Sessions.Num(); SessionIdx++)
	{
		DumpNamedSession(&Sessions[SessionIdx]);
	}
}

void FOnlineSessionPlayFab::RegisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnRegisterLocalPlayerCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(PlayerId, EOnJoinSessionCompleteResult::Success);
}

void FOnlineSessionPlayFab::UnregisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnUnregisterLocalPlayerCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(PlayerId, true);
}

void FOnlineSessionPlayFab::SetPortFromNetDriver(const FOnlineSubsystemPlayFab& Subsystem, const TSharedPtr<FOnlineSessionInfo>& SessionInfo)
{
	auto NetDriverPort = GetPortFromNetDriver(Subsystem.GetInstanceName());
	auto SessionInfoPlayFab = StaticCastSharedPtr<FOnlineSessionInfoPlayFab>(SessionInfo);
	if (SessionInfoPlayFab.IsValid() && SessionInfoPlayFab->HostAddr.IsValid())
	{
		SessionInfoPlayFab->HostAddr->SetPort(NetDriverPort);
	}
}
