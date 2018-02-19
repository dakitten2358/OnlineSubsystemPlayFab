// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/* Server's Name */
#define SETTING_SERVERNAME FName(TEXT("SERVERNAME"))
/* PlayFab friendly GameMode name */
#define SETTING_GAMENAME FName(TEXT("GAMENAME"))


// Searching for playfab server via matchmaker

/* PlayFab server id(For matchmaking a specific instance) */
#define SETTING_MATCHING_LOBBYID FName(TEXT("LOBBYID"))
/* PlayFab Matchmaker: Character to use for stats based matching */
#define SETTING_MATCHING_CHARACTERID FName(TEXT("MATCHCHARACTERID"))
/* PlayFab Matchmaker: Start a new server if no open slot is found */
#define SETTING_MATCHING_STARTNEW FName(TEXT("MATCHSTARTNEW"))


// Searching for playfab server via any means

/* PlayFab Statistic name to find statistic-based matches */
#define SETTING_MATCHING_STATISTICNAME FName(TEXT("MATCHSTATISTICNAME"))