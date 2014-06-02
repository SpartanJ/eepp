Entropia Engine++
=================

**Entropia Engine++** is a C++ 2D game engine designed for an easy cross-platform game development.

**eepp provides:**
------------------

**Cross platform functionality:**
---------------------------------

  * Official support for Linux, Windows, Mac OS X, iOS and Android.

  * Exports to HTML5 using emscripten with some limitations.

  * It should work on FreeBSD, Solaris and Haiku.


**Graphic Module:**
-------------------

  * OpenGL 2 ( fixed-pipeline ), OpenGL 3 ( programmable-pipeline ), OpenGL ES 2, OpenGL ES 1, and OpenGL Core Profile support.

  * Batch Renderer ( all the rendering is automatically batched by the engine ).

  * TTF and Texture fonts support.

  * Frame Buffer support.

  * Shaders support ( with automatic fixed pipeline shaders to programmable conversor ).

  * Vertex Buffer Object support.

  * Particle System.

  * Extendable Console.

  * Animated Sprites.

  * Texture Atlas support ( automatic creation and update of the texture atlas ).


**Window Module:**
------------------

  * Backend based module, this means that you can easily create a backend for the window/input handling.

  * Currently supports SDL 1.2, SDL 2 and SFML as backends.

  * Clipboard support ( SDL 2 backend and partial support with SDL 1.2 ).

  * Color hardware cursors ( SDL 2 fully supported, partial support with SDL 1.2 and SFML ).
  
  * Multiple windows

  * Joystick support.


**Audio Module:**
-----------------

  * OpenAL audio engine with extendable file format support. OGG support, and all the formats supported by libsndfile.


**Physics Module:**
-------------------

  * Full OOP chipmunk physics wrapper.
 

**System Module:**
------------------

  * Provides all the basics stuffs for the full multi-threading support of the engine, file formats support for packing, clocks, resource manager, and much more.


*Base Module:*
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


**Network Module**
------------------
  * Web Requests with HTTP client, with **SSL support**.
  
  * File Transfers with FTP client.
  
  * TCP and UDP sockets.


**UI Module:**
--------------

  * Base controls to manage the game objects as nodes, with all basic input interaction events ( clicks, keypress, mouser over, focus, etc ).

  * Fully featured UI system, with skinning support, animation support, scaling, rotating, clipping, events, messages, etc.
  
  * All the basic controls are implemented ( button, textbox, combobox, inputbox, menu, listbox, scrollbars, etc ).
  
  * Features as text selection and key bindings.


**Gaming Module:**
------------------

  * Tiled Maps with software dynamic lights.

  * Full featured map editor.

**General Features:**
---------------------

Support for multi-threaded resource loading ( textures, sounds, fonts, etc ).

**Code License**
--------------
[MIT License](http://www.opensource.org/licenses/mit-license.php)
