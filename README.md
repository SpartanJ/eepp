
# eepp - Entropia Engine++

**eepp** is an open source cross-platform game and application development
framework heavily focused on the development of rich graphical user interfaces.

[![Linux status](https://img.shields.io/github/workflow/status/SpartanJ/eepp/Linux/develop?label=Linux)](https://github.com/SpartanJ/eepp/actions?query=workflow%3ALinux)
[![Windows status](https://img.shields.io/github/workflow/status/SpartanJ/eepp/Windows/develop?label=Windows)](https://github.com/SpartanJ/eepp/actions?query=workflow%3AWindows)
[![macOS status](https://img.shields.io/github/workflow/status/SpartanJ/eepp/macOS/develop?label=macOS)](https://github.com/SpartanJ/eepp/actions?query=workflow%3AmacOS)

## Features

### Cross platform functionality

* Official support for Linux, Windows, macOS, Android and iOS.

* Exports to HTML5 using emscripten with some minor limitations.

* Also works on BSD and Haiku.

### UI Module

* Base widgets to manage the app/game objects as nodes, with all basic input interaction events ( clicks, keypress, mouser over, focus, etc ).

* Fully featured UI system, animation support, scaling, rotating, clipping, events, messages, etc.

* Themes and skins/decorations support.

* Pixel density support ( scaling of UI elements defined by the pixel density of the screen ).

* All the basic widgets are implemented ( button, textbox, combobox, inputbox, menu, listbox, scrollbars, etc ).

* Draw invalidation support. It can be used to make real apps, with low resource usage ( only redraws when is needed ).

* Layout system similar to Android layouts ( LinearLayout, RelativeLayout, GridLayout ).

* Advanced features as text selection, copy and paste, and key bindings.

* Load and style layouts from XMLs

* Styling with Cascading Style Sheets

### Graphics Module

* Renderers for OpenGL 2 ( fixed-pipeline ), OpenGL 3 ( programmable-pipeline ), OpenGL ES 2, OpenGL ES 1, and OpenGL Core Profile.

* Batch Renderer ( all the rendering is automatically batched by the engine ).

* Fonts support ( TrueType, BMFont and XNA Fonts ).

* Frame Buffer support.

* Shaders support ( with automatic fixed pipeline shaders to programmable conversor ).

* Vertex Buffer Object support.

* Particle System.

* Extendable Console.

* Animated Sprites.

* Texture Atlas support ( automatic creation and update of the texture atlas, editor included ).

* Clipping Masks ( stencil, scissors, planes )

* Nine Patch resizable bitmaps support.

* Primitives drawables.

* Many image formats supported ( included rasterized SVG ), compressed textures support ( direct upload to the GPU  when possible ).

### Window Module

* Backend based module, this means that you can easily create a backend for the window/input handling.

* Currently supports SDL 2 as backend.

* Clipboard support.

* Hardware cursors.

* Display Manager

* Joystick support.

### Audio Module

* OpenAL audio engine with extendable file format support. Read and write support for OGG and Wav, and read support for MP3 and FLAC.

### Physics Module

* Full OOP chipmunk physics wrapper.

### System Module

* Provides all the basics stuffs for the full multi-threading support of the library, file formats support for packing, clocks, resource manager, translator, and much more.

* Virtual File System class ( abstract assets providers into a single virtual file system, abstracting zip files and local file system into one for transparent load of resources, similar to [PhysicsFS](https://www.icculus.org/physfs/) ).

### Core Module

* Customizable Memory Manager. Used by default in debug mode to track memory leaks.

* UTF8, UTF-16, UTF-32, Ansi, Wide Char support.

* String class using UTF-32 chars internally.

* Debug macros

### Math Module

* General purpose functions and templates ( vector, quad, polygon, etc ).

* Interpolation classes with easing.

* Some minor math utilities, include Mersenne Twister random number generator implementation, perlin noise and more.

### Network Module

* Web Requests with HTTP client, with **TLS support** ( provided by mbedtls or openssl ).

* Asynchronous HTTP requests.

* File Transfers with FTP client and FTPS client ( FTP with explicit TLS ).

* TCP and UDP sockets.

* HTTP Content-Encoding and Transfer-Encoding support.

* HTTP Proxy Support.

* HTTP Compressed response support.

* Also HTTP resume/continue download support and automatic follow redirects.

### Scene Module

* Node based system for easy management of scenes.

* Full control of node events ( clicks, mouse over, focus, etc ).

* Event system.

* Node Message system.

* Programmable actions for nodes ( fade, rotate, move, scale, etc ).

### Maps Module

* Tiled Maps with software dynamic lights.

* Full featured map editor.

### Tools

* Very simple UI Editor. Load layouts from an XML file and see the changes being made in real time.

* Texture Atlas Editor. A very simple tool to allow the developer to create and edit texture atlases.

* Map Editor: A advanced but simple map editor for the game engine. It lacks several features since I didn't have the time to work on it, this particular tool will probably die in favor of TMX map support in the near future ( but i'm not a fan of TMX maps, so there's no decision for the moment ).

### General Features

* Support for multi-threaded resource loading ( textures, sounds, fonts, etc ).

## Documentation

Documentation is located [here](https://eepp.ensoft.dev). I'm currently working
on improving it. About 50% of the project is currently documented so still needs
a lot of work. Please check the code examples located in `src/examples` and you
can also check out the test ( `src/test` ) and tools ( `src/tools` ).

I'm putting my efforts on improving the documentation on the UI module since
currently is the most important and complex module but lacks of proper
documentation. If you have any question you can contact me anytime.

## Getting the code

The repository uses git submodules so you'll need to clone the repository and
its submodules, in order to achieve this easily you can simply clone with:

`git clone --recurse-submodules https://github.com/SpartanJ/eepp.git`

## UI Screenshots

![Map Editor](https://web.ensoft.dev/eepp/screenshots/eepp1.png)

![UI Elements with 2x pixel density](https://web.ensoft.dev/eepp/screenshots/eepp2.png)

![Texture Atlas Editor with 1.5x pixel density](https://web.ensoft.dev/eepp/screenshots/eepp3.png)

## UI Layout XML example

It should look really familiar to any Android developer. This is a window with
the most basic controls in a vertical linear layout display.

```xml
<window layout_width="300dp" layout_height="300dp" winflags="default|maximize">
  <LinearLayout id="testlayout" orientation="vertical" layout_width="match_parent" layout_height="match_parent" layout_margin="8dp">
    <TextView text="Hello World!" gravity="center" layout_gravity="center_horizontal" layout_width="match_parent" layout_height="wrap_content" backgroundColor="black" />
    <PushButton text="OK!" textSize="16dp" icon="ok" gravity="center" layout_gravity="center_horizontal" layout_width="match_parent" layout_height="wrap_content" />
    <Image src="thecircle" layout_width="match_parent" layout_height="32dp" flags="clip" />
    <Sprite src="gn" />
    <TextInput text="test" layout_width="match_parent" layout_height="wrap_content" />
    <DropDownList layout_width="match_parent" layout_height="wrap_content" selectedIndex="0">
      <item>Test Item</item>
      <item>@string/test_item</item>
    </DropDownList>
    <ListBox layout_width="match_parent" layout_height="match_parent" layout_weight="1">
      <item>Hello!</item>
      <item>World!</item>
    </ListBox>
  </LinearLayout>
</window>
```

## UI Widgets with C++ example

How does it look with real code?

```cpp
UITextView::New()->setText( "Text  on  test  1" )
         ->setCharacterSize( 12 )
         ->setLayoutMargin( Rect( 10, 10, 10, 10 ) )
         ->setLayoutSizeRules( LayoutSizeRule::MatchParent, LayoutSizeRule::WrapContent )
         ->setParent( layout );
```

## UI Styling

Element styling can be done with a custom implementation of Cascading Style
Sheets, most common CSS2 rules are available, plus several CSS3 rules (some
examples: [animations](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Animations/),
[transitions](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Transitions),
[custom properties](https://developer.mozilla.org/en-US/docs/Web/CSS/Using_CSS_custom_properties),
[media queries](https://developer.mozilla.org/en-US/docs/Web/CSS/Media_Queries/Using_media_queries),
[@font-face at rule](https://developer.mozilla.org/en-US/docs/Web/CSS/@font-face),
[:root element](https://developer.mozilla.org/en-US/docs/Web/CSS/:root)).
Here is a small example on how the CSS looks like:

```css
@font-face {
  font-family: "OpenSans Regular";
  src: url("https://raw.githubusercontent.com/SpartanJ/eepp/develop/bin/assets/fonts/OpenSans-Regular.ttf");
}

@import url("assets/layouts/imported.css") screen and (min-width: 800px);

:root {
  --font-color: black;
  --background-input-color: rgba(255, 255, 255, 0.7);
  --border-color: black;
  --border-width: 1dp;
}

.screen TextView {
  color: var(--font-color);
}

.form {
  background-image: @drawable/back;
  background-repeat: no-repeat;
  background-size: cover;
}

.form .form_inputs {
  background-color: var(--non-existent, var(--background-input-color));
  margin-left: 100dp;
  margin-right: 100dp;
  padding-top: 72dp;
  padding-left: 57dp;
  padding-right: 57dp;
  padding-bottom: 115dp;
}

.screen TextView.input,
.screen TextInput.input {
  font-family: AkzidenzGroteskBQ-Cnd;
  layout-width: match_parent;
  layout-height: 80dp;
  border-color: var(--border-color);
  border-width: var(--border-width);
  color: var(--font-color);
  padding-left: 40dp;
  padding-right: 40dp;
  margin-bottom: 32dp;
  skin: none;
  hint-font-family: AkzidenzGroteskBQ-Cnd;
  hint-font-size: 46dp;
  hint-color: #818285;
  background-color: #FFFFFF00;
  transition: all 0.125s;
}

.screen TextInput.input:focus {
  background-color: #FFFFFF66;
  border-color: #796500;
}

.screen TextInput.input:hover {
  background-color: #FFFFFF66;
}

@media screen and (max-width: 1024px) {

.form .form_inputs {
  background-color: red;
}

}
```

## How to build it

The library has very few external dependencies. Most of the time you will only
need **SDL2** and **OpenAL** libraries with the headers installed. Also
**premake4** or **premake5** is needed to generate the Makefiles or project
files to build the library. I will assume that you know what you are doing and
skip the basics.

Notice: OpenAL is not strictly necessary. If you want to avoid it, you
have the alternative to use [mojoAL](https://icculus.org/mojoAL/). The latter
is already integrated to the library but not enabled by default. To enable it
and disable the OpenAL dependency you need to add the parameter `--with-mojoal`
to any `premake` call ( ex: `premake5 --with-mojoal gmake2` ).

### GNU/Linux

In a Ubuntu system it would be something like ( also you will need gcc but it
will be installed anyways ):

`sudo apt-get install premake4 libsdl2-2.0-0 libsdl2-dev libopenal1 libopenal-dev`

Clone the repository and run on the repository root directory:

`premake4 gmake`

or if you have premake5 installed you can run:

`premake5 gmake2`

Then just build the library:

`make -C make/linux`

That's it. That will build the whole project.

### Windows

You have two options: build with [Visual Studio](https://visualstudio.microsoft.com/)
or with [MinGW](https://sourceforge.net/projects/mingw-w64/).
To be able to build the project with any of these options first you will need to
generate the project files with [premake4 or premake5](https://premake.github.io/download.html).
Then you will need to add the binary file to any of the executable paths defined
in `PATH` ( or add one, or use it from a local path ). Also you will need to
install the prebuild binaries and development libraries of [SDL2](http://libsdl.org/download-2.0.php)
and [openal-soft](http://kcat.strangesoft.net/openal.html#download).
Download *Visual Studio* or *MinGW* files depending on your needs.

#### Visual Studio

You will need to use premake5 and run:

`premake5.exe vs2019`

Then the project files should be found in `make/windows/`. A complete solution
and all the project will be available. Having installed everything, you'll be
able to build the *Visual Studio* solution as any other project.

If you are very new to programming there's an alternative to build the project
without external dependencies, in order to do that you need to generate the
project files with the command:

`premake5.exe --windows-vc-build --with-mojoal vs2019`

Then just build the solution in Visual Studio or run `MSBuild` manually in a
console:

`"%MSBUILD_PATH%\MSBuild.exe" .\make\windows\eepp.sln -m`

Where `%MSBUILD_PATH%` is the MSBuild.exe Visual Studio path, for example for
_VS2019 Community Edition_ the path usually is:

`C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\`

#### MinGW

`premake4.exe gmake`

 Then just build the project located in `make/windows/` with `mingw32-make.exe`
 or any equivalent:

`mingw32-make.exe -C make\windows`

### macOS

You will need the prebuild binaries and development libraries of
[SDL2](http://libsdl.org/download-2.0.php), OpenAL is included with the OS.
Install the SDL2 framework and you should be able to build the project.

You have two options to build the project: with *XCode* or with *Makefiles*.
To build with any of both options first you will also need to build the project
files with [premake4 or premake5](https://premake.github.io/download.html).

#### Makefiles

##### Using premake5

Generate the project:

`premake5 --use-frameworks gmake2`

And build it:

`make -C make/macosx`

##### Using premake4

You can use the `projects/osx/make.sh` script, that generates the *Makefiles*
and builds the project.

#### XCode

Run:

`premake5 --use-frameworks xcode4`

And open the XCode project generated at `make/macosx/` or simply build from the
command line with:

`xcodebuild -project make/macosx/project-name.xcodeproj`

### Android

There's a gradle project in `projects/android-project/`. It will build the
library with all the dependencies included. Use the example project as a base
for your project. Notice that there's a `eepp.mk` project file that builds the
library. That file can be used in you projects.

### iOS

The project provides two files to build the library and the demos. You can use
any of them depending on your needs.
The files are located in `projects/ios`:

#### gen-xcode4-proj.sh script

This script can be used to generate the xcode projects and solution of all the
included projects in eepp (demos, tools, shared lib, static lib, etc). It will
also download and build the SDL2 fat static library in order to be able to
reference the library to the project. After building the projects sadly you'll
need to make some minor changes to any/all of the projects you wan't to build
or test, since the project generated lacks some minor configurations. After
you run this script you'll need to open the solution located in
`make/ios/eepp.xcworkspace`. To build the static libraries you'll not find any
problem (that will work out of the box). But to test some of the examples it's
required to:

##### Add the Info.plist file

Select (click on the project name) the project you want to test, for example
 `eepp-empty-window`. You will se several tabs/options, go to _Build Settings_,
 and locate the option _Info.plist_ file, double click to edit and write:
 `Info.plist`. This will indicate to read that file that is located in the same
 directory than the project. The go to the tab _General_ and complete the
 _Bundle Identifier_ with an identifier name of the app bundle that will be
 generated, for this example you can use something like: `eepp-empty-window`.
 That will allow you to build and run the project.

##### Add resources to the project

This `eepp-empty-window` demo does not use any assets/resources, but other demos
will need to load assets, and this assets need to be added to the project in order
to be available to the app bundle. For example, the project `eepp-ui-hello-world`,
will require you to add the `assets` folder into the project. What you need to do
is: select the project and go to the _Build Phases_ tab, in _Copy Bundles Resources_
click in the plus icon (+), then go to _Add Other..._ and locate and select the
`bin/assets/` folder and _Finish_. That should be enough.

#### compile-all.sh script

This script can be used to build the SDL2 and eepp as two fat static libraries
with arm64 and x86_64 architectures in it (arm64 for iPhone/iPad and x86_64 for
the simulators). To generate a release build pass `config=release` as a parameter
fo the script (`sh compile-all.sh config=release`). The built files will be
located in `libs/ios/`, as `libSDL2.a` and `libeepp.a` (or `libeepp-debug.a` for
debug build). This two files can be integrated in your project.

### emscripten

You will first need to [download and install emscripten](https://emscripten.org/docs/getting_started/downloads.html).
Then there's a script for building the **emscripten** project in
`projects/emscripten/make.sh`. Before running this script remember to set the
emsdk enviroment, it should be something like: `source /path/to/emsdk/emsdk_env.sh`.
That should be enough in **GNU/Linux** or **macOS** ( only tested this on GNU/Linux ).

## Author comment

The library has been being developed for several years, it suffered many changes
since its beginnings, I'm making any changes that I find necessary to improve
it, so the API is still not totally stable (but close to be).
It's being used in several applications oriented to publicity campaigns mostly
developed for Android devices and Windows PCs.

I personally never had the time to use it to develop a complex game with the
library ( several frustrated projects ), but I made several UI oriented games
for clients.

The current project focus is on the UI module. And I'll continue working
putting my focus on this.

The plan is to provide an alternative UI toolkit fully hardware accelerated
similar to the Android toolkit but simpler ( as in easy to use ) and also
oriented to desktop apps.

Audio and Network modules were based the modules in SFML with several important
differences mentioned above.

I like to use what's well done and fit my needs, but since I have my personal
views on how to implement some things I prefer to take the code, to have full
control over it.

Also many ideas were/are taken from other projects. Some I can think about:
*cocos2d-x*, *raylib*, *Android SDK*, *libGDX*, *Godot*, *XNA*, *LÖVE*, and many
other projects.

If all this sounds interesting to you for some crazy reason, contact me and let
me know if I can help you to get into the library, and may be if you want, you
can contribute to it in the future. This project needs *contributors* more than
anything else.

The current state of the library is decent. In terms of features should be in a
similar position than the most used 2D game engines out there. But lacks of
course of the support+community that you can get from *Godot* or *cocos2d-x* to
mention a couple.

The main idea of this library is to focus on a better general approach to
develop heavily UI based apps ands games than the other options, with cleaner
code and implementation.

The main reason I developed the library is for _fun_ and to _learn_ new
technologies. I love spending time working on the library, but I know there's
probably no real reason to develop something like this with the immense number
of similar alternatives.

Regarding the code quality: this project started very long time ago and suffered
many modifications over time. A good chunk of the code base still uses old C++
practices (for example: raw pointers, own implementation of thread, mutex, etc).
Some of these things can be "modernized" others don't make much sense or
overhauling them would take too much time to justify the effort. I'm working on
"modernizing" some parts of the code, and new code usually tend to look more
modern.

### Plans/ideas for the future

Keep improving the UI system, adding new widgets and layouts and improving the CSS support.

Simplify and improve the UI widgets skinning/theming support.

Improve/create documentation for the UI module.

Add more examples and some tools.

Add Scripting support ( first I would like to stabilize the library, but I'm getting there ).

Add 2D skeletal animations support ( probably Spine2D, shouldn't be much work to implement ).

Probably deprecate the Maps module, since I will focus my efforts on the UI system.

## Acknowledgements

### Special thanks to

* Sean Barrett for stb_image and all the [stb](https://github.com/nothings/stb) libraries.

* Sam Latinga for [Simple DirectMedia Layer](https://www.libsdl.org/).

* Jonathan Dummer for the [Simple OpenGL Image Library](https://www.lonesock.net/soil.html).

* Laurent Gomila for [SFML](https://www.sfml-dev.org/)

* Yuri Kobets for [litehtml](https://github.com/litehtml/litehtml)

* Michael R. P. Ragazzon [RmlUI](https://github.com/mikke89/RmlUi)

* Ryan C. Gordon for [mojoAL](https://icculus.org/mojoAL/)

* David Reid for [dr_libs](https://github.com/mackron/dr_libs)

* Lion (Lieff) for [minimp3](https://github.com/lieff/minimp3) and more.

* Lewis Van Winkle for [PlusCallback](https://github.com/codeplea/pluscallback)

* Dieter Baron and Thomas Klausner for [libbzip](https://libzip.org/)

* Jean-loup Gailly and Mark Adler for [zlib](https://zlib.net/)

* Milan Ikits and Marcelo Magallon for [GLEW](http://glew.sourceforge.net/)

* Mikko Mononen for [nanosvg](https://github.com/memononen/nanosvg)

* Scott Lembcke for [Chipmunk2D](https://github.com/slembcke/Chipmunk2D)

* Christophe Riccio for [glm](https://github.com/g-truc/glm)

* Rich Geldreich for [imageresampler](https://github.com/richgel999/imageresampler) and [jpeg-compressor](https://github.com/richgel999/jpeg-compressor)

* Arseny Kapoulkine for [pugixml](https://github.com/zeux/pugixml)

* Jason Perkins for [premake](https://premake.github.io/)

* Martín Lucas Golini ( me ) and all the several contributors for [SOIL2](https://github.com/SpartanJ/SOIL2) and [efsw](https://github.com/SpartanJ/efsw)

* The Xiph open source community for [libogg](https://xiph.org/ogg/) and [libvorbis](https://xiph.org/vorbis/)

* The [ARMmbed](https://github.com/ARMmbed) community for [mbed TLS](https://tls.mbed.org/)

* [kcat](https://github.com/kcat) for [openal-soft](http://kcat.strangesoft.net/openal.html)

* The [FreeType Project](https://www.freetype.org/freetype2/docsindex.html)

* And a **lot** more people!

## Code License

[MIT License](http://www.opensource.org/licenses/mit-license.php)
