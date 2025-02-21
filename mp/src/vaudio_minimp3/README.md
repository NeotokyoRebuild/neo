# VAudio MiniMP3

VAudio MiniMP3 is a replacement MP3 playback module for the Source Engine.

It uses [MiniMP3](https://github.com/lieff/minimp3) for playback of sound files encoded with the MPEG Layer 3 codec instead of Miles.

It can be built for any branch of the Source Engine, including the standard public Source 2007, Source 2013 and Alien Swarm SDKs without any changes.

VAudio MiniMP3 supports regular MP3 playback as well as streaming MP3 playback.

## Why?

The current solution, VAudio Miles, uses Miles (duh!) and can be quite problematic on modern systems as it creates writable + executable pages, which makes it fail to work on systems utilising SELinux.

Miles requires a license to use, whereas MiniMP3 is open source and free!

There is no 64-bit support for the version of Miles shipped with the Source Engine.

## Build instructions

Simply insert this folder somewhere in your Source SDK code tree, and hook it up to `projects.vgc` and `groups.vgc` where applicable and build like any other part of the engine.

## ⚠️ Note about replacing DLLs! ⚠️

*Before you get any ideas...*

Do **NOT** replace the `vaudio_miles` binary with a renamed `vaudio_minimp3` in any multiplayer VAC-secured game unless you want to get banned.

This project is provided *solely* for use in your own mods/projects, experimentation in single player games, and educational purposes.

## License

VAudio MiniMP3 is licensed under MIT, MiniMP3 is licensed under CC0.
