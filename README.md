Binary IO module for Garry's Mod.

## API
Similar to http://wiki.garrysmod.com/page/Category:file
```
table, table  gaceio.List(path) // Returns two tables. Files and folders
              gaceio.CreateDir(path)
              gaceio.Delete(path)
boolean       gaceio.Exists(path)
boolean       gaceio.IsDir(path)
string        gaceio.Read(path)
number        gaceio.Size(path)
number        gaceio.Time(path)
              gaceio.Write(path, str)

string        gaceio.CRC(path) // Returns a CRC32

string        gaceio.BZip2(str) // Note: takes a str parameter, not a path
              gaceio.WriteBZip2(path, str) // Compresses and writes str
```

Path is relative to GarrysMod folder (aka the folder from which you launch srcds or gmod client). So for example to list addons you would use ```gaceio.List("./garrysmod/addons")```.

All functions return ```false, error_string``` on error, which means you might have to check for existence of ```error_string``` when using ```gaceio.Exists(name)``` or other functions that return a boolean.

## Usage
Grab binaries from Downloads tab, dump them into garrysmod/lua/bin, call ```require("gaceio")``` and have fun.

## Compilation
Included are 32 and 64 bit versions of [Garry's Bootil][https://github.com/garrynewman/bootil], just remove the "_x86" or "_64" before building, or if those don't work, build it yourself.

This project requires [garrysmod_common][1], a framework to facilitate the creation of compilations files (Visual Studio, make, XCode, etc). Simply set the environment variable '**GARRYSMOD\_COMMON**' or the premake option '**gmcommon**' to the path of your local copy of [garrysmod_common][1].

  [1]: https://github.com/danielga/garrysmod_common
