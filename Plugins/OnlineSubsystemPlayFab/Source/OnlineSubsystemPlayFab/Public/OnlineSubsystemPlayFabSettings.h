// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/* PlayFab server id */
#define SETTING_LOBBYID FName(TEXT("LOBBYID"))
/* PlayFab friendly GameMode name */
#define SETTING_GAMENAME FName(TEXT("GAMENAME"))
/* PlayFab Matchmaker: Character to use for stats based matching */
#define SETTING_MATCHING_CHARACTERID FName(TEXT("MATCHCHARACTERID"))
/* PlayFab Statistic name to find statistic-based matches */
#define SETTING_MATCHING_STATISTICNAME FName(TEXT("MATCHSTATISTICNAME"))
/* PlayFab Matchmaker: Start a new server if no open slot is found */
#define SETTING_MATCHING_STARTNEW FName(TEXT("MATCHSTARTNEW"))