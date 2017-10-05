# OnlineSubsystemPlayFab
This is an Online Subsystem for the Unreal Engine 4, using PlayFab.

## Requirements
This requires:  
PlayFab C++ from here: https://github.com/PlayFab/UnrealCppSdk  

For OnlineSubsystem in blueprints:  
Advanced Sessions Plugin from here: https://bitbucket.org/mordentral/advancedsessionsplugin/downloads/  
You only require AdvancedSessions, AdvancedSessionsSteam is for, well, Steam.  

# Installing Subsystem
Alright, so here we go, to get started, download the PlayFab C++ SDK from above.
Add the OnlineSubsystemPlayFab into your Plugins.
Open your Config/DefaultEngine.ini and add:

```ini
[OnlineSubsystem]
DefaultPlatformService=PlayFab

[OnlineSubsystemPlayFab]
bEnable=true
```

Now, you may build your project and open it. Goto your ProjectSettings and fill in the PlayFab Title Id and Secret key(Should only have Secret Key on servers, as PlayFab says)  
Assuming you also downloaded AdvancedSessions you can open blueprints and begin your work.  
This will most likely not be enough for everything to know that you are logged in, you may need to set the CachedUniqueNetId on the ULocalPlayer.(ULocalPlayer::SetCachedUniqueNetId)  

Sessions will use the Project Setting's build version to send to PlayFab, make sure they stay consistent!  

## All Config Variables
```ini
[OnlineSubsystemPlayFab]
bEnable=true
bEnableXpp=true
bCustomMatchmaker=false
HeartBeatInterval=60.0
XmppPort=5222
XmppHost=192.168.0.100
```

#### I've logged in to the subsystem, now what?
##### Clients
I have implemented most blueprint avaiable interfaces, so just give them a call.  
I haven't had time to test them *all* so report any bugs and I'll take a look!

Suggestions:
1. Find Sessions Advanced
2. Join Session
3. Get Friend List

Make sure to call Destroy Session when the player leaves a server, or else they can't join any others!  

##### Servers
So, if you are submitting builds to PlayFab, then your Secret key will be provided via command line, no worries about trying to provide it.  
If you're not submitting your builds to PlayFab, then you need to add an argument when running the server, "-title_secret_key=YOUR_KEY_HERE"(Unless you have Secret Key set in the Project Settings)  
Please, never ever ever give out your client/server builds with the secret key filled in, in the project settings!  

After that discussion, on to the actual steps of a server(These I call within GameSession, though you could use GameInstance or GameMode):  
1. On Init, call CreateSession/CreateAdvancedSession with the information filled. This will provide PlayFab with the server data(even if a PlayFab server, still call this!)
2. Call StartSession after Session is created and the game has begun(after any warm up)
3. Whenever something on the session changes, give UpdateSession a call(to update the server instance tags/instance data)
4. Call EndSession after the Game is done(entering a cooldown state such as showing scoreboard before sending users to home screen)
5. Call Destroy Session when server is closing(Call this especially for external servers, or else PlayFab will keep them listed!)

# XMPP, if you're into that sort of thing

## Picking your server
I personally am using [Prosody](https://prosody.im/), Epic Games is using [Tigase](http://tigase.net/).  
Don't use [eJabberd](https://www.ejabberd.im/), they don't play well with Jingle: https://www.ejabberd.im/node/1354  

## Enabling Subsystem XMPP
```ini
[OnlineSubsystemPlayFab]
bEnable=true
bEnableXmpp=true
XmppPort=5222
XmppHost=192.168.0.1
```

XmppHost can be a DNS address or IP Address.  
5222 is the default XMPP Port.  

Now, when the client logs into PlayFab, it will send the PlayFabId as the username, SessionTicket as the password, and the TitleId as the domain.  
You can view the implementation of this [here](https://gitlab.com/mtuska/OnlineSubsystemPlayFab/blob/master/Plugins/OnlineSubsystemPlayFab/Source/OnlineSubsystemPlayFab/Private/OnlineIdentityPlayFab.cpp#L390)  

## FAQ
##### Actually these are just random problems I'll just assume you'll run into, because I'm rude


1. ###### *Everything* seems to be failing! It's absurd!
##### Client:
Did you login first?
##### Server:
Welp, I don't have anything to say for you at the moment...

2. ###### *Help!* I'm receiving the error " - Cannot map local player to unique net ID" when trying to run anything after logging in!
Make sure you have logged in. if you have, make sure you can called "Show External Login UI" to finish the login processes.  

3. ###### How do I register new players?
The OnlineSubsystem has no implementations for Registration, as such you will just have to use PlayFab's API directly to register new users. I currently(this will be adjusted later) only use https://api.playfab.com/documentation/client/method/LoginWithPlayFab and as such, you will need to use https://api.playfab.com/documentation/client/method/RegisterPlayFabUser

4. ###### Region Lock  
In order to specify the region to create session in or find session in, which by default is USCentral, provide the "REGION" filter. This should correspond to one of the following region values:  
```
"Australia"
"Brazil"
"EUWest"
"Japan"
"Singapore"
"USCentral"
"USEast"
```

## Known Issues

1. LAN  
Yep, LAN isn't working correctly at the moment, I'll get to fixing it later
2. Authentication  
Currently, there is no proper authenticating of users. This is due to the limitations of the OnlineSubsystem, and I shall see what fixes I can provide as I continue to work on this.
