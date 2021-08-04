# Shader authoring (Windows setup)

## Compiling the shaders

* Some outline for shaders from the VDC wiki:
    * [Shader Authoring, general](https://developer.valvesoftware.com/wiki/Shader_Authoring)
    * [Example shader](https://developer.valvesoftware.com/wiki/Source_SDK_2013:_Your_First_Shader)
* You'll need Perl with the package String-CRC32.
* Compatible pre-compiled Perl installer is available [here](https://platform.activestate.com/Rainyan/Perl-SourceSDK2013). Strawberry Perl should most likely also work, but you'll need to confirm the String-CRC32 package is available.

### Troubleshooting

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
