# xTRD and xSCL Far plugins for Far2l (Linux)

This is a port of famous Far plugins by Alexander Medvedev to (Linux version of the Far Manager)[https://github.com/elfmz/far2l].

## Build
To build the plugins run following command in xSCL and xTRD directories
```
make FAR_PATH=/usr/src/far2l
```

Here /usr/src/far2l is your (Far)[https://github.com/elfmz/far2l] source tree location.

Then create /Plugins/xSCL/plug and /Plugins/xTRD/plug directories.
Put ./xSCL/xSCL.far-plug-mb and contents of ./xSCL/res to /Plugins/xSCL/plug directory. Put ./xTRD/xTRD.far-plug-mb and contents of ./xTRD/res to /Plugins/xTRD/plug directory. Run Far and make sure that the new configuration entries appeared in Configure Plugins window.

## Prebuilt version

You can also try to use prebuild version of plugin. Just unpack the archive to /Plugins directory of your Far installation.