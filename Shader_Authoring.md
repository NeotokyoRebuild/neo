# Shader authoring (Windows setup)

## Todo: Possible to compile on Linux?

## Compiling the shaders

* Some outline for shaders from the VDC wiki:
    * [Shader Authoring, general](https://developer.valvesoftware.com/wiki/Shader_Authoring)
    * [Example shader](https://developer.valvesoftware.com/wiki/Source_SDK_2013:_Your_First_Shader)
* I'm unsure if the [11/7/2008 DX SDK](https://www.microsoft.com/en-us/download/details.aspx?id=4064) mentioned in the above VDC article is needed; it seems like the shaders compile just fine on modern Windows even without installing it(?)
* At least on Windows, you will need Perl with the package String-CRC32.
* Compatible pre-packaged Perl installers are available [here](https://platform.activestate.com/Rainyan/ActivePerl-5.28-SourceSDK2013).

### Troubleshooting

* If Perl was not found when running shader scripts, make sure it's included in your PATH env var.
* You will probably need to slightly edit some of the build scripts (buildhl2mpshaders.bat/buildsdkshaders.bat/buildshaders.bat) to match your setup, and create some symlinks or copy files to get access to FileSystem_Steam.dll, and the other "\bin\..." shader compile tools.
* If you get errors with nmake not found, make sure you're running the scripts from a Visual Studio (x86) Native Tools Command Prompt. At least the 2017 x86 Tools Prompt confirmed working.
    * Alternatively, edit the "buildsdkshaders.bat" to call the appropriate vsvars32.bat for setting up nmake for your particular environment.
* If you get a "bin\something" not found error when already inside the bin folder, add a symlink for ".\bin" <--> "." as workaround.

---

Finally, you should be greeted with some output akin to:
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
