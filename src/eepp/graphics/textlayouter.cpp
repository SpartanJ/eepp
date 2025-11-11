#include <eepp/core/lrucache.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/textlayouter.hpp>
#include <eepp/graphics/textshaperun.hpp>

#ifdef EE_TEXT_SHAPER_ENABLED
#include <SheenBidi/SheenBidi.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb.h>
#endif

namespace EE::Graphics {

using LRULayoutCache = LRUCache<2048, Uint64, TextLayout>;

#ifdef EE_TEXT_SHAPER_ENABLED

struct TextSegment {
	std::size_t offset{};
	std::size_t length{};
	hb_script_t script{};
	hb_direction_t direction{};
};

// Split string into segments with uniform text properties
static void segmentString( String::View input, std::function<bool( const TextSegment& segment )> cb,
						   const std::function<void()>& newParagraphCb ) {
	const SBCodepointSequence codepointSequence{
		SBStringEncodingUTF32, static_cast<const void*>( input.data() ), input.size() };
	auto* const scriptLocator = SBScriptLocatorCreate();
	auto* const algorithm = SBAlgorithmCreate( &codepointSequence );
	SBUInteger paragraphOffset = 0;

	while ( paragraphOffset < input.size() ) {
		SBUInteger paragraphLength{};
		SBUInteger separatorLength{};
		SBAlgorithmGetParagraphBoundary( algorithm, paragraphOffset,
										 std::numeric_limits<SBUInteger>::max(), &paragraphLength,
										 &separatorLength );

		// If the paragraph contains characters besides the separator,
		// split the separator off into its own paragraph in the next iteration
		// We do this to ensure line breaks are inserted into segments last
		// after all character runs on the same line have already been inserted
		// This allows us to draw our segments in left-to-right top-to-bottom order
		if ( separatorLength < paragraphLength )
			paragraphLength -= separatorLength;

		auto* const paragraph = SBAlgorithmCreateParagraph( algorithm, paragraphOffset,
															paragraphLength, SBLevelDefaultLTR );
		auto* const line = SBParagraphCreateLine( paragraph, paragraphOffset, paragraphLength );
		const auto runCount = SBLineGetRunCount( line );
		const auto* runArray = SBLineGetRunsPtr( line );

		for ( SBUInteger i = 0; i < runCount; i++ ) {
			// Odd levels are RTL, even levels are LTR
			const auto direction = ( runArray[i].level % 2 ) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR;

			const SBCodepointSequence codepointSubsequence{
				SBStringEncodingUTF32,
				static_cast<const void*>( input.data() + runArray[i].offset ), runArray[i].length };

			SBScriptLocatorLoadCodepoints( scriptLocator, &codepointSubsequence );

			while ( SBScriptLocatorMoveNext( scriptLocator ) ) {
				const auto* agent = SBScriptLocatorGetAgent( scriptLocator );
				const auto script =
					hb_script_from_iso15924_tag( SBScriptGetUnicodeTag( agent->script ) );

				if ( !cb( TextSegment{ runArray[i].offset + agent->offset, agent->length, script,
									   direction } ) )
					break;
			}

			SBScriptLocatorReset( scriptLocator );
		}

		SBLineRelease( line );
		SBParagraphRelease( paragraph );

		paragraphOffset += paragraphLength;

		newParagraphCb();
	}

	SBAlgorithmRelease( algorithm );
	SBScriptLocatorRelease( scriptLocator );
}

static inline bool isSimpleScript( hb_script_t script ) {
	return script == HB_SCRIPT_LATIN || script == HB_SCRIPT_GREEK || script == HB_SCRIPT_CYRILLIC ||
		   script == HB_SCRIPT_INVALID || script == HB_SCRIPT_COMMON;
}

static bool shapeAndRun( const String& string, FontTrueType* font, Uint32 characterSize,
						 Uint32 style, Float outlineThickness,
						 const std::function<bool( hb_glyph_info_t*, hb_glyph_position_t*, Uint32,
												   const hb_segment_properties_t&,
												   const TextSegment&, TextShapeRun& )>& cb,
						 const std::function<void()>& newParagraphCb ) {
	String::View input = string.view();
	hb_buffer_t* hbBuffer = hb_buffer_create();
	bool completeRun = true;

	segmentString(
		input,
		[&]( const TextSegment& segment ) {
			TextShapeRun run( input.substr( segment.offset, segment.length ), font, characterSize,
							  style, outlineThickness, segment.direction == HB_DIRECTION_RTL );

			while ( run.hasNext() ) {
				FontTrueType* font = run.font();
				if ( font == nullptr ) { // empty line
					run.next();
					continue;
				}
				String::View curRun( run.curRun() );
				font->setCurrentSize( characterSize );
				hb_buffer_reset( hbBuffer );
				hb_buffer_set_cluster_level( hbBuffer,
											 HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS );
				hb_buffer_add_utf32( hbBuffer, (Uint32*)curRun.data(), curRun.size(), 0,
									 curRun.size() );

				hb_buffer_set_direction( hbBuffer, segment.direction );
				hb_buffer_set_script( hbBuffer, segment.script );
				hb_buffer_guess_segment_properties( hbBuffer );
				hb_segment_properties_t props;
				hb_buffer_get_segment_properties( hbBuffer, &props );
				std::uint32_t featuresEnabled = !isSimpleScript( segment.script ) ? 1 : 0;

				// We use our own kerning algo
				const hb_feature_t features[] = {
					hb_feature_t{ HB_TAG( 'k', 'e', 'r', 'n' ), featuresEnabled,
								  HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END },
					hb_feature_t{ HB_TAG( 'l', 'i', 'g', 'a' ), featuresEnabled,
								  HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END },
					hb_feature_t{ HB_TAG( 'c', 'l', 'i', 'g' ), featuresEnabled,
								  HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END },
					hb_feature_t{ HB_TAG( 'd', 'l', 'i', 'g' ), featuresEnabled,
								  HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END },
				};

				// whitelist cross-platforms shapers only
				static const char* shaper_list[] = { "ot", "graphite2", "fallback", nullptr };

				if ( !font || !font->hb() ) {
					eeASSERT( font && font->hb() );
					completeRun = false;
					break;
				}

				hb_shape_full( static_cast<hb_font_t*>( font->hb() ), hbBuffer, features,
							   eeARRAY_SIZE( features ), shaper_list );

				// from the shaped text we get the glyphs and positions
				unsigned int glyphCount;
				hb_glyph_info_t* glyphInfo = hb_buffer_get_glyph_infos( hbBuffer, &glyphCount );
				hb_glyph_position_t* glyphPos =
					hb_buffer_get_glyph_positions( hbBuffer, &glyphCount );

				if ( cb( glyphInfo, glyphPos, glyphCount, props, segment, run ) )
					run.next();
				else {
					completeRun = false;
					return false;
					break;
				}
			}

			return true;
		},
		[] {

		} );

	hb_buffer_destroy( hbBuffer );
	return completeRun;
}

#endif

template <typename StringType>
static inline Uint64 textLayoutHash( const StringType& string, Font* font,
									 const Uint32& characterSize, const Uint32& style,
									 const Uint32& tabWidth, const Float& outlineThickness,
									 std::optional<Float> tabOffset ) {
	return hashCombine( std::hash<StringType>()( string ), std::hash<Font*>()( font ),
						std::hash<Uint32>()( characterSize ), std::hash<Uint32>()( style ),
						std::hash<Uint32>()( tabWidth ), std::hash<Float>()( outlineThickness ),
						std::hash<std::optional<Float>>()( tabOffset ) );
}

template <typename StringType>
TextLayout TextLayouter::layout( const StringType& string, Font* font, const Uint32& characterSize,
								 const Uint32& style, const Uint32& tabWidth,
								 const Float& outlineThickness, std::optional<Float> tabOffset,
								 Uint32 textDrawHints ) {
	static LRULayoutCache sLayoutCache;
	TextLayout result;

	if ( !font || string.empty() ) {
		result.size = { 0.f, font ? (Float)font->getFontHeight( characterSize ) : 0.f };
		return result;
	}

	Uint64 hash = 0;
	if ( !Text::canSkipShaping( textDrawHints ) ) {
		hash = textLayoutHash( string, font, characterSize, style, tabWidth, outlineThickness,
							   tabOffset );

		auto cacheHit = sLayoutCache.get( hash );
		if ( cacheHit.has_value() )
			return *cacheHit;
	}

	bool bold = ( style & Text::Bold ) != 0;
	bool italic = ( style & Text::Italic ) != 0;
	Float hspace = font->getGlyph( ' ', characterSize, bold, italic, outlineThickness ).advance;
	Float vspace = font->getLineSpacing( characterSize );
	Vector2f pen;
	Float maxWidth = 0;

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( Text::TextShaperEnabled && font->getType() == FontType::TTF &&
		 !Text::canSkipShaping( textDrawHints ) ) {
		FontTrueType* rFont = static_cast<FontTrueType*>( font );
		shapeAndRun(
			string, rFont, characterSize, style, outlineThickness,
			[&]( hb_glyph_info_t* glyphInfo, hb_glyph_position_t* glyphPos, Uint32 glyphCount,
				 const hb_segment_properties_t& props, const TextSegment& segment,
				 TextShapeRun& run ) {
				FontTrueType* currentRunFont = run.font();
				if ( !currentRunFont )
					return true;
				result.shapedGlyphs.reserve( result.shapedGlyphs.size() + glyphCount );
				Uint32 prevGlyphIndex = 0;

				if ( isSimpleScript( props.script ) ) {
					for ( size_t i = 0; i < glyphCount; ++i ) {
						Uint32 cluster = glyphInfo[i].cluster;
						String::StringBaseType ch = string[segment.offset + run.pos() + cluster];

						if ( ch == '\t' ) {
							Float advance = Text::tabAdvance( hspace, tabWidth,
															  tabOffset ? pen.x + *tabOffset
																		: std::optional<Float>{} );
							ShapedGlyph sg;
							sg.font = currentRunFont;
							sg.glyphIndex = glyphInfo[i].codepoint;
							sg.stringIndex = segment.offset + run.pos() + cluster;
							sg.position = pen;
							sg.advance = { advance, 0 };
							result.shapedGlyphs.emplace_back( std::move( sg ) );

							pen.x += advance;
							prevGlyphIndex = 0; // Reset kerning after a tab
							continue;
						}

						Glyph currentGlyph = currentRunFont->getGlyphByIndex(
							glyphInfo[i].codepoint, characterSize, bold, italic, outlineThickness );

						if ( ch != '\n' && ch != '\r' ) {
							pen.x += currentRunFont->getKerningFromGlyphIndex(
								prevGlyphIndex, glyphInfo[i].codepoint, characterSize, bold, italic,
								outlineThickness );
						}

						ShapedGlyph sg;
						sg.font = currentRunFont;
						sg.glyphIndex = glyphInfo[i].codepoint;
						sg.stringIndex = segment.offset + run.pos() + glyphInfo[i].cluster;

						float offsetX = glyphPos[i].x_offset / 64.f;
						float offsetY = glyphPos[i].y_offset / 64.f;
						sg.advance = { offsetX, offsetY };
						sg.position.x = pen.x + offsetX;
						sg.position.y = pen.y - offsetY;
						result.shapedGlyphs.emplace_back( std::move( sg ) );

						pen.x += currentGlyph.advance;
						prevGlyphIndex = glyphInfo[i].codepoint;
					}
				} else {
					for ( size_t i = 0; i < glyphCount; ++i ) {
						Uint32 cluster = glyphInfo[i].cluster;
						String::StringBaseType ch = string[segment.offset + run.pos() + cluster];

						if ( ch == '\t' ) {
							Float advance = Text::tabAdvance( hspace, tabWidth,
															  tabOffset ? pen.x + *tabOffset
																		: std::optional<Float>{} );
							ShapedGlyph sg;
							sg.font = currentRunFont;
							sg.glyphIndex = glyphInfo[i].codepoint;
							sg.stringIndex = segment.offset + run.pos() + cluster;
							sg.advance = { advance, 0 };
							sg.position = pen;
							result.shapedGlyphs.emplace_back( std::move( sg ) );

							pen.x += advance;
							prevGlyphIndex = 0; // Reset kerning after a tab
							continue;
						}

						ShapedGlyph sg;
						sg.font = currentRunFont;
						sg.glyphIndex = glyphInfo[i].codepoint;
						sg.stringIndex = segment.offset + run.pos() + glyphInfo[i].cluster;
						float offsetX = glyphPos[i].x_offset / 64.f;
						float offsetY = glyphPos[i].y_offset / 64.f;
						sg.advance = { offsetX, offsetY };
						sg.position.x = std::round( pen.x + offsetX );
						sg.position.y = std::round( pen.y - offsetY );
						result.shapedGlyphs.emplace_back( std::move( sg ) );
						pen.x += Font::isEmojiCodePoint( ch )
									 ? currentRunFont
										   ->getGlyphByIndex( glyphInfo[i].codepoint, characterSize,
															  bold, italic, outlineThickness )
										   .advance
									 : glyphPos[i].x_advance / 64.f;
						pen.y += glyphPos[i].y_advance / 64.f;
					}
				}

				if ( run.runIsNewLine() ) {
					result.linesWidth.push_back( std::ceil( pen.x ) );
					maxWidth = eemax( maxWidth, result.linesWidth[result.linesWidth.size() - 1] );
					pen.x = 0;
					pen.y += vspace;
				}
				return true;
			},
			[&] {
				result.linesWidth.push_back( std::ceil( pen.x ) );
				maxWidth = eemax( maxWidth, result.linesWidth[result.linesWidth.size() - 1] );
				pen.x = 0;
				pen.y += vspace;
			} );
	} else
#endif
	{
		// Fallback for non-TrueType fonts or when shaper is disabled
		Uint32 prevChar = 0;
		for ( size_t i = 0; i < string.size(); ++i ) {
			Uint32 curChar = string[i];
			if ( curChar == '\n' ) {
				result.linesWidth.push_back( pen.x );
				maxWidth = eemax( maxWidth, pen.x );
				pen.x = 0;
				pen.y += vspace;
				prevChar = 0;
				continue;
			}
			if ( curChar == '\r' ) {
				prevChar = 0;
				continue;
			}

			pen.x += font->getKerning( prevChar, curChar, characterSize, bold, italic,
									   outlineThickness );
			prevChar = curChar;

			if ( curChar == '\t' ) {

				ShapedGlyph sg;
				sg.stringIndex = i;
				sg.advance = { Text::tabAdvance( hspace, tabWidth,
												 tabOffset ? ( tabOffset ? *tabOffset + pen.x
																		 : std::optional<Float>{} )
														   : std::optional<Float>{} ),
							   0 };
				sg.position = pen;
				pen.x += sg.advance.x;
				result.shapedGlyphs.emplace_back( std::move( sg ) );
				continue;
			}

			ShapedGlyph sg;
			sg.font = static_cast<FontTrueType*>( font );
			sg.glyphIndex = sg.font->getGlyphIndex( curChar );
			sg.stringIndex = i;
			sg.advance = {
				font->getGlyph( curChar, characterSize, bold, italic, outlineThickness ).advance,
				0 };
			sg.position = pen;
			pen.x += sg.advance.x;
			result.shapedGlyphs.emplace_back( std::move( sg ) );
		}
	}

	// pen.y doesn't have the last line height counted unless the last run ended with a new line
	if ( string[string.size() - 1] != '\n' )
		pen.y += vspace;

	result.linesWidth.push_back( std::ceil( pen.x ) );
	maxWidth = eemax( maxWidth, result.linesWidth[result.linesWidth.size() - 1] );
	result.size = { maxWidth, std::ceil( pen.y ) };

	sLayoutCache.put( hash, result );
	return result;
}

TextLayout TextLayouter::layout( const String& string, Font* font, const Uint32& fontSize,
								 const Uint32& style, const Uint32& tabWidth,
								 const Float& outlineThickness, std::optional<Float> tabOffset,
								 Uint32 textDrawHints ) {
	return TextLayouter::layout<String>( string, font, fontSize, style, tabWidth, outlineThickness,
										 tabOffset, textDrawHints );
}

TextLayout TextLayouter::layout( const String::View& string, Font* font, const Uint32& fontSize,
								 const Uint32& style, const Uint32& tabWidth,
								 const Float& outlineThickness, std::optional<Float> tabOffset,
								 Uint32 textDrawHints ) {
	return TextLayouter::layout<String::View>( string, font, fontSize, style, tabWidth,
											   outlineThickness, tabOffset, textDrawHints );
}

} // namespace EE::Graphics
