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

/**
	@TODO Update OpenAL Soft version for Android.
	@TODO Improve documentation.
	@TODO Improve Premake4 support. It should be really easy to compile eepp in Windows and OS X ( Linux is always easy thanks to package managers ).
	@TODO Add more examples, showing at least the basic usage of the engine ( 10 or more examples at least ).
	@TODO Create a default UI Theme for the engine ( get rid off the ugly Aqua Theme ).
	@TODO And may be, add some kind of support for TMX map files ( Tiles Map Editor ).
	@TODO Improve the map editor ( add triggers, tiles selection to copy paste in other zones of the map, undo/redo actions ).
	@TODO Add PVRTC and ETC support.
	@TODO Add Networking support ( may be just use SFML Network, i don't want to reinvent the wheel ).
	@TODO Support UI Theming from scripts or XML.
	@TODO Add Scripting support ( squirrel or angel script or lua ), at least some parts of the engine. Binding everthing seems too much work.
	@TODO Check for endianness problems, and make EEPP endianness agnostic ( binary maps and texture fonts ). Not a priority, all the eepp target os use little endian.
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
