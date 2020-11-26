// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemTypes.h"
#include "IPAddress.h"

// Define our own FUniqueNetIds, this way if something changes on UE4 or PlayFab, we can make changes here, instead of also having
// To update all the files using the FUniqueNetIdString. Just better code, plus when you see FUniqueNetIdPlayFabId vs FUniqueNetIdLobbyId
// you know specifically what you're looking at(though most of the time it's a FUniqueNetId reference, so, you're still screwed there)
static FName NAME_PlayFab = TEXT("PLAYFAB");

TEMP_UNIQUENETIDSTRING_SUBCLASS(FUniqueNetIdPlayFabId, NAME_PlayFab)
typedef FUniqueNetIdPlayFabId FUniqueNetIdLobbyId;

class FOnlineSubsystemPlayFab;

/** Possible session states */
namespace EPlayFabSession
{
	enum Type
	{
		/** Session is undefined */
		None,
		/** Session managed by PlayFab */
		AdvertisedSessionPlayFab,
		/** Session managed by PlayFab master server */
		AdvertisedSessionHost,
		/** Client copy of a game server session */
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
		case AdvertisedSessionPlayFab:
		{
			return TEXT("Advertised Session PlayFab");
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

	FOnlineSessionInfoPlayFab(EPlayFabSession::Type InSessionType, const FUniqueNetIdLobbyId& InSessionId, FString InMatchmakeTicket = "");

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
	FUniqueNetIdLobbyId SessionId;
	/** Only valid on the client, the ticket to authenticate to the server */
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
