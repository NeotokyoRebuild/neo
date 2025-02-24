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
    * This can also work on native (as long as it supports C++20) even with newer GCC/G++, mostly for development setup. At least install GCC and G++ multilib from your distro's package manager.
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
`gcc-multilib g++-multilib cmake ninja-build docker-ce docker-ce-cli containerd.io apt-transport-https ca-certificates curl gnupg gnupg-utils cmake build-essential`

**Note:** Depending on your operating system, the package names may vary, please consult google or your package manager for correct package names. The listed are for debian `apt`. (Includes Ubuntu, Linux Mint, etc)

Download and use the OCI image for Docker/Podman/Toolbx:

###### Fetching the container
* [Docker](https://www.docker.com/): `sudo docker pull registry.gitlab.steamos.cloud/steamrt/sniper/sdk`
* [Podman](https://podman.io/): `podman pull registry.gitlab.steamos.cloud/steamrt/sniper/sdk`
* [Toolbx](https://containertoolbx.org/): `toolbox create -i registry.gitlab.steamos.cloud/steamrt/sniper/sdk sniper`

###### Running the container
Docker:
```
# docker run -v /PATH_TO_REPO/neo/src:/root/neo/src --rm -it --entrypoint /bin/bash registry.gitlab.steamos.cloud/steamrt/sniper/sdk
$ cd /root/neo/src/
```

Podman: 
```
$ podman run -v /PATH_TO_REPO/neo/src:/root/neo/src --rm -it --entrypoint /bin/bash registry.gitlab.steamos.cloud/steamrt/sniper/sdk
$ cd /root/neo/src/
```

Toolbx: 
```
$ toolbox enter sniper
$ cd /PATH_TO_REPO/neo/src
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

### `-neopath` - Pointing to a non-default original NEOTOKYO directory

This is generally isn't necessary if NEOTOKYO is installed at a default location. However,
if you have it installed at a different location, adding `-neopath` to the launch option
can be used to direct to it.

## Further information
For further information for your platform, refer to the VDC wiki on setting up extras, chroot/containers, etc...:
https://developer.valvesoftware.com/wiki/Source_SDK_2013

For setting up the Steam mod:
https://developer.valvesoftware.com/wiki/Setup_mod_on_steam

## Server instructions
### Server Ops
#### Dedicated Server on Linux
These instructions have been written with a Debian 12 machine in mind, but they should work for other systems as well.
1. Install SteamCMD following these instructions: [LINK](https://developer.valvesoftware.com/wiki/SteamCMD#Linux)
2. Decide on a location you'd like to install the server to, for example, `/home/<username>/neoserver`, and create it.
3. Run SteamCMD: `steamcmd`
4. Enter the following commands in SteamCMD (Note that you need to use an absolute path for the install dir):
    ```
    force_install_dir <YOUR_LOCATION>/ognt/
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
6. Make a symlink for original NEOTOKYO so that NT;RE can find it's assets:

    Run the following command as root:
    ```
    ln -s <YOUR_LOCATION>/ognt /usr/share/neotokyo
    ```
    It should now be possible to access `/usr/share/neotokyo/NeotokyoSource`.
   
    This is the only command that needs root, so you can logout from root.
8. Make a symlink so that Src2013 dedicated server can see SteamCMD's binaries:

    (NOTE: I'm NOT sure if this is how it is on other systems other than Debian 12, so please, check first if you have `~/.steam/sdk32` before running these! If you have Desktop Steam installed, then you should have this directory, but it doesn't seem to be the case with SteamCMD, which is why we need to do this.)
    ```
    ln -s ~/.steam/steam/steamcmd/linux32 ~/.steam/sdk32
    ```
9. For firewall, open the following ports:
    * 27015 TCP+UDP (you can keep the TCP port closed if you don't need RCON support)
    * 27020 UDP
    * 27005 UDP
    * 26900 UDP
10. `cd` into `<YOUR_LOCATION>/ntrebuild/bin`.
11. Run these commands to make symlinks for needed files:
    ```
    ln -s vphysics_srv.so vphysics.so;
    ln -s studiorender_srv.so studiorender.so;
    ln -s soundemittersystem_srv.so soundemittersystem.so;
    ln -s shaderapiempty_srv.so shaderapiempty.so;
    ln -s scenefilecache_srv.so scenefilecache.so;
    ln -s replay_srv.so replay.so;
    ln -s materialsystem_srv.so materialsystem.so;
    ```
12. Run the following command to rename a file that is incompatible with NT;RE:
     ```
     mv libstdc++.so.6 libstdc++.so.6.bak
     ```
13. `cd` up a folder, so that you will be in `<YOUR_LOCATION>/ntrebuild`.
14. Extract the latest release of NT;RE into `<YOUR_LOCATION>/ntrebuild`, so you will have a directory `<YOUR_LOCATION>/ntrebuild/neo` with a `gameinfo.txt` inside.

Now you have a dedicated server setup for NT;RE. To run it, you will need to be in the `<YOUR_LOCATION>/ntrebuild` directory and run `srcds_run` with whatever arguments. You can adapt the following command to your own liking:
```
./srcds_run +sv_lan 0 -insecure -console -game neo +ip <YOUR_IP> -maxplayers <1-32> +map <MAP_NAME>
```
#### Dedicated Server on Windows
These instructions were tested on Windows Server 2016 and Windows 11 machines, they will probably work in all Windows versions.
1. Install SteamCMD following these instructions: [LINK](https://developer.valvesoftware.com/wiki/SteamCMD#Windows)
2. Choose a location for your server to be installed into, for example, `C:\NeotokyoServer\`, and create it. (In this case SteamCMD is also installed in this location)
3. Run SteamCMD: `steamcmd.exe`
4. Enter the following commands in SteamCMD:
    ```
    force_install_dir .\ognt\
    login anonymous
    app_update 313600 validate
    (wait for it to install)
    quit
    ```
5. Run SteamCMD again, and enter these commands:
    ```
    force_install_dir .\ntrebuild\
    (this will be the main directory of your server)
    login anonymous
    app_update 244310 validate
    (wait for it to install)
    quit
    ```
6. Extract the latest release of NT;RE into `<YOUR_LOCATION>\ntrebuild`, so you will have a directory `<YOUR_LOCATION>\ntrebuild\neo` with a `gameinfo.txt` inside.
7. Allow all Inbound and Outbound TCP and UDP requests for the following ports via Windows Firewall. [See how](https://learn.microsoft.com/en-us/windows/security/operating-system-security/network-security/windows-firewall/configure#create-an-inbound-port-rule)
    * 27015 TCP+UDP (you can keep the TCP port closed if you don't need RCON support)
    * 27020 UDP
    * 27005 UDP
    * 26900 UDP
8. Your server should be ready to go, launch it inside your main directory (`(...)\ntrebuild\`) with the following command: (You can alter any argument to your liking, except `-game` and `-neopath`) 
```
srcds.exe -game neo -neopath "..\ognt\NeotokyoSource" +ip <YOUR_IP> -maxplayers <1-32> +map <MAP_NAME>
```

### Testers/Devs
1. To run a server, install "Source SDK Base 2013 Dedicated Server" (appid 244310).
2. For firewall, open the following ports:
    * 27015 TCP+UDP (you can keep the TCP port closed if you don't need RCON support)
    * 27020 UDP
    * 27005 UDP
    * 26900 UDP
3. After it installed, go to the install directory in CMD, should see:
    * Windows: `srcds.exe`
    * Linux: `srcds_linux`
        * You'll also see `srcds_run` but that doesn't work properly with NT;RE so ignore it
4. Optional: Link or copy over neo, otherwise `-game <path_to_source>/game/neo` can be used also:
    * Windows: `mklink /J neo "<path_to_source>/game/neo"`
    * Linux:
        * Non-persistent bind mount: `mkdir neo && sudo mount --bind <path_to_source>/game/neo neo`
        * Or just copy over or use the directory directly
5. Linux-only: Symlink the names in `<PATH_TO_STEAM>/common/Source SDK Base 2013 Dedicated Server/bin` directory:
```
ln -s vphysics_srv.so vphysics.so;
ln -s studiorender_srv.so studiorender.so;
ln -s soundemittersystem_srv.so soundemittersystem.so;
ln -s shaderapiempty_srv.so shaderapiempty.so;
ln -s scenefilecache_srv.so scenefilecache.so;
ln -s replay_srv.so replay.so;
ln -s materialsystem_srv.so materialsystem.so;
```
6. Linux-only: Before running `srcds_linux`, some few environment variables need to setup:
    * `SteamEnv=1`
    * `LD_LIBRARY_PATH=$(<STEAM-RUNTIME-DIR>/run.sh printenv LD_LIBRARY_PATH):/home/YOUR_USER/.steam/steam/steamapps/Source SDK Base 2013 Dedicated Server/bin`
        * Where `<STEAM-RUNTIME-DIR>` can be found from: `$ find "$HOME" -type d -name 'steam-runtime' 2> /dev/null`
7. Run: `<srcds.exe|srcds_linux> +sv_lan 0 -insecure -game neo +map <some map> +maxplayers 24 -autoupdate -console`
    * Double check on the log that VAC is disabled before continuing
7. In-game on Windows it'll showup in the server list, on Linux it probably won't and you'll have to use `connect` command directly (EX: `connect 192.168.1.###` for LAN server)

### SourceMod server modification plugins
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

