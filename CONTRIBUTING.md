# Contributing

## Pull requests are welcome!

Come [join the Discord channel](https://steamcommunity.com/groups/ANPA/discussions/0/487876568238532577/) to discuss the project
(see the "nt_semantic_sequels" and other related channels under the channel group "Architects").

## Getting started

See [build instructions](BUILD_INSTRUCTIONS.md) in this repo for setting up your build environment (currently supporting Windows/Linux).

[Break pointing and stepping](https://developer.valvesoftware.com/wiki/Installing_and_Debugging_the_Source_Code) CServerGameDLL::GameFrame, or methods in C_NEO_Player (clientside player) / CNEO_Player (serverside player) can help with figuring out the general game loop. Neo specific files usually live in src/game/client/neo, src/game/server/neo, or src/game/shared/neo, similar to how most hl2mp code is laid out.

Ochii's impressive [reverse engineering project](https://github.com/Ochii/neotokyo-re) can also serve as reference for figuring things out. However, please refrain from copying reversed instructions line by line, as the plan is to write an open source reimplementation (and steer clear of any potential copyright issues). Same thing applies for original NT assets; you can depend on the original NT installation (it's mounted to the engine filesystem by [a shared neo header](mp/src/game/shared/neo/neo_mount_original.h)), but avoid pushing those assets in the repo.

It's recommended you fork [the dev branch](https://github.com/NeotokyoRevamp/neo/tree/dev), and pull request your work there back to it.
The dev branch will periodically get merged back to the master branch, as new things become stable enough.

## Preprocessor defines
In shared code, clientside code can be differentiated with CLIENT_DLL, vs. serverside's GAME_DLL. In more general engine files, Neotokyo specific code can be marked with a NEO ifdef. These are defined with the [VPC system](https://developer.valvesoftware.com/wiki/VPC), in files "[client_hl2mp.vpc](mp/src/game/client/client_hl2mp.vpc)" / "[server_hl2mp.vpc](mp/src/game/server/server_hl2mp.vpc)", under $PreprocessorDefinitions.

## On code style

No big restrictions, just try to more or less match the other SDK code style. Valve likes to ( space ) their arguments,
especially with macros, but it's not necessary. Tabs are preferred for whitespace, to be consistent with the SDK code.
