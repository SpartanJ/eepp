#ifndef EE_GRAPHICS_DRAWABLERESOURCE_HPP 
#define EE_GRAPHICS_DRAWABLERESOURCE_HPP

#include <eepp/core.hpp>
#include <eepp/graphics/drawable.hpp>

namespace  EE { namespace Graphics {

class EE_API DrawableResource : public Drawable
{
	public:
		/** @return The DrawableResource Id. The Id is the String::hash of the SubTexture name. */
		const Uint32& getId() const;

		/** @return The DrawableResource Name. */
		const std::string getName() const;

		/** Sets the DrawableResource Name, it will also change the Id. */
		void setName( const std::string& name );

		/** @return The Destination Size of the DrawableResource. */
		virtual const Sizef& getDestSize() const = 0;

		/** Sets the Destination Size of the DrawableResource.
		*	The size can be different from the original size of the DrawableResource.
		*	For example if the DrawableResource width is 32 pixels, by default the destination width is 32 pixels, but it can be changed to anything want. */
		virtual void setDestSize( const Sizef& destSize ) = 0;

		/** @return The DrawableResource default offset. The offset is added to the position passed when is drawed. */
		virtual const Vector2i& getOffset() const = 0;

		/** Set the SubTexture offset. */
		virtual void setOffset( const Vector2i& offset ) = 0;
	protected:
		std::string mName;
		Uint32		mId;

		DrawableResource( EE_DRAWABLE_TYPE drawableType );

		DrawableResource( EE_DRAWABLE_TYPE drawableType, const std::string& name );

		void createUnnamed();
};

}}

#endif
