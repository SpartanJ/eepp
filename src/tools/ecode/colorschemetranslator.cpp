#include "colorschemetranslator.hpp"

using namespace EE;

namespace ecode {

static const auto BASE_UI_THEME = R"css(

:root {
	--inherit-base-theme: true;

	/* Main Colors */
	--primary: type;
	--back: background;
	--font: text;
	--font-highlight: type;
	--font-hint: line_number2;

	/* Button styling */
	--button-back: line_highlight;
	--button-border: line_number;

	/* List & Separators */
	--list-back: widget_background;
	--separator: line_highlight;
	--item-hover: selection;
	--list-row-active: widget_background;

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
	--icon: symbol;
	--icon-active: type;
	--icon-back-hover: selection;
	--icon-line: line_number2;
	--icon-line-hover: text;
	--icon-back-alert: error;

	/* Menus */
	--menu-back: widget_background;
	--menu-font: text;
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

	droppable-hovering-color: minimap_visible_area;
}

)css";

std::string ColorSchemeTranslator::fromSyntaxColorScheme( const SyntaxColorScheme& colorScheme ) {
	std::string output( BASE_UI_THEME );

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
		output, "function",
		colorScheme.getSyntaxStyle( SyntaxStyleTypes::Function ).color.toHexString( true ) );

	return output;
}

} // namespace ecode
