#ifndef EE_UI_UISTYLE_HPP
#define EE_UI_UISTYLE_HPP

#include <eepp/graphics/fontstyleconfig.hpp>
#include <eepp/math/ease.hpp>
#include <eepp/ui/css/animationdefinition.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetstyle.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>
#include <eepp/ui/uistate.hpp>
#include <functional>
#include <set>

namespace EE { namespace Graphics {
class Font;
}} // namespace EE::Graphics

namespace EE { namespace UI { namespace CSS {
class StyleSheetPropertyAnimation;
}}} // namespace EE::UI::CSS

namespace EE { namespace UI {

class UIWidget;

class EE_API UIStyle : public UIState {
  public:
	static UIStyle* New( UIWidget* widget );

	explicit UIStyle( UIWidget* widget );

	virtual ~UIStyle();

	bool stateExists( const Uint32& state ) const;

	void load();

	void onStateChange();

	CSS::StyleSheetProperty getStatelessStyleSheetProperty( const Uint32& propertyId ) const;

	void setStyleSheetProperties( const CSS::StyleSheetProperties& properties );

	void setStyleSheetProperty( const CSS::StyleSheetProperty& property );

	bool hasTransition( const std::string& propertyName );

	CSS::StyleSheetPropertyAnimation* getAnimation( const CSS::PropertyDefinition* propertyDef );

	bool hasAnimation( const CSS::PropertyDefinition* propertyDef );

	CSS::TransitionDefinition getTransition( const std::string& propertyName );

	const bool& isChangingState() const;

	CSS::StyleSheetVariable getVariable( const std::string& variable );

	bool getForceReapplyProperties() const;

	void setForceReapplyProperties( bool forceReapplyProperties );

	bool getDisableAnimations() const;

	void setDisableAnimations( bool disableAnimations );

  protected:
	UIWidget* mWidget;
	CSS::StyleSheetStyleVector mCacheableStyles;
	CSS::StyleSheetStyleVector mNoncacheableStyles;
	CSS::StyleSheetStyle mElementStyle;
	CSS::StyleSheetProperties mProperties;
	CSS::StyleSheetVariables mVariables;
	std::vector<CSS::StyleSheetProperty> mTransitionProperties;
	std::vector<CSS::StyleSheetProperty> mAnimationProperties;
	CSS::TransitionsMap mTransitions;
	CSS::AnimationsMap mAnimations;
	std::set<UIWidget*> mRelatedWidgets;
	std::set<UIWidget*> mSubscribedWidgets;
	bool mChangingState;
	bool mForceReapplyProperties;
	bool mDisableAnimations;

	void tryApplyStyle( const CSS::StyleSheetStyle& style );

	void findVariables( const CSS::StyleSheetStyle& style );

	void applyVarValues( CSS::StyleSheetStyle& style );

	void setVariableFromValue( CSS::StyleSheetProperty& property, const std::string& value );

	void updateState();

	void subscribeNonCacheableStyles();

	void unsubscribeNonCacheableStyles();

	void subscribeRelated( UIWidget* widget );

	void unsubscribeRelated( UIWidget* widget );

	void removeFromSubscribedWidgets( UIWidget* widget );

	void removeRelatedWidgets();

	void applyStyleSheetProperty( const CSS::StyleSheetProperty& property,
								  CSS::StyleSheetProperties& prevProperties );

	void updateAnimationsPlayState();

	void updateAnimations();

	void startAnimations( const CSS::AnimationsMap& animations );

	void removeAllAnimations();

	void removeAnimation( const CSS::PropertyDefinition* propertyDefinition,
						  const Uint32& propertyIndex );

	std::string varToVal( const std::string& varDef );
};

}} // namespace EE::UI

#endif
