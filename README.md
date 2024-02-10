# ZX Spectrum Far plugins for Far2l (Linux)

This is a port of famous Far x-plugins by HalfElf and Alexander Medvedev to [Linux version of the Far Manager](https://github.com/elfmz/far2l).
These plugins allow one to work with various disk image formats (such as .TRD, .SCL, .FDI, .TD0 and .UDI) as archives and easily extract or add files.

☑ xCreate \
☑ xTRD \
☑ xSCL \
☑ xISD \
☐ xLook

## Installation

You may download prebuild packages from the [Releases](https://github.com/atsidaev/far2l-plugins-speccy/releases) section. If you are on Debian/Ubuntu, get the `far2l-plugins-speccy_<version>_amd64.deb` file and then install it with `dpkg`
```
$ sudo dpkg -i far2l-plugins-speccy_0.9_amd64.deb
```

If you use other Linux distributions, you could download `far2l-plugins-speccy_<version>_amd64.zip` archive and unzip it to the location of your `far2l` installation. Most likely it is `/usr/lib/far2l/Plugins`. 

These packages are built against very small amount of dependencies (libc, libm, libgcc, libstdc++), so they should work for any 64-bit Linux distrubutions. However, if you experience any issues, rebuild them yourself (see the next section).

## Build
This projects requires several header files from `far2l` itself. Unfortunately, there is no separate headers package for it. So we use whole `far2l` repo as submodule to reference files from it.

So, the first build step is to update submodules. This will fetch [`far2l`](https://github.com/elfmz/far2l) sources. None of them will be built, we only require few headers from there.
Then we are ready for compilation.
```
$ git submodule update --init
```

Then use `make deb` target to build `deb`-package, or use `make zip` target to build the plugins as simple `zip` archive. See Installation section for instruction for the next steps.

Alternative option is to build plugins without packaging them

```
make release
```

Then copy contents of the `release/usr/lib/far2l/Plugins` directory to your `far2l` installation location. E.g. for Ubuntu:
```
sudo cp -r release/usr/lib/far2l/Plugins /usr/lib/far2l/Plugins/
```