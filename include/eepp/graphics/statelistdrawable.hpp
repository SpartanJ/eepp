#ifndef EE_GRAPHICS_STATELISTDRAWABLE_HPP
#define EE_GRAPHICS_STATELISTDRAWABLE_HPP

#include <eepp/graphics/statefuldrawable.hpp>
#include <map>

namespace EE { namespace Graphics {

class EE_API StateListDrawable : public StatefulDrawable {
	public:
		static StateListDrawable * New();

		StateListDrawable();

		virtual ~StateListDrawable();

		virtual Sizef getSize();

		virtual void draw();

		virtual void draw( const Vector2f& position );

		virtual void draw( const Vector2f& position, const Sizef& size );

		virtual bool isStateful();

		virtual StatefulDrawable * setState( Uint32 state );

		virtual const Uint32& getState() const;

		virtual StateListDrawable * setStateDrawable( Uint32 state, Drawable * drawable );

		bool hasDrawableState( Uint32 state );

		void setIsDrawableOwner( const bool& isOwner );

		const bool& isDrawableOwner() const;

		void clearDrawables();
	protected:
		bool mDrawableOwner;
		Uint32 mCurrentState;
		Drawable * mCurrentDrawable;
		std::map<Uint32,Drawable*> mDrawables;
};

}}

#endif
