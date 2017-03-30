#ifndef EE_GRAPHICS_CLIPPINGMASK_HPP
#define EE_GRAPHICS_CLIPPINGMASK_HPP

#include <eepp/core.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/math/rect.hpp>
#include <list>

namespace EE { namespace Graphics {

class EE_API ClippingMask {
	public:
		enum Mode {
			Inclusive,
			Exclusive
		};

		/** Set the current Clipping area ( default the entire window, SCISSOR TEST ). */
		void clipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height );

		/** Disable the Clipping area */
		void clipDisable();

		/** Clip the area with a plane. */
		void clipPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height );

		/** Disable the clip plane area. */
		void clipPlaneDisable();

		std::size_t getMaskCount() const;

		const Drawable*& operator [](std::size_t index);

		const Drawable* const& operator [](std::size_t index) const;

		void clearMasks();

		void appendMask(const Drawable& drawable);

		void removeMask(const Drawable& drawable);

		Mode getMaskMode() const;

		void setMaskMode(Mode theMode);

		void stencilMaskEnable();

		void stencilMaskDisable( bool clearMasks = false );
	protected:
		std::list<Rectf> mScissorsClipped;
		bool mPushScissorClip;

		std::vector<const Drawable*> mDrawables;
		Mode mMode;

		void drawMask();
	private:
		friend class Renderer;

		ClippingMask();
};

}}

#endif
