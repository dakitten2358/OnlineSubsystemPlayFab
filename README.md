# OnlineSubsystemPlayFab
This is an Online Subsystem for the Unreal Engine 4, using PlayFab.

## Requirements
This requires:  
PlayFab C++ from here: https://github.com/PlayFab/UnrealCppSdk  

For OnlineSubsystem in blueprints:  
Advanced Sessions Plugin from here: https://bitbucket.org/mordentral/advancedsessionsplugin/downloads/  

## FAQ
##### Actually these are just random problems I'll just assume you'll run into, because I'm rude


1. ###### *Everything* seems to be failing! It's absurd!
##### Client:
Did you login first?
##### Server:
Welp, I don't have anything to say for you at the moment...


2. ###### *Help!* I'm receiving the error " - Cannot map local player to unique net ID" when trying to run anything after logging in!
You're running into a wonderful thing that makes no sense! So, after logging in, your PlayerState's UniqueId isn't being set! Why is this happening? Well, because Epic decided so. Personally, in C++ I have a OnLoginComplete callback that sets my PlayerState's UnqiueId via SetUniqueId. For AdvancedSessions, I will shoot them a request to try and have them add this fix for their plugin.