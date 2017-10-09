// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
 
#include "CoreMinimal.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "TimerManager.h"
#include "Core/PlayFabServerAPI.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabServerDataModels.h"
#include "Core/PlayFabClientDataModels.h"

/**
* Delegate fired when a player is authenicated(or not)
*
* @param UserId the user this authentication is for
* @param bWasSuccessful true if the authentication was valid, false if not or error
*/
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAuthenticatePlayerComplete, const FUniqueNetId& /*UserId*/, bool);
typedef FOnAuthenticatePlayerComplete::FDelegate FOnAuthenticatePlayerCompleteDelegate;

/**
* Interface definition for the online services session services
* Session services are defined as anything related managing a session
* and its state within a platform service
*/
class FOnlineSessionPlayFab : public IOnlineSession
{
private:

	/** Reference to the main PlayFab subsystem */
	class FOnlineSubsystemPlayFab* PlayFabSubsystem;

	/** Handles advertising sessions over LAN and client searches */
	class FLANSession* LANSession;

	FTimerHandle TimerHandle_PlayFabHeartbeat;
	FTimerDelegate TimerDelegate_PlayFabHeartbeat;

	void OnSuccessCallback_Client_GetCurrentGames(const PlayFab::ClientModels::FCurrentGamesResult& Result);
	void OnSuccessCallback_Client_Matchmake(const PlayFab::ClientModels::FMatchmakeResult& Result, FName SessionName);
	void OnErrorCallback_Client(const PlayFab::FPlayFabError& ErrorResult, FName FunctionName);
	void OnErrorCallback_Client(const PlayFab::FPlayFabError& ErrorResult, FName FunctionName, FName SessionName);

	void OnSuccessCallback_Server_RegisterGame(const PlayFab::ServerModels::FRegisterGameResponse& Result, FName SessionName);
	void OnSuccessCallback_Server_DeregisterGame(const PlayFab::ServerModels::FDeregisterGameResponse& Result, FName SessionName);
	void OnSuccessCallback_Server_InstanceState(const PlayFab::ServerModels::FSetGameServerInstanceStateResult& Result, FName SessionName);
	void OnSuccessCallback_Server_InstanceData(const PlayFab::ServerModels::FSetGameServerInstanceDataResult& Result, FName SessionName);
	void OnSuccessCallback_Server_AuthenticateSessionTicket(const PlayFab::ServerModels::FAuthenticateSessionTicketResult& Result, FName SessionName);
	void OnSuccessCallback_Server_RedeemMatchmakerTicket(const PlayFab::ServerModels::FRedeemMatchmakerTicketResult& Result, FName SessionName);
	void OnErrorCallback_Server(const PlayFab::FPlayFabError& ErrorResult, FName FunctionName, FName SessionName);
	void OnErrorCallback_Server(const PlayFab::FPlayFabError& ErrorResult, FName FunctionName, FName SessionName, TSharedRef<FUniqueNetId> PlayerId);

	void PlayFab_Server_Heartbeat(FName SessionName);

	void PlayerJoined(const FUniqueNetId& PlayerId, FName SessionName);
	void PlayerLeft(const FUniqueNetId& PlayerId, FName SessionName);

	/** Hidden on purpose */
	FOnlineSessionPlayFab()
		: PlayFabSubsystem(NULL)
		, CurrentSessionSearch(NULL)
	{}

	/**
	* Return the game server based session
	* NOTE: Assumes there is at most one, non-lobby session
	*
	* @return pointer to the struct if found, NULL otherwise
	*/
	inline FNamedOnlineSession* GetGameServerSession()
	{
		FScopeLock ScopeLock(&SessionLock);
		for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
		{
			FNamedOnlineSession& Session = Sessions[SearchIndex];
			if (Session.SessionInfo.IsValid())
			{
				FOnlineSessionInfoPlayFab* SessionInfo = (FOnlineSessionInfoPlayFab*)Session.SessionInfo.Get();
				if (SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionPlayFab || SessionInfo->SessionType == EPlayFabSession::AdvertisedSessionHost)
				{
					return &Sessions[SearchIndex];
				}
			}
		}
		return NULL;
	}

	/**
	* Ticks any lan beacon background tasks
	*
	* @param DeltaTime the time since the last tick
	*/
	void TickLanTasks(float DeltaTime);

	/**
	*	Create a game server session, advertised on the PlayFab backend
	*
	* @param HostingPlayerNum local index of the user initiating the request
	* @param Session newly allocated session to create
	*
	* @return ERROR_SUCCESS if successful, an error code otherwise
	*/
	uint32 CreateInternetSession(int32 HostingPlayerNum, class FNamedOnlineSession* Session);

	/**
	*	Create a local LAN session, managed by a beacon on the host
	*
	* @param HostingPlayerNum local index of the user initiating the request
	* @param Session newly allocated session to create
	*
	* @return ERROR_SUCCESS if successful, an error code otherwise
	*/
	uint32 CreateLANSession(int32 HostingPlayerNum, class FNamedOnlineSession* Session);

	/**
	*	Update online session data
	*
	* @param Session Session to update online service for
	*
	* @return ERROR_SUCCESS if successful, an error code otherwise
	*/
	uint32 UpdateInternetSession(FNamedOnlineSession* Session);

	/**
	*	Destroy an internet session, advertised on the PlayFab backend
	*
	* @param Session session to destroy
	* @param CompletionDelegate Delegate of completion
	*
	* @return ERROR_SUCCESS if successful, an error code otherwise
	*/
	uint32 DestroyInternetSession(class FNamedOnlineSession* Session, const FOnDestroySessionCompleteDelegate& CompletionDelegate);

	/**
	*	Join a game server session, advertised on the Steam backend
	*
	* @param PlayerNum local index of the user initiating the request
	* @param Session newly allocated session with join information
	* @param SearchSession the desired session to join
	*
	* @return ERROR_SUCCESS if successful, an error code otherwise
	*/
	uint32 JoinInternetSession(int32 PlayerNum, FNamedOnlineSession* Session, const FOnlineSession* SearchSession);

	/**
	*	Join a LAN session
	*
	* @param PlayerNum local index of the user initiating the request
	* @param Session newly allocated session with join information
	* @param SearchSession the desired session to join
	*
	* @return ERROR_SUCCESS if successful, an error code otherwise
	*/
	uint32 JoinLANSession(int32 PlayerNum, class FNamedOnlineSession* Session, const class FOnlineSession* SearchSession);

	/**
	* Builds a Steamworks query and submits it to the Steamworks backend
	*
	* @return an Error/success code
	*/
	uint32 FindInternetSession(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings);

	/**
	* Builds a LAN search query and broadcasts it
	*
	* @return ERROR_SUCCESS if successful, an error code otherwise
	*/
	uint32 FindLANSession();

	/**
	* Adds the game session data to the packet that is sent by the host
	* in response to a server query
	*
	* @param Packet the writer object that will encode the data
	* @param Session the session to add to the packet
	*/
	void AppendSessionToPacket(class FNboSerializeToBufferPlayFab& Packet, class FOnlineSession* Session);

	/**
	* Adds the game settings data to the packet that is sent by the host
	* in response to a server query
	*
	* @param Packet the writer object that will encode the data
	* @param SessionSettings the session settings to add to the packet
	*/
	void AppendSessionSettingsToPacket(class FNboSerializeToBufferPlayFab& Packet, FOnlineSessionSettings* SessionSettings);

	/**
	* Reads the settings data from the packet and applies it to the
	* specified object
	*
	* @param Packet the reader object that will read the data
	* @param SessionSettings the session settings to copy the data to
	*/
	void ReadSessionFromPacket(class FNboSerializeFromBufferPlayFab& Packet, class FOnlineSession* Session);

	/**
	* Reads the settings data from the packet and applies it to the
	* specified object
	*
	* @param Packet the reader object that will read the data
	* @param SessionSettings the session settings to copy the data to
	*/
	void ReadSettingsFromPacket(class FNboSerializeFromBufferPlayFab& Packet, FOnlineSessionSettings& SessionSettings);

	/**
	* Delegate triggered when the LAN beacon has detected a valid client request has been received
	*
	* @param PacketData packet data sent by the requesting client with header information removed
	* @param PacketLength length of the packet not including header size
	* @param ClientNonce the nonce returned by the client to return with the server packet
	*/
	void OnValidQueryPacketReceived(uint8* PacketData, int32 PacketLength, uint64 ClientNonce);

	/**
	* Delegate triggered when the LAN beacon has detected a valid host response to a client request has been received
	*
	* @param PacketData packet data sent by the requesting client with header information removed
	* @param PacketLength length of the packet not including header size
	*/
	void OnValidResponsePacketReceived(uint8* PacketData, int32 PacketLength);

	/**
	* Delegate triggered when the LAN beacon has finished searching (some time after last received host packet)
	*/
	void OnLANSearchTimeout();

	/**
	* Attempt to set the host port in the session info based on the actual port the netdriver is using.
	*/
	static void SetPortFromNetDriver(const FOnlineSubsystemPlayFab& Subsystem, const TSharedPtr<FOnlineSessionInfo>& SessionInfo);

PACKAGE_SCOPE:

	/** Critical sections for thread safe operation of session lists */
	mutable FCriticalSection SessionLock;

	/** Current session settings */
	TArray<FNamedOnlineSession> Sessions;

	/** Current search object */
	TSharedPtr<FOnlineSessionSearch> CurrentSessionSearch;

	TSharedPtr<FOnlineSessionSearch> CurrentMatchmakeSearch;
	FName CurrentMatchmakeName;

	/** Current search start time. */
	double SessionSearchStartInSeconds;

	FOnlineSessionPlayFab(class FOnlineSubsystemPlayFab* InPlayFabSubsystem) :
		PlayFabSubsystem(InPlayFabSubsystem),
		CurrentSessionSearch(NULL),
		SessionSearchStartInSeconds(0)
	{}

	/**
	* Session tick for various background tasks
	*/
	void Tick(float DeltaTime);

	// IOnlineSession
	class FNamedOnlineSession* AddNamedSession(FName SessionName, const FOnlineSessionSettings& SessionSettings) override
	{
		FScopeLock ScopeLock(&SessionLock);
		return new (Sessions) FNamedOnlineSession(SessionName, SessionSettings);
	}

	class FNamedOnlineSession* AddNamedSession(FName SessionName, const FOnlineSession& Session) override
	{
		FScopeLock ScopeLock(&SessionLock);
		return new (Sessions) FNamedOnlineSession(SessionName, Session);
	}

	/**
	* Parse the command line for invite/join information at launch
	*/
	void CheckPendingSessionInvite();

	/**
	* Registers and updates voice data for the given player id
	*
	* @param PlayerId player to register with the voice subsystem
	*/
	void RegisterVoice(const FUniqueNetId& PlayerId);

	/**
	* Unregisters a given player id from the voice subsystem
	*
	* @param PlayerId player to unregister with the voice subsystem
	*/
	void UnregisterVoice(const FUniqueNetId& PlayerId);

	/**
	* Registers all local players with the current session
	*
	* @param Session the session that they are registering in
	*/
	void RegisterLocalPlayers(class FNamedOnlineSession* Session);

public:

	virtual ~FOnlineSessionPlayFab() {}

	FNamedOnlineSession* GetNamedSession(FName SessionName) override
	{
		FScopeLock ScopeLock(&SessionLock);
		for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
		{
			if (Sessions[SearchIndex].SessionName == SessionName)
			{
				return &Sessions[SearchIndex];
			}
		}
		return NULL;
	}

	virtual void RemoveNamedSession(FName SessionName) override
	{
		FScopeLock ScopeLock(&SessionLock);
		for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
		{
			if (Sessions[SearchIndex].SessionName == SessionName)
			{
				Sessions.RemoveAtSwap(SearchIndex);
				return;
			}
		}
	}

	virtual EOnlineSessionState::Type GetSessionState(FName SessionName) const override
	{
		FScopeLock ScopeLock(&SessionLock);
		for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
		{
			if (Sessions[SearchIndex].SessionName == SessionName)
			{
				return Sessions[SearchIndex].SessionState;
			}
		}
		
		return EOnlineSessionState::NoSession;
	}

	virtual bool HasPresenceSession() override
	{
		FScopeLock ScopeLock(&SessionLock);
		for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
		{
			if (Sessions[SearchIndex].SessionSettings.bUsesPresence)
			{
				return true;
			}
		}

		return false;
	}

	static FOnlineSessionPlayFab* GetOnlineSessionPlayFab(IOnlineSubsystem* Subsystem);

	/**
	 * Begins authentication of the player specified
	 * NOTE: user authorization is an async process and does not complete
	 * until the OnAuthenticatePlayerComplete delegate is called.
	 *
	 * @param PlayerId the id to authenticate
	 * @param SessionName the name to use for this session so that multiple sessions can exist at the same time
	 * @param SessionTicket the authorization ticket
	 * @param bIsMatchmakeTicket is the ticket from a Client/Matchmake call
	 *
	 * @return true if successfully authenticating the player, false otherwise
	 */
	bool AuthenticatePlayer(const FUniqueNetId& PlayerId, FName SessionName, FString SessionTicket, bool bIsMatchmakeTicket);

	/**
	 * Delegate fired when a session create request has completed
	 *
	 * @param SessionName the name of the session this callback is for
	 * @param bWasSuccessful true if the async action completed without error, false if there was an error
	 */
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnAuthenticatePlayerComplete, const FUniqueNetId&, bool);

	// IOnlineSession
	virtual bool CreateSession(int32 HostingPlayerNum, FName SessionName, const FOnlineSessionSettings& NewSessionSettings) override;
	virtual bool CreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings) override;
	virtual bool StartSession(FName SessionName) override;
	virtual bool UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData = true) override;
	virtual bool EndSession(FName SessionName) override;
	virtual bool DestroySession(FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate = FOnDestroySessionCompleteDelegate()) override;
	virtual bool IsPlayerInSession(FName SessionName, const FUniqueNetId& UniqueId) override;
	virtual bool StartMatchmaking(const TArray< TSharedRef<const FUniqueNetId> >& LocalPlayers, FName SessionName, const FOnlineSessionSettings& NewSessionSettings, TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool CancelMatchmaking(int32 SearchingPlayerNum, FName SessionName) override;
	virtual bool CancelMatchmaking(const FUniqueNetId& SearchingPlayerId, FName SessionName) override;
	virtual bool FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool FindSessions(const FUniqueNetId& SearchingPlayerId, const TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool FindSessionById(const FUniqueNetId& SearchingUserId, const FUniqueNetId& SessionId, const FUniqueNetId& FriendId, const FOnSingleSessionResultCompleteDelegate& CompletionDelegate) override;
	virtual bool CancelFindSessions() override;
	virtual bool PingSearchResults(const FOnlineSessionSearchResult& SearchResult) override;
	virtual bool JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession) override;
	virtual bool JoinSession(const FUniqueNetId& PlayerId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession) override;
	virtual bool FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend) override;
	virtual bool FindFriendSession(const FUniqueNetId& LocalUserId, const FUniqueNetId& Friend) override;
	virtual bool FindFriendSession(const FUniqueNetId& LocalUserId, const TArray<TSharedRef<const FUniqueNetId>>& FriendList) override;
	virtual bool SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend) override;
	virtual bool SendSessionInviteToFriend(const FUniqueNetId& LocalUserId, FName SessionName, const FUniqueNetId& Friend) override;
	virtual bool SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends) override;
	virtual bool SendSessionInviteToFriends(const FUniqueNetId& LocalUserId, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends) override;
	virtual bool GetResolvedConnectString(FName SessionName, FString& ConnectInfo, FName PortType = GamePort) override;
	virtual bool GetResolvedConnectString(const class FOnlineSessionSearchResult& SearchResult, FName PortType, FString& ConnectInfo) override;
	virtual FOnlineSessionSettings* GetSessionSettings(FName SessionName) override;
	virtual bool RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited) override;
	virtual bool RegisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players, bool bWasInvited = false) override;
	virtual bool UnregisterPlayer(FName SessionName, const FUniqueNetId& PlayerId) override;
	virtual bool UnregisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players) override;
	virtual void RegisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnRegisterLocalPlayerCompleteDelegate& Delegate) override;
	virtual void UnregisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnUnregisterLocalPlayerCompleteDelegate& Delegate) override;
	virtual int32 GetNumSessions() override;
	virtual void DumpSessionState() override;
};

typedef TSharedPtr<FOnlineSessionPlayFab, ESPMode::ThreadSafe> FOnlineSessionPlayFabPtr;