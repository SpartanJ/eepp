#ifndef EE_UIUINODE_HPP
#define EE_UIUINODE_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uibackground.hpp>
#include <eepp/ui/uiborder.hpp>
#include <eepp/ui/uiskin.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene {
class Action;
class ActionManager;
}}
using namespace EE::Scene;

namespace EE { namespace UI {

class UITheme;

class EE_API UINode : public Node {
	public:
		static UINode * New();

		typedef cb::Callback1<void, const Event*> EventCallback;

		UINode();

		virtual ~UINode();

		void worldToNodeTranslation( Vector2f& position ) const;

		void nodeToWorldTranslation( Vector2f& position ) const;

		void worldToNode( Vector2i& pos );

		void nodeToWorld( Vector2i& pos );

		void worldToNode( Vector2f& pos );

		void nodeToWorld( Vector2f& pos );

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setPosition( const Vector2f& Pos );

		virtual Node * setPosition( const Float& x, const Float& y );

		void setPixelsPosition(const Vector2f & position );

		void setPixelsPosition( const Float& x, const Float& y );

		const Vector2f& getPosition() const;

		const Vector2f& getRealPosition() const;

		virtual Node * setSize( const Sizef& size );

		virtual Node * setSize( const Float& Width, const Float& Height );

		UINode * setPixelsSize( const Sizef& size );

		UINode * setPixelsSize( const Float& x, const Float& y );

		const Sizef& getSize();

		virtual const Sizef& getRealSize();

		Rect getRect() const;

		virtual void draw();

		virtual void update( const Time& time );

		Uint32 getHorizontalAlign() const;

		UINode * setHorizontalAlign( Uint32 halign );

		Uint32 getVerticalAlign() const;

		UINode * setVerticalAlign( Uint32 valign );

		UINode * setGravity( Uint32 hvalign );

		UIBackground * setBackgroundFillEnabled( bool enabled );

		UIBorder * setBorderEnabled( bool enabled );

		const Uint32& getFlags() const;

		virtual UINode * setFlags( const Uint32& flags );

		virtual UINode * unsetFlags( const Uint32& flags );

		virtual UINode * resetFlags( Uint32 newFlags = 0 );

		UIBackground * getBackground();

		UIBorder * getBorder();

		void setThemeByName( const std::string& Theme );

		virtual void setTheme( UITheme * Theme );

		virtual UINode * setThemeSkin( UITheme * Theme, const std::string& skinName );

		virtual UINode * setThemeSkin( const std::string& skinName );

		void setThemeToChilds( UITheme * Theme );

		UISkin * getSkin();

		virtual UINode * setSkin( const UISkin& Skin );

		UINode * setSkin( UISkin * skin );

		void removeSkin();

		void setSkinState( const Uint32& State );

		Sizef getSkinSize();

		void applyDefaultTheme();

		Node * getWindowContainer();

		bool isDragging() const;

		void setDragging( const bool& dragging );

		const Vector2f& getDragPoint() const;

		void setDragPoint( const Vector2f& Point );

		bool isDragEnabled() const;

		void setDragEnabled( const bool& enable );

		void setDragButton( const Uint32& Button );

		const Uint32& getDragButton() const;

		bool isAnimating();

		Interpolation1d * startAlphaAnim( const Float& From, const Float& To, const Time& TotalTime, const bool& alphaChilds = true, const Ease::Interpolation& type = Ease::Linear, Interpolation1d::OnPathEndCallback PathEndCallback = Interpolation1d::OnPathEndCallback() );

		Interpolation2d * startScaleAnim( const Vector2f& From, const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation2d::OnPathEndCallback PathEndCallback = Interpolation2d::OnPathEndCallback() );

		Interpolation2d * startScaleAnim( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation2d::OnPathEndCallback PathEndCallback = Interpolation2d::OnPathEndCallback() );

		Interpolation2d * startTranslation( const Vector2f& From, const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation2d::OnPathEndCallback PathEndCallback = Interpolation2d::OnPathEndCallback() );

		Interpolation1d * startRotation( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation1d::OnPathEndCallback PathEndCallback = Interpolation1d::OnPathEndCallback() );

		Interpolation1d * startAlphaAnim( const Float& To, const Time& TotalTime, const bool& alphaChilds = true, const Ease::Interpolation& type = Ease::Linear, Interpolation1d::OnPathEndCallback PathEndCallback = Interpolation1d::OnPathEndCallback() );

		Interpolation2d * startScaleAnim( const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation2d::OnPathEndCallback PathEndCallback = Interpolation2d::OnPathEndCallback() );

		Interpolation2d * startScaleAnim( const Float& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation2d::OnPathEndCallback PathEndCallback = Interpolation2d::OnPathEndCallback() );

		Interpolation2d * startTranslation( const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation2d::OnPathEndCallback PathEndCallback = Interpolation2d::OnPathEndCallback() );

		Interpolation1d * startRotation( const Float& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation1d::OnPathEndCallback PathEndCallback = Interpolation1d::OnPathEndCallback() );

		Interpolation1d * createFadeIn( const Time& Time, const bool& alphaChilds = true, const Ease::Interpolation& type = Ease::Linear );

		Interpolation1d * createFadeOut( const Time& Time, const bool& alphaChilds = true, const Ease::Interpolation& type = Ease::Linear );

		Interpolation1d * closeFadeOut( const Time& Time, const bool& alphaChilds = true, const Ease::Interpolation& type = Ease::Linear );

		Interpolation1d * disableFadeOut( const Time & Time, const bool& alphaChilds = true, const Ease::Interpolation& type = Ease::Linear );

		bool isFadingOut();

		virtual void setFocus();
	protected:
		Vector2f		mDpPos;
		Sizef			mDpSize;
		Uint32			mFlags;
		UISkinState *	mSkinState;
		UIBackground *	mBackground;
		UIBorder *		mBorder;
		Vector2f		mDragPoint;
		Uint32			mDragButton;

		virtual Uint32 onMouseDown( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseUp( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onValueChange();

		virtual void onStateChange();

		virtual void onAlignChange();

		virtual void drawSkin();

		virtual void drawBackground();

		virtual void drawBorder();

		virtual void onThemeLoaded();

		virtual void onChildCountChange();

		virtual Uint32 onDrag( const Vector2f& position );

		virtual Uint32 onDragStart( const Vector2i& position );

		virtual Uint32 onDragStop( const Vector2i& position );

		virtual Uint32 onMouseEnter( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseExit( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onFocus();

		void checkClose();

		virtual void internalDraw();

		virtual void onWidgetFocusLoss();

		void setPrevSkinState();

		void writeFlag( const Uint32& Flag, const Uint32& Val );

		Rectf makePadding( bool PadLeft = true, bool PadRight = true, bool PadTop = true, bool PadBottom = true, bool SkipFlags = false );

		Sizef getSkinSize( UISkin * Skin, const Uint32& State = UISkinState::StateNormal );

		void drawHighlightFocus();

		void drawOverNode();

		void drawDebugData();

		void drawBox();

		void setInternalPosition( const Vector2f& Pos );

		virtual void setInternalSize(const Sizef& size );

		void setInternalPixelsSize( const Sizef& size );

		void setInternalPixelsWidth( const Float& width );

		void setInternalPixelsHeight( const Float& height );

};

}}

#endif
