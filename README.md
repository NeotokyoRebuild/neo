# NEOTOKYO Rebuild

* NEOTOKYO rebuild in Source SDK 2013 Multiplayer
* Forked from: https://github.com/NeotokyoRevamp/neo
* License: SOURCE 1 SDK LICENSE, see [LICENSE](LICENSE) for details
* See [CONTRIBUTING.md](CONTRIBUTING.md) for instructions on how the codebase work and contribute

## Building the SDK

### Building requirements

* Windows: MSVC v143, use either:
    * Full IDE/tooling: Visual Studio 2022
    * or just the [Microsoft C/C++ Build Tools v17](https://aka.ms/vs/17/release/vs_BuildTools.exe)
* Linux: Steam-runtimes chroot/docker
* Both:
    * cmake
    * ninja (optional, can use nmake/make/VS instead)

### Building
If using VS2022/qtcreator, you can go through there to build the project, otherwise if using CLI:
```
$ cd /PATH_TO_REPO/neo/mp/src

$ # To build in Release mode:
$ cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
$ cmake --build build/release --parallel

$ # To build in Debug mode:
$ cmake -S . -B build/debug -G Ninja
$ cmake --build build/debug --parallel
```

On Windows CLI, make sure the "x86 Native Tools Command Prompt for VS2022" is used instead of the default.

#### QtCreator
Generally setting it up "just works", however by default builds are not done in parallel. To fix this, just
from the sidebar go to: Project > [YOUR KIT (under Build & Run)] > Build > Build Steps. Just add `--parallel` to
CMake arguments.

### Additional setup + steam mod setup
See instructions for your platform, refer to the VDC wiki on setting up extras, chroot/containers, etc...:
https://developer.valvesoftware.com/wiki/Source_SDK_2013

For setting up the Steam mod:
https://developer.valvesoftware.com/wiki/Setup_mod_on_steam

### Linux extra notes
#### Arch Linux
Install `lib32-gperftools`, and set: `LD_PRELOAD=/usr/lib32/libtcmalloc.so %command% -game /path/to/mp/game/neo` as launch argument

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

