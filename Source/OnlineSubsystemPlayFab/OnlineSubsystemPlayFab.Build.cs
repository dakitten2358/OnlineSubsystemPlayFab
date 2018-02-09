// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class OnlineSubsystemPlayFab : ModuleRules
{
    public OnlineSubsystemPlayFab(ReadOnlyTargetRules Target)
        : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        Definitions.Add("ONLINESUBSYSTEMPLAYFAB_PACKAGE=1");

        PrivateDependencyModuleNames.AddRange(
            new string[] {
				"Core",
				"CoreUObject",
                "Engine",
                "Sockets",
                "OnlineSubsystem",
                "OnlineSubsystemUtils",
                "ICMP",
                "Json",
                "XMPP",// WANT TO MAKE A NOTE HERE, EPIC GAMES USES "tigase" FOR XMPP SERVER
                "PlayFab"
			}
            );
    }
}
