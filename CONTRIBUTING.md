# Contributing

## Pull requests are welcome!

Come [join the Discord channel](https://steamcommunity.com/groups/ANPA/discussions/0/487876568238532577/) to discuss the project
(see the "nt_semantic_sequels" and other related channels under the channel group "Architects").

## Getting started

### Cloning & merging

It's recommended you fork [the dev branch](https://github.com/NeotokyoRevamp/neo/tree/dev), and pull request your work there back to it.

The dev branch will periodically get merged back to the master branch, as new things become stable enough.

### Building

See [build instructions](BUILD_INSTRUCTIONS.md) in this repo for setting up your build environment (currently supporting Windows/Linux).

### Debugging
To be safe and avoid problems with VAC, it's recommended to add a [-insecure](https://developer.valvesoftware.com/wiki/Command_Line_Options) launch flag before attaching your debugger.

Example settings for debugging from Visual Studio solutions:

| Configuration Properties->Debugging | Example value |
| :---------------------------------- | :------------ |
| Command | C:\Steam\steamapps\common\Source SDK Base 2013 Multiplayer\hl2.exe |
| Command Arguments | -allowdebug -insecure -dev -sw -game "C:\Steam\steamapps\sourcemods\neo" |
| Working Directory | C:\\steamapps\common\Source SDK Base 2013 Multiplayer |

### Game loop and reference material

[Break pointing and stepping](https://developer.valvesoftware.com/wiki/Installing_and_Debugging_the_Source_Code) the method [CServerGameDLL::GameFrame](mp/src/game/server/gameinterface.cpp), or relevant methods in [C_NEO_Player](mp/src/game/client/neo/c_neo_player.h) (clientside player) / [CNEO_Player](mp/src/game/server/neo/neo_player.h) (serverside player) can help with figuring out the general game loop. Neo specific files usually live in [game/client/neo](mp/src/game/client/neo), [game/server/neo](mp/src/game/server/neo), or [game/shared/neo](mp/src/game/shared/neo), similar to how most hl2mp code is laid out.

Ochii's impressive [reverse engineering project](https://github.com/Ochii/neotokyo-re) can also serve as reference for figuring things out. However, please refrain from copying reversed instructions line by line, as the plan is to write an open(ed) source (wherever applicable, note the Source SDK license) reimplementation, and steer clear of any potential copyright issues. Same thing applies for original NT assets; you can depend on the original NT installation (it's mounted to the engine filesystem by [a shared neo header](mp/src/game/shared/neo/neo_mount_original.h)), but avoid pushing those assets in the repo.

## Good to know

### Solutions/makefiles

This project uses Valve's [VPC system](https://developer.valvesoftware.com/wiki/VPC) to generate its makefiles and VS solutions. When modifying the project file structure, instead of pushing your solution/makefile, edit the relevant VPC files instead (most commonly "[client_hl2mp.vpc](mp/src/game/client/client_hl2mp.vpc)" and "[server_hl2mp.vpc](mp/src/game/server/server_hl2mp.vpc)").

Running the VPC scripts after a change will generate the solutions and makefiles on all platforms. (You may sometimes have to purge your object file cache if you get linker errors after restructuring existing translation units.) Note that on Windows, this will also overwrite any of your hand edited solution preferences.

### Preprocessor definitions
In shared code, clientside code can be differentiated with CLIENT_DLL, vs. serverside's GAME_DLL. In more general engine files, Neotokyo specific code can be marked with a NEO ifdef. These are also defined with the VPC system described above, in the $PreprocessorDefinitions section.

### Code style

No big restrictions, just try to more or less match the other SDK code style.

* C++11
* Valve likes to ( space ) their arguments, especially with macros, but it's not necessary to strictly follow everywhere.
* Tabs are preferred for whitespace, to be consistent with the SDK code.
* For classes running on both client and server, you should generally follow Valve's <i>C_Thing</i> (client) -- <i>CThing</i> (server) convention. On shared files, this might mean #defining serverclass for client, or vice versa. There's plenty of examples of this pattern in Valve's classes for reference, [for example here](https://github.com/NeotokyoRevamp/neo/blob/f749c07a4701d285bbb463686d5a5a50c20b9528/mp/src/game/shared/hl2mp/weapon_357.cpp#L20).
