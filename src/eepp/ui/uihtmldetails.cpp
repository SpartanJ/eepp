#include <eepp/scene/keyevent.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uihtmldetails.hpp>
#include <eepp/ui/uihtmlliststyle.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextnode.hpp>

namespace EE { namespace UI {

static bool htmlBoolAttributeIsTrue( const StyleSheetProperty& property ) {
	if ( property.value().empty() )
		return true;

	std::string value( property.value() );
	String::toLowerInPlace( value );
	if ( value == property.getName() )
		return true;
	if ( value == "false" || value == "0" || value == "no" )
		return false;
	return property.asBool();
}

UIHTMLDetails* UIHTMLDetails::New() {
	return eeNew( UIHTMLDetails, () );
}

UIHTMLDetails::UIHTMLDetails() : UIRichText( "details" ) {}

Uint32 UIHTMLDetails::getType() const {
	return UI_TYPE_HTML_DETAILS;
}

bool UIHTMLDetails::isType( const Uint32& type ) const {
	return UIHTMLDetails::getType() == type ? true : UIRichText::isType( type );
}

void UIHTMLDetails::loadFromXmlNode( const pugi::xml_node& node ) {
	UIRichText::loadFromXmlNode( node );
	syncDetailsChildrenVisibility();
}

bool UIHTMLDetails::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	if ( attribute.getPropertyDefinition()->getPropertyId() == PropertyId::Open ) {
		setOpen( htmlBoolAttributeIsTrue( attribute ) );
		return true;
	}

	return UIRichText::applyProperty( attribute );
}

std::string UIHTMLDetails::getPropertyString( const PropertyDefinition* propertyDef,
											  const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Open:
			return mOpen ? "true" : "false";
		default:
			return UIRichText::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIHTMLDetails::getPropertiesImplemented() const {
	auto props = UIRichText::getPropertiesImplemented();
	props.push_back( PropertyId::Open );
	return props;
}

void UIHTMLDetails::updateLayout() {
	syncDetailsChildrenVisibility();
	UIRichText::updateLayout();
}

bool UIHTMLDetails::isOpen() const {
	return mOpen;
}

void UIHTMLDetails::setOpen( bool open ) {
	if ( mOpen == open )
		return;

	mOpen = open;
	syncDetailsChildrenVisibility();
	markSummaryMarkersDirty();
	sendCommonEvent( Event::OnToggle );
	setLayoutDirty( LayoutInvalidation::ParentReplacedFormatting );
	invalidateDraw();
}

UIHTMLSummary* UIHTMLDetails::findSummaryChild() const {
	Node* child = getFirstChild();
	while ( child ) {
		if ( child->isType( UI_TYPE_HTML_SUMMARY ) ) {
			auto* summary = child->asType<UIHTMLSummary>();
			if ( summary != mAutoSummary )
				return summary;
		}
		child = child->getNextNode();
	}

	return mAutoSummary && mAutoSummary->getParent() == this ? mAutoSummary : nullptr;
}

bool UIHTMLDetails::isActiveSummary( const UIHTMLSummary* summary ) const {
	return summary != nullptr && findSummaryChild() == summary;
}

void UIHTMLDetails::markSummaryMarkersDirty() {
	Node* child = getFirstChild();
	while ( child ) {
		if ( child->isType( UI_TYPE_HTML_SUMMARY ) )
			child->asType<UIHTMLSummary>()->setDisclosureMarkerDirty();
		child = child->getNextNode();
	}
}

void UIHTMLDetails::ensureAutoSummary() {
	Node* child = getFirstChild();
	while ( child ) {
		if ( child->isType( UI_TYPE_HTML_SUMMARY ) && child != mAutoSummary )
			return;
		child = child->getNextNode();
	}

	if ( mAutoSummary && mAutoSummary->getParent() == this )
		return;

	mAutoSummary = UIHTMLSummary::New();
	mAutoSummary->setParent( this );
	mAutoSummary->toBack();

	auto* text = UITextNode::New();
	text->setText( "Details" );
	text->setParent( mAutoSummary );
}

void UIHTMLDetails::syncDetailsChildrenVisibility() {
	if ( mSyncingDetailsChildren )
		return;

	mSyncingDetailsChildren = true;
	ensureAutoSummary();
	UIHTMLSummary* activeSummary = findSummaryChild();

	for ( auto it = mForcedHiddenChildren.begin(); it != mForcedHiddenChildren.end(); ) {
		if ( it->first->getParent() != this )
			it = mForcedHiddenChildren.erase( it );
		else
			++it;
	}

	Node* child = getFirstChild();
	while ( child ) {
		Node* next = child->getNextNode();
		if ( child == mAutoSummary && activeSummary != mAutoSummary ) {
			mForcedHiddenChildren.erase( child );
			child->setVisible( false );
		} else if ( child == activeSummary ) {
			auto forcedIt = mForcedHiddenChildren.find( child );
			if ( forcedIt != mForcedHiddenChildren.end() ) {
				child->setVisible( forcedIt->second );
				mForcedHiddenChildren.erase( forcedIt );
			} else if ( !child->isVisible() ) {
				child->setVisible( true );
			}
		} else if ( mOpen ) {
			auto forcedIt = mForcedHiddenChildren.find( child );
			if ( forcedIt != mForcedHiddenChildren.end() ) {
				child->setVisible( forcedIt->second );
				mForcedHiddenChildren.erase( forcedIt );
			}
		} else {
			if ( mForcedHiddenChildren.find( child ) == mForcedHiddenChildren.end() )
				mForcedHiddenChildren[child] = child->isVisible();
			child->setVisible( false );
		}
		child = next;
	}
	mSyncingDetailsChildren = false;
}

void UIHTMLDetails::onChildCountChange( Node* child, const bool& removed ) {
	if ( removed ) {
		mForcedHiddenChildren.erase( child );
		if ( child == mAutoSummary && !mSyncingDetailsChildren )
			mAutoSummary = nullptr;
	}

	UIRichText::onChildCountChange( child, removed );
	if ( !mSyncingDetailsChildren )
		syncDetailsChildrenVisibility();
}

UIHTMLSummary* UIHTMLSummary::New() {
	return eeNew( UIHTMLSummary, () );
}

UIHTMLSummary::UIHTMLSummary() : UIRichText( "summary" ) {
	mDisplay = CSSDisplay::ListItem;
	setFlags( UI_CREATING_NODE );
	setStyleSheetProperty( StyleSheetProperty( "cursor", "pointer" ) );
	setStyleSheetProperty( StyleSheetProperty( "padding-left", "20dp" ) );
	setStyleSheetProperty( StyleSheetProperty( "list-style-type", "disclosure-closed" ) );
	unsetFlags( UI_CREATING_NODE );
	setFlags( UI_TAB_FOCUSABLE );
}

Uint32 UIHTMLSummary::getType() const {
	return UI_TYPE_HTML_SUMMARY;
}

bool UIHTMLSummary::isType( const Uint32& type ) const {
	return UIHTMLSummary::getType() == type ? true : UIRichText::isType( type );
}

void UIHTMLSummary::draw() {
	UIRichText::draw();
	if ( mVisible && 0.f != mAlpha ) {
		const CSSListStyleType markerType = getDisclosureMarkerType();
		if ( UIHTMLListStyle::isPrimitiveMarker( markerType ) ) {
			UIHTMLListStyle::drawPrimitiveMarker( markerType, mScreenPos, mPaddingPx,
												  mRichText.getFontStyleConfig() );
		} else if ( mListMarkerText && !mListMarkerText->getString().empty() ) {
			const Float fontSize = mRichText.getFontStyleConfig().CharacterSize;
			const Float offset = 0.25f * fontSize;
			const Float markerX =
				mScreenPos.x + mPaddingPx.Left - mListMarkerText->getTextWidth() - offset;
			mListMarkerText->draw( markerX, mScreenPos.y + mPaddingPx.Top, Vector2f::One, 0.f,
								   getBlendMode() );
		}
	}
}

void UIHTMLSummary::setDisclosureMarkerDirty() {
	invalidateDraw();
}

UIHTMLDetails* UIHTMLSummary::findParentDetails() const {
	Node* parent = getParent();
	while ( parent ) {
		if ( parent->isType( UI_TYPE_HTML_DETAILS ) )
			return parent->asType<UIHTMLDetails>();
		parent = parent->getParent();
	}
	return nullptr;
}

bool UIHTMLSummary::toggleParentDetails() {
	UIHTMLDetails* details = findParentDetails();
	if ( details && details->isActiveSummary( this ) ) {
		details->setOpen( !details->isOpen() );
		return true;
	}
	return false;
}

void UIHTMLSummary::setListStyleType( CSSListStyleType type ) {
	if ( mListStyleType != type ) {
		mListStyleType = type;
		syncDefaultListStylePadding();
		invalidateListMarker();
	}
}

bool UIHTMLSummary::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::ListStyleType:
			setListStyleType( CSSListStyleTypeHelper::fromString( attribute.value() ) );
			return true;
		case PropertyId::PaddingLeft:
			if ( !isCreatingNode() && !mApplyingDefaultListStylePadding &&
				 attribute.getSpecificity() != 0 )
				mUsingDefaultListStylePadding = false;
			break;
		default:
			break;
	}

	return UIRichText::applyProperty( attribute );
}

std::string UIHTMLSummary::getPropertyString( const PropertyDefinition* propertyDef,
											  const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::ListStyleType:
			return CSSListStyleTypeHelper::toString( mListStyleType );
		default:
			return UIRichText::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIHTMLSummary::getPropertiesImplemented() const {
	auto props = UIRichText::getPropertiesImplemented();
	props.push_back( PropertyId::ListStyleType );
	return props;
}

CSSListStyleType UIHTMLSummary::getDisclosureMarkerType() const {
	UIHTMLDetails* details = findParentDetails();
	if ( mListStyleType == CSSListStyleType::DisclosureClosed && details && details->isOpen() )
		return CSSListStyleType::DisclosureOpen;
	return mListStyleType;
}

int UIHTMLSummary::countPrecedingSummarySiblings() const {
	int count = 0;
	Node* prev = getPrevNode();
	while ( prev ) {
		if ( prev->isWidget() && prev->asType<UIWidget>()->getElementTag() == "summary" )
			count++;
		prev = prev->getPrevNode();
	}
	return count;
}

void UIHTMLSummary::invalidateListMarker() {
	if ( !UIHTMLListStyle::isTextMarker( mListStyleType ) ) {
		mListMarkerText.reset();
	} else {
		String markerStr =
			UIHTMLListStyle::getTextMarkerString( mListStyleType, countPrecedingSummarySiblings() );
		if ( !markerStr.empty() ) {
			if ( !mListMarkerText )
				mListMarkerText = std::make_unique<Text>();
			mListMarkerText->setString( markerStr );
			mListMarkerText->setStyleConfig( mRichText.getFontStyleConfig() );
		} else {
			mListMarkerText.reset();
		}
	}
	invalidateDraw();
}

void UIHTMLSummary::syncDefaultListStylePadding() {
	if ( !mUsingDefaultListStylePadding )
		return;

	mApplyingDefaultListStylePadding = true;
	setStyleSheetProperty( StyleSheetProperty(
		"padding-left", mListStyleType == CSSListStyleType::None ? "0dp" : "20dp" ) );
	mApplyingDefaultListStylePadding = false;
}

Uint32 UIHTMLSummary::onMouseClick( const Vector2i& position, const Uint32& flags ) {
	if ( toggleParentDetails() )
		return 1;
	return UIRichText::onMouseClick( position, flags );
}

Uint32 UIHTMLSummary::onKeyDown( const KeyEvent& event ) {
	if ( event.getKeyCode() == KEY_RETURN || event.getKeyCode() == KEY_KP_ENTER ||
		 event.getKeyCode() == KEY_SPACE ) {
		if ( toggleParentDetails() )
			return 1;
	}
	return UIRichText::onKeyDown( event );
}

void UIHTMLSummary::onFontChanged() {
	UIRichText::onFontChanged();
	invalidateListMarker();
}

void UIHTMLSummary::onFontStyleChanged() {
	UIRichText::onFontStyleChanged();
	invalidateListMarker();
}

}} // namespace EE::UI
