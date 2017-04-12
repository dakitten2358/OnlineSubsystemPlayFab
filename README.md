# OnlineSubsystemPlayFab
This is an Online Subsystem for the Unreal Engine 4, using PlayFab.  
#### This is the master branch, which means it is probably not stable. Use at your own discretion.

## Requirements
This requires:  
PlayFab C++ from here: https://github.com/PlayFab/UnrealCppSdk  

For OnlineSubsystem in blueprints:  
Advanced Sessions Plugin from here: https://bitbucket.org/mordentral/advancedsessionsplugin/downloads/  
You only require AdvancedSessions, AdvancedSessionsSteam is for, well, Steam.  

# Installing Subsystem
Alright, so here we go, to get started, download the PlayFab C++ SDK from above.  
Now that we have that, go into PlayFab/Source and delete the OnlineSubsystemPlayFab.  
That standard OnlineSubsystem is useless, full of null pointers, so it had no effect.  
Now, download this plugin, and merge it with the PlayFab plugin.  
To finish, open your Config/DefaultEngine.ini and add:

```ini
[OnlineSubsystem]
DefaultPlatformService=PlayFab

[OnlineSubsystemPlayFab]
bEnabled=true
```

Now, you may build your project and open it. Goto your ProjectSettings and fill in the PlayFab Title Id and Secret key(Only have filled in editor!!)  
Assuming you also downloaded AdvancedSessions you can open blueprints and begin your work.  

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
If you're not submitting your builds to PlayFab, then you need to add an argument when running the server, "-title_secret_key=YOUR_KEY_HERE"  
Please, never ever ever give out your client/server builds with the secret key filled in, in the project settings!  

After that discussion, on to the actual steps of a server(I call most of these inside GameInstance, so assume those functions):  
1. On Init, call CreateAdvancedSession with the information filled. This will provide PlayFab with the server data(even if a PlayFab server, still call this!)
2. Whenever something on the server changes, give UpdateSession a call
3. Call Destroy Session when server is closing(Call this especially for external servers, or else PlayFab will keep them listed!)

That should be it for the server.
  
  
  
  
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


## Known Issues

1. LAN  
Yep, LAN isn't working correctly at the moment, I'll get to fixing it later
2. Authentication  
Currently, there is no proper authenticating of users. This is due to the limitations of the OnlineSubsystem, and I shall see what fixes I can provide as I continue to work on this.