# NEOTOKYO Rebuild

* NEOTOKYO rebuild in Source SDK 2013 Multiplayer (2025 TF2 SDK Update)
* Forked from: https://github.com/NeotokyoRevamp/neo
* License: SOURCE 1 SDK LICENSE, see [LICENSE](LICENSE) for details
* See [CONTRIBUTING.md](CONTRIBUTING.md) for instructions on how the codebase work and contribute

## Table of contents
To see the Table of Contents, please use the "Outline" feature on GitHub by clicking the button located in the top right of this document.

## Building NT;RE

### Requirements

* Windows: [Visual Studio 2022 (MSVC v143)](https://visualstudio.microsoft.com/downloads/)
    * Make sure to include C++ development environment, C++ MFC Library, Windows 10/11 SDK, and CMake during installation
* Linux: [Steam Runtime 3 "Sniper"](https://gitlab.steamos.cloud/steamrt/sniper/sdk)
    * GCC/G++ 10 toolchain
    * Compiled in the sniper's Docker/Podman/Toolbx container, schroot, or systemd-nspawn
    * This can also work on native (as long as it supports C++20) even with newer GCC/G++, mostly for development setup. At least install GCC and G++ from your distro's package manager.
    * `mimalloc` - To run with a debugger
* Both:
    * [CMake](https://cmake.org/)
    * [ninja](https://ninja-build.org/) (optional, can use nmake/make/VS instead)

### Building
NT;RE can be built using [VS2022 IDE](#visual-studio-2022-windows), [Qt Creator IDE](#qt-creator-linux), and the [CLI](#cli-with-ninja-windows--linux) directly.

#### Visual Studio 2022 (Windows)
1. Open up VS2022 without a project, then go to: `File > Open > CMake...`
2. Open the `CMakeLists.txt` found in `src`
3. To switch to the CMake view, right-click and click on "Switch to CMake Targets View" in the "Solution Explorer", it'll be under the "Folder View".

After that, it should be able to compile. For debugger/run CMake configuration, refer to: [CONTRIBUTING.md - Debugging - VS2022 + CMake (Windows)](CONTRIBUTING.md#vs2022--cmake-windows).

#### Qt Creator (Linux)
1. On the "Welcome" screen, click on "Open Project..."
2. Open the `CMakeLists.txt` found in `src`
3. By default, the build is not done in parallel but rather sequentiality. Note, parallel builds at the default setting could deadlock the system or make it unresponsive during the process. Available since CMake 3.12, the amount of jobs can be tweaked using `--parallel <jobs>` where `<jobs>` is a number to specify parallel build level, or just simply don't apply it to turn it off. To turn on parallel builds in Qt Creator: On the "Projects" screen, in [YOUR KIT (under Build & Run)] > Build, go to "Build Steps" section, expand by clicking on "Details", and add `--parallel` to the CMake arguments.

After that, it should be able to compile. For debugger/running configuration, refer to: [CONTRIBUTING.md - Debugging - Qt Creator (Linux)](CONTRIBUTING.md#qt-creator-linux)

#### CLI (with ninja, Windows + Linux)
##### Windows prerequisite
Make sure the "x64 Native Tools Command Prompt for VS2022" is used instead of the default.

##### Linux prerequisite - Steam Runtime 3 "Sniper" Container
Your system must include the following packages:
`gcc g++ cmake ninja-build docker-ce docker-ce-cli containerd.io apt-transport-https ca-certificates curl gnupg gnupg-utils cmake build-essential`

> [!NOTE]
> Depending on your distro, the package names may vary; please consult Google or your package manager for correct package names. The listed are for Debian `apt`. (Includes Ubuntu, Linux Mint, etc).
> 
> If using Docker, also note that following the [official install guide](https://docs.docker.com/desktop/setup/install/linux/) steps will likely configure sources for all transitive dependencies like
> `containerd.io`, so if they were not previously available by your package manager sources, it's worth checking again after installing Docker, before proceeding to add/install them manually. This is
> the case at least for APT.

Download and use the OCI image for Docker/Podman/Toolbx:

###### Fetching the container
* [Docker](https://www.docker.com/): `sudo docker pull registry.gitlab.steamos.cloud/steamrt/sniper/sdk`
* [Podman](https://podman.io/): `podman pull registry.gitlab.steamos.cloud/steamrt/sniper/sdk`
* [Toolbx](https://containertoolbx.org/): `toolbox create -i registry.gitlab.steamos.cloud/steamrt/sniper/sdk sniper`

###### Running the container
Docker:
```
# docker run -v /PATH_TO_REPO/:/root/neo/ --rm -it --entrypoint /bin/bash registry.gitlab.steamos.cloud/steamrt/sniper/sdk
$ cd /root/neo/src/
```

Podman: 
```
$ podman run -v /PATH_TO_REPO/:/root/neo/ --rm -it --entrypoint /bin/bash registry.gitlab.steamos.cloud/steamrt/sniper/sdk
$ cd /root/neo/src/
```

Toolbx: 
```
$ toolbox enter sniper
$ cd /PATH_TO_REPO/src
```

Depending on the terminal, you may need to install an additional terminfo in the container just to make it usable.
The container is based on Debian 11 "bullseye", just look for and install the relevant terminfo package if needed.

##### CLI Building steps
Using with the ninja build system, to build NT;RE using the CLI can be done with:

```
$ cd /PATH_TO_REPO/neo/src
$ cmake --preset PRESET_NAME
$ cmake --build --preset PRESET_NAME
```

Available PRESET_NAME values: `windows-debug`, `windows-release`, `linux-debug`, `linux-release`.

## Steam mod setup
To make it appear in Steam, the install files have to appear under the sourcemods directory or
be directed to it.

There are three options: copying the files over to the sourcemods directory, symlinking to the
sourcemods directory, or using `-game` option and pointing to it. If you are installing
from a tagged build, copying over the directory is fine. If you are developing or tracking
git branches, symlinking or just using `-game` is preferred.

After apply the file copy or symlink to the sourcemods directory, launch/restart Steam
and "Neotokyo: Rebuild" should appear. To run the game, original [NEOTOKYO](https://store.steampowered.com/app/244630/NEOTOKYO/)
have to be installed and NT;RE will try to automatically mount it.

The following examples assumes the default directory, but adjust if needed:

### Windows symlink
```
> cd C:\Program Files (x86)\Steam\steamapps\sourcemods
> mklink /J neo "<PATH_TO_NEO_SOURCE>/game/neo"
```

### Linux symlink
NOTE: This is not persistent, use `-game` method instead for persistent usage (`/etc/fstab`
might not work for persistent mount bind and could cause boot error if not careful):
```
$ cd $HOME/.steam/steam/steamapps/sourcemods
$ mkdir neo && sudo mount --bind <PATH_TO_NEO_SOURCE>/game/neo neo
```

### `-game` option in Source SDK 2013MP launch option
Another way is just add the `-game` option to "Source SDK Base 2013 Multiplayer":
```
%command% -game <PATH_TO_NEO_SOURCE>/game/neo
```

## SDK tools and SRCDS original NEOTOKYO mounting
By default the `gameinfo.txt` provided utilizes TF2-SDK's new `|appid_244630|` appid based search path to mount the original NEOTOKYO assets. However, some SDK tools and SRCDS either refuses to mount or crashes when trying to use this. If the appid based search path an issue, just comment this line out and uncomment `|gameinfo_path|../../NEOTOKYO/NeotokyoSource` line below and alter if needed to the actual path of your original `NeotokyoSource` installation.

## Using `-tools`
Engine tools have been broken since the TF2-SDK/64-bit update due to libraries being misplaced. To fix this, go to your Source SDK Base 2013 Multiplayer installation folder and move all files in `bin/tools/x64` to `bin/x64/tools`. You may have to create a tools folder if one does not exist already.

## Failing to load fonts and sounds properly in Linux
Fonts and sounds with non-lowercase filepaths have been broken since the TF2-SDK/64-bit update on Linux, affecting the NEUROPOL2 and X-SCALE fonts, and jitte sounds. To fix this, go to your NEOTOKYO installation folder and copy then rename the following to lowercase variant:

* `NeotokyoSource/resource/NEUROPOL2.ttf` to `NeotokyoSource/resource/neuropol2.ttf`
* `NeotokyoSource/resource/X-SCALE_.TTF` to `NeotokyoSource/resource/x-scale_.ttf`
* `NeotokyoSource/sound/weapons/Jitte/` to `NeotokyoSource/sound/weapons/jitte/`

## Further information
For further information for your platform, refer to the VDC wiki on setting up extras, chroot/containers, etc...:
https://developer.valvesoftware.com/wiki/Source_SDK_2013

For setting up the Steam mod:
https://developer.valvesoftware.com/wiki/Setup_mod_on_steam

## Server instructions

### Hosting from the client - Steam Networking
Since the TF2-SDK/64-bit update, NT;RE has Steam Networking support, meaning hosting an online server is
as easy as doing: `"Create Server" -> "Start"`. Just make sure "Use Steam networking" is set to "Enabled" and
alter the relevant configurations.

### Dedicated server hosting

These instructions have been written for and tested on Debian 12 and Arch for Linux, and Windows Server 2016 and Windows 11 for Windows, but should work with a fairly up to date Linux and Windows systems as well.

#### Downloading server files - SteamCMD

> [!TIP]
> Server operators should always use the SteamCMD setup, otherwise if doing quick tests or developing and have Steam installed, you can just install "Source SDK Base 2013 Dedicated Server" (appid 244310) from Steam and assuming you've already have NT;RE client setup can skip the SteamCMD steps. Any instructions referring to `<YOUR_LOCATION>/ntrebuild` will instead use: `<PATH_TO_STEAM>/common/Source SDK Base 2013 Dedicated Server`.

> [!WARNING]
> As of the TF2-SDK update, if trying to use the dedicated server downloaded from Steam but not through the detailed SteamCMD instructions, Linux will fail to find the files necessary to startup a working server. Just follow the SteamCMD server setup instructions for now.

1. Install SteamCMD following these instructions: [LINK](https://developer.valvesoftware.com/wiki/SteamCMD#Downloading_SteamCMD)
2. Choose a location you'd like to install the server to, for example on Linux: `/home/<username>/neoserver` or on Windows: `C:\NeotokyoServer\`, and create it. **This will be referred as `<YOUR_LOCATION>` in the later instructions. Also for Windows, replace `/` with `\` as directories separators.**
3. Run SteamCMD: `steamcmd`/`steamcmd.exe`
4. Enter the following commands in SteamCMD (Note that you need to use an absolute path for the install dir):
    ```
    force_install_dir <YOUR_LOCATION>/NEOTOKYO/
    login anonymous
    app_update 313600 validate
    (wait for it to install)
    quit
    ```
5. Run SteamCMD again, and enter these commands:
    ```
    force_install_dir <YOUR_LOCATION>/ntrebuild/
    login anonymous
    app_update 244310 validate
    (wait for it to install)
    quit
    ```
6. The distribution of the dedicated servers currently misplaced a few files:
    * Linux:
      ```
      srcds_linux64
      srcds_run_64 # NOTE - Only TF2's dedicated server have this script
      bin/linux64/libsteam_api.so
      ```
    * Windows:
      ```
      srcds_win64.exe
      ```

    You may either fetch them from the Source SDK 2013 Multiplayer client (appid 243750) or TF2's Dedicated Server (appid 232250).
    Once they're fetched, place them in the `<YOUR_LOCATION>/ntrebuild` directory.

#### Linux-only additional instructions
1. **Linux only:** Make a symlink so that Src2013 dedicated server can see SteamCMD's binaries:

    (NOTE: I'm NOT sure if this is how it is on other systems other than Debian 12, so please, check first if you have `~/.steam/sdk64` before running these! If you have Desktop Steam installed, then you should have this directory, but it doesn't seem to be the case with SteamCMD, which is why we need to do this.)
    ```
    ln -s ~/.steam/steam/steamcmd/linux64 ~/.steam/sdk64
    ```
2. **Linux only:** Change directory into `<YOUR_LOCATION>/ntrebuild/bin/linux64` then run these commands to make symlinks for needed files:
    ```
    ln -s datacache_srv.so datacache.so;
    ln -s dedicated_srv.so dedicated.so;
    ln -s engine_srv.so engine.so;
    ln -s libtier0_srv.so libtier0.so;
    ln -s libvstdlib_srv.so libvstdlib.so;
    ln -s materialsystem_srv.so materialsystem.so;
    ln -s replay_srv.so replay.so;
    ln -s scenefilecache_srv.so scenefilecache.so;
    ln -s shaderapiempty_srv.so shaderapiempty.so;
    ln -s soundemittersystem_srv.so soundemittersystem.so;
    ln -s studiorender_srv.so studiorender.so;
    ln -s vphysics_srv.so vphysics.so;
    ln -s vscript_srv.so vscript.so;
    ```
3. `cd` up directories twice, so that you will be in `<YOUR_LOCATION>/ntrebuild`.

#### Extracting NT;RE and editing gameinfo.txt
1. Extract the latest release of NT;RE into `<YOUR_LOCATION>/ntrebuild`, so you will have a directory `<YOUR_LOCATION>/ntrebuild/neo` with a `gameinfo.txt` inside.
2. Open up gameinfo.txt with your text editor and comment out `|appid_244630|` line and un-comment `|gameinfo_path|../../NEOTOKYO/NeotokyoSource` line. This is to ensure OG:NT mounting works properly with srcds.

##### Developer-only: Mounting NT;RE
If you're developing and just testing the server, it may be more useful to mount it to your `game/neo` from the git checkout.

* Windows: `mklink /J neo "<path_to_source>/game/neo"`
* Linux: Non-persistent bind mount: `mkdir neo && sudo mount --bind <path_to_source>/game/neo neo`

Or just use the directory directly when passing to the srcds command line arguments: `-game <path_to_source>/game/neo`.

#### Exposing the server to the wider internet

There are two ways on exposing your dedicated server to the wider internet: Steam Networking and Port forwarding.
Steam Networking is recommended as it doesn't expose your server directly, however if you need a persistent IP address,
OK with showing public IP address and ports, or/and don't want to route through Valve's servers, then port forwarding is still available.

##### Steam Networking

Since the TF2-SDK update, NT;RE gained the ability to use "Steam Networking" which can be useful in situations where:

* You cannot setup port forwarding or/and altering firewall settings
* You don't want to expose the server's IP address and device directly

Just add `+sv_use_steam_networking 1` to the `srcds` command line arguments to use Steam Networking for your dedicated server.

##### Firewall - Port forwarding
* Open the following ports:
    * 27015 TCP+UDP (you can keep the TCP port closed if you don't need RCON support)
    * 27020 UDP
    * 27005 UDP
    * 26900 UDP
* **Windows-only:** Allow all Inbound and Outbound TCP and UDP requests for the given ports via Windows Firewall. [See how](https://learn.microsoft.com/en-us/windows/security/operating-system-security/network-security/windows-firewall/configure#create-an-inbound-port-rule)

#### Running the server
You will need to be in the `<YOUR_LOCATION>/ntrebuild` directory and run `srcds_win64`/`srcds_run_64`/`srcds_linux64` with whatever arguments. You can adapt the following command to your own liking:

* Windows:
    ```
    srcds_win64.exe -game neo +sv_lan 0 -insecure +ip <YOUR_IP> -maxplayers <1-32> +map <MAP_NAME>
    ```
* Linux with `srcds_run_64` script:
    ```
    ./srcds_run_64 -game neo +sv_lan 0 -insecure -console +ip <YOUR_IP> -maxplayers <1-32> +map <MAP_NAME>
    ```
* Linux with `srcds_linux64` directly:
    1. Before running `srcds_linux64`, some few environment variables need to setup:
        * `SteamEnv=1`
        * `LD_LIBRARY_PATH="<YOUR_LOCATION>/ntrebuild/bin/linux64"` - So it can find the server libraries
    2. Run: `srcds_linux64 -game neo +sv_lan 0 -insecure -console +map <some map> +maxplayers 24`
        * In-game it probably won't showup in the server list and you'll have to use `connect` command directly (EX: `connect 192.168.1.###` for LAN server)

If using `-insecure` and intend to playtest without VAC for debugging purposes, double check on the log that VAC is disabled before continuing.

### SourceMod server modification plugins
> [!WARNING]
> As of the TF2-SDK update, sourcemod is broken until metamod patches and updated for the 64-bit SDK update

[SourceMod](https://www.sourcemod.net/) plugins should generally work with NT;RE, however they have to be added to the `ShowMenu` whitelist to make the menu display properly. To do this:
1. Go to directory `addons/sourcemod/gamedata/core.games` where you should find `common.games.txt`
2. Create the directory `custom` and make a copy of `common.games.txt` into it
3. Open `custom/common.games.txt` and look for a comment that says "Which games support ShowMenu?"
4. Add `neo` to that list like this: `"game" "neo"`

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
== buildshaders stdshader_dx9_30 -game C:\git\neo\src\materialsystem\stdshaders\..\..\..\game\neo -source ..\.. -dx9_30 -force30 ==
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
* [ValveSoftware/source-sdk-2013](https://github.com/ValveSoftware/source-sdk-2013) - Source SDK 2013 (2025 TF2 SDK Update)
    * Updated as of the TF2 SDK update: [Commit 0759e2e8e179d5352d81d0d4aaded72c1704b7a9](https://github.com/ValveSoftware/source-sdk-2013/commit/0759e2e8e179d5352d81d0d4aaded72c1704b7a9)
* [Nbc66/source-sdk-2013-ce](https://github.com/Nbc66/source-sdk-2013-ce) - Community Edition for additional fixes prior to the TF2 SDK update
* [tonysergi/source-sdk-2013](https://github.com/tonysergi/source-sdk-2013) - tonysergi's commits that were missing from the original SDK

