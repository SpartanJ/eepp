#ifndef EE_HPP
#define EE_HPP
/**
	@mainpage Entropia Engine++

	Developed by: Martín Lucas Golini

	2D Game Engine designed for an easy cross-platform game development.
	The project aims to provide a simple and powerfull framework that takes advantage of C++, OpenGL, OpenAL, SDL and more.

	Thanks to: \n
		* Sean Barrett for the stb_vorbis and stb_image libraries. \n
		* Sam Latinga for Simple DirectMedia Layer library. \n
		* Jonathan Dummer for the Simple OpenGL Image Library. \n
		* Laurent Gomila for the SFML library ( eepp audio module is heavily based on the SFML audio module ) \n
		* OGRE staff for the Timer implementation \n
		* Lewis Van Winkle for PlusCallback \n
		* Dieter Baron and Thomas Klausner for libzip \n
		* Jean-loup Gailly and Mark Adler for zlib \n
		* Milan Ikits and Marcelo Magallon for GLEW \n
		* And a lot more people!
**/

/** PLANS:

	Short-term plans:

	@TODO Stabilize the API
				STATE:	It's looking good.

	@TODO Improve documentation.
				STATE:	EE ( Base ) commented what is needed.
						EE::Window mostly commented, need to add some examples for the input triggers.
						EE::System 60 % commented, easy to understand.
						EE::Graphics 50 % commented.
						EE::Audio 50 % commented.
						EE::Math 30 to 40 % commented.
						EE::UI Not commented at all.
						EE::Physics Not commented at all, chipmunk documentation should help.
						EE::Gaming Not commented at all.

	@TODO Improve Premake4 support. It should be really easy to compile eepp in Windows and OS X ( Linux is always easy thanks to package managers ).
				STATE:	Seems to be done, need more testing.

	@TODO Add more commented examples, showing at least the basic usage of the engine ( 10 or more examples at least ).
				STATE:	2 examples available. ( 20 % )

	@TODO Improve the map editor ( add triggers, tiles selection to copy paste in other zones of the map, undo/redo actions ).
				STATE:	Features are implemented.
						Copy-Paste will be moved for a middle to long plan,
						Tile selection i'm not decided yet.

	@TODO Add PVRTC and ETC support.
				STATE:	Not started.

	@TODO Add a cLock class in EE::System.

	Middle-term plans:

	@TODO Create a default UI Theme for the engine ( get rid off the ugly Aqua Theme ).
				STATE:	Not started, i need an... artist.

	@TODO Add Networking support.
				STATE:	I'll use SFML network adapted to the engine coding style.

	Long-term plans:

	@TODO Add audio recording from the SFML implementation, adapted to the engine coding style.

	@TODO Add Scripting support ( squirrel or angel script or lua ), at least some parts of the engine. Binding everything seems too much work.
				STATE:	I've binded some classes with lua+luabind and is awesome, but... luabind is bloated ( and for my needs it's the only good option ), and adds A LOT of compile time.
						squirrel+sqrat almost does everything i need, but it have some bugs that seems to be unfixable ( none enum parameters, char parameters segfaults, no constructor overloading ).

	@TODO Support UI Theming from scripts or XML.

	@TODO Add some kind of support for TMX map files ( Tiles Map Editor ).

*/

	// General includes and declarations
	#include <eepp/base.hpp>
	#include <eepp/version.hpp>
	using namespace EE;

	// Math
	#include <eepp/math.hpp>
	using namespace EE::Math;

	// System
	#include <eepp/system.hpp>
	using namespace EE::System;

	// Audio
	#include <eepp/audio.hpp>
	using namespace EE::Audio;

	// Window
	#include <eepp/window.hpp>
	using namespace EE::Window;

	// Graphics
	#include <eepp/graphics.hpp>
	using namespace EE::Graphics;

	// UI
	#include <eepp/ui.hpp>
	using namespace EE::UI;
	using namespace EE::UI::Tools;

	// Gaming
	#include <eepp/gaming.hpp>
	using namespace EE::Gaming;
	using namespace EE::Gaming::MapEditor;

	// Physics
	#include <eepp/physics.hpp>
	using namespace EE::Physics;
#endif
