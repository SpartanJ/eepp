#ifndef EE_UI_CSS_PROPERTYDEFINITION_HPP
#define EE_UI_CSS_PROPERTYDEFINITION_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>

namespace EE { namespace UI { namespace CSS {

/* Shorthands:
 *
 * background
 * foreground
 * margin
 * layout_margin
 * padding
 * background-position
 * rotation-origin-point
 * scale-origin-point
 * */

enum class PropertyId : Uint32 {
	Id = String::hash( "id" ),
	Class = String::hash( "class" ),
	X = String::hash( "x" ),
	Y = String::hash( "y" ),
	Width = String::hash( "width" ),
	Height = String::hash( "height" ),
	BackgroundColor = String::hash( "background-color" ),
	BackgroundImage = String::hash( "background-image" ),
	BackgroundPositionX = String::hash( "background-position-x" ),
	BackgroundPositionY = String::hash( "background-position-y" ),
	BackgroundRepeat = String::hash( "background-repeat" ),
	BackgroundSize = String::hash( "background-size" ),
	ForegroundColor = String::hash( "foreground-color" ),
	ForegroundImage = String::hash( "foreground-image" ),
	ForegroundPositionX = String::hash( "foreground-position-x" ),
	ForegroundPositionY = String::hash( "foreground-position-y" ),
	ForegroundRepeat = String::hash( "foreground-repeat" ),
	ForegroundSize = String::hash( "foreground-size" ),
	ForegroundRadius = String::hash( "foreground-radius" ),
	BorderColor = String::hash( "border-color" ),
	BorderWidth = String::hash( "border-width" ),
	BorderRadius = String::hash( "border-radius" ),
	Visible = String::hash( "visible" ),
	Enabled = String::hash( "enabled" ),
	Theme = String::hash( "theme" ),
	Skin = String::hash( "skin" ),
	SkinColor = String::hash( "skin-color" ),
	Gravity = String::hash( "gravity" ),
	Flags = String::hash( "flags" ),
	MarginLeft = String::hash( "margin-left" ),
	MarginRight = String::hash( "margin-right" ),
	MarginTop = String::hash( "margin-top" ),
	MarginBottom = String::hash( "margin-bottom" ),
	Tooltip = String::hash( "tooltip" ),
	LayoutWeight = String::hash( "layout-weight" ),
	LayoutGravity = String::hash( "layout-gravity" ),
	LayoutWidth = String::hash( "layout-width" ),
	LayoutHeight = String::hash( "layout-height" ),
	LayoutToLeftOf = String::hash( "layout_to_left_of" ),
	LayoutToRightOf = String::hash( "layout_to_right_of" ),
	LayoutToTopOf = String::hash( "layout_to_top_of" ),
	LayoutToBottomOf = String::hash( "layout_to_bottom_of" ),
	Clip = String::hash( "clip" ),
	Rotation = String::hash( "rotation" ),
	Scale = String::hash( "scale" ),
	RotationOriginPointX = String::hash( "rotation-origin-point-x" ),
	RotationOriginPointY = String::hash( "rotation-origin-point-y" ),
	ScaleOriginPointX = String::hash( "scale-origin-point-x" ),
	ScaleOriginPointY = String::hash( "scale-origin-point-y" ),
	BlendMode = String::hash( "blend-mode" ),
	PaddingLeft = String::hash( "padding-left" ),
	PaddingRight = String::hash( "padding-right" ),
	PaddingTop = String::hash( "padding-top" ),
	PaddingBottom = String::hash( "padding-bottom" ),
	Opacity = String::hash( "opacity" ),
	Cursor = String::hash( "cursor" ),
	Text = String::hash( "text" ),
	Color = String::hash( "color" ),
	ShadowColor = String::hash( "shadow-color" ),
	SelectedColor = String::hash( "selected-color" ),
	SelectionBackColor = String::hash( "selection-back-color" ),
	FontFamily = String::hash( "font-family" ),
	FontSize = String::hash( "font-size" ),
	FontStyle = String::hash( "font-style" ),
	TextDecoration = String::hash( "text-decoration" ),
	Wordwrap = String::hash( "wordwrap" ),
	TextStrokeWidth = String::hash( "text-stroke-width" ),
	TextStrokeColor = String::hash( "text-stroke-color" ),
	TextSelection = String::hash( "text-selection" ),
	TextAlign = String::hash( "text-align" )
};

class EE_API PropertyDefinition {
  public:
	enum RelativeTarget {
		None,
		ContainingBlockWidth,
		ContainingBlockHeight,
		FontSize,
		ParentFontSize,
		LineHeight
	};

	static PropertyDefinition* New( const std::string& name, const std::string& defaultValue,
							 const bool& inherited = false );

	PropertyDefinition( const std::string& name, const std::string& defaultValue,
						const bool& inherited = false );

	const std::string& getName() const;

	const Uint32& getId() const;

	const std::string& getDefaultValue() const;

	bool getInherited() const;

	const RelativeTarget& getRelativeTarget() const;

	PropertyDefinition& setRelativeTarget( const RelativeTarget& relativeTarget );

	PropertyDefinition& addAlias( const std::string& alias );

	bool isAlias( const std::string& alias ) const;

	bool isAlias( const Uint32& id ) const;

	bool isDefinition( const std::string& name ) const;

	bool isDefinition( const Uint32& id ) const;

  protected:
	std::string mName;
	Uint32 mId;
	std::vector<std::string> mAliases;
	std::vector<Uint32> mAliasesHash;
	std::string mDefaultValue;
	bool mInherited;
	RelativeTarget mRelativeTarget;
};

}}} // namespace EE::UI::CSS

#endif
