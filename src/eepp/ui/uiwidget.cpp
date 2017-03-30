#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/helper/pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIWidget * UIWidget::New() {
	return eeNew( UIWidget, () );
}

UIWidget::UIWidget() :
	UIControlAnim(),
	mTheme( NULL ),
	mTooltip( NULL ),
	mMinControlSize(),
	mLayoutWeight(0),
	mLayoutGravity(0),
	mLayoutWidthRules(WRAP_CONTENT),
	mLayoutHeightRules(WRAP_CONTENT),
	mLayoutPositionRule(NONE),
	mLayoutPositionRuleWidget(NULL)
{
	mControlFlags |= UI_CTRL_FLAG_COMPLEX;

	updateAnchorsDistances();
}

UIWidget::~UIWidget() {
	eeSAFE_DELETE( mTooltip );
}

Uint32 UIWidget::getType() const {
	return UI_TYPE_WIDGET;
}

bool UIWidget::isType( const Uint32& type ) const {
	return UIWidget::getType() == type ? true : UIControlAnim::isType( type );
}

void UIWidget::updateAnchorsDistances() {
	if ( NULL != mParentCtrl ) {
		mDistToBorder	= Recti( mRealPos.x, mRealPos.y, mParentCtrl->getRealSize().x - ( mRealPos.x + mRealSize.x ), mParentCtrl->getRealSize().y - ( mRealPos.y + mRealSize.y ) );
	}
}

Recti UIWidget::getLayoutMargin() const {
	return mLayoutMargin;
}

UIWidget * UIWidget::setLayoutMargin(const Recti & margin) {
	mLayoutMargin = margin;
	mRealMargin = PixelDensity::dpToPxI( margin );
	notifyLayoutAttrChange();
	return this;
}

Float UIWidget::getLayoutWeight() const {
	return mLayoutWeight;
}

UIWidget * UIWidget::setLayoutWeight(const Float & weight) {
	mLayoutWeight = weight;
	notifyLayoutAttrChange();
	return this;
}

Uint32 UIWidget::getLayoutGravity() const {
	return mLayoutGravity;
}

UIWidget * UIWidget::setLayoutGravity(const Uint32 & layoutGravity) {
	mLayoutGravity = layoutGravity;
	notifyLayoutAttrChange();
	return this;
}

LayoutSizeRules UIWidget::getLayoutWidthRules() const {
	return mLayoutWidthRules;
}

UIWidget * UIWidget::setLayoutWidthRules(const LayoutSizeRules & layoutWidthRules) {
	mLayoutWidthRules = layoutWidthRules;
	notifyLayoutAttrChange();
	return this;
}

LayoutSizeRules UIWidget::getLayoutHeightRules() const {
	return mLayoutHeightRules;
}

UIWidget * UIWidget::setLayoutHeightRules(const LayoutSizeRules & layoutHeightRules) {
	mLayoutHeightRules = layoutHeightRules;
	notifyLayoutAttrChange();
	return this;
}

UIWidget * UIWidget::setLayoutSizeRules(const LayoutSizeRules & layoutWidthRules, const LayoutSizeRules & layoutHeightRules) {
	mLayoutWidthRules = layoutWidthRules;
	mLayoutHeightRules = layoutHeightRules;
	notifyLayoutAttrChange();
	return this;
}

UIWidget * UIWidget::setLayoutPositionRule(const LayoutPositionRules & layoutPositionRule, UIWidget * of) {
	mLayoutPositionRule = layoutPositionRule;
	mLayoutPositionRuleWidget  = of;
	notifyLayoutAttrChange();
	return this;
}

UIWidget * UIWidget::getLayoutPositionRuleWidget() const {
	return mLayoutPositionRuleWidget;
}

LayoutPositionRules UIWidget::getLayoutPositionRule() const {
	return mLayoutPositionRule;
}

void UIWidget::update() {
	if ( mVisible && NULL != mTooltip && mTooltip->getText().size() ) {
		if ( isMouseOverMeOrChilds() ) {
			UIManager * uiManager = UIManager::instance();
			UIThemeManager * themeManager = UIThemeManager::instance();

			Vector2i Pos = uiManager->getMousePos();
			Pos.x += themeManager->getCursorSize().x;
			Pos.y += themeManager->getCursorSize().y;

			if ( Pos.x + mTooltip->getRealSize().getWidth() > uiManager->getMainControl()->getRealSize().getWidth() ) {
				Pos.x = uiManager->getMousePos().x - mTooltip->getRealSize().getWidth();
			}

			if ( Pos.y + mTooltip->getRealSize().getHeight() > uiManager->getMainControl()->getRealSize().getHeight() ) {
				Pos.y = uiManager->getMousePos().y - mTooltip->getRealSize().getHeight();
			}

			if ( Time::Zero == themeManager->getTooltipTimeToShow() ) {
				if ( !mTooltip->isVisible() || themeManager->getTooltipFollowMouse() )
					mTooltip->setPosition( PixelDensity::pxToDpI( Pos ) );

				mTooltip->show();
			} else {
				if ( -1.f != mTooltip->getTooltipTime().asMilliseconds() ) {
					mTooltip->addTooltipTime( getElapsed() );
				}

				if ( mTooltip->getTooltipTime() >= themeManager->getTooltipTimeToShow() ) {
					if ( mTooltip->getTooltipTime().asMilliseconds() != -1.f ) {
						mTooltip->setPosition( PixelDensity::pxToDpI( Pos ) );

						mTooltip->show();

						mTooltip->setTooltipTime( Milliseconds( -1.f ) );
					}
				}
			}

			if ( themeManager->getTooltipFollowMouse() ) {
				mTooltip->setPosition( PixelDensity::pxToDpI( Pos ) );
			}
		} else {
			mTooltip->setTooltipTime( Milliseconds( 0.f ) );

			if ( mTooltip->isVisible() )
				mTooltip->hide();
		}
	}

	UIControlAnim::update();
}

void UIWidget::createTooltip() {
	if ( NULL != mTooltip )
		return;

	mTooltip = UITooltip::New();
	mTooltip->setVisible( false )->setEnabled( false );
	mTooltip->setTooltipOf( this );
}

UIWidget * UIWidget::setTooltipText( const String& Text ) {
	if ( NULL == mTooltip ) {	// If the tooltip wasn't created it will avoid to create a new one if the string is ""
		if ( Text.size() ) {
			createTooltip();

			mTooltip->setText( Text );
		}
	} else { // but if it's created, i will allow it
		mTooltip->setText( Text );
	}

	return this;
}

String UIWidget::getTooltipText() {
	if ( NULL != mTooltip )
		return mTooltip->getText();

	return String();
}

void UIWidget::tooltipRemove() {
	mTooltip = NULL;
}

UIControl * UIWidget::setSize( const Sizei& size ) {
	Sizei s( size );

	if ( s.x < mMinControlSize.x )
		s.x = mMinControlSize.x;

	if ( s.y < mMinControlSize.y )
		s.y = mMinControlSize.y;

	return UIControlAnim::setSize( s );
}

UIControl * UIWidget::setFlags(const Uint32 & flags) {
	if ( flags & ( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) {
		updateAnchorsDistances();
	}

	if ( flags & UI_AUTO_SIZE ) {
		onAutoSize();
	}

	return UIControlAnim::setFlags( flags );
}

UIControl * UIWidget::unsetFlags(const Uint32 & flags) {
	if ( flags & ( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) {
		updateAnchorsDistances();
	}

	return UIControlAnim::unsetFlags( flags );
}

UIWidget * UIWidget::setAnchors(const Uint32 & flags) {
	mFlags &= ~(UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM);
	mFlags |= flags;
	updateAnchorsDistances();
	return this;
}

void UIWidget::setTheme( UITheme * Theme ) {
	mTheme = Theme;
}

UIControl * UIWidget::setSize( const Int32& Width, const Int32& Height ) {
	return UIControlAnim::setSize( Width, Height );
}

const Sizei& UIWidget::getSize() {
	return UIControlAnim::getSize();
}

void UIWidget::onParentSizeChange( const Vector2i& SizeChange ) {
	updateAnchors( SizeChange );
	UIControlAnim::onParentSizeChange( SizeChange );
}

void UIWidget::onPositionChange() {
	updateAnchorsDistances();
	UIControlAnim::onPositionChange();
}

void UIWidget::onVisibilityChange() {
	updateAnchorsDistances();
	UIControlAnim::onVisibilityChange();
}

void UIWidget::onAutoSize() {
}

void UIWidget::notifyLayoutAttrChange() {
	UIMessage msg( this, UIMessage::MsgLayoutAttributeChange );
	messagePost( &msg );
}

void UIWidget::updateAnchors( const Vector2i& SizeChange ) {
	if ( !( mFlags & ( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) )
		return;

	Sizei newSize( mSize );

	if ( !( mFlags & UI_ANCHOR_LEFT ) ) {
		setInternalPosition( Vector2i( mPos.x += SizeChange.x, mPos.y ) );
	}

	if ( mFlags & UI_ANCHOR_RIGHT ) {
		if ( NULL != mParentCtrl ) {
			newSize.x = mParentCtrl->getSize().getWidth() - mPos.x - PixelDensity::pxToDpI( mDistToBorder.Right );

			if ( newSize.x < mMinControlSize.getWidth() )
				newSize.x = mMinControlSize.getWidth();
		}
	}

	if ( !( mFlags & UI_ANCHOR_TOP ) ) {
		setInternalPosition( Vector2i( mPos.x, mPos.y += SizeChange.y ) );
	}

	if ( mFlags & UI_ANCHOR_BOTTOM ) {
		if ( NULL != mParentCtrl ) {
			newSize.y = mParentCtrl->getSize().y - mPos.y - PixelDensity::pxToDpI( mDistToBorder.Bottom );

			if ( newSize.y < mMinControlSize.getHeight() )
				newSize.y = mMinControlSize.getHeight();
		}
	}

	if ( newSize != mSize )
		setSize( newSize );
}

void UIWidget::alignAgainstLayout() {
	Vector2i pos = mPos;

	switch ( fontHAlignGet( mLayoutGravity ) ) {
		case UI_HALIGN_CENTER:
			pos.x = ( getParent()->getSize().getWidth() - mSize.getWidth() ) / 2;
			break;
		case UI_HALIGN_RIGHT:
			pos.x = getParent()->getSize().getWidth() - mLayoutMargin.Right;
			break;
		case UI_HALIGN_LEFT:
			pos.x = mLayoutMargin.Left;
			break;
	}

	switch ( fontVAlignGet( mLayoutGravity ) ) {
		case UI_VALIGN_CENTER:
			pos.y = ( getParent()->getSize().getHeight() - mSize.getHeight() ) / 2;
			break;
		case UI_VALIGN_BOTTOM:
			pos.y = getParent()->getSize().getHeight() - mLayoutMargin.Bottom;
			break;
		case UI_VALIGN_TOP:
			pos.y = mLayoutMargin.Top;
			break;
	}

	setInternalPosition( pos );
}

void UIWidget::loadFromXmlNode( const pugi::xml_node& node ) {
	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "id" == name ) {
			setId( ait->value() );
		} else if ( "x" == name ) {
			setInternalPosition( Vector2i( PixelDensity::toDpFromStringI( ait->as_string() ), mPos.y ) );
		} else if ( "y" == name ) {
			setInternalPosition( Vector2i( mPos.x, PixelDensity::toDpFromStringI( ait->as_string() ) ) );
		} else if ( "width" == name ) {
			setInternalWidth( PixelDensity::toDpFromStringI( ait->as_string() ) );
			notifyLayoutAttrChange();
		} else if ( "height" == name ) {
			setInternalHeight( PixelDensity::toDpFromStringI( ait->as_string() ) );
			notifyLayoutAttrChange();
		} else if ( "backgroundcolor" == name ) {
			setBackgroundFillEnabled( true )->setColor( ColorA::fromString( ait->as_string() ) );
		} else if ( "bordercolor" == name ) {
			setBorderEnabled( true )->setColor( ColorA::fromString( ait->as_string() ) );
		} else if ( "borderwidth" == name ) {
			setBorderEnabled( true )->setWidth( PixelDensity::toDpFromStringI( ait->as_string("1") ) );
		} else if ( "visible" == name ) {
			setVisible( ait->as_bool() );
		} else if ( "enabled" == name ) {
			setEnabled( ait->as_bool() );
		} else if ( "theme" == name ) {
			setThemeByName( ait->as_string() );
		} else if ( "skin" == name ) {
			setThemeControl( ait->as_string() );
		} else if ( "gravity" == name ) {
			std::string gravity = ait->as_string();
			String::toLowerInPlace( gravity );
			std::vector<std::string> strings = String::split( gravity, '|' );

			if ( strings.size() ) {
				for ( std::size_t i = 0; i < strings.size(); i++ ) {
					std::string cur = strings[i];
					String::toLowerInPlace( cur );

					if ( "left" == cur )
						setHorizontalAlign( UI_HALIGN_LEFT );
					else if ( "right" == cur )
						setHorizontalAlign( UI_HALIGN_RIGHT );
					else if ( "center_horizontal" == cur )
						setHorizontalAlign( UI_HALIGN_CENTER );
					else if ( "top" == cur )
						setVerticalAlign( UI_VALIGN_TOP );
					else if ( "bottom" == cur )
						setVerticalAlign( UI_VALIGN_BOTTOM );
					else if ( "center_vertical" == cur ) {
						setVerticalAlign( UI_VALIGN_CENTER );
					} else if ( "center" == cur ) {
						setHorizontalAlign( UI_HALIGN_CENTER );
						setVerticalAlign( UI_VALIGN_CENTER );
					}
				}

				notifyLayoutAttrChange();
			}
		} else if ( "flags" == name ) {
			std::string flags = ait->as_string();
			String::toLowerInPlace( flags );
			std::vector<std::string> strings = String::split( flags, '|' );

			if ( strings.size() ) {
				for ( std::size_t i = 0; i < strings.size(); i++ ) {
					std::string cur = strings[i];
					String::toLowerInPlace( cur );

					if ( "auto_size" == cur || "autosize" == cur ) {
						setFlags( UI_AUTO_SIZE );
						notifyLayoutAttrChange();
					} else if ( "clip" == cur ) {
						setFlags( UI_CLIP_ENABLE );
					} else if ( "word_wrap" == cur || "wordwrap" == cur ) {
						setFlags( UI_WORD_WRAP );
					} else if ( "multi" == cur ) {
						setFlags(  UI_MULTI_SELECT );
					} else if ( "auto_padding" == cur || "autopadding" == cur ) {
						setFlags( UI_AUTO_PADDING );
						notifyLayoutAttrChange();
					} else if ( "reportsizechangetochilds" == cur || "report_size_change_to_childs" == cur ) {
						setFlags( UI_REPORT_SIZE_CHANGE_TO_CHILDS );
					}
				}
			}
		} else if ( "layout_margin" == name ) {
			int val = PixelDensity::toDpFromStringI( ait->as_string() );
			setLayoutMargin( Recti( val, val, val, val ) );
		} else if ( "layout_marginleft" == name ) {
			setLayoutMargin( Recti( PixelDensity::toDpFromStringI( ait->as_string() ), mLayoutMargin.Top, mLayoutMargin.Right, mLayoutMargin.Bottom ) );
		} else if ( "layout_marginright" == name ) {
			setLayoutMargin( Recti( mLayoutMargin.Left, mLayoutMargin.Top, PixelDensity::toDpFromStringI( ait->as_string() ), mLayoutMargin.Bottom ) );
		} else if ( "layout_margintop" == name ) {
			setLayoutMargin( Recti( mLayoutMargin.Left, PixelDensity::toDpFromStringI( ait->as_string() ), mLayoutMargin.Right, mLayoutMargin.Bottom ) );
		} else if ( "layout_marginbottom" == name ) {
			setLayoutMargin( Recti( mLayoutMargin.Left, mLayoutMargin.Top, mLayoutMargin.Right, PixelDensity::toDpFromStringI( ait->as_string() ) ) );
		} else if ( "tooltip" == name ) {
			setTooltipText( ait->as_string() );
		} else if ( "layout_weight" == name ) {
			setLayoutWeight( ait->as_float() );
		} else if ( "layout_gravity" == name ) {
			std::string gravityStr = ait->as_string();
			String::toLowerInPlace( gravityStr );
			std::vector<std::string> strings = String::split( gravityStr, '|' );
			Uint32 gravity = 0;

			if ( strings.size() ) {
				for ( std::size_t i = 0; i < strings.size(); i++ ) {
					std::string cur = strings[i];
					String::toLowerInPlace( cur );

					if ( "left" == cur )
						gravity |= UI_HALIGN_LEFT;
					else if ( "right" == cur )
						gravity |= UI_HALIGN_RIGHT;
					else if ( "center_horizontal" == cur )
						gravity |= UI_HALIGN_CENTER;
					else if ( "top" == cur )
						gravity |= UI_VALIGN_TOP;
					else if ( "bottom" == cur )
						gravity |= UI_VALIGN_BOTTOM;
					else if ( "center_vertical" == cur ) {
						gravity |= UI_VALIGN_CENTER;
					} else if ( "center" == cur ) {
						gravity |= UI_VALIGN_CENTER | UI_HALIGN_CENTER;
					}
				}

				setLayoutGravity( gravity );
			}
		} else if ( "layout_width" == name ) {
			std::string val = ait->as_string();
			String::toLowerInPlace( val );

			if ( "match_parent" == val ) {
				setLayoutWidthRules( MATCH_PARENT );
			} else if ( "wrap_content" == val ) {
				setLayoutWidthRules( WRAP_CONTENT );
			} else if ( "fixed" == val ) {
				setLayoutWidthRules( FIXED );
			} else {
				setLayoutWidthRules( FIXED );
				setInternalWidth( PixelDensity::toDpFromStringI( val ) );
			}
		} else if ( "layout_height" == name ) {
			std::string val = ait->as_string();
			String::toLowerInPlace( val );

			if ( "match_parent" == val ) {
				setLayoutHeightRules( MATCH_PARENT );
			} else if ( "wrap_content" == val ) {
				setLayoutHeightRules( WRAP_CONTENT );
			} else if ( "fixed" == val ) {
				setLayoutHeightRules( FIXED );
			} else {
				setLayoutHeightRules( FIXED );
				setInternalHeight( PixelDensity::toDpFromStringI( val ) );
			}
		} else if ( String::startsWith( name, "layout_to_" ) || String::startsWith( name, "layoutto" ) ) {
			LayoutPositionRules rule = NONE;
			if ( "layout_to_left_of" == name || "layouttoleftof" == name ) rule = LEFT_OF;
			else if ( "layout_to_right_of" == name || "layouttorightof" == name ) rule = RIGHT_OF;
			else if ( "layout_to_top_of" == name || "layouttotopof" == name ) rule = TOP_OF;
			else if ( "layout_to_bottom_of" == name || "layouttobottomof" == name ) rule = BOTTOM_OF;

			std::string id = ait->as_string();

			UIControl * control = getParent()->find( id );

			if ( NULL != control && control->isWidget() ) {
				UIWidget * widget = static_cast<UIWidget*>( control );

				setLayoutPositionRule( rule, widget );
			}
		} else if ( "clip" == name ) {
			if ( ait->as_bool() )
				setFlags( UI_CLIP_ENABLE );
			else
				unsetFlags( UI_CLIP_ENABLE );
		}
	}
}

}}
