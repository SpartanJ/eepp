#ifndef EE_UI_DOC_SYNTAXCOLORSCHEME_HPP
#define EE_UI_DOC_SYNTAXCOLORSCHEME_HPP

#include <eepp/system/color.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/pack.hpp>
#include <vector>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

using SyntaxStyleType = String::HashType;

constexpr auto operator""_sst( const char* s, std::size_t ) noexcept {
	if constexpr ( std::is_same_v<SyntaxStyleType, std::string> )
		return s;
	else if constexpr ( std::is_same_v<SyntaxStyleType, String::HashType> )
		return String::hash( s );
	else
		return SyntaxStyleType{};
}

class SyntaxStyleTypes {
  public:
	static constexpr auto Normal = "normal"_sst;
	static constexpr auto Symbol = "symbol"_sst;
	static constexpr auto Comment = "comment"_sst;
	static constexpr auto Keyword = "keyword"_sst;
	static constexpr auto Keyword2 = "keyword2"_sst;
	static constexpr auto Keyword3 = "keyword3"_sst;
	static constexpr auto Number = "number"_sst;
	static constexpr auto Literal = "literal"_sst;
	static constexpr auto String = "string"_sst;
	static constexpr auto Operator = "operator"_sst;
	static constexpr auto Function = "function"_sst;
	static constexpr auto Link = "link"_sst;
	static constexpr auto LinkHover = "link_hover"_sst;
	static constexpr auto Background = "background"_sst;
	static constexpr auto Text = "text"_sst;
	static constexpr auto Caret = "caret"_sst;
	static constexpr auto Selection = "selection"_sst;
	static constexpr auto LineHighlight = "line_highlight"_sst;
	static constexpr auto LineNumber = "line_number"_sst;
	static constexpr auto LineNumber2 = "line_number2"_sst;
	static constexpr auto GutterBackground = "gutter_background"_sst;
	static constexpr auto Whitespace = "whitespace"_sst;
	static constexpr auto LineBreakColumn = "line_break_column"_sst;
	static constexpr auto MatchingBracket = "matching_bracket"_sst;
	static constexpr auto MatchingSelection = "matching_selection"_sst;
	static constexpr auto MatchingSearch = "matching_search"_sst;
	static constexpr auto Suggestion = "suggestion"_sst;
	static constexpr auto SuggestionScrollbar = "suggestion_scrollbar"_sst;
	static constexpr auto SuggestionSelected = "suggestion_selected"_sst;
	static constexpr auto Error = "error"_sst;
	static constexpr auto Warning = "warning"_sst;
	static constexpr auto Notice = "notice"_sst;
	static constexpr auto SelectionRegion = "selection_region"_sst;
	static constexpr auto MinimapBackground = "minimap_background"_sst;
	static constexpr auto MinimapCurrentLine = "minimap_current_line"_sst;
	static constexpr auto MinimapHover = "minimap_hover"_sst;
	static constexpr auto MinimapSelection = "minimap_selection"_sst;
	static constexpr auto MinimapHighlight = "minimap_highlight"_sst;
	static constexpr auto MinimapVisibleArea = "minimap_visible_area"_sst;

	template <typename Type> static bool needsToBeCached( const Type& style ) {
		if constexpr ( std::is_same_v<Type, std::string> ) {
			return false;
		} else if constexpr ( std::is_same_v<Type, String::HashType> ) {
			switch ( style ) {
				case SyntaxStyleTypes::Normal:
				case SyntaxStyleTypes::Symbol:
				case SyntaxStyleTypes::Comment:
				case SyntaxStyleTypes::Keyword:
				case SyntaxStyleTypes::Keyword2:
				case SyntaxStyleTypes::Keyword3:
				case SyntaxStyleTypes::Number:
				case SyntaxStyleTypes::Literal:
				case SyntaxStyleTypes::String:
				case SyntaxStyleTypes::Operator:
				case SyntaxStyleTypes::Function:
				case SyntaxStyleTypes::Link:
				case SyntaxStyleTypes::LinkHover:
				case SyntaxStyleTypes::Background:
				case SyntaxStyleTypes::Text:
				case SyntaxStyleTypes::Caret:
				case SyntaxStyleTypes::Selection:
				case SyntaxStyleTypes::LineHighlight:
				case SyntaxStyleTypes::LineNumber:
				case SyntaxStyleTypes::LineNumber2:
				case SyntaxStyleTypes::GutterBackground:
				case SyntaxStyleTypes::Whitespace:
				case SyntaxStyleTypes::LineBreakColumn:
				case SyntaxStyleTypes::MatchingBracket:
				case SyntaxStyleTypes::MatchingSelection:
				case SyntaxStyleTypes::MatchingSearch:
				case SyntaxStyleTypes::Suggestion:
				case SyntaxStyleTypes::SuggestionScrollbar:
				case SyntaxStyleTypes::SuggestionSelected:
				case SyntaxStyleTypes::Error:
				case SyntaxStyleTypes::Warning:
				case SyntaxStyleTypes::Notice:
				case SyntaxStyleTypes::SelectionRegion:
				case SyntaxStyleTypes::MinimapBackground:
				case SyntaxStyleTypes::MinimapCurrentLine:
				case SyntaxStyleTypes::MinimapHover:
				case SyntaxStyleTypes::MinimapSelection:
				case SyntaxStyleTypes::MinimapHighlight:
				case SyntaxStyleTypes::MinimapVisibleArea:
					return false;
				default:
					break;
			}
		}
		return true;
	}

	template <typename Type> static std::string toString( const Type& style ) {
		if constexpr ( std::is_same_v<Type, std::string> ) {
			return style;
		} else if constexpr ( std::is_same_v<Type, String::HashType> ) {
			switch ( style ) {
				case SyntaxStyleTypes::Normal:
					return "normal";
				case SyntaxStyleTypes::Symbol:
					return "symbol";
				case SyntaxStyleTypes::Comment:
					return "comment";
				case SyntaxStyleTypes::Keyword:
					return "keyword";
				case SyntaxStyleTypes::Keyword2:
					return "keyword2";
				case SyntaxStyleTypes::Keyword3:
					return "keyword3";
				case SyntaxStyleTypes::Number:
					return "number";
				case SyntaxStyleTypes::Literal:
					return "literal";
				case SyntaxStyleTypes::String:
					return "string";
				case SyntaxStyleTypes::Operator:
					return "operator";
				case SyntaxStyleTypes::Function:
					return "function";
				case SyntaxStyleTypes::Link:
					return "link";
				case SyntaxStyleTypes::LinkHover:
					return "link_hover";
				case SyntaxStyleTypes::Background:
					return "background";
				case SyntaxStyleTypes::Text:
					return "text";
				case SyntaxStyleTypes::Caret:
					return "caret";
				case SyntaxStyleTypes::Selection:
					return "selection";
				case SyntaxStyleTypes::LineHighlight:
					return "line_highlight";
				case SyntaxStyleTypes::LineNumber:
					return "line_number";
				case SyntaxStyleTypes::LineNumber2:
					return "line_number2";
				case SyntaxStyleTypes::GutterBackground:
					return "gutter_background";
				case SyntaxStyleTypes::Whitespace:
					return "whitespace";
				case SyntaxStyleTypes::LineBreakColumn:
					return "line_break_column";
				case SyntaxStyleTypes::MatchingBracket:
					return "matching_bracket";
				case SyntaxStyleTypes::MatchingSelection:
					return "matching_selection";
				case SyntaxStyleTypes::MatchingSearch:
					return "matching_search";
				case SyntaxStyleTypes::Suggestion:
					return "suggestion";
				case SyntaxStyleTypes::SuggestionScrollbar:
					return "suggestion_scrollbar";
				case SyntaxStyleTypes::SuggestionSelected:
					return "suggestion_selected";
				case SyntaxStyleTypes::Error:
					return "error";
				case SyntaxStyleTypes::Warning:
					return "warning";
				case SyntaxStyleTypes::Notice:
					return "notice";
				case SyntaxStyleTypes::SelectionRegion:
					return "selection_region";
				case SyntaxStyleTypes::MinimapBackground:
					return "minimap_background";
				case SyntaxStyleTypes::MinimapCurrentLine:
					return "minimap_current_line";
				case SyntaxStyleTypes::MinimapHover:
					return "minimap_hover";
				case SyntaxStyleTypes::MinimapSelection:
					return "minimap_selection";
				case SyntaxStyleTypes::MinimapHighlight:
					return "minimap_highlight";
				case SyntaxStyleTypes::MinimapVisibleArea:
					return "minimap_visible_area";
				default:
					break;
			}
		}
		return String::toString( style );
	}
};

template <typename Type> constexpr auto SyntaxStyleTypeHash( const Type& type ) noexcept {
	if constexpr ( std::is_same_v<Type, std::string> )
		return String::hash( type );
	else if constexpr ( std::is_same_v<Type, Uint32> )
		return type;
	else
		return SyntaxStyleType{};
}

template <typename T> static auto toSyntaxStyleType( const T& s ) noexcept {
	if constexpr ( std::is_same_v<SyntaxStyleType, std::string> && std::is_same_v<T, std::string> )
		return s;
	else if constexpr ( std::is_same_v<SyntaxStyleType, String::HashType> &&
						std::is_same_v<T, std::string> )
		return String::hash( s );
	else if constexpr ( std::is_same_v<SyntaxStyleType, std::string> &&
						std::is_same_v<T, String::HashType> )
		return String::toString( s );
	else
		return SyntaxStyleType{};
}

constexpr auto SyntaxStyleEmpty() {
	if constexpr ( std::is_same_v<SyntaxStyleType, std::string> )
		return "";
	else if constexpr ( std::is_same_v<SyntaxStyleType, String::HashType> )
		return 0;
	else
		return SyntaxStyleType{};
}

/**
 * Syntax colors types accepted/used are:
 * "normal", "symbol", "comment", "keyword", "keyword2",
 * "number", "literal", "string", "operator", "function",
 * "link"
 *
 * Editor colors types accepted/used are:
 * "background", "text", "caret"
 * "selection", "gutter_background",
 * "line_number", "line_number2", "line_highlight",
 * "gutter_background", "whitespace", "line_break_column",
 * "matching_bracket", "matching_selection", "suggestion", "suggestion_selected"
 *
 * Following the lite editor syntax colors (https://github.com/rxi/lite).
 */
class EE_API SyntaxColorScheme {
  public:
	static SyntaxColorScheme getDefault();

	static std::vector<SyntaxColorScheme> loadFromStream( IOStream& stream );

	static std::vector<SyntaxColorScheme> loadFromFile( const std::string& path );

	static std::vector<SyntaxColorScheme> loadFromMemory( const void* data,
														  std::size_t sizeInBytes );

	static std::vector<SyntaxColorScheme> loadFromPack( Pack* pack, std::string filePackPath );

	struct Style {
		Style(){};
		Style( const Color& color ) : color( color ) {}
		Style( const Color& color, const Color& background, const Uint32& style ) :
			color( color ), background( background ), style( style ) {}
		Color color{ Color::White };
		Color background{ Color::Transparent };
		Uint32 style{ 0 };
		Color outlineColor{ Color::Black };
		Float outlineThickness{ 0.f };
	};

	SyntaxColorScheme();

	SyntaxColorScheme( const std::string& name,
					   const UnorderedMap<SyntaxStyleType, Style>& syntaxColors,
					   const UnorderedMap<SyntaxStyleType, Style>& editorColors );

	const Style& getSyntaxStyle( const SyntaxStyleType& type ) const;

	bool hasSyntaxStyle( const SyntaxStyleType& type ) const;

	void setSyntaxStyles( const UnorderedMap<SyntaxStyleType, Style>& styles );

	void setSyntaxStyle( const SyntaxStyleType& type, const Style& style );

	const Style& getEditorSyntaxStyle( const SyntaxStyleType& type ) const;

	const Color& getEditorColor( const SyntaxStyleType& type ) const;

	void setEditorSyntaxStyles( const UnorderedMap<SyntaxStyleType, Style>& styles );

	void setEditorSyntaxStyle( const SyntaxStyleType& type, const Style& style );

	const std::string& getName() const;

	void setName( const std::string& name );

  protected:
	std::string mName;
	UnorderedMap<SyntaxStyleType, Style> mSyntaxColors;
	UnorderedMap<SyntaxStyleType, Style> mEditorColors;
	mutable UnorderedMap<SyntaxStyleType, Style> mStyleCache;

	template <typename SyntaxStyleType>
	const SyntaxColorScheme::Style& getSyntaxStyleFromCache( const SyntaxStyleType& type ) const;
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXCOLORSCHEME_HPP
