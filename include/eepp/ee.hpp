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
		* Laurent Gomila for the SFML library.
		* OGRE staff for the Timer implementation.
		* Lewis Van Winkle for PlusCallback.
		* Dieter Baron and Thomas Klausner for libzip.
		* Jean-loup Gailly and Mark Adler for zlib.
		* Milan Ikits and Marcelo Magallon for GLEW.
		* And a lot more people!
*/

	// General includes and declarations
	#include <eepp/core.hpp>
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
	using namespace EE::Network::SSL;

	// Scnee
	#include <eepp/scene.hpp>
	using namespace EE::Scene;

	// UI
	#include <eepp/ui.hpp>
	using namespace EE::UI;
	using namespace EE::UI::Tools;

	// Maps
	#include <eepp/maps.hpp>
	using namespace EE::Maps;

	// Physics
	#include <eepp/physics.hpp>
	using namespace EE::Physics;
#endif
