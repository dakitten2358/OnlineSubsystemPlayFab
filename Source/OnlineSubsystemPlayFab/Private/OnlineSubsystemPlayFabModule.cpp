// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPlayFabModule.h"
#include "OnlineSubsystemPlayFabPrivatePCH.h"
#include "OnlineSubsystemPlayFab.h"
#include "Modules/ModuleManager.h"
#include "PlayFab.h"

IMPLEMENT_MODULE(FOnlineSubsystemPlayFabModule, OnlineSubsystemPlayFab);

/**
* Class responsible for creating instance(s) of the subsystem
*/
class FOnlineFactoryPlayFab : public IOnlineFactory
{
public:

	FOnlineFactoryPlayFab() {}
	virtual ~FOnlineFactoryPlayFab() {}

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName)
	{
		FOnlineSubsystemPlayFabPtr OnlineSub = MakeShareable(new FOnlineSubsystemPlayFab(FName("OnlineSubsystemPlayFab")));
		if (OnlineSub->IsEnabled())
		{
			if (!OnlineSub->Init())
			{
				UE_LOG_ONLINE(Warning, TEXT("PlayFab API failed to initialize!"));
				OnlineSub->Shutdown();
				OnlineSub = NULL;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("PlayFab API disabled!"));
			OnlineSub->Shutdown();
			OnlineSub = NULL;
		}

		return OnlineSub;
	}
};

void FOnlineSubsystemPlayFabModule::StartupModule()
{
	// Hopefully load PlayFab before needed
	//FModuleManager::LoadModuleChecked<IPlayFabModuleInterface>("PlayFab");

	UE_LOG(LogOnline, Log, TEXT("PlayFab Startup!"));
	
	PlayFabFactory = new FOnlineFactoryPlayFab();

	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(PLAYFAB_SUBSYSTEM, PlayFabFactory);
}

void FOnlineSubsystemPlayFabModule::ShutdownModule()
{
	UE_LOG(LogOnline, Log, TEXT("PlayFab Shutdown!"));
	
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(PLAYFAB_SUBSYSTEM);

	delete PlayFabFactory;
	PlayFabFactory = NULL;
}
