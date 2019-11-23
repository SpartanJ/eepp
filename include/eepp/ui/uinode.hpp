#ifndef EE_UIUINODE_HPP
#define EE_UIUINODE_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uistate.hpp>
#include <eepp/ui/uiskin.hpp>
#include <eepp/ui/uiskinstate.hpp>
#include <eepp/scene/node.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>

namespace EE { namespace Graphics {
class Drawable;
class RectangleDrawable;
}}

namespace EE { namespace Scene {
class Action;
class ActionManager;
}}
using namespace EE::Scene;

namespace EE { namespace UI {

class UITheme;
class UINodeDrawable;

class EE_API UINode : public Node {
	public:
		static UINode * New();

		typedef std::function<void( const Event* )> EventCallback;

		UINode();

		virtual ~UINode();

		void worldToNodeTranslation( Vector2f& position ) const;

		void nodeToWorldTranslation( Vector2f& position ) const;

		void worldToNode( Vector2i& pos ) const;

		void nodeToWorld( Vector2i& pos ) const;

		void worldToNode( Vector2f& pos ) const;

		void nodeToWorld( Vector2f& pos ) const;

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setPosition( const Vector2f& Pos );

		virtual Node * setPosition( const Float& x, const Float& y );

		void setPixelsPosition(const Vector2f & position );

		void setPixelsPosition( const Float& x, const Float& y );

		const Vector2f& getPosition() const;

		const Vector2f& getPixelsPosition() const;

		virtual Node * setSize( const Sizef& size );

		virtual Node * setSize( const Float& Width, const Float& Height );

		UINode * setPixelsSize( const Sizef& size );

		UINode * setPixelsSize( const Float& x, const Float& y );

		const Sizef& getSize() const;

		Rect getRect() const;

		virtual void draw();

		Uint32 getHorizontalAlign() const;

		UINode * setHorizontalAlign( Uint32 halign );

		Uint32 getVerticalAlign() const;

		UINode * setVerticalAlign( Uint32 valign );

		UINode * setGravity( Uint32 hvalign );

		UINodeDrawable * setBackgroundFillEnabled( bool enabled );

		UINode * setBackgroundDrawable( Drawable * drawable, bool ownIt = false, int index = 0 );

		UINode * setBackgroundColor( const Color& color );

		UINode * setBackgroundPosition( const std::string& positionEq, int index = 0 );

		UINode * setBackgroundRepeat( const std::string& repeatRule, int index = 0 );

		UINode * setBackgroundSize( const std::string& sizeEq, int index = 0 );

		Color getBackgroundColor() const;

		UINode * setBorderRadius( const unsigned int& corners );

		Uint32 getBorderRadius() const;

		UINodeDrawable * setForegroundFillEnabled( bool enabled );

		UINode * setForegroundDrawable( Drawable * drawable , bool ownIt = false, int index = 0 );

		UINode * setForegroundColor( const Color& color );

		UINode * setForegroundPosition( const std::string& positionEq, int index = 0 );

		UINode * setForegroundRepeat( const std::string& repeatRule, int index = 0 ) ;

		Color getForegroundColor() const;

		UINode * setForegroundRadius( const unsigned int& corners );

		Uint32 getForegroundRadius() const;

		RectangleDrawable * setBorderEnabled( bool enabled );

		UINode * setBorderColor( const Color& color );

		Color getBorderColor();

		UINode * setBorderWidth( const unsigned int& width );

		Float getBorderWidth() const;

		const Uint32& getFlags() const;

		virtual UINode * setFlags( const Uint32& flags );

		virtual UINode * unsetFlags( const Uint32& flags );

		virtual UINode * resetFlags( Uint32 newFlags = 0 );

		UINodeDrawable * getBackground();

		UINodeDrawable* getForeground();

		RectangleDrawable * getBorder();

		void setThemeByName( const std::string& Theme );

		virtual void setTheme( UITheme * Theme );

		virtual UINode * setThemeSkin( UITheme * Theme, const std::string& skinName );

		virtual UINode * setThemeSkin( const std::string& skinName );

		void setThemeToChilds( UITheme * Theme );

		UISkin * getSkin() const;

		virtual UINode * setSkin( const UISkin& Skin );

		UINode * setSkin( UISkin * skin );

		UINode * setSkinColor( const Color& color );

		const Color& getSkinColor() const;

		void removeSkin();

		virtual void pushState( const Uint32& State, bool emitEvent = true );

		virtual void popState( const Uint32& State, bool emitEvent = true );

		Sizef getSkinSize() const;

		void applyDefaultTheme();

		Node * getWindowContainer() const;

		bool isDragging() const;

		void setDragging( const bool& dragging );

		const Vector2f& getDragPoint() const;

		void setDragPoint( const Vector2f& Point );

		bool isDragEnabled() const;

		void setDragEnabled( const bool& enable );

		void setDragButton( const Uint32& Button );

		const Uint32& getDragButton() const;

		virtual void setFocus();

		virtual Float lengthAsPixels( const CSS::StyleSheetLength& length, const Sizef& drawableSize, const bool& percentAsWidth = true );

		Float lengthAsDp( const CSS::StyleSheetLength& length, const Sizef& drawableSize, const bool& percentAsWidth = true );
	protected:
		Vector2f		mDpPos;
		Sizef			mDpSize;
		Uint32			mFlags;
		Uint32			mState;
		UISkinState *	mSkinState;
		UINodeDrawable *	mBackground;
		UINodeDrawable *	mForegroundState;
		RectangleDrawable *	mBorder;
		Vector2f		mDragPoint;
		Uint32			mDragButton;
		Color			mSkinColor;

		virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

		virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

		virtual Uint32 onValueChange();

		virtual void onStateChange();

		virtual void onEnabledChange();

		virtual void onAlignChange();

		virtual void drawSkin();

		virtual void drawBackground();

		virtual void drawForeground();

		virtual void drawBorder();

		virtual void onThemeLoaded();

		virtual void onChildCountChange();

		virtual Uint32 onCalculateDrag( const Vector2f& position, const Uint32& flags );

		virtual Uint32 onDrag( const Vector2f& position, const Uint32& flags );

		virtual Uint32 onDragStart( const Vector2i& position, const Uint32& flags );

		virtual Uint32 onDragStop( const Vector2i& position, const Uint32& flags );

		virtual Uint32 onMouseOver( const Vector2i& position, const Uint32& flags );

		virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );

		virtual Uint32 onFocus();

		virtual Uint32 onFocusLoss();

		void checkClose();

		virtual void internalDraw();

		virtual void onWidgetFocusLoss();

		void writeFlag( const Uint32& Flag, const Uint32& Val );

		Rectf makePadding( bool PadLeft = true, bool PadRight = true, bool PadTop = true, bool PadBottom = true, bool SkipFlags = false );

		Sizef getSkinSize( UISkin * Skin, const Uint32& State = UIState::StateFlagNormal ) const;

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
