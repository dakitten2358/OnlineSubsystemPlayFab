// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystemTypes.h"
#include "IPAddress.h"

class FOnlineSubsystemPlayFab;

/** Possible session states */
namespace EPlayFabSession
{
	enum Type
	{
		/** Session is undefined */
		None,
		/** Session managed as a lobby on backend */
		LobbySession,
		/** Session managed by master server publishing */
		AdvertisedSessionHost,
		/** Session client of a game server session */
		AdvertisedSessionClient,
		/** Session managed by LAN beacon */
		LANSession,
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EPlayFabSession::Type SessionType)
	{
		switch (SessionType)
		{
		case None:
		{
			return TEXT("Session undefined");
		}
		case LobbySession:
		{
			return TEXT("Lobby session");
		}
		case AdvertisedSessionHost:
		{
			return TEXT("Advertised Session Host");
		}
		case AdvertisedSessionClient:
		{
			return TEXT("Advertised Session Client");
		}
		case LANSession:
		{
			return TEXT("LAN Session");
		}
		}
		return TEXT("");
	}
}

/**
* Implementation of session information
*/
class FOnlineSessionInfoPlayFab : public FOnlineSessionInfo
{
protected:

	/** Hidden on purpose */
	FOnlineSessionInfoPlayFab(const FOnlineSessionInfoPlayFab& Src)
	{
	}

	/** Hidden on purpose */
	FOnlineSessionInfoPlayFab& operator=(const FOnlineSessionInfoPlayFab& Src)
	{
		return *this;
	}

PACKAGE_SCOPE:

	/** Constructor */
	FOnlineSessionInfoPlayFab(EPlayFabSession::Type InSessionType = EPlayFabSession::None);

	FOnlineSessionInfoPlayFab(EPlayFabSession::Type InSessionType, const FUniqueNetIdString& InSessionId);

	FOnlineSessionInfoPlayFab(EPlayFabSession::Type InSessionType, const FUniqueNetIdString& InSessionId, FString InMatchmakeTicket);

	/**
	* Initialize a PlayFab session info with the address of this machine
	* and an id for the session
	*/
	void Init(const FOnlineSubsystemPlayFab& Subsystem);

	/**
	* Initialize a Null session info with the address of this machine
	* and an id for the session
	*/
	void InitLAN(const FOnlineSubsystemPlayFab& Subsystem);

	/** Type of session this is, affects interpretation of id below */
	EPlayFabSession::Type SessionType;
	/** The ip & port that the host is listening on (valid for LAN/GameServer) */
	TSharedPtr<class FInternetAddr> HostAddr;
	/** Unique Id for this session */
	FUniqueNetIdString SessionId;
	FString MatchmakeTicket;

public:

	virtual ~FOnlineSessionInfoPlayFab() {}

	bool operator==(const FOnlineSessionInfoPlayFab& Other) const
	{
		return false;
	}

	virtual const uint8* GetBytes() const override
	{
		return NULL;
	}

	virtual int32 GetSize() const override
	{
		return sizeof(uint64) + sizeof(TSharedPtr<class FInternetAddr>);
	}

	virtual bool IsValid() const override
	{
		// LAN case
		return HostAddr.IsValid() && HostAddr->IsValid();
	}

	virtual FString ToString() const override
	{
		return SessionId.ToString();
	}

	virtual FString ToDebugString() const override
	{
		return FString::Printf(TEXT("HostIP: %s SessionId: %s"),
			HostAddr.IsValid() ? *HostAddr->ToString(true) : TEXT("INVALID"),
			*SessionId.ToDebugString());
	}

	virtual const FUniqueNetId& GetSessionId() const override
	{
		return SessionId;
	}
};

