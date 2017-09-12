// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSessionPlayFab.h"
#include "OnlineIdentityInterface.h"
#include "OnlineSubsystemPlayFab.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemSessionSettings.h"
#include "SocketSubsystem.h"
#include "NboSerializerPlayFab.h"
#include "LANBeacon.h"
#include "Json.h"
#include "Core/PlayFabServerAPI.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabServerDataModels.h"
#include "Core/PlayFabClientDataModels.h"
#include "PlayFab.h"


FOnlineSessionInfoPlayFab::FOnlineSessionInfoPlayFab(EPlayFabSession::Type InSessionType)
	: HostAddr(NULL)
	, SessionId(TEXT("INVALID"))
	, SessionType(InSessionType)
{
}

FOnlineSessionInfoPlayFab::FOnlineSessionInfoPlayFab(EPlayFabSession::Type InSessionType, const FUniqueNetIdString& InSessionId)
	: HostAddr(NULL)
	, SessionId(InSessionId)
	, SessionType(InSessionType)
{

}

FOnlineSessionInfoPlayFab::FOnlineSessionInfoPlayFab(EPlayFabSession::Type InSessionType, const FUniqueNetIdString& InSessionId, FString InMatchmakeTicket)
	: HostAddr(NULL)
	, SessionId(InSessionId)
	, SessionType(InSessionType)
	, MatchmakeTicket(InMatchmakeTicket)
{

}

void FOnlineSessionInfoPlayFab::Init(const FOnlineSubsystemPlayFab& Subsystem)
{

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

	FGuid OwnerGuid;
	FPlatformMisc::CreateGuid(OwnerGuid);
	SessionId = FUniqueNetIdString(OwnerGuid.ToString());
}

void FOnlineSessionPlayFab::OnSuccessCallback_Client_GetCurrentGames(const PlayFab::ClientModels::FCurrentGamesResult& Result)
{
	SuccessDelegate_Client_GetCurrentGames.Unbind();
	ErrorDelegate_Client.Unbind();

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
			//NewSession->OwningUserId = MakeShareable(new FUniqueNetIdString(""));
			//NewSession->OwningUserName =
			/** Available Slots */
			NewSession->NumOpenPrivateConnections = 0;
			NewSession->NumOpenPublicConnections = GameInfo.MaxPlayers - GameInfo.PlayerUserIds.Num();

			FOnlineSessionInfoPlayFab* PlayFabSessionInfo = new FOnlineSessionInfoPlayFab(EPlayFabSession::LobbySession, FUniqueNetIdString(GameInfo.LobbyID));
			PlayFabSessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
			bool bIsValid;
			PlayFabSessionInfo->HostAddr->SetIp(*GameInfo.ServerHostname, bIsValid);
			PlayFabSessionInfo->HostAddr->SetPort((!GameInfo.ServerPort.isNull() && GameInfo.ServerPort.mValue !=0) ? GameInfo.ServerPort.mValue : 7777);
			//Packet >> *PlayFabSessionInfo;
			NewSession->SessionInfo = MakeShareable(PlayFabSessionInfo);

			FOnlineSessionSettings& SessionSettings = NewSession->SessionSettings;

			// Clear out any old settings
			SessionSettings.Settings.Empty();

			TMap<FString, FString> Tags = GameInfo.Tags;

			// Read all the data
			SessionSettings.bAllowInvites = Tags["bAllowInvites"].ToBool();
			Tags.Remove("bAllowInvites");
			SessionSettings.bAllowJoinInProgress = Tags["bAllowJoinInProgress"].ToBool();
			Tags.Remove("bAllowJoinInProgress");
			SessionSettings.bAllowJoinViaPresence = Tags["bAllowJoinViaPresence"].ToBool();
			Tags.Remove("bAllowJoinViaPresence");
			SessionSettings.bAllowJoinViaPresenceFriendsOnly = Tags["bAllowJoinViaPresenceFriendsOnly"].ToBool();
			Tags.Remove("bAllowJoinViaPresenceFriendsOnly");
			SessionSettings.bAntiCheatProtected = Tags["bAntiCheatProtected"].ToBool();
			Tags.Remove("bAntiCheatProtected");
			SessionSettings.bIsDedicated = true;
			SessionSettings.bIsLANMatch = false;
			SessionSettings.bShouldAdvertise = Tags["bShouldAdvertise"].ToBool();
			Tags.Remove("bShouldAdvertise");
			SessionSettings.bUsesPresence = Tags["bUsesPresence"].ToBool();
			Tags.Remove("bUsesPresence");
			SessionSettings.bUsesStats = Tags["bUsesStats"].ToBool();
			Tags.Remove("bUsesStats");
			SessionSettings.NumPrivateConnections = FCString::Atoi(*Tags["NumPrivateConnections"]);
			Tags.Remove("NumPrivateConnections");
			SessionSettings.NumPublicConnections = FCString::Atoi(*Tags["NumPublicConnections"]);
			Tags.Remove("NumPublicConnections");

			// BuildId
			SessionSettings.BuildUniqueId = FCString::Atoi(*GameInfo.Tags["BuildUniqueId"]);
			Tags.Remove("BuildUniqueId");

			for (auto Setting : Tags)
			{
				FVariantData Data;
				if (Data.FromString(Setting.Value))
				{
					SessionSettings.Settings.FindOrAdd(FName(*Setting.Key)).Data = Data;
				}
			}
		}
		TriggerOnFindSessionsCompleteDelegates(true);
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to create new online game settings object"));
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

	FOnlineSessionInfoPlayFab* PlayFabSessionInfo = new FOnlineSessionInfoPlayFab(EPlayFabSession::LobbySession, FUniqueNetIdString(Result.LobbyID), Result.Ticket);
	PlayFabSessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool bIsValid;
	PlayFabSessionInfo->HostAddr->SetIp(*Result.ServerHostname, bIsValid);
	PlayFabSessionInfo->HostAddr->SetPort((!Result.ServerPort.isNull() && Result.ServerPort.mValue != 0) ? Result.ServerPort.mValue : 7777);

	NewSession.SessionInfo = MakeShareable(PlayFabSessionInfo);
	NewResult.Session = NewSession;

	CurrentMatchmakeSearch->SearchState = EOnlineAsyncTaskState::Done;
	CurrentMatchmakeSearch->SearchResults.Add(NewResult);
	TriggerOnMatchmakingCompleteDelegates(SessionName, true);
}

void FOnlineSessionPlayFab::OnErrorCallback_Client(const PlayFab::FPlayFabError& ErrorResult)
{
	SuccessDelegate_Client_GetCurrentGames.Unbind();
	ErrorDelegate_Client.Unbind();

	TriggerOnFindSessionsCompleteDelegates(false);
}

void FOnlineSessionPlayFab::OnSuccessCallback_Server_RegisterGame(const PlayFab::ServerModels::FRegisterGameResponse& Result, FName SessionName)
{
	SuccessDelegate_Server_RegisterGame.Unbind();
	ErrorDelegate_Server.Unbind();

	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// Setup the host session info
		UE_LOG_ONLINE(Log, TEXT("Received new LobbyId: %s"), *Result.LobbyId);
		FOnlineSessionInfoPlayFab* NewSessionInfo = new FOnlineSessionInfoPlayFab(EPlayFabSession::AdvertisedSessionHost, FUniqueNetIdString(*Result.LobbyId));
		NewSessionInfo->Init(*PlayFabSubsystem);

		FString cmdVal;
		if (FParse::Value(FCommandLine::Get(), TEXT("server_host_domain"), cmdVal)) {
			bool bIsValid;
			NewSessionInfo->HostAddr->SetIp(*cmdVal, bIsValid);
		}
		if (FParse::Value(FCommandLine::Get(), TEXT("server_host_port"), cmdVal)) {
			NewSessionInfo->HostAddr->SetPort(FCString::Atoi(*cmdVal));
		}

		Session->SessionInfo = MakeShareable(NewSessionInfo);
		Session->SessionState = EOnlineSessionState::Pending;
		RegisterLocalPlayers(Session);

		TimerDelegate_PlayFabHeartbeat = FTimerDelegate::CreateRaw(this, &FOnlineSessionPlayFab::PlayFab_Server_Heartbeat, SessionName);
		UWorld* World = GetWorldForOnline(PlayFabSubsystem->GetInstanceName());
		if (World != nullptr)
		{
			World->GetTimerManager().SetTimer(TimerHandle_PlayFabHeartbeat, TimerDelegate_PlayFabHeartbeat, 60.f, true);
		}
	}
}

void FOnlineSessionPlayFab::OnSuccessCallback_Server_DeregisterGame(const PlayFab::ServerModels::FDeregisterGameResponse& Result, FName SessionName)
{
	SuccessDelegate_Server_DeregisterGame.Unbind();
	ErrorDelegate_Server.Unbind();

	RemoveNamedSession(SessionName);
	TriggerOnDestroySessionCompleteDelegates(SessionName, true);
}

void FOnlineSessionPlayFab::OnErrorCallback_Server(const PlayFab::FPlayFabError& ErrorResult, FName SessionName)
{
	UE_LOG_ONLINE(Error, TEXT("%s"), *ErrorResult.ErrorMessage);
	if (SuccessDelegate_Server_RegisterGame.IsBound())
	{
		RemoveNamedSession(SessionName);
	}

	SuccessDelegate_Server_RegisterGame.Unbind();
	ErrorDelegate_Server.Unbind();
}

void FOnlineSessionPlayFab::PlayFab_Server_Heartbeat(FName SessionName)
{
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());

		PlayFabServerPtr ServerAPI = IPlayFabModuleInterface::Get().GetServerAPI();
		if (ServerAPI.IsValid())
		{
			PlayFab::ServerModels::FRefreshGameServerInstanceHeartbeatRequest Request;
			Request.LobbyId = SessionInfo->SessionId.ToString();

			ErrorDelegate_Server = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, SessionName);
			ServerAPI->RefreshGameServerInstanceHeartbeat(Request, PlayFab::UPlayFabServerAPI::FRefreshGameServerInstanceHeartbeatDelegate(), ErrorDelegate_Server);
		}
	}
}

bool FOnlineSessionPlayFab::CreateSession(int32 HostingPlayerNum, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
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
			Session->OwningUserId = MakeShareable(new FUniqueNetIdString(FString::Printf(TEXT("%d"), HostingPlayerNum)));
			Session->OwningUserName = FString(TEXT("PlayFabUser"));
		}

		// Unique identifier of this build for compatibility
		Session->SessionSettings.BuildUniqueId = GetBuildUniqueId();

		if (!Session->SessionSettings.bIsLANMatch)
		{
			if (Session->SessionSettings.bUsesPresence)
			{
				//Result = CreateLobbySession(HostingPlayerNum, Session);
			}
			else
			{
				Result = CreateInternetSession(HostingPlayerNum, Session);
			}
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
	// todo: use proper	HostingPlayerId
	return CreateSession(0, SessionName, NewSessionSettings);
}

uint32 FOnlineSessionPlayFab::CreateInternetSession(int32 HostingPlayerNum, class FNamedOnlineSession* Session)
{
	uint32 Result = E_FAIL;

	// Only allowed one published session with PlayFab
	FNamedOnlineSession* MasterSession = GetGameServerSession();
	if (MasterSession == NULL)
	{
		if (Session->SessionSettings.bIsDedicated)
		{
			if (!ErrorDelegate_Server.IsBound())
			{
				PlayFabServerPtr ServerAPI = IPlayFabModuleInterface::Get().GetServerAPI();

				if (ServerAPI.IsValid())
				{
					TMap<FString, FString> Tags;

					Tags.Add("bAllowInvites", Session->SessionSettings.bAllowInvites ? TEXT("true") : TEXT("false"));
					Tags.Add("bAllowJoinInProgress", Session->SessionSettings.bAllowJoinInProgress ? TEXT("true") : TEXT("false"));
					Tags.Add("bAllowJoinViaPresence", Session->SessionSettings.bAllowJoinViaPresence ? TEXT("true") : TEXT("false"));
					Tags.Add("bAllowJoinViaPresenceFriendsOnly", Session->SessionSettings.bAllowJoinViaPresenceFriendsOnly ? TEXT("true") : TEXT("false"));
					Tags.Add("bAntiCheatProtected", Session->SessionSettings.bAntiCheatProtected ? TEXT("true") : TEXT("false"));
					Tags.Add("bShouldAdvertise", Session->SessionSettings.bShouldAdvertise ? TEXT("true") : TEXT("false"));
					Tags.Add("bUsesPresence", Session->SessionSettings.bUsesPresence ? TEXT("true") : TEXT("false"));
					Tags.Add("bUsesStats", Session->SessionSettings.bUsesStats ? TEXT("true") : TEXT("false"));
					Tags.Add("NumPublicConnections", FString::FromInt(Session->SessionSettings.NumPublicConnections));
					Tags.Add("NumPrivateConnections", FString::FromInt(Session->SessionSettings.NumPrivateConnections));
					//Tags.Add("Settings", Session->SessionSettings.Settings);

					Tags.Add("BuildUniqueId", FString::FromInt(Session->SessionSettings.BuildUniqueId));

					for (FSessionSettings::TConstIterator It(Session->SessionSettings.Settings); It; ++It)
					{
						FName Key = It.Key();
						const FOnlineSessionSetting& Setting = It.Value();

						if (Setting.AdvertisementType == EOnlineDataAdvertisementType::ViaOnlineService)
						{
							Tags.Add(Key.ToString(), Setting.Data.ToString());
						}
					}

					FString cmdVal;
					if (FParse::Value(FCommandLine::Get(), TEXT("game_id"), cmdVal))
					{
						// Server is already registered with PlayFab, update data
						PlayFab::ServerModels::FSetGameServerInstanceTagsRequest Request;

						Request.LobbyId = cmdVal;
						Request.Tags = Tags;

						ErrorDelegate_Server = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, FName(*Session->SessionName.ToString()));
						ServerAPI->SetGameServerInstanceTags(Request, NULL, ErrorDelegate_Server);
					}
					else
					{
						// Server isn't registered with PlayFab, let's register it
						PlayFab::ServerModels::FRegisterGameRequest Request;

						Request.Build = PlayFabSubsystem->GetBuildVersion();
						if (Session->SessionSettings.Settings.Find(TEXT("GAMENAME")))
						{
							Request.GameMode = Session->SessionSettings.Settings[TEXT("GAMENAME")].Data.ToString();
						}
						else if (Session->SessionSettings.Settings.Find(SETTING_GAMEMODE))
						{
							Request.GameMode = Session->SessionSettings.Settings[SETTING_GAMEMODE].Data.ToString();
						}

						const FOnlineSessionSetting* RegionSetting = Session->SessionSettings.Settings.Find(SETTING_REGION);
						Request.pfRegion = PlayFab::readRegionFromValue(RegionSetting->ToString());

						bool bCanBindAll;
						TSharedPtr<class FInternetAddr> HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);
						Request.ServerHost = HostAddr->ToString(false);
						int32 port = HostAddr->GetPort();
						port = port != 0 ? port : 7777;
						Request.ServerPort = FString::FromInt(port);
						Request.Tags = Tags;

						SuccessDelegate_Server_RegisterGame = PlayFab::UPlayFabServerAPI::FRegisterGameDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnSuccessCallback_Server_RegisterGame, FName(*Session->SessionName.ToString()));
						ErrorDelegate_Server = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, FName(*Session->SessionName.ToString()));
						ServerAPI->RegisterGame(Request, SuccessDelegate_Server_RegisterGame, ErrorDelegate_Server);
					}

					Result = ERROR_IO_PENDING;
				}
				else
				{
					UE_LOG_ONLINE(Verbose, TEXT("CreateInternetSession: Failed to initialize game server with PlayFab."));
				}
			}
			else
			{
				UE_LOG_ONLINE(Verbose, TEXT("CreateInternetSession: Already waiting for LobbyId."));
			}
		}
		else
		{
			UE_LOG_ONLINE(Verbose, TEXT("CreateInternetSession: PlayFab can not currently create sessions for anything EXCEPT dedicated servers."));
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

bool FOnlineSessionPlayFab::StartSession(FName SessionName)
{
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
				Result = ERROR_SUCCESS;
				Session->SessionState = EOnlineSessionState::InProgress;

				/*if (SteamFriends() != NULL)
				{
					for (int32 PlayerIdx = 0; PlayerIdx < Session->RegisteredPlayers.Num(); PlayerIdx++)
					{
						FUniqueNetIdSteam& Player = (FUniqueNetIdSteam&)Session->RegisteredPlayers[PlayerIdx].Get();
						SteamFriends()->SetPlayedWith(Player);
					}
				}*/
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
		if (!Session->SessionSettings.bIsLANMatch)
		{
			FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());

			if (SessionInfo->SessionType == EPlayFabSession::LobbySession && SessionInfo->SessionId.IsValid())
			{
				// Lobby update
				//FOnlineAsyncTaskSteamUpdateLobby* NewTask = new FOnlineAsyncTaskSteamUpdateLobby(SteamSubsystem, SessionName, bShouldRefreshOnlineData, UpdatedSessionSettings);
				//SteamSubsystem->QueueAsyncTask(NewTask);
			}
			else if (SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionHost)
			{
				// Gameserver update
				bool bUsesPresence = Session->SessionSettings.bUsesPresence;
				if (bUsesPresence != UpdatedSessionSettings.bUsesPresence)
				{
					UE_LOG_ONLINE(Warning, TEXT("Can't change presence settings on existing session %s, ignoring."), *SessionName.ToString());
				}

				Session->SessionSettings = UpdatedSessionSettings;
				Session->SessionSettings.bUsesPresence = bUsesPresence;

				PlayFabServerPtr ServerAPI = IPlayFabModuleInterface::Get().GetServerAPI();

				if (bShouldRefreshOnlineData && ServerAPI.IsValid())
				{
					PlayFab::ServerModels::FSetGameServerInstanceTagsRequest Request;

					Request.LobbyId = SessionInfo->SessionId.ToString();
					Request.Tags.Add("bAllowInvites", Session->SessionSettings.bAllowInvites ? TEXT("true") : TEXT("false"));
					Request.Tags.Add("bAllowJoinInProgress", Session->SessionSettings.bAllowJoinInProgress ? TEXT("true") : TEXT("false"));
					Request.Tags.Add("bAllowJoinViaPresence", Session->SessionSettings.bAllowJoinViaPresence ? TEXT("true") : TEXT("false"));
					Request.Tags.Add("bAllowJoinViaPresenceFriendsOnly", Session->SessionSettings.bAllowJoinViaPresenceFriendsOnly ? TEXT("true") : TEXT("false"));
					Request.Tags.Add("bAntiCheatProtected", Session->SessionSettings.bAntiCheatProtected ? TEXT("true") : TEXT("false"));
					Request.Tags.Add("bShouldAdvertise", Session->SessionSettings.bShouldAdvertise ? TEXT("true") : TEXT("false"));
					Request.Tags.Add("bUsesPresence", Session->SessionSettings.bUsesPresence ? TEXT("true") : TEXT("false"));
					Request.Tags.Add("bUsesStats", Session->SessionSettings.bUsesStats ? TEXT("true") : TEXT("false"));
					Request.Tags.Add("NumPublicConnections", FString::FromInt(Session->SessionSettings.NumPublicConnections));
					Request.Tags.Add("NumPrivateConnections", FString::FromInt(Session->SessionSettings.NumPrivateConnections));

					Request.Tags.Add("BuildUniqueId", FString::FromInt(Session->SessionSettings.BuildUniqueId));

					for (FSessionSettings::TConstIterator It(Session->SessionSettings.Settings); It; ++It)
					{
						FName Key = It.Key();
						const FOnlineSessionSetting& Setting = It.Value();

						if (Setting.AdvertisementType == EOnlineDataAdvertisementType::ViaOnlineService)
						{
							Request.Tags.Add(Key.ToString(), Setting.Data.ToString());
						}
					}
					// We must send them ALL again, as this will overwrite all tags(any not written will be nil)

					ErrorDelegate_Server = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, FName(*Session->SessionName.ToString()));
					ServerAPI->SetGameServerInstanceTags(Request, NULL, ErrorDelegate_Server);
				}
			}
		}
		else
		{
			// @TODO ONLINE update LAN settings
			Session->SessionSettings = UpdatedSessionSettings;
			TriggerOnUpdateSessionCompleteDelegates(SessionName, bWasSuccessful);
		}

		/*// @TODO ONLINE update LAN settings
		Session->SessionSettings = UpdatedSessionSettings;
		TriggerOnUpdateSessionCompleteDelegates(SessionName, bWasSuccessful);*/
	}

	return bWasSuccessful;
}

bool FOnlineSessionPlayFab::EndSession(FName SessionName)
{
	uint32 Result = E_FAIL;

	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// Can't end a match that isn't in progress
		if (Session->SessionState == EOnlineSessionState::InProgress)
		{
			Session->SessionState = EOnlineSessionState::Ended;

			if (!Session->SessionSettings.bIsLANMatch)
			{
				//Result = EndInternetSession(Session);
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
	uint32 Result = E_FAIL;
	// Find the session in question
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		if (Session->SessionState != EOnlineSessionState::Destroying)
		{
			if (!Session->SessionSettings.bIsLANMatch)
			{
				if (Session->SessionSettings.bUsesPresence)
				{
					//Result = DestroyLobbySession(Session, CompletionDelegate);
				}
				else
				{
					Result = DestroyInternetSession(Session, CompletionDelegate);
				}
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
	Session->SessionState = EOnlineSessionState::Destroying;

	if (Session->SessionInfo.IsValid())
	{
		FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)(Session->SessionInfo.Get());
		check(SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionHost || SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionClient);

		if (PlayFabSubsystem->IsServer()) // true if Listen or Dedicated
		{
			PlayFabServerPtr ServerAPI = IPlayFabModuleInterface::Get().GetServerAPI();
			if (ServerAPI.IsValid())
			{
				PlayFab::ServerModels::FDeregisterGameRequest Request;
				Request.LobbyId = SessionInfo->SessionId.ToString();

				SuccessDelegate_Server_DeregisterGame = PlayFab::UPlayFabServerAPI::FDeregisterGameDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnSuccessCallback_Server_DeregisterGame, FName(*Session->SessionName.ToString()));
				ErrorDelegate_Server = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Server, FName(*Session->SessionName.ToString()));
				ServerAPI->DeregisterGame(Request, SuccessDelegate_Server_DeregisterGame, ErrorDelegate_Server);
			}
		}
		else
		{
			RemoveNamedSession(Session->SessionName);
			TriggerOnDestroySessionCompleteDelegates(Session->SessionName, true);
			return ERROR_SUCCESS;
		}
	}

	return ERROR_IO_PENDING;
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
		Request.BuildVersion = PlayFabSubsystem->GetBuildVersion();
		Request.StartNewIfNoneFound = true;

		FString OutValue;
		if (SearchSettings->QuerySettings.Get(SETTING_REGION, OutValue))
		{
			Request.pfRegion = PlayFab::readRegionFromValue(OutValue);
		}
		else
		{
			Request.pfRegion = PlayFab::readRegionFromValue(OutValue);
		}
		//Request.pfRegion = PlayFab::ClientModels::Region::RegionUSCentral;

		SuccessDelegate_Client_Matchmake = PlayFab::UPlayFabClientAPI::FMatchmakeDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnSuccessCallback_Client_Matchmake, SessionName);
		ErrorDelegate_Client = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Client);
		ClientAPI->Matchmake(Request, SuccessDelegate_Client_Matchmake, ErrorDelegate_Client);
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
	return CancelMatchmaking(0, SessionName);
}

bool FOnlineSessionPlayFab::FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
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

bool FOnlineSessionPlayFab::FindSessionById(const FUniqueNetId& SearchingUserId, const FUniqueNetId& SessionId, const FUniqueNetId& FriendId, const FOnSingleSessionResultCompleteDelegate& CompletionDelegates)
{
	FOnlineSessionSearchResult EmptyResult;
	CompletionDelegates.ExecuteIfBound(0, false, EmptyResult);
	return true;
}

uint32 FOnlineSessionPlayFab::FindInternetSession(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	bool PresenceSearch = false;
	if (SearchSettings->QuerySettings.Get(SEARCH_PRESENCE, PresenceSearch) && PresenceSearch)
	{
		//FOnlineAsyncTaskSteamFindLobbies* NewTask = new FOnlineAsyncTaskSteamFindLobbies(SteamSubsystem, SearchSettings);
		//SteamSubsystem->QueueAsyncTask(NewTask);
	}
	else
	{
		PlayFabClientPtr ClientAPI = PlayFabSubsystem->GetClientAPI(SearchingPlayerNum);
		if (ClientAPI.IsValid())
		{
			PlayFab::ClientModels::FCurrentGamesRequest Request;
			Request.BuildVersion = PlayFabSubsystem->GetBuildVersion();

			FString OutValue;
			if (SearchSettings->QuerySettings.Get(SETTING_REGION, OutValue))
			{
				Request.pfRegion = PlayFab::readRegionFromValue(OutValue);
			}
			else
			{
				Request.pfRegion = PlayFab::readRegionFromValue(OutValue);
			}
			//Request.pfRegion = PlayFab::ClientModels::Region::RegionUSCentral;

			SuccessDelegate_Client_GetCurrentGames = PlayFab::UPlayFabClientAPI::FGetCurrentGamesDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnSuccessCallback_Client_GetCurrentGames);
			ErrorDelegate_Client = PlayFab::FPlayFabErrorDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnErrorCallback_Client);
			ClientAPI->GetCurrentGames(Request, SuccessDelegate_Client_GetCurrentGames, ErrorDelegate_Client);
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("PlayFab Client Interface not available"));
		}
	}

	return ERROR_IO_PENDING;
}

uint32 FOnlineSessionPlayFab::FindLANSession()
{
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
	uint32 Return = E_FAIL;
	if (CurrentSessionSearch.IsValid() && CurrentSessionSearch->SearchState == EOnlineAsyncTaskState::InProgress)
	{
		// Make sure it's the right type
		if (CurrentSessionSearch->bIsLanQuery)
		{
			Return = ERROR_SUCCESS;
			LANSession->StopLANSession();
			CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Failed;
		}
		else
		{
			// We can't stop the PlayFab search currently so, not stop...
			Return = ERROR_IO_PENDING;
			/*// We can't stop the PlayFab search currently...
 			Return = ERROR_SUCCESS;
			// Just clear it out
			CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Failed;
			CurrentSessionSearch = NULL;*/
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Can't cancel a search that isn't in progress"));
	}

	if (Return != ERROR_IO_PENDING)
	{
		TriggerOnCancelFindSessionsCompleteDelegates(true);
	}

	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

bool FOnlineSessionPlayFab::JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
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

				if (DesiredSession.Session.SessionSettings.bUsesPresence)
				{
					FOnlineSessionInfoPlayFab* NewSessionInfo = new FOnlineSessionInfoPlayFab(EPlayFabSession::LobbySession, SearchSessionInfo->SessionId);
					Session->SessionInfo = MakeShareable(NewSessionInfo);

					//Return = JoinLobbySession(PlayerNum, Session, &DesiredSession.Session);
				}
				else
				{
					FOnlineSessionInfoPlayFab* NewSessionInfo = new FOnlineSessionInfoPlayFab(EPlayFabSession::AdvertisedSessionClient, SearchSessionInfo->SessionId);
					Session->SessionInfo = MakeShareable(NewSessionInfo);

					Return = JoinInternetSession(PlayerNum, Session, &DesiredSession.Session);
				}
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
		SessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr(IpAddr, SearchSessionInfo->HostAddr->GetPort());
		Result = ERROR_SUCCESS;
	}

	return Result;
}

bool FOnlineSessionPlayFab::FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Null subsystem
	TArray<FOnlineSessionSearchResult> EmptySearchResults;
	TriggerOnFindFriendSessionCompleteDelegates(LocalUserNum, false, EmptySearchResults);
	return false;
};

bool FOnlineSessionPlayFab::FindFriendSession(const FUniqueNetId& LocalUserId, const FUniqueNetId& Friend)
{
	// @todo: use proper LocalUserId
	return FindFriendSession(0, Friend);
}


bool FOnlineSessionPlayFab::FindFriendSession(const FUniqueNetId& LocalUserId, const TArray<TSharedRef<const FUniqueNetId>>& FriendList)
{
	TArray<FOnlineSessionSearchResult> EmptySearchResult;
	TriggerOnFindFriendSessionCompleteDelegates(0, false, EmptySearchResult);
	return false;
}

bool FOnlineSessionPlayFab::SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Null subsystem
	return false;
};

bool FOnlineSessionPlayFab::SendSessionInviteToFriend(const FUniqueNetId& LocalUserId, FName SessionName, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Null subsystem
	return false;
}

bool FOnlineSessionPlayFab::SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Null subsystem
	return false;
};

bool FOnlineSessionPlayFab::SendSessionInviteToFriends(const FUniqueNetId& LocalUserId, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Null subsystem
	return false;
}

bool FOnlineSessionPlayFab::PingSearchResults(const FOnlineSessionSearchResult& SearchResult)
{
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

			// These URL token will have to be authenticated in the GameMode...
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
	Players.Add(MakeShareable(new FUniqueNetIdString(PlayerId)));
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
	Players.Add(MakeShareable(new FUniqueNetIdString(PlayerId)));
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
	TickLanTasks(DeltaTime);
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
	Packet << *StaticCastSharedPtr<const FUniqueNetIdString>(Session->OwningUserId)
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
	FUniqueNetIdString* UniqueId = new FUniqueNetIdString;
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
