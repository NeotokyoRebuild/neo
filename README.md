# NEOTOKYO Rebuild

* NEOTOKYO rebuild in Source SDK 2013 Multiplayer
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
* Both:
    * [CMake](https://cmake.org/)
    * [ninja](https://ninja-build.org/) (optional, can use nmake/make/VS instead)

### Building
NT;RE can be built using [VS2022 IDE](#visual-studio-2022-windows), [Qt Creator IDE](#qt-creator-linux), and the [CLI](#cli-with-ninja-windows--linux) directly.

#### Visual Studio 2022 (Windows)
1. Open up VS2022 without a project, then go to: `File > Open > CMake...`
2. Open the `CMakeLists.txt` found in `mp\src`
3. In the top section, you may see "x64-Debug"/"x64-Release" as a selected profile. This needs to be changed to "x86-Debug"/"x86-Release" as this is a 32-bit project. To do so...
4. Click on the dropdown, go to: "Manage Configurations..."
5. Click the green plus button and select "x86-Debug" for debug or "x86-Release" for release mode and apply the configuration.
6. Then make sure to change it over to "x86-Debug" or "x86-Release".
7. In the "Solution Explorer", it'll be under the "Folder View". To switch to the CMake view, right-click and click on "Switch to CMake Targets View".

After that, it should be able to compile. For debugger/run CMake configuration, refer to: [CONTRIBUTING.md - Debugging - VS2022 + CMake (Windows)](CONTRIBUTING.md#vs2022--cmake-windows).

#### Qt Creator (Linux)
1. On the "Welcome" screen, click on "Open Project..."
2. Open the `CMakeLists.txt` found in `mp/src`
3. It may ask about kit configuration, tick both Debug and Release configuration and set their build directories ending in "...build/debug" and "...build/release" respectively.
4. By default, the build is not done in parallel but rather sequentiality. Note, parallel builds at the default setting could deadlock the system or make it unresponsive during the process. Available since CMake 3.12, the amount of jobs can be tweaked using `--parallel <jobs>` where `<jobs>` is a number to specify parallel build level, or just simply don't apply it to turn it off. To turn on parallel builds in Qt Creator: On the "Projects" screen, in [YOUR KIT (under Build & Run)] > Build, go to "Build Steps" section, expand by clicking on "Details", and add `--parallel` to the CMake arguments.

After that, it should be able to compile. For debugger/running configuration, refer to: [CONTRIBUTING.md - Debugging - Qt Creator (Linux)](CONTRIBUTING.md#qt-creator-linux)

#### CLI (with ninja, Windows + Linux)
##### Windows prerequisite
Make sure the "x86 Native Tools Command Prompt for VS2022" is used instead of the default.

##### Linux prerequisite - Steam Runtime 3 "Sniper" Container
Just download and use the OCI image for Docker/Podman/Toolbx:

###### Fetching the container
* [Docker](https://www.docker.com/): `sudo docker pull registry.gitlab.steamos.cloud/steamrt/sniper/sdk`
* [Podman](https://podman.io/): `podman pull registry.gitlab.steamos.cloud/steamrt/sniper/sdk`
* [Toolbx](https://containertoolbx.org/): `toolbox create -i registry.gitlab.steamos.cloud/steamrt/sniper/sdk sniper`

###### Running the container
Docker:
```
# docker run -v /PATH_TO_REPO/neo/mp/src:/root/neo/mp/src --rm -it --entrypoint /bin/bash registry.gitlab.steamos.cloud/steamrt/sniper/sdk
$ cd /root/neo/mp/src/
```

Podman: 
```
$ podman run -v /PATH_TO_REPO/neo/mp/src:/root/neo/mp/src --rm -it --entrypoint /bin/bash registry.gitlab.steamos.cloud/steamrt/sniper/sdk
$ cd /root/neo/mp/src/
```

Toolbx: 
```
$ toolbox enter sniper
$ cd /PATH_TO_REPO/neo/mp/src
```

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
and "Neotokyo: Revamp" should appear. To run the game, original [NEOTOKYO](https://store.steampowered.com/app/244630/NEOTOKYO/)
have to be installed and NT;RE will try to automatically mount it.

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
NOTE: This doesn't work on linux when running the mod from Steam.

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
and set: `LD_PRELOAD=/usr/lib32/libtcmalloc.so %command%` as the launch options.

Source 2013 Multiplayer also bundles with itself an outdated version of SDL2, due to which there might be false key presses (see: [link](https://github.com/ValveSoftware/Source-1-Games/issues/3335)) and doubled mouse sensitivity when compared to windows/other source games (see: [link](https://github.com/ValveSoftware/Source-1-Games/issues/1834)).
To fix these issues as well, install `lib32-sdl2`, and set: `LD_PRELOAD=/usr/lib32/libtcmalloc.so:/usr/lib32/libSDL2-2.0.so.0 %command%` as the launch options instead.

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
    ln -s ~/.steam/sdk32 ~/.steam/steam/steamcmd/linux32
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
2. Choose a location for your server to be installed into, for example, `C:\NeotokyoServer\`, and create it. (In my case SteamCMD was also in this location)
3. Run SteamCMD: `steamcmd.exe`
4. Enter the following commands in SteamCMD (Note that you need to use an absolute path for the install dir):
    ```
    force_install_dir <YOUR_LOCATION>\ognt\
    login anonymous
    app_update 313600 validate
    (wait for it to install)
    quit
    ```
5. Run SteamCMD again, and enter these commands:
    ```
    force_install_dir <YOUR_LOCATION>\ntrebuild\
    (this will be the main directory of your server)
    login anonymous
    app_update 244310 validate
    (wait for it to install)
    quit
    ```
6.  Copy all files from the latest release of NT;RE under `mp\game\neo\` into `<YOUR_LOCATION>\ognt\NeotokyoSource` and replace all existing files.
7.  Extract the latest release of NT;RE into `<YOUR_LOCATION>\ntrebuild`, so you will have a directory `<YOUR_LOCATION>\ntrebuild\neo` with a `gameinfo.txt` inside.
8. Allow all Inbound and Outbound TCP and UDP requests for the following ports via Windows Firewall. [See how](https://learn.microsoft.com/en-us/windows/security/operating-system-security/network-security/windows-firewall/configure#create-an-inbound-port-rule)
    * 27015 TCP+UDP (you can keep the TCP port closed if you don't need RCON support)
    * 27020 UDP
    * 27005 UDP
    * 26900 UDP
9. Your server should be ready to go, launch it inside your main directory (`...\ntrebuild\`) with the following command: (You can alter any argument to your liking, except `-game` and `-neopath`) 
```
srcds.exe -game neo -neopath "<YOUR_LOCATION>\ognt\NeotokyoSource" +ip <YOUR_IP> -maxplayers <1-32> +map <MAP_NAME>
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
4. Optional: Link or copy over neo, otherwise `-game <path_to_source>/mp/game/neo` can be used also:
    * Windows: `mklink /J neo "<path_to_source>/mp/game/neo"`
    * Linux:
        * Non-persistent bind mount: `mkdir neo && sudo mount --bind <path_to_source>/mp/game/neo neo`
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

