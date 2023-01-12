# xTRD and xSCL Far plugins for Far2l (Linux)

This is a port of famous Far plugins xSCL and xTRD by HalfElf and Alexander Medvedev to [Linux version of the Far Manager](https://github.com/elfmz/far2l).
These plugins allow one to work with .TRD and .SCL files as archives and easily extract or add files.

## Build
First we update submodules. This will fetch [`far2l`](https://github.com/elfmz/far2l) sources. None of them will be built, we only require few headers from there.
Then we are ready for compilation.
```
git submodule update --init
make release
```

Then copy contents of the `release` directory to your `far2l` installation location. E.g. for Ubuntu:
```
sudo cp -r release/* /usr/lib/far2l/Plugins/
```


## Prebuilt version

You can also try to use prebuild version of plugin. Just unpack [this archive](https://github.com/atsidaev/xTRD-xSCL-far2l/releases/download/v0.1/xTRD-xSCL-far2l-0.1.zip) to /Plugins directory of your Far installation. 

It is not built specially for any specific Linux distro, however, it will work in many cases just because these plugins use only libc, liobm, libgcc, libstdc++ libraries.