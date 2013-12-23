#ifndef EE_HPP
#define EE_HPP
/**
	@mainpage Entropia Engine++

	Developed by: Martín Lucas Golini

	2D Game Engine designed for an easy cross-platform game development.
	The project aims to provide a simple and powerfull framework that takes advantage of C++, OpenGL, OpenAL, SDL and more.

	Thanks to:
		* Sean Barrett for the stb_vorbis and stb_image libraries.
		* Sam Latinga for Simple DirectMedia Layer library.
		* Jonathan Dummer for the Simple OpenGL Image Library.
		* Laurent Gomila for the SFML library ( eepp audio and network modules are heavily based on the SFML audio module )
		* OGRE staff for the Timer implementation
		* Lewis Van Winkle for PlusCallback
		* Dieter Baron and Thomas Klausner for libzip
		* Jean-loup Gailly and Mark Adler for zlib
		* Milan Ikits and Marcelo Magallon for GLEW
		* And a lot more people!

	ROADMAP:

	Short-term plans:

	@todo Improve documentation.
				STATE:	EE ( Base ) documented what is needed.
						EE::Window documented.
						EE::System documented.
						EE::Graphics documented.
						EE::Audio documented.
						EE::Math documented.
						EE::Network documented.
						EE::UI Not documented at all.
						EE::Physics Not documented at all, chipmunk documentation should help.
						EE::Gaming Not documented at all.

	@todo Add more commented examples, showing at least the basic usage of the engine ( 10 or more examples at least ).
				STATE:	7 examples available.

	@todo Improve the map editor ( add triggers, tiles selection to copy paste in other zones of the map, undo/redo actions ).
				STATE:	Needs at least to reoffset tiles and objects for the map resizing.
						Tile selection i'm not decided yet.
						Copy-Paste will be moved for a middle to long term plan.

	Middle-term plans:

	@todo Add audio recording from the SFML implementation, adapted to the engine coding style.

	@todo Add Scripting support ( squirrel or angel script or lua ), at least some parts of the engine. Binding everything seems too much work.
				STATE:	I've binded some classes with lua+luabind and is awesome, but... luabind is bloated ( and for my needs it's the only good option ), and adds A LOT of compile time.
						squirrel+sqrat almost does everything i need, but it have some bugs that seems to be unfixable ( none enum parameters, char parameters segfaults, no constructor overloading ).

	@todo Support UI Theming from scripts or XML.

	@todo Add some kind of support for TMX map files ( Tiles Map Editor ).

	@todo Pathfinding and AI helpers ( A*, FSM ).
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

	// Network
	#include <eepp/network.hpp>
	using namespace EE::Network;

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
