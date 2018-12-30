#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiscenenode.hpp>

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

bool UIStyle::stateExists( const EE::Uint32 & state  ) {
	return mStates.find( state ) != mStates.end();
}

void UIStyle::addAttribute( int state, NodeAttribute attribute ) {
	mStates[ state ][ attribute.getName() ] = attribute;
}

void UIStyle::load() {
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

			addAttribute( state, NodeAttribute( property.name, property.value ) );
		}
	}
}

void UIStyle::onStateChange() {
	if ( NULL != mWidget && stateExists( mCurrentState ) ) {
		AttributesMap& attrs = mStates[ mCurrentState ];

		for ( auto it = attrs.begin(); it != attrs.end(); ++it ) {
			NodeAttribute& nodeAttr = it->second;

			mWidget->setAttribute( nodeAttr, mCurrentState );
		}
	}
}

}}
