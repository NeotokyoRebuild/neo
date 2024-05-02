# Server instructions

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

