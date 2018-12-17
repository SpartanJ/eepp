#ifndef EE_GRAPHICS_STATELISTDRAWABLE_HPP
#define EE_GRAPHICS_STATELISTDRAWABLE_HPP

#include <eepp/graphics/statefuldrawable.hpp>
#include <map>

namespace EE { namespace Graphics {

class EE_API StateListDrawable : StatefulDrawable {
	public:
		StateListDrawable();

		virtual ~StateListDrawable();

		virtual Sizef getSize();

		virtual void draw();

		virtual void draw( const Vector2f& position );

		virtual void draw( const Vector2f& position, const Sizef& size );

		virtual bool isStateful();

		virtual StatefulDrawable * setState( int state );

		virtual const int& getState() const;

		virtual StateListDrawable * setStateDrawable( int state, Drawable * drawable );

		bool hasDrawableState( int state );

		void setIsDrawableOwner( const bool& isOwner );

		const bool& isDrawableOwner() const;

		void clearDrawables();
	protected:
		bool mDrawableOwner;
		int mCurrentState;
		Drawable * mCurrentDrawable;
		std::map<int,Drawable*> mDrawables;
};

}}

#endif
