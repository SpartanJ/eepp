#include "colorschemetranslator.hpp"

#include <algorithm>
#include <cmath>

using namespace EE;
using namespace EE::UI;

namespace ecode {

static const auto BASE_UI_THEME = R"css(

:root {
	--inherit-base-theme: true;

	/* Main Colors */
	--primary: type;
	--back: background;
	--font: ui_font;
	--font-highlight: type;
	--font-hint: ui_hint;
	--font-selected-pressed: line_highlight;

	/* Button styling */
	--button-back: line_highlight;
	--button-border: line_number;

	/* List & Separators */
	--list-back: widget_background;
	--separator: line_highlight;
	--item-hover: selection;

	/* Sliders */
	--slider-back: line_number;
	--slider-button: line_highlight;
	--slider-border: line_number2;

	/* Scrollbars */
	--scrollbar-border: line_highlight;
	--scrollbar-button: suggestion_scrollbar;
	--scrollbar-hback-hover: selection_region;

	/* Tabs */
	--tab-back: line_highlight;
	--tab-line: line_number;
	--tab-active: background;
	--tab-hover: selection;
	--tab-close: line_number2;
	--tab-close-hover: error;
	--tab-font-active: type;
	--tab-font-inactive: line_number2;

	/* Icons */
	--icon: normal;
	--icon-active: type;
	--icon-back-hover: selection;
	--icon-line: line_number2;
	--icon-line-hover: text;
	--icon-back-alert: error;

	/* Menus */
	--menu-back: widget_background;
	--menu-font: ui_font;
	--menu-border: line_number;
	--menu-font-active: widget_background;
	--menu-font-disabled: line_number2;

	/* Floating & Terminal */
	--floating-icon: line_number2;
	--term-back-color: background;
	--term-font-color: text;

	/* Highlights & Status */
	--highlight-primary: minimap_selection;
	--disabled-color: line_number2;
	--disabled-border: line_break_column;

	--theme-error: error;
	--theme-warning: warning;
	--theme-success: function;
}

)css";

std::string ColorSchemeTranslator::fromSyntaxColorScheme( const SyntaxColorScheme& colorScheme ) {
	std::string output( BASE_UI_THEME );

	const Color background( colorScheme.getEditorColor( SyntaxStyleTypes::Background ), 255 );
	const Color foreground = getFontColor( colorScheme );
	// Hints start at 65% of the UI text color, with the same minimum contrast as body text.
	const Color hint =
		ensureTextContrast( Color::blend( Color( foreground, 166 ), background ), background );
	String::replaceAll( output, "ui_font", foreground.toHexString( true ) );
	String::replaceAll( output, "ui_hint", hint.toHexString( true ) );

	String::replaceAll( output, "widget_background",
						colorScheme.getEditorSyntaxStyle( SyntaxStyleTypes::WidgetBackground )
							.color.toHexString( true ) );

	String::replaceAll( output, "gutter_background",
						colorScheme.getEditorSyntaxStyle( SyntaxStyleTypes::GutterBackground )
							.color.toHexString( true ) );

	String::replaceAll( output, "background",
						colorScheme.getEditorSyntaxStyle( SyntaxStyleTypes::Background )
							.color.toHexString( true ) );

	String::replaceAll(
		output, "text",
		colorScheme.getEditorSyntaxStyle( SyntaxStyleTypes::Text ).color.toHexString( true ) );

	String::replaceAll(
		output, "caret",
		colorScheme.getEditorSyntaxStyle( SyntaxStyleTypes::Caret ).color.toHexString( true ) );

	String::replaceAll( output, "line_highlight",
						colorScheme.getEditorSyntaxStyle( SyntaxStyleTypes::LineHighlight )
							.color.toHexString( true ) );

	String::replaceAll(
		output, "selection_region",
		colorScheme.getEditorColor( SyntaxStyleTypes::SelectionRegion ).toHexString( true ) );

	String::replaceAll(
		output, "minimap_selection",
		colorScheme.getEditorColor( SyntaxStyleTypes::MinimapSelection ).toHexString( true ) );

	String::replaceAll(
		output, "selection",
		colorScheme.getEditorColor( SyntaxStyleTypes::Selection ).toHexString( true ) );

	String::replaceAll(
		output, "line_number2",
		colorScheme.getEditorColor( SyntaxStyleTypes::LineNumber2 ).toHexString( true ) );

	String::replaceAll(
		output, "line_number",
		colorScheme.getEditorColor( SyntaxStyleTypes::LineNumber ).toHexString( true ) );

	String::replaceAll(
		output, "suggestion_scrollbar",
		colorScheme.getEditorColor( SyntaxStyleTypes::SuggestionScrollbar ).toHexString( true ) );

	String::replaceAll(
		output, "line_break_column",
		colorScheme.getEditorColor( SyntaxStyleTypes::LineBreakColumn ).toHexString( true ) );

	String::replaceAll(
		output, "minimap_visible_area",
		colorScheme.getEditorColor( SyntaxStyleTypes::MinimapVisibleArea ).toHexString( true ) );

	String::replaceAll( output, "error",
						colorScheme.getEditorColor( SyntaxStyleTypes::Error ).toHexString( true ) );

	String::replaceAll(
		output, "warning",
		colorScheme.getEditorColor( SyntaxStyleTypes::Warning ).toHexString( true ) );

	String::replaceAll(
		output, "notice",
		colorScheme.getEditorColor( SyntaxStyleTypes::Notice ).toHexString( true ) );

	String::replaceAll(
		output, "whitespace",
		colorScheme.getEditorColor( SyntaxStyleTypes::Whitespace ).toHexString( true ) );

	// Syntax Styles (Using getSyntaxStyle)
	String::replaceAll(
		output, "parameter",
		colorScheme.getSyntaxStyle( SyntaxStyleTypes::Parameter ).color.toHexString( true ) );

	String::replaceAll(
		output, "keyword",
		colorScheme.getSyntaxStyle( SyntaxStyleTypes::Keyword ).color.toHexString( true ) );

	String::replaceAll(
		output, "type",
		colorScheme.getSyntaxStyle( SyntaxStyleTypes::Type ).color.toHexString( true ) );

	String::replaceAll(
		output, "symbol",
		colorScheme.getSyntaxStyle( SyntaxStyleTypes::Symbol ).color.toHexString( true ) );

	String::replaceAll(
		output, "normal",
		colorScheme.getSyntaxStyle( SyntaxStyleTypes::Normal ).color.toHexString( true ) );

	String::replaceAll(
		output, "function",
		colorScheme.getSyntaxStyle( SyntaxStyleTypes::Function ).color.toHexString( true ) );

	return output;
}

ColorSchemePreference
ColorSchemeTranslator::getColorSchemePreference( const SyntaxColorScheme& colorScheme ) {
	return colorScheme.getEditorColor( SyntaxStyleTypes::Background ).perceivedLuminance() > 128
			   ? ColorSchemePreference::Light
			   : ColorSchemePreference::Dark;
}

Color ColorSchemeTranslator::getFontColor( const SyntaxColorScheme& colorScheme ) {
	Color foreground = colorScheme.getEditorColor( SyntaxStyleTypes::Text );
	// The shared UI foreground is also used on list/menu and button/tab surfaces.
	for ( auto type : { SyntaxStyleTypes::Background, SyntaxStyleTypes::WidgetBackground,
						SyntaxStyleTypes::LineHighlight } ) {
		foreground =
			ensureTextContrast( foreground, Color( colorScheme.getEditorColor( type ), 255 ) );
	}
	return foreground;
}

Color ColorSchemeTranslator::ensureTextContrast( Color foreground, const Color& background ) {
	const auto luminance = []( const Color& color ) {
		const auto linear = []( Uint8 channel ) {
			const double value = channel / 255.0;
			return value <= 0.04045 ? value / 12.92 : std::pow( ( value + 0.055 ) / 1.055, 2.4 );
		};
		return 0.2126 * linear( color.r ) + 0.7152 * linear( color.g ) + 0.0722 * linear( color.b );
	};
	const double backgroundLuminance = luminance( background );
	const auto contrast = [backgroundLuminance, &luminance]( const Color& color ) {
		const double foregroundLuminance = luminance( color );
		return ( std::max( foregroundLuminance, backgroundLuminance ) + 0.05 ) /
			   ( std::min( foregroundLuminance, backgroundLuminance ) + 0.05 );
	};
	// Account for translucent palette colors when deciding whether to keep them unchanged.
	if ( contrast( Color::blend( foreground, background ) ) >= 4.5 )
		return foreground;
	foreground = Color::blend( foreground, background );
	const Color target =
		contrast( Color::White ) > contrast( Color::Black ) ? Color::White : Color::Black;

	// Find the smallest correction toward a contrasting neutral, keeping the original hue.
	// This runs only when generating a theme, with at most eight search iterations.
	int low = 0;
	int high = 255;
	while ( low < high ) {
		const int alpha = ( low + high ) / 2;
		const Color corrected =
			Color::blend( Color( target, static_cast<Uint8>( alpha ) ), foreground );
		if ( contrast( corrected ) >= 4.5 )
			high = alpha;
		else
			low = alpha + 1;
	}
	return Color::blend( Color( target, static_cast<Uint8>( low ) ), foreground );
}

} // namespace ecode
