# ZX Spectrum Far plugins for Far2l (Linux)

This is a port of famous Far x-plugins by HalfElf and Alexander Medvedev to [Linux version of the Far Manager](https://github.com/elfmz/far2l).
These plugins allow one to work with various disk image formats (such as .TRD, .SCL, .FDI, .TD0 and .UDI) as archives and easily extract or add files.

☑ xCreate \
☑ xTRD \
☑ xSCL \
☑ xISD \
☑ xLook \
☑ xHRiP \
☑ xZXZIP

`xLook` opens supported ZX Spectrum files as panels, where their decoded contents can be extracted with F5. It recognises HoBeta files and headerless files, including bare `.scr` screens.

Supported content includes:

- Packed files: Hrust 1.x/2.x, HRUM, LZS, MSPACK, DSQ, PCD, CC3 and TRUSH.
- BASIC programs and assembler sources: ALASM, MASM, TASM, XAS, ZXASM and STORM, exported as text.
- Graphics: 32 formats, including colour and monochrome screens, GigaScreen, multicolour, chunks, bitplanes, fonts, Laser Compact, Screen Crasher and sprites, exported as RGB PNG images.
- Tracker modules: text summaries with the tracker name, title and author where available.

Graphics with multiple frames or interpretations appear as separate panel entries (`name.png`, `name_2.png`, ...). These include FLASH phases, screens without attributes, GigaScreen frames and their blended image. Each image can be extracted separately.

Use `xv:path/to/file` to view decoded contents or `xe:path/to/file` to open them in an editor. Unrecognised files open in the ordinary far2l viewer or editor. For graphics, `xv:` opens an external image viewer; the bundled `xLook/plug/associat.ion` uses `xdg-open`. Edit its `view=` command to select another viewer, for example `feh %f` or `eog %f`. `%f` is replaced with the filename, and arguments may be quoted. Commands run directly without shell expansion or pipelines. All decoded images are saved together in a temporary folder, allowing viewers with folder navigation to browse them.

`xHRiP` and `xZXZIP` open HRiP and ZXZip archives (including multi-volume ones) as panels and unpack files from them into HoBeta files or an SCL image.

## Installation

You may download prebuild packages from the [Releases](https://github.com/atsidaev/far2l-plugins-speccy/releases) section. If you are on Debian/Ubuntu (only recent versions, which include far2l in repo), get the `far2l-plugins-speccy_<version>_amd64.deb` file and then install it with `dpkg`
```
sudo dpkg -i far2l-plugins-speccy_0.9_amd64.deb
```

If you are on Arch/Manjaro, get the `far2l-plugins-speccy-<version>-1-x86_64.pkg.tar.zst` package and install it with:
```
sudo pacman -U far2l-plugins-speccy-0.9-1-x86_64.pkg.tar.zst
```

If you use other Linux distributions, you could download `far2l-plugins-speccy_<version>_amd64.zip` and unzip it to the location of your `far2l` installation. Most likely it is `/usr/lib/far2l/Plugins`. 

These packages are built against very small amount of dependencies (libc, libm, libgcc, libstdc++), so they should work for any 64-bit Linux distrubutions. However, if you experience any issues, rebuild them yourself (see the next section).

## Build
This projects requires several header files from `far2l` itself. Unfortunately, there is no separate headers package for it. So we use whole `far2l` repo as submodule to reference files from it.

So, the first build step is to update submodules. This will fetch [`far2l`](https://github.com/elfmz/far2l) sources. None of them will be built, we only require few headers from there.
Then we are ready for compilation.
```
git submodule update --init
```

Then use `make deb` for a Debian package, `make zst` for an Arch/Manjaro package, or `make zip` for a plain plugin archive. See Installation section for the next steps.

Alternative option is to build plugins without packaging them

```
make release
```

Then copy contents of the `release/usr/lib/far2l/Plugins` directory to your `far2l` installation location. E.g. for Ubuntu:
```
sudo cp -r release/usr/lib/far2l/Plugins /usr/lib/far2l/Plugins/
```