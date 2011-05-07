#ifndef EE_H
#define EE_H
/**
	@mainpage Entropia Engine++

	Developed by: Martín Lucas Golini

	2D Game Engine designed for an easy cross-platform game development.
	The project aims to provide a simple and powerfull framework that takes advantage of C++, OpenGL, OpenAL, SDL and more.

	Thanks to: \n
		* Sean Barrett for the stb_vorbis and stb_image libraries. \n
		* Sam Latinga for Simple DirectMedia Layer library. \n
		* Jonathan Dummer for the Simple OpenGL Image Library. \n
		* Laurent Gomila for the SFML library ( eepp audio module is based on the SFML audio module ) \n
		* OGRE staff for the Timer implementation \n
		* Lewis Van Winkle for PlusCallback \n
		* Dieter Baron and Thomas Klausner for libzip \n
		* Jean-loup Gailly and Mark Adler for zlib \n
		* Milan Ikits and Marcelo Magallon for GLEW \n
		* And a lot more people!
**/

/**
	@TODO Check for endianness problems, and make EEPP endianness agnostic.
	@TODO Add backend for SDL 1.3 ( support for Android ). And may be SFML backend ( may be implement as the default build-in backend with sfml-window static linked ).
	@TODO Support for Android and iOS.
	@TODO Support color cursors \n
				SDL 1.2					Not even posible ( win32 and x11 backends for this ready ) \n
				Allegro 5				DONE
	@TODO Add Scripting support ( squirrel or python ).
	@TODO Fix classes bad padding, optimize memory consumption.
*/

	// General includes and declarations
	#include "base.hpp"
	using namespace EE;

	// Utils
	#include "utils/vector2.hpp"
	#include "utils/vector3.hpp"
	#include "utils/size.hpp"
	#include "utils/line2.hpp"
	#include "utils/triangle2.hpp"
	#include "utils/quad2.hpp"
	#include "utils/colors.hpp"
	#include "utils/polygon2.hpp"
	#include "utils/rect.hpp"
	#include "utils/cwaypoints.hpp"
	#include "utils/cinterpolation.hpp"
	#include "utils/cperlinnoise.hpp"
	#include "utils/string.hpp"
	#include "utils/utils.hpp"
	#include "utils/easing.hpp"
	using namespace EE::Utils;
	using namespace EE::Utils::easing;

	// Math
	#include "math/math.hpp"
	#include "math/cmtrand.hpp"
	using namespace EE::Math;

	// System
	#include "system/tsingleton.hpp"
	#include "system/cthread.hpp"
	#include "system/cmutex.hpp"
	#include "system/clog.hpp"
	#include "system/ctimer.hpp"
	#include "system/ctimeelapsed.hpp"
	#include "system/cinifile.hpp"
	#include "system/cpack.hpp"
	#include "system/cpak.hpp"
	#include "system/czip.hpp"
	#include "system/crc4.hpp"
	#include "system/cobjectloader.hpp"
	#include "system/cresourceloader.hpp"
	#include "system/tresourcemanager.hpp"
	using namespace EE::System;

	// Audio
	#include "audio/openal.hpp"
	#include "audio/caudiodevice.hpp"
	#include "audio/caudiolistener.hpp"
	#include "audio/caudioresource.hpp"
	#include "audio/csoundfile.hpp"
	#include "audio/csoundfiledefault.hpp"
	#include "audio/csoundfileogg.hpp"
	#include "audio/csound.hpp"
	#include "audio/csoundbuffer.hpp"
	#include "audio/csoundstream.hpp"
	#include "audio/cmusic.hpp"
	#include "audio/tsoundloader.hpp"
	#include "audio/tsoundmanager.hpp"
	using namespace EE::Audio;

	// Window
	#include "window/cinput.hpp"
	#include "window/cinputtextbuffer.hpp"
	#include "window/cview.hpp"
	#include "window/cwindow.hpp"
	#include "window/cclipboard.hpp"
	#include "window/ccursor.hpp"
	#include "window/ccursormanager.hpp"
	#include "window/cjoystick.hpp"
	#include "window/cjoystickmanager.hpp"
	#include "window/cengine.hpp"
	using namespace EE::Window;

	// Graphics
	#include "graphics/renderer/cgl.hpp"
	#include "graphics/renderer/crenderergl.hpp"
	#include "graphics/renderer/crenderergl3.hpp"
	#include "graphics/renders.hpp"
	#include "graphics/cimage.hpp"
	#include "graphics/ctexture.hpp"
	#include "graphics/ctextureloader.hpp"
	#include "graphics/ctexturefactory.hpp"
	#include "graphics/ctexturepacker.hpp"
	#include "graphics/cshape.hpp"
	#include "graphics/cshapegroup.hpp"
	#include "graphics/cglobalshapegroup.hpp"
	#include "graphics/cshapegroupmanager.hpp"
	#include "graphics/csprite.hpp"
	#include "graphics/cparticle.hpp"
	#include "graphics/cparticlesystem.hpp"
	#include "graphics/cfont.hpp"
	#include "graphics/ctexturefont.hpp"
	#include "graphics/cttffont.hpp"
	#include "graphics/ctexturefontloader.hpp"
	#include "graphics/cttffontloader.hpp"
	#include "graphics/cfontmanager.hpp"
	#include "graphics/cprimitives.hpp"
	#include "graphics/cscrollparallax.hpp"
	#include "graphics/cconsole.hpp"
	#include "graphics/cbatchrenderer.hpp"
	#include "graphics/cglobalbatchrenderer.hpp"
	#include "graphics/ctextcache.hpp"
	#include "graphics/pixelperfect.hpp"
	#include "graphics/cshader.hpp"
	#include "graphics/cshaderprogram.hpp"
	#include "graphics/cshaderprogrammanager.hpp"
	#include "graphics/ctexturegrouploader.hpp"
	#include "graphics/cframebuffer.hpp"
	#include "graphics/cframebufferfbo.hpp"
	#include "graphics/cframebufferpbuffer.hpp"
	#include "graphics/cvertexbuffer.hpp"
	#include "graphics/cvertexbufferogl.hpp"
	#include "graphics/cvertexbuffervbo.hpp"
	using namespace EE::Graphics;

	// Gaming
	#include "gaming/clight.hpp"
	#include "gaming/cisomap.hpp"
	using namespace EE::Gaming;

	// UI
	#include "ui/cuibackground.hpp"
	#include "ui/cuiborder.hpp"
	#include "ui/cuievent.hpp"
	#include "ui/cuieventkey.hpp"
	#include "ui/cuieventmouse.hpp"
	#include "ui/cuimessage.hpp"
	#include "ui/cuimanager.hpp"
	#include "ui/cuiskin.hpp"
	#include "ui/cuiskinsimple.hpp"
	#include "ui/cuiskincomplex.hpp"
	#include "ui/cuitheme.hpp"
	#include "ui/cuithememanager.hpp"
	#include "ui/cuicontrol.hpp"
	#include "ui/cuidragable.hpp"
	#include "ui/cuicontrolanim.hpp"
	#include "ui/cuigfx.hpp"
	#include "ui/cuitextbox.hpp"
	#include "ui/cuitextinput.hpp"
	#include "ui/cuipushbutton.hpp"
	#include "ui/cuicheckbox.hpp"
	#include "ui/cuiradiobutton.hpp"
	#include "ui/cuislider.hpp"
	#include "ui/cuispinbox.hpp"
	#include "ui/cuiscrollbar.hpp"
	#include "ui/cuiprogressbar.hpp"
	#include "ui/cuilistbox.hpp"
	#include "ui/cuilistboxitem.hpp"
	#include "ui/cuidropdownlist.hpp"
	#include "ui/cuicombobox.hpp"
	#include "ui/cuimenu.hpp"
	#include "ui/cuimenuitem.hpp"
	#include "ui/cuiseparator.hpp"
	#include "ui/cuipopupmenu.hpp"
	#include "ui/cuisprite.hpp"
	#include "ui/cuitextedit.hpp"
	#include "ui/cuigridcell.hpp"
	#include "ui/cuigenericgrid.hpp"
	#include "ui/cuiwindow.hpp"
	#include "ui/cuiselectbutton.hpp"
	#include "ui/cuiwinmenu.hpp"
	#include "ui/cuicommondialog.hpp"
	using namespace EE::UI;

	#include "physics/cphysicsmanager.hpp"
	#include "physics/cshape.hpp"
	#include "physics/cshapecircle.hpp"
	#include "physics/cshapesegment.hpp"
	#include "physics/cshapepoly.hpp"
	#include "physics/cspace.hpp"
	#include "physics/cbody.hpp"
	#include "physics/constraints/cconstraint.hpp"
	#include "physics/constraints/cdampedrotaryspring.hpp"
	#include "physics/constraints/cdampedspring.hpp"
	#include "physics/constraints/cgearjoint.hpp"
	#include "physics/constraints/cgroovejoint.hpp"
	#include "physics/constraints/cpinjoint.hpp"
	#include "physics/constraints/cpivotjoint.hpp"
	#include "physics/constraints/cratchetjoint.hpp"
	#include "physics/constraints/crotarylimitjoint.hpp"
	#include "physics/constraints/csimplemotor.hpp"
	#include "physics/constraints/cslidejoint.hpp"
	#include "physics/moment.hpp"
	#include "physics/area.hpp"
	#include "physics/cshapepolysprite.hpp"
	#include "physics/cshapecirclesprite.hpp"
	using namespace EE::Physics;
#endif
