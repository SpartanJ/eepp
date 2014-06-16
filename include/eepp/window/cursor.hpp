#ifndef EE_WINDOWCCURSOR_HPP
#define EE_WINDOWCCURSOR_HPP

#include <eepp/core.hpp>

#include <eepp/graphics/cimage.hpp>
#include <eepp/graphics/ctexture.hpp>
using namespace EE::Graphics;

#include <eepp/window/windowhandle.hpp>

namespace EE { namespace Window {

class Window;

class EE_API Cursor {
	public:
		/** @return The cursor id */
		const Uint32& Id() const;

		/** @return The cursor name */
		const std::string& Name() const;

		/** @return The cursor hotspot, this means, the position inside the cursor image, where the click is taken */
		const Vector2i& HotSpot() const;

		/** @return The pointer to the image that represents the cursor */
		cImage * Image() const;
		
		virtual ~Cursor();
	protected:
		friend class CursorManager;

		Uint32			mId;
		std::string		mName;
		cImage *		mImage;
		Vector2i		mHotSpot;
		EE::Window::Window *		mWindow;

		Cursor( cTexture * tex, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		Cursor( cImage * img, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		Cursor( const std::string& path, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		virtual void Create() = 0;
};

}}

#endif
