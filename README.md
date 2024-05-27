# NEOTOKYO Rebuild

* NEOTOKYO rebuild in Source SDK 2013 Multiplayer
* Forked from: https://github.com/NeotokyoRevamp/neo
* License: SOURCE 1 SDK LICENSE, see [LICENSE](LICENSE) for details
* See [CONTRIBUTING.md](CONTRIBUTING.md) for instructions on how the codebase work and contribute

## Table of contents
<!-- Generated with: https://github.com/jonschlinkert/markdown-toc -->
* [Building NT;RE](#building-ntre)
    + [Requirements](#requirements)
    + [Building](#building)
        - [Visual Studio 2022 (Windows)](#visual-studio-2022-windows)
        - [Qt Creator 6+ (Linux)](#qt-creator-6-linux)
        - [CLI (with ninja, Windows + Linux)](#cli-with-ninja-windows--linux)
            * [Windows prerequisite](#windows-prerequisite)
            * [Linux prerequisite - Steam Runtime 3 "Sniper" Container](#linux-prerequisite---steam-runtime-3-sniper-container)
            * [CLI Building steps](#cli-building-steps)
* [Steam mod setup](#steam-mod-setup)
    + [Windows symlink](#windows-symlink)
    + [Linux symlink](#linux-symlink)
    + [`-game` option in Source SDK 2013MP launch option](#-game-option-in-source-sdk-2013mp-launch-option)
    + [`-neopath` - Pointing to a non-default original NEOTOKYO directory](#-neopath---pointing-to-a-non-default-original-neotokyo-directory)
* [Further information](#further-information)
    + [Linux extra notes](#linux-extra-notes)
        - [Arch Linux](#arch-linux)
* [Server instructions](#server-instructions)
* [Shader authoring (Windows setup)](#shader-authoring-windows-setup)
    + [Compiling the shaders](#compiling-the-shaders)
        - [Troubleshooting](#troubleshooting)
* [Credits](#credits)

## Building NT;RE

### Requirements

* Windows: [Visual Studio 2022 (MSVC v143)](https://visualstudio.microsoft.com/downloads/)
    * Make sure to include C++ development environment, C++ MFC Library, Windows 10/11 SDK, and cmake during installation
* Linux: [Steam Runtime 3 "Sniper"](https://gitlab.steamos.cloud/steamrt/sniper/sdk)
    * GCC/G++ 10 toolchain
    * Compiled in the sniper's Docker/Podman/Toolbx container, schroot, or systemd-nspawn
    * This can also work on native even with newer GCC/G++, mostly for development setup. At least install GCC and G++ multilib from your distro's package manager.
* Both:
    * [cmake](https://cmake.org/)
    * [ninja](https://ninja-build.org/) (optional, can use nmake/make/VS instead)

### Building
NT;RE can be built using [VS2022 IDE](#visual-studio-2022-windows), [Qt Creator 6+ IDE](#qt-creator-6-linux), and the [CLI](#cli-with-ninja-windows--linux) directly.

#### Visual Studio 2022 (Windows)
1. Open up VS2022 without a project, then go to: `File > Open > CMake...`
2. Open the `CMakeLists.txt` found in `mp\src`
3. In the top section, you may see "x64-Debug"/"x64-Release" as a selected profile. This needs to be changed to "x86-Debug"/"x86-Release" as this is a 32-bit project. To do so...
4. Click on the dropdown, go to: "Manage Configurations..."
5. Click the green plus button and select "x86-Debug" for debug or "x86-Release" for release mode and apply the configuration.
6. Then make sure to change it over to "x86-Debug" or "x86-Release".
7. In the "Solution Explorer", it'll be under the "Folder View". To switch to the cmake view, right-click and click on "Switch to CMake Targets View".

After that, it should be able to compile. For debugger/run cmake configuration, refer to: [CONTRIBUTING.md - Debugging - VS2022 + cmake (Windows)](CONTRIBUTING.md#vs2022--cmake-windows).

#### Qt Creator 6+ (Linux)
1. On the "Welcome" screen, click on "Open Project..."
2. Open the `CMakeLists.txt` found in `mp/src`
3. It may ask about kit configuration, tick both Debug and Release configuration and set their build directories ending in "...build/debug" and "...build/release" respectively.
4. By default, the build is not done in parallel but rather sequentiality. Note, parallel builds at the default setting could deadlock the system or make it unresponsive during the process. Available since cmake 3.12, the amount of jobs can be tweaked using `--parallel <jobs>` where `<jobs>` is a number to specify parallel build level, or just simply don't apply it to turn it off. To turn on parallel builds in Qt Creator: On the "Projects" screen, in [YOUR KIT (under Build & Run)] > Build, go to "Build Steps" section, expand by clicking on "Details", and add `--parallel` to the CMake arguments.

After that, it should be able to compile. For debugger/running configuration, refer to: [CONTRIBUTING.md - Debugging - Qt Creator 6+ (Linux)](CONTRIBUTING.md#qt-creator-6-linux)

#### CLI (with ninja, Windows + Linux)
##### Windows prerequisite
Make sure the "x86 Native Tools Command Prompt for VS2022" is used instead of the default.

##### Linux prerequisite - Steam Runtime 3 "Sniper" Container
Just download and use the OCI image for Docker/Podman/Toolbx:
* [Docker](https://www.docker.com/): `sudo docker pull registry.gitlab.steamos.cloud/steamrt/sniper/sdk`
* [Podman](https://podman.io/): `podman pull registry.gitlab.steamos.cloud/steamrt/sniper/sdk`
* [Toolbx](https://containertoolbx.org/): `toolbox create -i registry.gitlab.steamos.cloud/steamrt/sniper/sdk sniper; toolbox enter sniper`

Depending on the terminal, you may need to install an additional terminfo in the container just to make it usable.
The container is based on Debian 11 "bullseye", just look for and install the relevant terminfo package if needed.

##### CLI Building steps
Using with the ninja build system, to build NT;RE using the CLI can be done with:

```
$ cd /PATH_TO_REPO/neo/mp/src

$ # To build in Release mode:
$ cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
$ cmake --build build/release --parallel

$ # To build in Debug mode:
$ cmake -S . -B build/debug -G Ninja
$ cmake --build build/debug --parallel
```

## Steam mod setup
To make it appear in Steam, the install files have to appear under the sourcemods directory or
be directed to it.

There are three options: copying the files over to the sourcemods directory, symlinking to the
sourcemods directory, or using `-game` option and pointing to it. If you are installing
from a tagged build, copying over the directory is fine. If you are developing or tracking
git branches, symlinking or just using `-game` is preferred.

After apply the file copy or symlink to the sourcemods directory, launch/restart Steam
and "Neotokyo: Revamp" should appear.

The following examples assumes the default directory, but adjust if needed:

### Windows symlink
```
> cd C:\Program Files (x86)\Steam\steamapps\sourcemods
> mklink /J neo "<PATH_TO_NEO_SOURCE>/mp/game/neo"
```

### Linux symlink
NOTE: This is not persistent, use `-game` method instead for persistent usage (`/etc/fstab`
might not work for persistent mount bind and could cause boot error if not careful):
```
$ cd $HOME/.steam/steam/steamapps/sourcemods
$ mkdir neo && sudo mount --bind <PATH_TO_NEO_SOURCE>/mp/game/neo neo
```

### `-game` option in Source SDK 2013MP launch option
Another way is just add the `-game` option to "Source SDK Base 2013 Multiplayer":
```
%command% -game <PATH_TO_NEO_SOURCE>/mp/game/neo
```

### `-neopath` - Pointing to a non-default original NEOTOKYO directory
This is generally isn't necessary if NEOTOKYO is installed at a default location. However,
if you have it installed at a different location, adding `-neopath` to the launch option
can be used to direct to it.

## Further information
For further information for your platform, refer to the VDC wiki on setting up extras, chroot/containers, etc...:
https://developer.valvesoftware.com/wiki/Source_SDK_2013

For setting up the Steam mod:
https://developer.valvesoftware.com/wiki/Setup_mod_on_steam

### Linux extra notes
#### Arch Linux
You may or may not need this, but if NT;RE crashes/segfaults on launch, then install `lib32-gperftools`,
and set: `LD_PRELOAD=/usr/lib32/libtcmalloc.so %command%` as the launch argument.

## Server instructions
1. To run a server, install "Source SDK Base 2013 Dedicated Server".
2. For firewall, open the following ports:
    * 27015 TCP+UDP
    * 27020 UDP
    * 27005 UDP
    * 26900 UDP
3. After it installed, go to the install directory in CMD, should see `srcds.exe`
4. Link neo:
    * Windows: `mklink /J neo "<path_to_source>/mp/game/neo"`
    * Linux: `mkdir neo && sudo mount --bind <path_to_source>/mp/game/neo neo`
5. Linux-only: Symlink the names in "Source SDK Base 2013 Dedicated Server" "bin" directory:
```
ln -s vphysics_srv.so vphysics.so;
ln -s studiorender_srv.so studiorender.so;
ln -s soundemittersystem_srv.so soundemittersystem.so;
ln -s shaderapiempty_srv.so shaderapiempty.so;
ln -s scenefilecache_srv.so scenefilecache.so;
ln -s replay_srv.so replay.so;
ln -s materialsystem_srv.so materialsystem.so;
```
6. Run: `srcds.exe +sv_lan 0 -insecure -game neo +map <some map> +maxplayers 24 -autoupdate -console`
    * It'll be `./srcds_run` for Linux
    * Double check on the log that VAC is disabled before continuing
7. In-game, it'll showup in the server list

## Shader authoring (Windows setup)

### Compiling the shaders

* Some outline for shaders from the VDC wiki:
    * [Shader Authoring, general](https://developer.valvesoftware.com/wiki/Shader_Authoring)
    * [Example shader](https://developer.valvesoftware.com/wiki/Source_SDK_2013:_Your_First_Shader)
* You'll need Perl with the package String-CRC32.
* Compatible pre-compiled Perl installer is available [here](https://platform.activestate.com/Rainyan/Perl-SourceSDK2013). Strawberry Perl should most likely also work, but you'll need to confirm the String-CRC32 package is available.

#### Troubleshooting

* If Perl was not found when running shader scripts, make sure it's included in your PATH env var (or use the ActiveState Platform [State CLI](https://docs.activestate.com/platform/state/), if applicable for your particular Perl setup).
* You will probably need to slightly edit some of the build scripts (buildhl2mpshaders.bat/buildsdkshaders.bat/buildshaders.bat) to match your setup, and create some symlinks or copy files to get access to FileSystem_Steam.dll, and the other "\bin\..." shader compile tools.
* If you get errors with nmake not found, make sure you're running the scripts from a Visual Studio (x86) Native Tools Command Prompt, or have the paths (VsDevCmd.bat / vsvars32.bat) set up by some other means.
    * Alternatively, edit the "buildsdkshaders.bat" to call the appropriate vsvars32.bat for setting up nmake for your particular environment.
* If you get a "bin\something" not found error when already inside the bin folder, add a symlink for ".\bin" <--> "." as workaround.
    * `mklink /j .\bin .`

---

Finally, you should be greeted with some compiler output akin to:
```
== buildshaders stdshader_dx9_30 -game C:\git\neo\mp\src\materialsystem\stdshaders\..\..\..\game\neo -source ..\.. -dx9_30 -force30 ==
10.41
Building inc files, asm vcs files, and VMPI worklist for stdshader_dx9_30...
Publishing shader inc files to target...
shaders\fxc\example_model_ps20b.vcs
shaders\fxc\example_model_vs20.vcs
shaders\fxc\neo_test_pixelshader_ps20.vcs
shaders\fxc\neo_test_pixelshader_ps20b.vcs
shaders\fxc\sdk_bloomadd_ps20.vcs
shaders\fxc\sdk_bloomadd_ps20b.vcs
shaders\fxc\sdk_bloom_ps20.vcs
shaders\fxc\sdk_bloom_ps20b.vcs
shaders\fxc\sdk_screenspaceeffect_vs20.vcs
9 File(s) copied
10.41
```

## Credits
* [NeotokyoRevamp/neo](https://github.com/NeotokyoRevamp/neo) - Where this is forked from
* [ValveSoftware/source-sdk-2013](https://github.com/ValveSoftware/source-sdk-2013) - Source SDK 2013
* [Nbc66/source-sdk-2013-ce](https://github.com/Nbc66/source-sdk-2013-ce) - Community Edition for additional fixes
* [tonysergi/source-sdk-2013](https://github.com/tonysergi/source-sdk-2013) - tonysergi's commits that were missing from the original SDK

