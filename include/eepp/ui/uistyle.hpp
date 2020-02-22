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

	CSS::TransitionDefinition getTransition( const std::string& propertyName );

	const bool& isChangingState() const;

	CSS::StyleSheetVariable getVariable( const std::string& variable );

  protected:
	UIWidget* mWidget;
	CSS::StyleSheetStyleVector mCacheableStyles;
	CSS::StyleSheetStyleVector mNoncacheableStyles;
	CSS::StyleSheetStyle mElementStyle;
	CSS::StyleSheetProperties mProperties;
	CSS::StyleSheetVariables mVariables;
	std::vector<CSS::StyleSheetProperty> mTransitionAttributes;
	std::vector<CSS::StyleSheetProperty> mAnimationAttributes;
	CSS::TransitionsMap mTransitions;
	CSS::AnimationsMap mAnimations;
	std::set<UIWidget*> mRelatedWidgets;
	std::set<UIWidget*> mSubscribedWidgets;
	bool mChangingState;

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
};

}} // namespace EE::UI

#endif
