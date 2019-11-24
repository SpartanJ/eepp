#ifndef EE_UI_UINODEDRAWABLE_HPP
#define EE_UI_UINODEDRAWABLE_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/scene/action.hpp>
#include <eepp/math/ease.hpp>
#include <vector>
#include <map>

using namespace EE::Graphics;
using namespace EE::Scene;

namespace EE { namespace UI {

class UINode;

class EE_API UINodeDrawable : public Drawable {
	public:
		enum Repeat {
			RepeatXY,
			RepeatX,
			RepeatY,
			NoRepeat
		};

		static Repeat repeatFromText( const std::string& text );

		class EE_API MoveAction : public Action {
			public:
				static MoveAction * New( const std::string& posEqStart, const std::string& posEqEnd, const Time& duration, const Ease::Interpolation& type = Ease::Linear );

				void start() override;

				void stop() override;

				void update( const Time& time ) override;

				bool isDone() override;

				Action * clone() const override;

				Action * reverse() const override;

				const Vector2f& getOffset() const { return mOffset; }
			protected:
				MoveAction( const std::string& posEqStart, const std::string& posEqEnd, const Time& duration, const Ease::Interpolation& type = Ease::Linear );

				void onStart() override;

				void onUpdate( const Time& time ) override;

				std::string mStart;
				std::string mEnd;
				Time mDuration;
				Ease::Interpolation mType;
				Time mElapsed;
				Vector2f mOffset;
		};

		class EE_API LayerDrawable : public Drawable {
			public:
				static LayerDrawable * New( UINodeDrawable * container );

				LayerDrawable( UINodeDrawable * container );

				virtual ~LayerDrawable();

				bool isStateful() { return false; }

				virtual void draw();

				virtual void draw( const Vector2f& position );

				virtual void draw( const Vector2f& position, const Sizef& size );

				virtual Sizef getSize();

				virtual void setSize( const Sizef& size );

				Drawable* getDrawable() const;

				void setDrawable( Drawable* drawable, const bool& ownIt );

				void setOffset(const Vector2f & offset);

				const Vector2f& getOffset() const;

				std::string getOffsetEq();

				void setPositionEq( const std::string& offset );

				const std::string& getPositionEq() const { return mPositionEq; }

				void setSizeEq( const std::string& size );

				const std::string& getSizeEq() const { return mDrawableSizeEq; }

				const Repeat& getRepeat() const;

				void setRepeat( const Repeat& repeat );

				void invalidate();

				const Sizef& getDrawableSize() const;

				void setDrawableSize( const Sizef& drawableSize );

				Sizef calcDrawableSize( const std::string& drawableSizeEq );

				Vector2f calcPosition( const std::string& positionEq );

				MoveAction * getMoveAction() const;

				void setMoveAction(MoveAction * moveAction);
			protected:
				UINodeDrawable * mContainer;
				Sizef mSize;
				Sizef mDrawableSize;
				Vector2f mOffset;
				std::string mPositionEq;
				std::string mDrawableSizeEq;
				bool mNeedsUpdate;
				bool mOwnsDrawable;
				Drawable * mDrawable;
				Uint32 mResourceChangeCbId;
				Repeat mRepeat;
				MoveAction * mMoveAction;
				Uint32 mMoveActionCbId;

				virtual void onPositionChange();

				void update();
		};

		static UINodeDrawable * New( UINode * owner );

		UINodeDrawable( UINode * owner );

		virtual ~UINodeDrawable();

		virtual Sizef getSize();

		virtual void setSize( const Sizef& size );

		virtual void draw();

		virtual void draw( const Vector2f& position );

		virtual void draw( const Vector2f& position, const Sizef& size );

		void draw( const Vector2f& position, const Sizef& size, const Uint32& alpha );

		virtual bool isStateful() { return false; }

		void clearDrawables();

		void setBorderRadius( const Uint32& corners );

		Uint32 getBorderRadius() const;

		bool layerExists( int index );

		LayerDrawable * getLayer( int index );

		void setDrawable( int index, Drawable * drawable, bool ownIt );

		void setDrawablePosition( int index, const std::string& positionEq );

		void setDrawableRepeat( int index, const std::string& repeatRule );

		void setDrawableSize( int index, const std::string& sizeEq );

		void setBackgroundColor( const Color& color );

		Color getBackgroundColor() const;

		bool getClipEnabled() const;

		void setClipEnabled(bool clipEnabled);

		void invalidate();

		UINode * getOwner() { return mOwner; }
	protected:
		UINode * mOwner;
		RectangleDrawable mBackgroundColor;
		std::map<int,LayerDrawable*> mGroup;
		Sizef mSize;
		bool mNeedsUpdate;
		bool mClipEnabled;

		virtual void onPositionChange();

		virtual void onSizeChange();

		void update();
};

}}

#endif
