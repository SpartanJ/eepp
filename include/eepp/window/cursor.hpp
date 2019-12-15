#ifndef EE_WINDOWCCURSOR_HPP
#define EE_WINDOWCCURSOR_HPP

#include <eepp/core.hpp>
#include <eepp/window/windowhandle.hpp>
#include <eepp/math/vector2.hpp>
using namespace EE::Math;

namespace EE { namespace Graphics {
class Texture;
class Image;
}}
using namespace EE::Graphics;


namespace EE { namespace Window {

class Window;

class EE_API Cursor {
	public:
		enum Type {
			Arrow = 0, /**< Arrow */
			IBeam,     /**< I-beam */
			Wait,      /**< Wait */
			Crosshair, /**< Crosshair */
			WaitArrow, /**< Small wait cursor (or Wait if not available) */
			SizeNWSE,  /**< Double arrow pointing northwest and southeast */
			SizeNESW,  /**< Double arrow pointing northeast and southwest */
			SizeWE,    /**< Double arrow pointing west and east */
			SizeNS,    /**< Double arrow pointing north and south */
			SizeAll,   /**< Four pointed arrow pointing north, south, east, and west */
			NoCursor,        /**< Slashed circle or crossbones */
			Hand,      /**< Hand */
			CursorCount
		};

		static Cursor::Type fromName( std::string name );

		/** @enum SysType list the system cursors that can be used */
		enum SysType {
			SysArrow = 0, /**< Arrow */
			SysIBeam,     /**< I-beam */
			SysWait,      /**< Wait */
			SysCrosshair, /**< Crosshair */
			SysWaitArrow, /**< Small wait cursor (or Wait if not available) */
			SysSizeNWSE,  /**< Double arrow pointing northwest and southeast */
			SysSizeNESW,  /**< Double arrow pointing northeast and southwest */
			SysSizeWE,    /**< Double arrow pointing west and east */
			SysSizeNS,    /**< Double arrow pointing north and south */
			SysSizeAll,   /**< Four pointed arrow pointing north, south, east, and west */
			SysNoCursor,        /**< Slashed circle or crossbones */
			SysHand,      /**< Hand */
			SysCursorCount,
			SysCursorNone
		};

		/** @return The cursor id */
		const Uint32& getId() const;

		/** @return The cursor name */
		const std::string& getName() const;

		/** @return The cursor hotspot, this means, the position inside the cursor image, where the click is taken */
		const Vector2i& getHotSpot() const;

		/** @return The pointer to the image that represents the cursor */
		Graphics::Image * getImage() const;

		virtual ~Cursor();
	protected:
		friend class CursorManager;

		Uint32			mId;
		std::string		mName;
		Graphics::Image *		mImage;
		Vector2i		mHotSpot;
		EE::Window::Window *		mWindow;

		Cursor( Texture * tex, const Vector2i& hotspot, const std::string& getName, EE::Window::Window * window );

		Cursor( Graphics::Image * img, const Vector2i& hotspot, const std::string& getName, EE::Window::Window * window );

		Cursor( const std::string& path, const Vector2i& hotspot, const std::string& getName, EE::Window::Window * window );

		virtual void create() = 0;
};

}}

#endif
