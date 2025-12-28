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
To see the Table of Contents, please use the "Outline" feature on GitHub by clicking the button located in the top right of this document.

## Getting started

### Cloning & merging

It's recommended you fork [the master branch](https://github.com/NeotokyoRebuild/neo/tree/master), and pull request your work there back to it.

### Building

See [README.md](README.md) in this repo for setting up your build environment (currently supporting Windows/Linux).

### Debugging
To be safe and avoid problems with VAC, it's recommended to add a [-insecure](https://developer.valvesoftware.com/wiki/Command_Line_Options) launch flag before attaching your debugger.

#### VS2022 + CMake (Windows)
In the CMake Target View, right-click "client (shared library)" and click on "Add Debug Configuration". This should generate the `src/.vs/launch.vs.json` config file.

Modify the config to fix the directory paths; an example is provided below:

```
{
  "version": "0.2.1",
  "defaults": {},
  "configurations": [
    {
      "type": "dll",
      "exe": "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Source SDK Base 2013 Multiplayer\\hl2_win64.exe",
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
        "C:\\PATH\\TO\\REPO_ROOT\\neo\\game\\neo"
      ]
    }
  ]
}
```

#### Qt Creator (Linux)
On the sidebar, click "Projects" then under your current kit, click "Run". Set the following:

| Property | Example value |
| :---------------------------------- | :------------ |
| Executable | `~/.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer/hl2_linux64` |
| Command line arguments | `-allowdebug -insecure -dev -sw -game "/PATH/TO/NEO_REPO/game/neo"` |
| Working directory | `~/.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer` |

Then under **Environment**, expand by clicking "Details" and add in:
```
LD_LIBRARY_PATH=[INSERT_OUTPUT_HERE]:/home/YOUR_USER/.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer/bin/linux64
SDL_VIDEODRIVER=x11
SteamEnv=1
```

Next is finding the 64-bits steam-runtime under the Steam installation. This can be found
using the following command, replacing `$HOME` if Steam is installed at another directory:
```
$ find "$HOME" -type d -name 'steam-runtime' 2> /dev/null
```

Assuming default directory, it might be in either: `~/.steam/steam/steamapps/common/SteamLinuxRuntime/var/steam-runtime` or if using a Debian based distribution: `[TODO]`.

Then change to that directory and replace `[INSERT_OUTPUT_HERE]` to the output of:
```
$ cd <STEAM-RUNTIME-DIR>
$ ./run.sh printenv LD_LIBRARY_PATH
```

After this, you should be able to run and debug NT;RE, just make sure to have Steam open in the background.

#### VS2022 Visual Studio Solutions (Windows)
Example settings for debugging from Visual Studio solutions:

| Configuration Properties->Debugging | Example value |
| :---------------------------------- | :------------ |
| Command | C:\Program Files (x86)\Steam\steamapps\common\Source SDK Base 2013 Multiplayer\hl2_win64.exe |
| Command Arguments | -allowdebug -insecure -dev -sw -game "C:\git\neo\game\neo" |
| Working Directory | C:\Program Files (x86)\Steam\steamapps\common\Source SDK Base 2013 Multiplayer |

### Game loop and reference material

[Break pointing and stepping](https://developer.valvesoftware.com/wiki/Installing_and_Debugging_the_Source_Code) the method [CServerGameDLL::GameFrame](src/game/server/gameinterface.cpp), or relevant methods in [C_NEO_Player](src/game/client/neo/c_neo_player.h) (clientside player) / [CNEO_Player](src/game/server/neo/neo_player.h) (serverside player) can help with figuring out the general game loop. Neo specific files usually live in [game/client/neo](src/game/client/neo), [game/server/neo](src/game/server/neo), or [game/shared/neo](src/game/shared/neo), similar to how most hl2mp code is laid out.

Ochii's impressive [reverse engineering project](https://github.com/Ochii/neotokyo-re) can also serve as reference for figuring things out. However, please refrain from copying reversed instructions line by line, as the plan is to write an open(ed) source (wherever applicable, note the Source SDK license) reimplementation, and steer clear of any potential copyright issues. Same thing applies for original NT assets; you can depend on the original NT installation (it's mounted to the engine filesystem by gameinfo.txt `|appid_244630|`), but avoid pushing those assets in the repo.

## Good to know

### CMake

This project uses the [CMake](https://cmake.org/) build system to generate ninja/makefiles and is integrated with IDEs such as VS2022 and Qt Creator. When modifying the project file structure, look into `CMakeLists.txt` and `cmake/*.cmake`.

### Preprocessor definitions
In shared code, clientside code can be differentiated with CLIENT_DLL, vs. serverside's GAME_DLL. In more general engine files, Neotokyo specific code can be marked with a NEO ifdef.

## Supported compilers

Only the x64 (64-bit) architecture is supported.

### Windows
* MSVC v143 - VS 2022 C++ x64/x86 build tools (Latest)
* MSVC version used by the latest `windows-2025` [runner image](https://github.com/actions/runner-images/blob/main/images/windows/Windows2025-Readme.md) (Microsoft.VisualStudio.Component.VC.Tools.x86.x64)

### Linux
* GCC 10 [steamrt3 'sniper'](https://gitlab.steamos.cloud/steamrt/sniper/sdk)
* GCC 14 [steamrt3 'sniper'](https://gitlab.steamos.cloud/steamrt/sniper/sdk)
* GCC 14 [steamrt4](https://gitlab.steamos.cloud/steamrt/steamrt4/sdk)
* Clang 19 [steamrt4](https://gitlab.steamos.cloud/steamrt/steamrt4/sdk)

### Code style

* C++20, within the [supported compilers'](#supported-compilers) capabilities.
* Formatting:
  * No big restrictions on general format, but try to more or less match the surrounding SDK code style for consistency.
* Warnings are treated as errors.
  * You may choose to suppress a warning with compiler-specific preprocessing directives if it is a false positive, but **please do not suppress valid warnings**; instead resolve it by fixing the underlying issue.
  * For local development, you may disable warnings-as-errors by modifying your `CMAKE_COMPILE_WARNING_AS_ERROR` option, eg. by modifying the entry in your `CMakeCache.txt` file.
  * For the CI runners, the following cases are exempt from the *warnings-as-errors* rule:
    * Tagged release builds (`v<major>.<minor>`...)
    * Branch name or its latest commit message containing the phrase `nwae` (acronym for: *"no warnings as errors"*)
      * The `nwae` phrase also works locally, but you may need to refresh the CMake cache for it to take effect.
  * Please note that the above methods of disabling warnings-as-errors are intended as temporary workarounds to allow devs to resolve exceptional situations (eg. the un-pinned MSVC compiler version changes unexpectedly changing warnings behaviour), and it should *not* be regularly relied upon to dodge warnings. The preferred long-term solution is almost always to properly fix the warning! 
* STL and platform-specific headers may be used when needed, but generally the SDK libraries should be preferred. If you do use such includes, be careful with not bringing in conflicting macro definitions or performance-costly code.
* The `min` and `max` from the SDK macros have been disabled, because they were polluting namespaces and making things like like `::max` awkward to use. You may use the templated `Min` and `Max` functions from `basetypes.h` instead.
* Some nonstandard casting helpers are available:
  * `assert_cast`:
    * The preferred cast for pointer conversion where an incompatible cast may occur by accident
      * If you know for a fact it is always a safe cast, you can use `static_cast` instead
    * For debug builds, will runtime-validate the cast with `ptr == dynamic_cast<T>(ptr)`
    * For release builds, it is identical to `static_cast`
  * `narrow_cast`:
    * The preferred cast for narrowing conversions where loss of information may occur
      * If you know for a fact it is always a safe cast, you can use `static_cast` instead
    * For debug builds, will runtime-validate narrowing conversions for:
      * Roundtrip equality: `value v of type A equals itself after A->B->A type conversion`
      * Signed/unsigned conversion overflow
    * For release builds, it is identical to `static_cast`
  * `neo::bit_cast`:
    * Well-formed type punning helper. Mostly used to avoid UB in the SDK code.
    * For the general case, it is a constexpr wrapper for `std::bit_cast` if it's available, but will also support `memcpy` based conversions for when this is not the case (either `std::bit_cast` unavailable or its constraints not satisfied).
    * For debug builds, a runtime assertion test is available as an additional parameter:
      * `auto output = neo::bit_cast<T>(BC_TEST(input, expectedOutput));`
      * When replacing ill-formed type puns, this test syntax can be used to ensure the output of `neo::bit_cast<T>(input)` remains identical to `expectedOutput`
    * For release builds, the above test optimizes away, into:
      * `auto output = neo::bit_cast<T>(input);`
* Valve likes to `( space )` indent their parentheses, especially with macros, but it's not necessary to strictly follow everywhere.
* Tabs are preferred for indentation, to be consistent with the SDK code.
* When using a TODO/FIXME/HACK... style comment, use the format `// NEO TODO (Your-username): Example comment.` to make it easier to search for `NEO` specific todos/fixmes (opposed to Valve ones of the original SDK), and at a glance figure out who has written them.
* For classes running on both client and server, you should generally follow Valve's <i>C_Thing</i> (client) -- <i>CThing</i> (server) convention. On shared files, this might mean #defining serverclass for client, or vice versa. There's plenty of examples of this pattern in Valve's classes for reference, [for example here](https://github.com/NeotokyoRebuild/neo/blob/f749c07a4701d285bbb463686d5a5a50c20b9528/mp/src/game/shared/hl2mp/weapon_357.cpp#L20).
