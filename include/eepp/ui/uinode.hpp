#ifndef EE_UIUINODE_HPP
#define EE_UIUINODE_HPP

#include <eepp/scene/node.hpp>
#include <eepp/ui/base.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/uiclip.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uiskin.hpp>
#include <eepp/ui/uiskinstate.hpp>
#include <eepp/ui/uistate.hpp>

namespace EE { namespace Graphics {
class Drawable;
}} // namespace EE::Graphics

namespace EE { namespace Scene {
class Action;
class ActionManager;
}} // namespace EE::Scene
using namespace EE::Scene;

namespace EE { namespace UI {

class UISceneNode;
class UITheme;
class UINodeDrawable;
class UIBorderDrawable;

class EE_API UINode : public Node {
  public:
	static UINode* New();

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

	virtual Node* setPosition( const Float& x, const Float& y );

	UINode* setPixelsPosition( const Vector2f& position );

	UINode* setPixelsPosition( const Float& x, const Float& y );

	const Vector2f& getPosition() const;

	const Vector2f& getPixelsPosition() const;

	virtual Node* setSize( const Sizef& size );

	virtual Node* setSize( const Float& Width, const Float& Height );

	UINode* setPixelsSize( const Sizef& size );

	UINode* setPixelsSize( const Float& x, const Float& y );

	const Sizef& getSize() const;

	Rect getRect() const;

	Rectf getRectBox() const;

	virtual void draw();

	Uint32 getHorizontalAlign() const;

	UINode* setHorizontalAlign( Uint32 halign );

	Uint32 getVerticalAlign() const;

	UINode* setVerticalAlign( Uint32 valign );

	UINode* setGravity( Uint32 hvalign );

	UINodeDrawable* setBackgroundFillEnabled( bool enabled );

	UINode* setBackgroundDrawable( Drawable* drawable, bool ownIt = false, int index = 0 );

	UINode* setBackgroundDrawable( const std::string& drawable, int index );

	UINode* setBackgroundColor( const Color& color );

	UINode* setBackgroundTint( const Color& color, int index );

	UINode* setBackgroundPositionX( const std::string& positionX, int index = 0 );

	UINode* setBackgroundPositionY( const std::string& positionY, int index = 0 );

	UINode* setBackgroundRepeat( const std::string& repeatRule, int index = 0 );

	UINode* setBackgroundSize( const std::string& size, int index = 0 );

	Color getBackgroundColor() const;

	Color getBackgroundTint( int index = 0 ) const;

	UINode* setBorderRadius( const unsigned int& corners );

	UINode* setTopLeftRadius( const std::string& radius );

	UINode* setTopRightRadius( const std::string& radius );

	UINode* setBottomLeftRadius( const std::string& radius );

	UINode* setBottomRightRadius( const std::string& radius );

	Uint32 getBorderRadius() const;

	UINodeDrawable* setForegroundFillEnabled( bool enabled );

	UINode* setForegroundDrawable( Drawable* drawable, bool ownIt = false, int index = 0 );

	UINode* setForegroundDrawable( const std::string& drawable, int index = 0 );

	UINode* setForegroundColor( const Color& color );

	UINode* setForegroundTint( const Color& color, int index );

	UINode* setForegroundPositionX( const std::string& positionX, int index = 0 );

	UINode* setForegroundPositionY( const std::string& positionY, int index = 0 );

	UINode* setForegroundRepeat( const std::string& repeatRule, int index = 0 );

	UINode* setForegroundSize( const std::string& size, int index = 0 );

	Color getForegroundColor() const;

	Color getForegroundTint( int index ) const;

	UINode* setForegroundRadius( const unsigned int& corners );

	Uint32 getForegroundRadius() const;

	UIBorderDrawable* setBorderEnabled( bool enabled ) const;

	UINode* setBorderColor( const Color& color );

	Color getBorderColor();

	UINode* setBorderWidth( const unsigned int& width );

	Float getBorderWidth() const;

	const Uint32& getFlags() const;

	virtual UINode* setFlags( const Uint32& flags );

	virtual UINode* unsetFlags( const Uint32& flags );

	virtual UINode* resetFlags( Uint32 newFlags = 0 );

	UINodeDrawable* getBackground() const;

	bool hasBackground() const;

	UINodeDrawable* getForeground() const;

	bool hasForeground() const;

	UIBorderDrawable* getBorder() const;

	void setThemeByName( const std::string& Theme );

	virtual void setTheme( UITheme* Theme );

	virtual UINode* setThemeSkin( UITheme* Theme, const std::string& skinName );

	virtual UINode* setThemeSkin( const std::string& skinName );

	void setThemeToChilds( UITheme* Theme );

	UISkin* getSkin() const;

	virtual UINode* setSkin( const UISkin& Skin );

	UINode* setSkin( UISkin* skin );

	UINode* setSkinColor( const Color& color );

	const Color& getSkinColor() const;

	void removeSkin();

	virtual void pushState( const Uint32& State, bool emitEvent = true );

	virtual void popState( const Uint32& State, bool emitEvent = true );

	Sizef getSkinSize( const Uint32& state = UIState::StateFlagNormal ) const;

	void applyDefaultTheme();

	Node* getWindowContainer() const;

	bool isTabFocusable() const;

	bool isDragging() const;

	void setDragging( const bool& dragging );

	void startDragging( const Vector2f& position );

	bool ownsChildPosition() const;

	const Vector2f& getDragPoint() const;

	void setDragPoint( const Vector2f& Point );

	bool isDragEnabled() const;

	void setDragEnabled( const bool& enable );

	void setDragButton( const Uint32& Button );

	const Uint32& getDragButton() const;

	virtual Node* setFocus( NodeFocusReason reason = NodeFocusReason::Unknown );

	Float
	getPropertyRelativeTargetContainerLength( const CSS::PropertyRelativeTarget& relativeTarget,
											  const Float& defaultValue = 0,
											  const Uint32& propertyIndex = 0 ) const;

	virtual Float convertLength( const CSS::StyleSheetLength& length,
								 const Float& containerLength ) const;

	Float convertLengthAsDp( const CSS::StyleSheetLength& length,
							 const Float& containerLength ) const;

	Float lengthFromValue( const std::string& value,
						   const CSS::PropertyRelativeTarget& relativeTarget,
						   const Float& defaultValue = 0, const Uint32& propertyIndex = 0 ) const;

	Float lengthFromValue( const CSS::StyleSheetProperty& property, const Float& defaultValue = 0 );

	Float lengthFromValueAsDp( const std::string& value,
							   const CSS::PropertyRelativeTarget& relativeTarget,
							   const Float& defaultValue = 0,
							   const Uint32& propertyIndex = 0 ) const;

	Float lengthFromValueAsDp( const CSS::StyleSheetProperty& property,
							   const Float& defaultValue = 0 ) const;

	UISceneNode* getUISceneNode() const;

	void setMinWidth( const Float& width );

	void setMinHeight( const Float& height );

	void setMinSize( const Sizef& size );

	const Sizef& getCurMinSize() const;

	Rectf getLocalDpBounds() const;

	virtual void nodeDraw();

	void clearForeground();

	void clearBackground();

	const ClipType& getClipType() const;

	UINode* setClipType( const ClipType& clipType );

	bool hasBorder() const;

	virtual const Rectf& getPixelsPadding() const;

	const std::string& getMinWidthEq() const;

	void setMinSizeEq( const std::string& minWidthEq, const std::string& minHeightEq );

	void setMinWidthEq( const std::string& minWidthEq );

	const std::string& getMinHeightEq() const;

	void setMinHeightEq( const std::string& minHeightEq );

	const std::string& getMaxWidthEq() const;

	void setMaxSizeEq( const std::string& maxWidthEq, const std::string& maxHeightEq );

	void setMaxWidthEq( const std::string& maxWidthEq );

	const std::string& getMaxHeightEq() const;

	void setMaxHeightEq( const std::string& maxHeightEq );

	Sizef getMinSize() const;

	Sizef getMaxSize() const;

	Sizef getMinSizePx() const;

	Sizef getMaxSizePx() const;

	Sizef fitMinMaxSizeDp( const Sizef& size ) const;

	Sizef fitMinMaxSizePx( const Sizef& size ) const;

	virtual bool isScrollable() const;

  protected:
	Vector2f mDpPos;
	Sizef mDpSize;
	Sizef mMinSize;
	Uint32 mFlags;
	Uint32 mState;
	UISkinState* mSkinState;
	mutable UINodeDrawable* mBackground;
	mutable UINodeDrawable* mForeground;
	mutable UIBorderDrawable* mBorder;
	Vector2f mDragPoint;
	Uint32 mDragButton;
	Color mSkinColor;
	UISceneNode* mUISceneNode;
	Rectf mPadding;
	Rectf mPaddingPx;
	UIClip mClip;
	std::string mMinWidthEq;
	std::string mMinHeightEq;
	std::string mMaxWidthEq;
	std::string mMaxHeightEq;

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

	virtual void onChildCountChange( Node* child, const bool& removed );

	virtual Uint32 onCalculateDrag( const Vector2f& position, const Uint32& flags );

	virtual Uint32 onDrag( const Vector2f& position, const Uint32& flags, const Sizef& dragDiff );

	virtual Uint32 onDragStart( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onDragStop( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onDrop( UINode* widget );

	virtual Uint32 onMouseOver( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onFocus( NodeFocusReason reason );

	virtual Uint32 onFocusLoss();

	virtual void onSceneChange();

	virtual void drawDroppableHovering();

	void checkClose();

	virtual void onWidgetFocusLoss();

	void writeFlag( const Uint32& Flag, const Uint32& Val );

	Rectf makePadding( bool PadLeft = true, bool PadRight = true, bool PadTop = true,
					   bool PadBottom = true, bool SkipFlags = false ) const;

	Sizef getSkinSize( UISkin* Skin, const Uint32& State = UIState::StateFlagNormal ) const;

	void drawHighlightFocus();

	void drawOverNode();

	void updateDebugData();

	void drawBox();

	void setInternalPosition( const Vector2f& Pos );

	virtual void setInternalSize( const Sizef& size );

	void setInternalPixelsSize( const Sizef& size );

	void setInternalPixelsWidth( const Float& width );

	void setInternalPixelsHeight( const Float& height );

	virtual void updateOriginPoint();

	void smartClipStart( const ClipType& reqClipType, bool needsClipPlanes );

	void smartClipEnd( const ClipType& reqClipType, bool needsClipPlanes );

	void smartClipStart( const ClipType& reqClipType );

	void smartClipEnd( const ClipType& reqClipType );
};

}} // namespace EE::UI

#endif
