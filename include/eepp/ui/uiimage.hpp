#ifndef EE_UI_UIIMAGE_HPP
#define EE_UI_UIIMAGE_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIImage : public UIWidget {
	public:
		static UIImage * New();

		UIImage();

		virtual ~UIImage();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void draw();

		virtual void setAlpha( const Float& alpha );

		Drawable * getDrawable() const;

		UIImage * setDrawable( Drawable * drawable );

		const Color& getColor() const;

		UIImage * setColor( const Color& col );

		const Vector2f& getAlignOffset() const;

		virtual bool setAttribute( const NodeAttribute& attribute, const Uint32& state = UIState::StateFlagNormal );

		Uint32 getScaleType() const;

		UIImage * setScaleType(const Uint32 & scaleType);
	protected:
		Uint32			mScaleType;
		Drawable *		mDrawable;
		Color			mColor;
		Vector2f		mAlignOffset;
		Vector2f		mDestSize;
		bool			mDrawableOwner;

		virtual void onSizeChange();

		virtual void onAlignChange();

		void onAutoSize();

		void calcDestSize();

		void autoAlign();

		void safeDeleteDrawable();
};

}}


#endif
