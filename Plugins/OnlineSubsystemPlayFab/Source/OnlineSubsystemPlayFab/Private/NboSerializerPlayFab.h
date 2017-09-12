// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NboSerializer.h"
#include "OnlineSubsystemPlayFabTypes.h"

/**
* Serializes data in network byte order form into a buffer
*/
class FNboSerializeToBufferPlayFab : public FNboSerializeToBuffer
{
public:
	/** Default constructor zeros num bytes*/
	FNboSerializeToBufferPlayFab() :
		FNboSerializeToBuffer(512)
	{
	}

	/** Constructor specifying the size to use */
	FNboSerializeToBufferPlayFab(uint32 Size) :
		FNboSerializeToBuffer(Size)
	{
	}

	/**
	* Adds Null session info to the buffer
	*/
	friend inline FNboSerializeToBufferPlayFab& operator<<(FNboSerializeToBufferPlayFab& Ar, const FOnlineSessionInfoPlayFab& SessionInfo)
	{
		check(SessionInfo.HostAddr.IsValid());
		// Skip SessionType (assigned at creation)
		Ar << SessionInfo.SessionId;
		Ar << *SessionInfo.HostAddr;
		return Ar;
	}

	/**
	* Adds Null Unique Id to the buffer
	*/
	friend inline FNboSerializeToBufferPlayFab& operator<<(FNboSerializeToBufferPlayFab& Ar, const FUniqueNetIdString& UniqueId)
	{
		Ar << UniqueId.UniqueNetIdStr;
		return Ar;
	}
};

/**
* Class used to write data into packets for sending via system link
*/
class FNboSerializeFromBufferPlayFab : public FNboSerializeFromBuffer
{
public:
	/**
	* Initializes the buffer, size, and zeros the read offset
	*/
	FNboSerializeFromBufferPlayFab(uint8* Packet, int32 Length) :
		FNboSerializeFromBuffer(Packet, Length)
	{
	}

	/**
	* Reads Null session info from the buffer
	*/
	friend inline FNboSerializeFromBufferPlayFab& operator >> (FNboSerializeFromBufferPlayFab& Ar, FOnlineSessionInfoPlayFab& SessionInfo)
	{
		check(SessionInfo.HostAddr.IsValid());
		// Skip SessionType (assigned at creation)
		Ar >> SessionInfo.SessionId;
		Ar >> *SessionInfo.HostAddr;
		return Ar;
	}

	/**
	* Reads Null Unique Id from the buffer
	*/
	friend inline FNboSerializeFromBufferPlayFab& operator >> (FNboSerializeFromBufferPlayFab& Ar, FUniqueNetIdString& UniqueId)
	{
		Ar >> UniqueId.UniqueNetIdStr;
		return Ar;
	}
};