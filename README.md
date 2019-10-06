



Entropia Engine++
=================

**Entropia Engine++** is a C++ 2D game engine/library designed for an easy cross-platform game development.

**eepp provides:**
------------------

**Cross platform functionality:**
---------------------------------

  * Official support for Linux, Windows, Mac OS X, iOS and Android.

  * Exports to HTML5 using emscripten with some limitations.

  * It should work on FreeBSD, Solaris and Haiku.


**Graphic Module:**
-------------------

  * Renderers for OpenGL 2 ( fixed-pipeline ), OpenGL 3 ( programmable-pipeline ), OpenGL ES 2, OpenGL ES 1, and OpenGL Core Profile.

  * Batch Renderer ( all the rendering is automatically batched by the engine ).

  * Full fonts support. True Type, BMFont and XNA Fonts.

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

**Window Module:**
------------------

  * Backend based module, this means that you can easily create a backend for the window/input handling.

  * Currently supports SDL 2 as backend.

  * Clipboard support.

  * Hardware cursors.

  * Display Manager

  * Joystick support.


**Audio Module:**
-----------------

  * OpenAL audio engine with extendable file format support. Read and write support for OGG and Wav, and read support for MP3 and FLAC. Audio module is based on an old version of the SFML Audio module.


**Physics Module:**
-------------------

  * Full OOP chipmunk physics wrapper.


**System Module:**
------------------

  * Provides all the basics stuffs for the full multi-threading support of the library, file formats support for packing, clocks, resource manager, translator, and much more.

  * Virtual File System class ( abstract assets providers into a single virtual file system, abstracting zip files and local file system into one for transparent load of resources, similar to PhysicsFS ).


**Core Module:**
--------------

  * Customizable Memory Manager. Used by default in debug mode to track memory leaks.

  * UTF8, UTF-16, UTF-32, Ansi, Wide Char support.

  * String class using UTF-32 chars internally.

  * Debug macros


**Math Module:**
----------------

  * General purpose functions and templates ( vector, quad, polygon, etc ).

  * Interpolation classes with easing.

  * Some minor math utilities, include Mersenne Twister random number generator implementation, perlin noise and more.


**Network Module:**
------------------
  * Web Requests with HTTP client, with **SSL support** ( provided by mbedtls or openssl ).

  * Asynchronous HTTP requests.

  * File Transfers with FTP client.

  * TCP and UDP sockets.

  * HTTP Content-Encoding and Transfer-Encoding support.

  * HTTP Proxy Support.

Module was originally based on the SFML Network module implementation, though currently differs a lot from it.


**Scene Module:**
---------------------
  * Node based system for easy management of scenes.

  * Full control of node events ( clicks, mouse over, focus, etc ).

  * Event system.

  * Node Message system.

  * Programmable actions for nodes ( fade, rotate, move, scale, etc ).


**UI Module:**
--------------

  * Base controls to manage the game objects as nodes, with all basic input interaction events ( clicks, keypress, mouser over, focus, etc ).

  * Fully featured UI system, animation support, scaling, rotating, clipping, events, messages, etc.

  * Themes and skins support.

  * Pixel density support ( scaling of UI elements defined by the pixel density of the screen ).

  * All the basic controls are implemented ( button, textbox, combobox, inputbox, menu, listbox, scrollbars, etc ).

  * Draw invalidation support. It can be used to make real apps, with low resource usage ( only redraws when needed ).

  * Layout system similar to Android layouts ( LinearLayout, RelativeLayout, GridLayout ).

  * Advanced features as text selection and key bindings.

  * Load layouts from XMLs

  * Styling with CSS like format


**Maps Module:**
------------------

  * Tiled Maps with software dynamic lights.

  * Full featured map editor.

**Tools:**
------------
  * Very simple UI Editor. Load layouts from an XML file and see the changes being made in real time.

  * Texture Atlas Editor. A very simple tool to allow the developer to create and edit texture atlases.

  * Map Editor: A advanced but simple map editor for the game engine. It lacks several features since i didn't have the time to work on it, this particular tool will probably die in favor of TMX map support in the near future ( but i'm not a fan of TMX maps, so there's no decision for the moment ).

**General Features:**
---------------------

  * Support for multi-threaded resource loading ( textures, sounds, fonts, etc ).


**UI Screenshots:**
--------------------
![Map Editor](https://web.ensoft.dev/eepp/screenshots/eepp1.png)

![UI Elements with 2x pixel density](https://web.ensoft.dev/eepp/screenshots/eepp2.png)

![Texture Atlas Editor with 1.5x pixel density](https://web.ensoft.dev/eepp/screenshots/eepp3.png)

**UI Layout XML example:**
--------------------
It should look really familiar to any Android developer. This is a window with the most basic controls in a vertical linear layout display.

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

**UI Widgets with C++ example:**
-------------------------------------
How does it look with real code?

```c++
UITextView::New()->setText(  "Text  on  test  1"  )
				 ->setCharacterSize(  12  )
				 ->setLayoutMargin(  Rect(  10,  10,  10,  10  )  )
				 ->setLayoutSizeRules(  WRAP_CONTENT,  WRAP_CONTENT  )
				 ->setParent(  layout  );
```

**How can i learn to use the library?**
----------------------------------------------
This is the current big issue with the library. Since i'm solo working, it's really difficult keep up the documentation updated. But i'll work to improve that in the near future. For the moment you have two options: build the documentation and read code examples. The documentation is not complete but most modules are documented. Sadly not the UI module ( the biggest one ), but it's easy to get along with the examples. The library is supper easy to use, but is kind of big, so, knowing about everything i'll take time. I you're interested you can contact me anytime.

**How to build it?**
------------------------

The library has very few external dependencies. Most of the time you will only need SDL2 and OpenAL libraries with the headers installed. Also **premake4** or **premake5** is needed to generate the Makefiles or project files to build the library. I will assume that you know what you are doing and skip the basics.

**_GNU/Linux_**

In a Ubuntu system it would be something like ( also you will need gcc and freetype, but it will be installed anyways ):

`sudo apt-get install premake4 libsdl2-2.0-0 libsdl2-dev libopenal1 libopenal-dev`

Clone the repository and on the repository root directory run:
`premake4 gmake`

Then just build the library:
`cd make/linux`
`make`

That's it. That will build the whole project.


**_Windows_**

You have two options: build with *Visual Studio* or with *mingw*.
To build with any of both options first you will need to build the project files with [premake4](https://premake.github.io/download.html). Then add the premake4.exe file to any of the executable paths defined in `PATH` ( or add one ).

For *Visual Studio*:
`premake5.exe vs2019`
or
`premake4.exe vs2010`

For *mingw*:
`premake4.exe gmake`

Then you will need the prebuild binaries and development libraries of [*SDL2*](http://libsdl.org/download-2.0.php) and [*openal-soft*](http://kcat.strangesoft.net/openal.html#download). Download the ones needed ( *VS* or *mingw* ).

Then the project files should be found in `make/windows/`. A solution (sln) for *Visual Studio* or the corresponding `Makefiles` for *mingw*.

Having installed everything, you'll be able to build for *Visual Studio* as any other project.
And for *mingw* just make the project with: `mingw32-make` or any equivalent.

**_macOS_**

You have two options to build the project: with XCode or with gcc/clang manually.
To build with any of both options first you will also need to build the project files with [premake4](https://premake.github.io/download.html).
Then you will need the prebuild binaries and development libraries of [*SDL2*](http://libsdl.org/download-2.0.php), OpenAL is included with the OS. Install the *SDL2* framework and you should be able to build the project.

For a CLI build you can use the `projects/osx/make.sh` script, that generates the *Makefiles* and builds the project, also fix the dylibs generated.

For *XCode* :
`premake4 xcode4`

And open the XCode project generated in `make/osx/`


**_Android_**

There's a gradle project in `projects/android-project/`. It will build the library with all the dependencies included. Use the example project as a base for your project. Notice that there's a `eepp.mk` project file that builds the library. That can be used in you projects.

**_iOS_**

I've compiled the project for *iOS* many times in the past but there's not a recent build of it, so it might fail. But you can get the scripts to build it in `projects/ios/`. You'll need some inspiration to make this work, but i promise that i'll work on make this easier in the near future.

**_emscripten_**

There's a script for building the *emscripten* project in `projects/emscripten/make.sh`. That should be enough in *GNU/Linux* or *macOS* ( only tested this on *Linux* ).

**Author comment:**
------------------------
The library has been being developed for several years, it suffered many changes since its beginnings, i make any change that i find necessary to improve it. It's being used in several applications oriented to publicity campaigns mostly developed for Android devices and Windows PCs. I personally never had the time to use it to develop a real game with the library ( several frustrated projects ). The current project focus is on the UI module. And i'll continue to work mostly on this, the plan is to provide an alternative UI toolkit fully hardware accelerated similar to the Android toolkit but simpler and also oriented to desktop apps.

Audio and Network modules are almost exactly the same that the ones in SFML2 with some minor differences ( for example Network module supports SSL/TLS ). I like to use what's well done and fit my needs, but since i have my personal views on how to implement some things i prefer to take the code, to have full control over it. Also many ideas were/are taken from other projects. Some i can think about: *cocos2d-x*, *raylib*, *Android SDK*, *libgdx*, *Godot*, *XNA*, *LÖVE*, and many other projects.

If all this sounds interesting to you for some crazy reason, contact me and let me know if i can help you to get into the library, and may be, contribute to it in the future. This project needs *contributors* more than anything else.

The current state of the library/engine is decent. In terms of features should be in a similar position than the most used 2d game engines out there. But lacks of course of the support+community that you can get from *Godot* or *cocos2d-x* to mention a couple. The idea of this library is to focus on a better general approach to develop heavily UI based apps/games than the other options, with cleaner code and implementation.


**_Plans/ideas for the future:_**

Improve documentation ( Scene, UI, Physics and Maps modules are undocumented ).
Add more examples and also some tools.
There's planned some kind of style-sheet support similar to QSS or CSS ( this would be *great* but seems a lot of work needs to be done ).
Support 2D skeletal animations ( probably Spine2D, shouldn't be much work to implement ).
Scripting support ( first i would like to stabilize the library, but i'm getting there ).
Rebuild the map editor with undo/redo support ( or deprecate it in favor of TMX, but it's limited ).

**Acknowledgements**
-----------------------------
**_Special thanks to:_**

  *  Sean  Barrett  for  stb_image and all the [stb](https://github.com/nothings/stb) libraries.

  *  Sam  Latinga  for [Simple DirectMedia Layer](https://www.libsdl.org/).
  *  Jonathan  Dummer  for  the  [Simple  OpenGL  Image  Library](https://www.lonesock.net/soil.html).

  *  Laurent  Gomila  for [SFML](https://www.sfml-dev.org/)

  *  Lewis  Van  Winkle  for [PlusCallback](https://github.com/codeplea/pluscallback)

  *  Dieter  Baron  and  Thomas  Klausner  for [libbzip](https://libzip.org/)

  *  Jean-loup  Gailly  and  Mark  Adler  for [zlib](https://zlib.net/)

  *  Milan  Ikits  and  Marcelo  Magallon  for [GLEW](http://glew.sourceforge.net/)

  * Mikko Mononen for [nanosvg](https://github.com/memononen/nanosvg)

  * Scott Lembcke for [Chipmunk2D](https://github.com/slembcke/Chipmunk2D)

  * Christophe Riccio for [glm](https://github.com/g-truc/glm)

  * Rich Geldreich for [imageresampler](https://github.com/richgel999/imageresampler) and [jpeg-compressor](https://github.com/richgel999/jpeg-compressor)

  * Arseny Kapoulkine for [pugixml](https://github.com/zeux/pugixml)

  * Jason Perkins for [premake](https://premake.github.io/)

  * Martín Lucas Golini ( me ) and all the several contributors for [SOIL2](https://bitbucket.org/SpartanJ/soil2) and [efsw](https://bitbucket.org/SpartanJ/efsw)

  * The Xiph open source community for [libogg](https://xiph.org/ogg/) and [libvorbis](https://xiph.org/vorbis/)

  * The [ARMmbed](https://github.com/ARMmbed) community for [mbed TLS](https://tls.mbed.org/)

  * [kcat](https://github.com/kcat) for [openal-soft](http://kcat.strangesoft.net/openal.html)

  * The [FreeType Project](https://www.freetype.org/freetype2/docsindex.html)

  *  And  a  **lot**  more  people!


**Code License**
--------------------
[MIT License](http://www.opensource.org/licenses/mit-license.php)
