#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistacklayout.hpp>

namespace EE { namespace UI {

UIStackLayout* UIStackLayout::NewWithTag( const std::string& tag ) {
	return eeNew( UIStackLayout, ( tag ) );
}

UIStackLayout* UIStackLayout::New() {
	return eeNew( UIStackLayout, () );
}

UIStackLayout::UIStackLayout() : UIStackLayout( "stacklayout" ) {}

UIStackLayout::UIStackLayout( const std::string& tag ) : UILayout( tag ) {
	mFlags |= UI_OWNS_CHILDS_POSITION;
	setClipType( ClipType::ContentBox );
	setGravity( UI_HALIGN_LEFT | UI_VALIGN_TOP );
	listenParent();
}

UIStackLayout::~UIStackLayout() {
	clearListeners();
}

Uint32 UIStackLayout::getType() const {
	return UI_TYPE_STACK_LAYOUT;
}

bool UIStackLayout::isType( const Uint32& type ) const {
	return UIStackLayout::getType() == type ? true : UILayout::isType( type );
}

void UIStackLayout::applySizePolicyOnChilds() {
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

void UIStackLayout::setRowValign( const std::string& rowValign ) {
	if ( rowValign == "top" ) {
		setRowValign( UIStackLayout::RowValign::Top );
	} else if ( rowValign == "center" ) {
		setRowValign( UIStackLayout::RowValign::Center );
	} else if ( rowValign == "bottom" ) {
		setRowValign( UIStackLayout::RowValign::Bottom );
	}
}

std::string UIStackLayout::rowValignToStr( const RowValign& rowValign ) {
	switch ( rowValign ) {
		case UIStackLayout::RowValign::Top:
			return "top";
		case UIStackLayout::RowValign::Center:
			return "center";
		case UIStackLayout::RowValign::Bottom:
		default:
			return "bottom";
	}
}

void UIStackLayout::clearListeners() {
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

void UIStackLayout::listenParent() {
	clearListeners();

	mParentRef = getParent();
	mParentSizeChangeCb =
		mParentRef->addEventListener( Event::OnSizeChange, [this]( const Event* ) {
			if ( getLayoutWidthPolicy() == SizePolicy::WrapContent &&
				 getUISceneNode()->isUpdatingLayouts() &&
				 getParent()->getPixelsSize().getWidth() > 0 && mSize.x != getMatchParentWidth() ) {
				runOnMainThread( [this]() { setLayoutDirty(); } );
			}
		} );
	mParentCloseCb = mParentRef->addEventListener(
		Event::OnClose, [this]( const Event* ) { mParentRef = nullptr; } );
}

void UIStackLayout::onParentChange() {
	listenParent();
}

std::string UIStackLayout::getPropertyString( const PropertyDefinition* propertyDef,
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

std::vector<PropertyId> UIStackLayout::getPropertiesImplemented() const {
	auto props = UILayout::getPropertiesImplemented();
	auto local = { PropertyId::GravityOwner, PropertyId::RowValign };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIStackLayout::applyProperty( const StyleSheetProperty& attribute ) {
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

Uint32 UIStackLayout::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			tryUpdateLayout();
			return 1;
		}
	}

	return 0;
}

struct NodeLine {
	std::vector<UIWidget*> nodes;
	Float maxY{ 0 };
	Float width{ 0 };
};

void UIStackLayout::updateLayout() {
	if ( mPacking )
		return;
	mPacking = true;

	if ( !mVisible ) {
		setInternalPixelsSize( Sizef::Zero );
		notifyLayoutAttrChangeParent();
		mPacking = false;
		mDirtyLayout = false;
		return;
	}

	Sizef size( getSizeFromLayoutPolicy() );

	if ( getLayoutWidthPolicy() == SizePolicy::WrapContent )
		size.x = getMatchParentWidth();

	if ( size != getPixelsSize() )
		setInternalPixelsSize( size );

	applySizePolicyOnChilds();

	Float curX = mPaddingPx.Left;
	Node* child = mChild;
	std::vector<NodeLine> lines = { {} };
	Uint32 curLine = 0;
	bool addedLine = false;

	auto addLine = [&]() {
		curX = mPaddingPx.Left;
		++curLine;
		lines.push_back( { {} } );
		addedLine = true;
	};

	while ( NULL != child ) {
		if ( child->isWidget() && child->isVisible() ) {
			UIWidget* widget = static_cast<UIWidget*>( child );
			const Rectf& margin = widget->getLayoutPixelsMargin();

			if ( curX + margin.Left + widget->getPixelsSize().getWidth() >= mSize.getWidth() &&
				 !addedLine && !lines[curLine].nodes.empty() )
				addLine();

			addedLine = false;

			curX += eeceil( margin.Left );

			Vector2f pos( curX, mPaddingPx.Top );

			widget->setPixelsPosition( pos );

			curX += eeceil( widget->getPixelsSize().getWidth() + margin.Right );

			lines[curLine].nodes.push_back( widget );
			lines[curLine].width = curX;
			if ( widget->getLayoutHeightPolicy() != SizePolicy::MatchParent ) {
				lines[curLine].maxY = eeceil(
					eemax( lines[curLine].maxY, ( widget->getPixelsSize().getHeight() +
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
	for ( auto& line : lines ) {
		if ( curLine > 0 && line.maxY == 0 )
			line.maxY = lines[curLine - 1].maxY;
		height += line.maxY;
		totHeight += line.maxY;
	}
	totHeight += mPaddingPx.Bottom;

	curLine = 0;
	for ( const auto& line : lines ) {
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
				case UIStackLayout::RowValign::Center:
					pos.y = yDisplacement + maxY +
							eeceil( ( line.maxY - widget->getPixelsSize().getHeight() ) * 0.5f );
					break;
				case UIStackLayout::RowValign::Bottom:
					pos.y = yDisplacement + maxY + line.maxY - widget->getPixelsSize().getHeight() -
							widget->getLayoutPixelsMargin().Bottom;
					break;
				case UIStackLayout::RowValign::Top:
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
		 ( ( lines.size() == 1 && !lines[0].nodes.empty() ) ||
		   ( lines.size() == 2 && lines[1].nodes.empty() ) ) ) {
		setInternalPixelsWidth( curX );
		notifyLayoutAttrChangeParent();
	}

	if ( getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
		if ( totHeight != (int)getPixelsSize().getHeight() ) {
			setInternalPixelsHeight( totHeight );
			notifyLayoutAttrChangeParent();
		}
	}

	if ( getParent()->isUINode() &&
		 ( !getParent()->asType<UINode>()->ownsChildPosition() || isGravityOwner() ) ) {
		alignAgainstLayout();
	}

	mPacking = false;
	mDirtyLayout = false;
}

const UIStackLayout::RowValign& UIStackLayout::getRowValign() const {
	return mRowValign;
}

void UIStackLayout::setRowValign( const RowValign& rowValign ) {
	mRowValign = rowValign;
}

}} // namespace EE::UI
