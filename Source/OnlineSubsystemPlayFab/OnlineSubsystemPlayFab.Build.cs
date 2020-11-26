// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class OnlineSubsystemPlayFab : ModuleRules
{
    public OnlineSubsystemPlayFab(ReadOnlyTargetRules Target)
        : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDefinitions.Add("ONLINESUBSYSTEMPLAYFAB_PACKAGE=1");

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

        PrivateDependencyModuleNames.AddRange(
            new string[] {
				"Core",
				"CoreUObject",
                "Engine",
                "Sockets",
                "Json",
                "Icmp",
                "XMPP",// WANT TO MAKE A NOTE HERE, EPIC GAMES USES "tigase" FOR XMPP SERVER
                "PlayFab",
                "PlayFabCpp",
			}
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "OnlineSubsystem",
                "OnlineSubsystemUtils",
            }
        );
    }
}
