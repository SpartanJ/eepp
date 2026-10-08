#include <eepp/ui/uiflowlayout.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace UI {

UIFlowLayout* UIFlowLayout::NewWithTag( const std::string& tag ) {
	return eeNew( UIFlowLayout, ( tag ) );
}

UIFlowLayout* UIFlowLayout::New() {
	return eeNew( UIFlowLayout, () );
}

UIFlowLayout::UIFlowLayout() : UIFlowLayout( "flowlayout" ) {}

UIFlowLayout::UIFlowLayout( const std::string& tag ) : UILayout( tag ) {
	mFlags |= UI_OWNS_CHILDREN_POSITION;
	setClipType( ClipType::ContentBox );
	setGravity( UI_HALIGN_LEFT | UI_VALIGN_TOP );
	listenParent();
}

UIFlowLayout::~UIFlowLayout() {
	clearListeners();
}

Uint32 UIFlowLayout::getType() const {
	return UI_TYPE_FLOW_LAYOUT;
}

bool UIFlowLayout::isType( const Uint32& type ) const {
	return UIFlowLayout::getType() == type ? true : UILayout::isType( type );
}

void UIFlowLayout::applySizePolicyOnChildren() {
	Node* child = mChild;

	while ( NULL != child ) {
		if ( child->isWidget() && child->isVisible() ) {
			UIWidget* widget = static_cast<UIWidget*>( child );

			switch ( widget->getLayoutWidthPolicy() ) {
				case SizePolicy::WrapContent: {
					widget->setFlags( UI_AUTO_SIZE );
					break;
				}
				case SizePolicy::MatchParent: {
					int w = getMatchParentWidth();

					if ( (int)widget->getPixelsSize().getWidth() != w && w > 0 )
						widget->setPixelsSize( w, widget->getPixelsSize().getHeight() );

					break;
				}
				case SizePolicy::Fixed:
				default: {
				}
			}

			switch ( widget->getLayoutHeightPolicy() ) {
				case SizePolicy::WrapContent: {
					widget->setFlags( UI_AUTO_SIZE );
					break;
				}
				case SizePolicy::MatchParent: {
					int h = getMatchParentHeight();

					if ( h != (int)widget->getPixelsSize().getHeight() && h > 0 )
						widget->setPixelsSize( widget->getPixelsSize().getWidth(), h );

					break;
				}
				case SizePolicy::Fixed:
				default: {
				}
			}
		}

		child = child->getNextNode();
	}
}

void UIFlowLayout::setRowValign( const std::string& rowValign ) {
	if ( rowValign == "top" ) {
		setRowValign( UIFlowLayout::RowValign::Top );
	} else if ( rowValign == "center" ) {
		setRowValign( UIFlowLayout::RowValign::Center );
	} else if ( rowValign == "bottom" ) {
		setRowValign( UIFlowLayout::RowValign::Bottom );
	}
}

std::string UIFlowLayout::rowValignToStr( const RowValign& rowValign ) {
	switch ( rowValign ) {
		case UIFlowLayout::RowValign::Top:
			return "top";
		case UIFlowLayout::RowValign::Center:
			return "center";
		case UIFlowLayout::RowValign::Bottom:
		default:
			return "bottom";
	}
}

void UIFlowLayout::clearListeners() {
	if ( mParentRef ) {
		if ( mParentSizeChangeCb > 0 ) {
			mParentRef->removeEventListener( mParentSizeChangeCb );
			mParentSizeChangeCb = 0;
		}
		if ( mParentCloseCb > 0 ) {
			mParentRef->removeEventListener( mParentCloseCb );
			mParentCloseCb = 0;
		}
	}
}

void UIFlowLayout::listenParent() {
	clearListeners();

	mParentRef = getParent();
	mParentSizeChangeCb = mParentRef->on( Event::OnSizeChange, [this]( const Event* ) {
		if ( getLayoutWidthPolicy() == SizePolicy::WrapContent &&
			 getUISceneNode()->isUpdatingLayouts() && getParent()->getPixelsSize().getWidth() > 0 &&
			 mSize.x != getMatchParentWidth() ) {
			runOnMainThread( [this]() { setLayoutDirty( LayoutInvalidation::ContainerLayout ); } );
		}
	} );
	mParentCloseCb =
		mParentRef->on( Event::OnClose, [this]( const Event* ) { mParentRef = nullptr; } );
}

void UIFlowLayout::onParentChange() {
	listenParent();
}

std::string UIFlowLayout::getPropertyString( const PropertyDefinition* propertyDef,
											 const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::GravityOwner:
			return isGravityOwner() ? "true" : "false";
		case PropertyId::RowValign:
			return rowValignToStr( mRowValign );
		default:
			return UILayout::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIFlowLayout::getPropertiesImplemented() const {
	auto props = UILayout::getPropertiesImplemented();
	auto local = { PropertyId::GravityOwner, PropertyId::RowValign };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIFlowLayout::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::GravityOwner: {
			setGravityOwner( attribute.asBool() );
			break;
		}
		case PropertyId::RowValign: {
			setRowValign( attribute.value() );
			break;
		}
		default:
			return UILayout::applyProperty( attribute );
	}

	return true;
}

Uint32 UIFlowLayout::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			tryUpdateLayout();
			return 1;
		}
	}

	return 0;
}

void UIFlowLayout::updateLayout() {
	if ( mPacking )
		return;
	mPacking = true;
	for ( auto& line : mLines ) {
		line.nodes.clear();
		line.maxY = 0;
		line.width = 0;
	}
	if ( mLines.empty() )
		mLines.emplace_back();

	if ( !mVisible ) {
		setInternalPixelsSize( Sizef::Zero );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentChildChange );
		mPacking = false;
		mDirtyLayout = false;
		return;
	}

	Sizef size( getSizeFromLayoutPolicy() );

	if ( getLayoutWidthPolicy() == SizePolicy::WrapContent )
		size.x = getMatchParentWidth();

	if ( size != getPixelsSize() )
		setInternalPixelsSize( size );

	applySizePolicyOnChildren();

	Float curX = mPaddingPx.Left;
	Node* child = mChild;
	Uint32 curLine = 0;
	bool addedLine = false;

	auto addLine = [&]() {
		curX = mPaddingPx.Left;
		++curLine;
		if ( curLine >= mLines.size() )
			mLines.emplace_back();
		addedLine = true;
	};

	while ( NULL != child ) {
		if ( child->isWidget() && child->isVisible() ) {
			UIWidget* widget = static_cast<UIWidget*>( child );
			const Rectf& margin = widget->getLayoutPixelsMargin();

			if ( curX + margin.Left + widget->getPixelsSize().getWidth() >= mSize.getWidth() &&
				 !addedLine && !mLines[curLine].nodes.empty() )
				addLine();

			addedLine = false;

			curX += eeceil( margin.Left );

			Vector2f pos( curX, mPaddingPx.Top );

			widget->setPixelsPosition( pos );

			curX += eeceil( widget->getPixelsSize().getWidth() + margin.Right );

			mLines[curLine].nodes.push_back( widget );
			mLines[curLine].width = curX;
			if ( widget->getLayoutHeightPolicy() != SizePolicy::MatchParent ) {
				mLines[curLine].maxY = eeceil(
					eemax( mLines[curLine].maxY, ( widget->getPixelsSize().getHeight() +
												   widget->getLayoutPixelsMargin().Top +
												   widget->getLayoutPixelsMargin().Bottom ) ) );
			}

			if ( curX > mSize.getWidth() )
				addLine();
		}

		child = child->getNextNode();
	}

	Float maxY = mPaddingPx.Top;
	Float height = 0.f;
	Float totHeight = maxY;
	const Uint32 lineCount = curLine + 1;
	for ( Uint32 i = 0; i < lineCount; ++i ) {
		auto& line = mLines[i];
		if ( curLine > 0 && line.maxY == 0 )
			line.maxY = mLines[curLine - 1].maxY;
		height += line.maxY;
		totHeight += line.maxY;
	}
	totHeight += mPaddingPx.Bottom;

	curLine = 0;
	for ( Uint32 i = 0; i < lineCount; ++i ) {
		const auto& line = mLines[i];
		Float xDisplacement = 0.f;
		Float yDisplacement = 0.f;

		switch ( Font::getHorizontalAlign( getHorizontalAlign() ) ) {
			case UI_HALIGN_CENTER:
				xDisplacement = eeceil( ( getPixelsSize().getWidth() - line.width ) * 0.5f );
				break;
			case UI_HALIGN_RIGHT:
				xDisplacement = getPixelsSize().getWidth() - line.width;
				break;
			case UI_HALIGN_LEFT:
			default:
				break;
		}

		Float innerHeight = getPixelsSize().getHeight() - mPaddingPx.Top - mPaddingPx.Bottom;
		if ( height < innerHeight ) {
			switch ( Font::getVerticalAlign( getVerticalAlign() ) ) {
				case UI_VALIGN_CENTER:
					yDisplacement = eeceil( ( innerHeight - height ) * 0.5f );
					break;
				case UI_VALIGN_BOTTOM:
					yDisplacement = ( innerHeight - height );
					break;
				case UI_VALIGN_TOP:
				default:
					break;
			}
		}

		for ( const auto& widget : line.nodes ) {
			Vector2f pos( widget->getPixelsPosition() );

			if ( widget->getLayoutHeightPolicy() == SizePolicy::MatchParent &&
				 widget->getPixelsSize().getHeight() != line.maxY )
				widget->setPixelsSize( widget->getPixelsSize().getWidth(), line.maxY );

			switch ( Font::getHorizontalAlign( getHorizontalAlign() ) ) {
				case UI_HALIGN_CENTER:
				case UI_HALIGN_RIGHT:
					pos.x = xDisplacement + widget->getPixelsPosition().x;
					break;
				case UI_HALIGN_LEFT:
				default:
					break;
			}

			switch ( mRowValign ) {
				case UIFlowLayout::RowValign::Center:
					pos.y = yDisplacement + maxY +
							eeceil( ( line.maxY - widget->getPixelsSize().getHeight() ) * 0.5f );
					break;
				case UIFlowLayout::RowValign::Bottom:
					pos.y = yDisplacement + maxY + line.maxY - widget->getPixelsSize().getHeight() -
							widget->getLayoutPixelsMargin().Bottom;
					break;
				case UIFlowLayout::RowValign::Top:
				default:
					pos.y = yDisplacement + maxY + widget->getLayoutPixelsMargin().Top;
					break;
			}

			widget->setPixelsPosition( pos );
		}

		maxY += line.maxY;
		curLine++;
	}

	if ( getLayoutWidthPolicy() == SizePolicy::WrapContent && curX < mSize.getWidth() &&
		 ( ( lineCount == 1 && !mLines[0].nodes.empty() ) ||
		   ( lineCount == 2 && mLines[1].nodes.empty() ) ) ) {
		setInternalPixelsWidth( curX );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentChildChange );
	}

	if ( getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
		if ( totHeight != (int)getPixelsSize().getHeight() ) {
			setInternalPixelsHeight( totHeight );
			notifyLayoutAttrChangeParent( LayoutInvalidation::ParentChildChange );
		}
	}

	if ( getParent()->isUINode() &&
		 ( !getParent()->asType<UINode>()->ownsChildPosition() || isGravityOwner() ) ) {
		alignAgainstLayout();
	}

	mPacking = false;
	mDirtyLayout = false;
}

const UIFlowLayout::RowValign& UIFlowLayout::getRowValign() const {
	return mRowValign;
}

void UIFlowLayout::setRowValign( const RowValign& rowValign ) {
	mRowValign = rowValign;
}

}} // namespace EE::UI
