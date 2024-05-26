*Note: It's recommended you read the [master branch version of this document](https://github.com/NeotokyoRebuild/neo/blob/master/CONTRIBUTING.md) to make sure all the information is up to date.*

# Contributing

## Pull requests are welcome!

We've moved to using GitHub Issues for issue tracking. Feel free to [take a look](https://github.com/NeotokyoRebuild/neo/issues) and assign yourself if you're interested in working on something, or report bugs/features not yet included.

Also come [join the Discord channel](https://steamcommunity.com/groups/ANPA/discussions/0/487876568238532577/) to discuss the project with others
(see the *"nt_semantic_sequels"* and other related channels under the channel group "Architects").

## Goal of the project
The overall goal is to create a reimplementation of Neotokyo for Source 2013 MP engine, with the source code opened for viewing and editing (with certain limitations; see the Source SDK license for legalese). Original NT assets/IP are mounted as dependencies, and aren't part of the repository. It's like a mod of a mod.

The end result should hopefully be a shinier and less error-prone rendition of NT, with the source code and more cvars providing room to fine tune game balance, or come up with completely new modes entirely.

## Table of contents
<!-- Generated with: https://github.com/jonschlinkert/markdown-toc -->
* [Getting started](#getting-started)
	+ [Cloning & merging](#cloning--merging)
    + [Building](#building)
    + [Debugging](#debugging)
    + [Game loop and reference material](#game-loop-and-reference-material)
* [Good to know](#good-to-know)
    + [Solutions/makefiles](#solutionsmakefiles)
    + [Preprocessor definitions](#preprocessor-definitions)
    + [Code style](#code-style)

## Getting started

### Cloning & merging

It's recommended you fork [the master branch](https://github.com/NeotokyoRebuild/neo/tree/master), and pull request your work there back to it.

The master branch will periodically get merged back to the master branch, as new things become stable enough.

### Building

See [README.md](README.md) in this repo for setting up your build environment (currently supporting Windows/Linux).

### Debugging
To be safe and avoid problems with VAC, it's recommended to add a [-insecure](https://developer.valvesoftware.com/wiki/Command_Line_Options) launch flag before attaching your debugger.

Example settings for debugging from Visual Studio solutions:

| Configuration Properties->Debugging | Example value |
| :---------------------------------- | :------------ |
| Command | C:\Program Files (x86)\Steam\steamapps\common\Source SDK Base 2013 Multiplayer\hl2.exe |
| Command Arguments | -allowdebug -insecure -dev -sw -game "C:\git\neo\mp\game\neo" |
| Working Directory | C:\Program Files (x86)\Steam\steamapps\common\Source SDK Base 2013 Multiplayer |

#### VS2022 + cmake (Windows)
In the CMake Target View, right-click "client (shared library)" and click on "Add Debug Configuration". This should show a json file. Then, make sure it's similar to this (changing the game path to where you have it):

```
{
  "version": "0.2.1",
  "defaults": {},
  "configurations": [
    {
      "type": "dll",
      "exe": "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Source SDK Base 2013 Multiplayer\\hl2.exe",
      "project": "CMakeLists.txt",
      "projectTarget": "client.dll (game\\client\\client.dll)",
      "name": "client.dll (game\\client\\client.dll)",
      "currentDir": "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Source SDK Base 2013 Multiplayer",
      "args": [
        "-allowdebug",
        "-insecure",
        "-dev",
        "-sw",
        "-game",
        "C:\\PATH\\TO\\REPO_ROOT\\neo\\mp\\game\\neo"
      ]
    }
  ]
}
```

#### Qt Creator 13 (Linux)
On the sidebar, click "Projects" then under your current kit, click "Run". Set the following:

| Property | Example value |
| :---------------------------------- | :------------ |
| Executable | `~/.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer/hl2_linux` |
| Command line arguments | `-allowdebug -insecure -dev -sw -game "/PATH/TO/NEO_REPO/mp/game/neo"` |
| Working directory | `~/.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer` |

Then under **Environment**, expand by clicking "Details" and add in:
```
LD_LIBRARY_PATH=[INSERT_OUTPUT_HERE]:/home/YOUR_USER/.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer/bin
SDL_VIDEODRIVER=x11
SteamEnv=1
```

Where replacing `[INSERT_OUTPUT_HERE]` is the output of:
```
$ ~/.local/share/Steam/ubuntu12_32/steam-runtime/run.sh printenv LD_LIBRARY_PATH
```

After this, you should be able to run and debug NT;RE, just make sure to have Steam open in the background.

### Game loop and reference material

[Break pointing and stepping](https://developer.valvesoftware.com/wiki/Installing_and_Debugging_the_Source_Code) the method [CServerGameDLL::GameFrame](mp/src/game/server/gameinterface.cpp), or relevant methods in [C_NEO_Player](mp/src/game/client/neo/c_neo_player.h) (clientside player) / [CNEO_Player](mp/src/game/server/neo/neo_player.h) (serverside player) can help with figuring out the general game loop. Neo specific files usually live in [game/client/neo](mp/src/game/client/neo), [game/server/neo](mp/src/game/server/neo), or [game/shared/neo](mp/src/game/shared/neo), similar to how most hl2mp code is laid out.

Ochii's impressive [reverse engineering project](https://github.com/Ochii/neotokyo-re) can also serve as reference for figuring things out. However, please refrain from copying reversed instructions line by line, as the plan is to write an open(ed) source (wherever applicable, note the Source SDK license) reimplementation, and steer clear of any potential copyright issues. Same thing applies for original NT assets; you can depend on the original NT installation (it's mounted to the engine filesystem by [a shared neo header](mp/src/game/shared/neo/neo_mount_original.h)), but avoid pushing those assets in the repo.

## Good to know

### Current: cmake

This project currently use the [cmake](https://cmake.org/) build system to generate ninja/makefiles and is integrated with IDEs such as VS2022 and qtcreator. When modifying the project file structure, look into `CMakeLists.txt` and `cmake/*.cmake`.

### Legacy: Solutions/makefiles

This project used to use Valve's [VPC system](https://developer.valvesoftware.com/wiki/VPC) to generate its makefiles and VS solutions. When modifying the project file structure, instead of pushing your solution/makefile, edit the relevant VPC files instead (most commonly "[client_neo.vpc](mp/src/game/client/client_neo.vpc)" and "[server_neo.vpc](mp/src/game/server/server_neo.vpc)").

Running the VPC scripts in mp/src/... after a change will regenerate the solutions and makefiles on all platforms. You may sometimes have to purge your object file cache if you get linker errors after restructuring existing translation units.

### Preprocessor definitions
In shared code, clientside code can be differentiated with CLIENT_DLL, vs. serverside's GAME_DLL. In more general engine files, Neotokyo specific code can be marked with a NEO ifdef. These are also defined with the VPC system described above, in the $PreprocessorDefinitions section.

### Code style

No big restrictions on general code format, just try to more or less match the other SDK code style.

* C++11 but within GCC 4.6+ supports. This means:
    * No `constexpr`
    * Use `OVERRIDE` instead of `override`
    * STL generally shouldn't be included in as it may conflicts with existing similar functions
* Valve likes to ( space ) their arguments, especially with macros, but it's not necessary to strictly follow everywhere.
* Tabs are preferred for indentation, to be consistent with the SDK code.
* When using a TODO/FIXME/HACK... style comment, use the format "// NEO TODO (Your-username): Example comment." to make it easier to search NEO specific todos/fixmes (opposed to Valve ones), and at a glance figure out who has written them.
* For classes running on both client and server, you should generally follow Valve's <i>C_Thing</i> (client) -- <i>CThing</i> (server) convention. On shared files, this might mean #defining serverclass for client, or vice versa. There's plenty of examples of this pattern in Valve's classes for reference, [for example here](https://github.com/NeotokyoRebuild/neo/blob/f749c07a4701d285bbb463686d5a5a50c20b9528/mp/src/game/shared/hl2mp/weapon_357.cpp#L20).
