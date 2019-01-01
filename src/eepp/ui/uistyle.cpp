#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/fontmanager.hpp>

namespace EE { namespace UI {

UIStyle * EE::UI::UIStyle::New( UIWidget * widget ) {
	return eeNew( UIStyle, ( widget ) );
}

UIStyle::UIStyle( UIWidget * widget ) :
	mWidget( widget )
{
	load();
}

UIStyle::~UIStyle()
{}

bool UIStyle::stateExists( const EE::Uint32 & state  ) const {
	return mStates.find( state ) != mStates.end();
}

void UIStyle::addAttribute( int state, NodeAttribute attribute ) {
	mStates[ state ][ attribute.getName() ] = attribute;
}

void UIStyle::load() {
	mStates.clear();

	UISceneNode * uiSceneNode = mWidget->getSceneNode()->isUISceneNode() ? static_cast<UISceneNode*>( mWidget->getSceneNode() ) : NULL;

	if ( NULL != uiSceneNode ) {
		CSS::StyleSheet& styleSheet = uiSceneNode->getStyleSheet();

		if ( !styleSheet.isEmpty() ) {
			addStyleSheetProperties( StateFlagNormal, styleSheet.getElementProperties( mWidget, "" ) );
			addStyleSheetProperties( StateFlagFocus, styleSheet.getElementProperties( mWidget, "focus" ) );
			addStyleSheetProperties( StateFlagSelected, styleSheet.getElementProperties( mWidget, "selected" ) );
			addStyleSheetProperties( StateFlagHover, styleSheet.getElementProperties( mWidget, "hover" ) );
			addStyleSheetProperties( StateFlagPressed, styleSheet.getElementProperties( mWidget, "pressed" ) );
			addStyleSheetProperties( StateFlagSelectedHover, styleSheet.getElementProperties( mWidget, "selectedhover" ) );
			addStyleSheetProperties( StateFlagSelectedPressed, styleSheet.getElementProperties( mWidget, "selectedpressed" ) );
			addStyleSheetProperties( StateFlagDisabled, styleSheet.getElementProperties( mWidget, "disabled" ) );
		}
	}
}

void UIStyle::addStyleSheetProperties( int state, const CSS::StyleSheetProperties& properties ) {
	if ( !properties.empty() ) {
		for ( auto it = properties.begin(); it != properties.end(); ++it ) {
			CSS::StyleSheetProperty property = it->second;

			addAttribute( state, NodeAttribute( property.getName(), property.getValue() ) );
		}
	}
}

void UIStyle::onStateChange() {
	if ( NULL != mWidget && stateExists( mCurrentState ) ) {
		AttributesMap& attrs = mStates[ mCurrentState ];

		if ( !attrs.empty() ) {
			mWidget->beginAttributesTransaction();

			for ( auto it = attrs.begin(); it != attrs.end(); ++it ) {
				NodeAttribute& nodeAttr = it->second;

				mWidget->setAttribute( nodeAttr, mCurrentState );
			}

			mWidget->endAttributesTransaction();
		}
	}
}

NodeAttribute UIStyle::getAttribute( const Uint32& state, std::vector<std::string> attributeNames ) const {
	if ( !attributeNames.empty() && stateExists( state ) ) {
		const AttributesMap& attributesMap = mStates.at( state );

		for ( size_t i = 0; i < attributeNames.size(); i++ ) {
			const std::string& name = attributeNames[i];
			auto attributeFound = attributesMap.find( name );

			if ( attributeFound != attributesMap.end() ) {
				return attributeFound->second;
			}

		}
	}

	return NodeAttribute();
}

Font * UIStyle::getFontFamily( const Uint32& state ) const {
	NodeAttribute attribute = getAttribute( state, { "fontfamily", "fontname" } );

	if ( !attribute.isEmpty() ) {
		return FontManager::instance()->getByName( attribute.asString() );
	}

	return UIThemeManager::instance()->getDefaultFont();
}

int UIStyle::getFontCharacterSize(const Uint32& state, const int& defaultValue ) const {
	NodeAttribute attribute = getAttribute( state, { "textsize", "fontsize", "charactersize" } );

	if ( !attribute.isEmpty() ) {
		return attribute.asDpDimensionI();
	}

	return defaultValue;
}

Color UIStyle::getTextColor( const Uint32& state ) const {
	NodeAttribute attribute = getAttribute( state, { "textcolor" } );

	return attribute.isEmpty() ? Color::White : attribute.asColor();
}

Color UIStyle::getTextShadowColor( const Uint32& state ) const {
	NodeAttribute attribute = getAttribute( state, { "textshadowcolor" } );

	return attribute.isEmpty() ? Color::Black : attribute.asColor();
}

Uint32 UIStyle::getTextStyle( const Uint32& state ) const {
	NodeAttribute attribute = getAttribute( state, { "textstyle" } );

	return attribute.isEmpty() ? 0 : attribute.asFontStyle();
}

Float UIStyle::getFontOutlineThickness( const Uint32& state ) const {
	NodeAttribute attribute = getAttribute( state, { "fontoutlinethickness" } );

	return attribute.isEmpty() ? 0.f : attribute.asFloat();
}

Color UIStyle::getFontOutlineColor( const Uint32& state ) const {
	NodeAttribute attribute = getAttribute( state, { "fontoutlinecolor" } );

	return attribute.isEmpty() ? Color::White : attribute.asColor();
}

FontStyleConfig UIStyle::getFontStyleConfig( const Uint32& state ) const {
	FontStyleConfig fontStyleConfig;
	fontStyleConfig.Font = getFontFamily( state );
	fontStyleConfig.CharacterSize = getFontCharacterSize( state );
	fontStyleConfig.Style = getTextStyle( state );
	fontStyleConfig.FontColor = getTextColor( state );
	fontStyleConfig.ShadowColor = getTextShadowColor( state );
	fontStyleConfig.OutlineColor = getFontOutlineColor( state );
	fontStyleConfig.OutlineThickness = getFontOutlineThickness( state );
	return fontStyleConfig;
}

void UIStyle::updateState() {
	for ( int i = StateFlagCount - 1; i >= 0; i-- ) {
		if ( ( mState & getStateFlag(i) ) == getStateFlag(i) ) {
			if ( stateExists( getStateFlag(i) ) ) {
				if ( mCurrentState != getStateFlag(i) ) {
					mCurrentState = getStateFlag(i);
					onStateChange();
					return;
				}
			}
		}
	}

	Uint32 currentState = mCurrentState;

	mCurrentState = StateFlagNormal;

	if ( currentState != StateFlagNormal ) {
		onStateChange();
	}
}

}}
