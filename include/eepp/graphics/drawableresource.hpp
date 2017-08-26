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
	protected:
		std::string mName;
		Uint32		mId;

		DrawableResource( EE_DRAWABLE_TYPE drawableType );

		DrawableResource( EE_DRAWABLE_TYPE drawableType, const std::string& name );

		void createUnnamed();
};

}}

#endif
