#include <eepp/core/lrucache.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/textlayout.hpp>
#include <eepp/graphics/textshaperun.hpp>

#ifdef EE_TEXT_SHAPER_ENABLED
#include <SheenBidi/SheenBidi.h>
#include <SheenBidi/Source/SBScriptLocator.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb.h>
#endif

namespace EE::Graphics {

using LRULayoutCache = LRUCache<2048, Uint64, TextLayout::Cache>;

#ifdef EE_TEXT_SHAPER_ENABLED

struct TextSegment {
	std::size_t offset{};
	std::size_t length{};
	hb_script_t script{};
	hb_direction_t direction{};
};

static inline bool isSimpleScript( hb_script_t script ) {
	return script == HB_SCRIPT_LATIN || script == HB_SCRIPT_GREEK || script == HB_SCRIPT_CYRILLIC ||
		   script == HB_SCRIPT_INVALID || script == HB_SCRIPT_COMMON;
}

// Helper function to get a thread-local, reusable HarfBuzz buffer.
static hb_buffer_t* getThreadLocalHbBuffer() {
	struct HbBufferDeleter {
		void operator()( hb_buffer_t* buf ) const {
			if ( buf )
				hb_buffer_destroy( buf );
		}
	};
	thread_local static std::unique_ptr<hb_buffer_t, HbBufferDeleter> sHbBuffer(
		hb_buffer_create() );
	return sHbBuffer.get();
}

// Helper function to get a thread-local, reusable SheenBidi script locator.
static SBScriptLocator* getThreadLocalSbScriptLocator() {
	// Custom deleter for SBScriptLocator
	struct SbScriptLocatorDeleter {
		void operator()( SBScriptLocator* loc ) const {
			if ( loc )
				SBScriptLocatorRelease( loc );
		}
	};

	thread_local static std::unique_ptr<SBScriptLocator, SbScriptLocatorDeleter> sSbScriptLocator(
		SBScriptLocatorCreate() );
	return sSbScriptLocator.get();
}

// Split string into segments with uniform text properties
template <typename Callable>
static void segmentString( TextLayout& result, String::View input, Callable cb,
						   TextDirection direction ) {
	const SBCodepointSequence codepointSequence{
		SBStringEncodingUTF32, static_cast<const void*>( input.data() ), input.size() };
	auto* const scriptLocator = getThreadLocalSbScriptLocator();
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

		SBLevel level = 0;

		switch ( direction ) {
			case TextDirection::LeftToRight:
				level = 0;
				break;
			case TextDirection::RightToLeft:
				level = 1;
				break;
			case TextDirection::TopToBottom:
			case TextDirection::BottomToTop:
			case TextDirection::Unspecified:
				level = SBLevelDefaultLTR;
				break;
		}

		auto* const paragraph =
			SBAlgorithmCreateParagraph( algorithm, paragraphOffset, paragraphLength, level );
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

		if ( runCount > 0 && paragraphOffset == 0 ) {
			result.direction = ( runArray[0].level % 2 ) != 0 ? TextDirection::RightToLeft
															  : TextDirection::LeftToRight;
		}

		SBLineRelease( line );
		SBParagraphRelease( paragraph );

		paragraphOffset += paragraphLength;
	}

	SBAlgorithmRelease( algorithm );
}

template <typename Callable>
static void shapeAndRun( TextLayout& result, const String& string, FontTrueType* font,
						 Uint32 characterSize, Uint32 style, Float outlineThickness,
						 TextDirection baseDirection, Callable cb ) {
	String::View input = string.view();
	hb_buffer_t* hbBuffer = getThreadLocalHbBuffer();

	segmentString(
		result, input,
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
					return false;
				}
			}

			return true;
		},
		baseDirection );
}

#endif

static inline Uint64 textLayoutHash( const String::View& string, Font* font,
									 const Uint32& characterSize, const Uint32& style,
									 const Uint32& tabWidth, const Float& outlineThickness,
									 std::optional<Float> tabOffset, TextDirection direction,
									 LineWrapMode wrapMode, Uint32 wrapWidth,
									 bool keepIndentation ) {
	return hashCombine( std::hash<String::View>()( string ), std::hash<Font*>()( font ),
						std::hash<Uint32>()( characterSize ), std::hash<Uint32>()( style ),
						std::hash<Uint32>()( tabWidth ), std::hash<Float>()( outlineThickness ),
						std::hash<std::optional<Float>>()( tabOffset ),
						std::hash<std::underlying_type_t<TextDirection>>()(
							static_cast<std::underlying_type_t<TextDirection>>( direction ) ),
						std::hash<std::underlying_type_t<LineWrapMode>>()(
							static_cast<std::underlying_type_t<LineWrapMode>>( wrapMode ) ),
						std::hash<Uint32>()( wrapWidth ), std::hash<bool>()( keepIndentation ) );
}

TextLayout::Cache TextLayout::layout( const String::View& string, Font* font,
									  const Uint32& characterSize, const Uint32& style,
									  const Uint32& tabWidth, const Float& outlineThickness,
									  std::optional<Float> tabOffset, Uint32 textDrawHints,
									  TextDirection baseDirection, LineWrapMode wrapMode,
									  Uint32 wrapWidth, bool keepIndentation,
									  Float initialXOffset ) {
	static LRULayoutCache sLayoutCache;

	if ( !font || string.empty() ) {
		auto layout = std::make_shared<TextLayout>();
		layout->paragraphs.push_back( {} );
		layout->size = { 0.f, font ? (Float)font->getFontHeight( characterSize ) : 0.f };
		return layout;
	}

	Uint64 hash = 0;
	if ( !Text::canSkipShaping( textDrawHints ) ) {
		hash = textLayoutHash( string, font, characterSize, style, tabWidth, outlineThickness,
							   tabOffset, baseDirection, wrapMode, wrapWidth, keepIndentation );

		auto cacheHit = sLayoutCache.get( hash );
		if ( cacheHit.has_value() )
			return *cacheHit;
	}

	bool bold = ( style & Text::Bold ) != 0;
	bool italic = ( style & Text::Italic ) != 0;
	Uint32 spaceGlyphIndex = 0;
	Float hspace = font->getGlyph( ' ', characterSize, bold, italic, outlineThickness ).advance;
	Float vspace = font->getLineSpacing( characterSize );
	Vector2f pen{ initialXOffset, 0 };
	Float maxWidth = 0;

	auto resultPtr = std::make_shared<TextLayout>();
	TextLayout& result = *resultPtr;
	result.paragraphs.push_back( {} );
	ShapedTextParagraph* curParagraph = &result.paragraphs.back();
	curParagraph->wrapInfo.wraps.push_back( 0 );
	struct GlyphDirCounter {
		int ltr{ 0 };
		int rtl{ 0 };
		int ttb{ 0 };
		int btt{ 0 };
		int other{ 0 };
	};
	GlyphDirCounter gdc;

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( Text::TextShaperEnabled && font->getType() == FontType::TTF &&
		 !Text::canSkipShaping( textDrawHints ) ) {
		FontTrueType* rFont = static_cast<FontTrueType*>( font );
		shapeAndRun(
			result, string, rFont, characterSize, style, outlineThickness, baseDirection,
			[&]( hb_glyph_info_t* glyphInfo, hb_glyph_position_t* glyphPos, Uint32 glyphCount,
				 const hb_segment_properties_t& props, const TextSegment& segment,
				 TextShapeRun& run ) {
				FontTrueType* currentRunFont = run.font();
				if ( !currentRunFont )
					return true;

				curParagraph->shapedGlyphs.reserve( curParagraph->shapedGlyphs.size() +
													glyphCount );
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
							if ( spaceGlyphIndex == 0 && font->getType() == FontType::TTF ) {
								spaceGlyphIndex =
									static_cast<FontTrueType*>( font )->getGlyphIndex( ' ' );
							}
							sg.glyphIndex = spaceGlyphIndex;
							sg.stringIndex = segment.offset + run.pos() + cluster;
							sg.position = pen;
							sg.advance = { advance, 0 };
							sg.direction = (TextDirection)segment.direction;
							sg.script = (LangScript)props.script;
							curParagraph->shapedGlyphs.emplace_back( std::move( sg ) );

							pen.x += advance;
							prevGlyphIndex = 0; // Reset kerning after a tab
							continue;
						}

						Glyph currentGlyph = currentRunFont->getGlyphByIndex(
							glyphInfo[i].codepoint, characterSize, bold, italic, outlineThickness,
							rFont->getPage( characterSize ) );

						if ( ch != '\n' && ch != '\r' &&
							 !( textDrawHints & TextHints::NoKerning ) ) {
							pen.x += currentRunFont->getKerningFromGlyphIndex(
								prevGlyphIndex, glyphInfo[i].codepoint, characterSize, bold, italic,
								outlineThickness );
						}

						ShapedGlyph sg;
						sg.font = currentRunFont;
						sg.glyphIndex = glyphInfo[i].codepoint;
						sg.stringIndex = segment.offset + run.pos() + glyphInfo[i].cluster;
						sg.advance = { currentGlyph.advance, 0 };
						sg.direction = (TextDirection)segment.direction;
						sg.script = (LangScript)props.script;
						sg.position.x = pen.x + ( glyphPos[i].x_offset / 64.f );
						sg.position.y = pen.y - ( glyphPos[i].y_offset / 64.f );
						curParagraph->shapedGlyphs.emplace_back( std::move( sg ) );

						pen.x += currentGlyph.advance;
						prevGlyphIndex = glyphInfo[i].codepoint;
					}
				} else {
					for ( size_t i = 0; i < glyphCount; ++i ) {
						Uint32 cluster = glyphInfo[i].cluster;
						String::StringBaseType ch = string[segment.offset + run.pos() + cluster];

						switch ( segment.direction ) {
							case HB_DIRECTION_INVALID:
								gdc.other++;
								break;
							case HB_DIRECTION_LTR:
								gdc.ltr++;
								break;
							case HB_DIRECTION_RTL:
								gdc.rtl++;
								break;
							case HB_DIRECTION_TTB:
								gdc.ttb++;
								break;
							case HB_DIRECTION_BTT:
								gdc.btt++;
								break;
						}

						if ( ch == '\t' ) {
							Float advance = Text::tabAdvance( hspace, tabWidth,
															  tabOffset ? pen.x + *tabOffset
																		: std::optional<Float>{} );
							ShapedGlyph sg;
							sg.font = currentRunFont;
							sg.glyphIndex = glyphInfo[i].codepoint;
							sg.stringIndex = segment.offset + run.pos() + cluster;
							sg.advance = { advance, 0 };
							sg.direction = (TextDirection)segment.direction;
							sg.script = (LangScript)props.script;
							sg.position = pen;
							curParagraph->shapedGlyphs.emplace_back( std::move( sg ) );

							pen.x += advance;
							prevGlyphIndex = 0; // Reset kerning after a tab
							continue;
						}

						ShapedGlyph sg;
						sg.font = currentRunFont;
						sg.glyphIndex = glyphInfo[i].codepoint;
						sg.stringIndex = segment.offset + run.pos() + glyphInfo[i].cluster;
						sg.advance = {
							Font::isEmojiCodePoint( ch )
								? currentRunFont
									  ->getGlyphByIndex( glyphInfo[i].codepoint, characterSize,
														 bold, italic, outlineThickness,
														 rFont->getPage( characterSize ) )
									  .advance
								: glyphPos[i].x_advance / 64.f,
							glyphPos[i].y_advance / 64.f };
						sg.direction = (TextDirection)segment.direction;
						sg.script = (LangScript)props.script;
						sg.position.x = std::round( pen.x + ( glyphPos[i].x_offset / 64.f ) );
						sg.position.y = std::round( pen.y - ( glyphPos[i].y_offset / 64.f ) );
						curParagraph->shapedGlyphs.emplace_back( std::move( sg ) );
						pen.x += sg.advance.x;
						pen.y += sg.advance.y;
					}
				}

				if ( run.runIsNewLine() ) {
					curParagraph->size.x = std::ceil( pen.x );
					maxWidth = eemax( maxWidth, curParagraph->size.x );
					pen.x = 0;
					pen.y += vspace;
					curParagraph->size.y = pen.y;
					result.paragraphs.push_back( {} );
					curParagraph = &result.paragraphs.back();
					curParagraph->wrapInfo.wraps.push_back( segment.offset + segment.length - 1 );
				}
				return true;
			} );
	} else
#endif
	{
		// Fallback for non-TrueType fonts or when shaper is disabled
		Uint32 prevChar = 0;
		for ( size_t i = 0; i < string.size(); ++i ) {
			Uint32 curChar = string[i];
			if ( curChar == '\n' ) {
				curParagraph->size.x = pen.x;
				maxWidth = eemax( maxWidth, pen.x );
				pen.x = 0;
				pen.y += vspace;
				curParagraph->size.y = pen.y;
				prevChar = 0;
				result.paragraphs.push_back( {} );
				curParagraph = &result.paragraphs.back();
				curParagraph->wrapInfo.wraps.push_back( i );
				continue;
			}
			if ( curChar == '\r' ) {
				prevChar = 0;
				continue;
			}

			if ( !( textDrawHints & TextHints::NoKerning ) ) {
				pen.x += font->getKerning( prevChar, curChar, characterSize, bold, italic,
										   outlineThickness );
			}
			prevChar = curChar;

			if ( curChar == '\t' ) {

				ShapedGlyph sg;
				sg.stringIndex = i;
				sg.advance = { Text::tabAdvance( hspace, tabWidth,
												 tabOffset ? ( tabOffset ? *tabOffset + pen.x
																		 : std::optional<Float>{} )
														   : std::optional<Float>{} ),
							   0 };
				sg.direction = TextDirection::LeftToRight;
				sg.script = LangScript::LATIN;
				sg.position = pen;
				pen.x += sg.advance.x;
				curParagraph->shapedGlyphs.emplace_back( std::move( sg ) );
				continue;
			}

			ShapedGlyph sg;
			sg.font = static_cast<FontTrueType*>( font );
			sg.glyphIndex = sg.font->getGlyphIndex( curChar );
			sg.stringIndex = i;
			sg.advance = {
				font->getGlyph( curChar, characterSize, bold, italic, outlineThickness ).advance,
				0 };
			sg.direction = TextDirection::LeftToRight;
			sg.script = LangScript::LATIN;
			sg.position = pen;
			pen.x += sg.advance.x;
			curParagraph->shapedGlyphs.emplace_back( std::move( sg ) );
		}
	}

	// pen.y doesn't have the last line height counted unless the last run ended with a new line
	if ( string[string.size() - 1] != '\n' )
		pen.y += vspace;

	curParagraph->size.x = std::ceil( pen.x );
	curParagraph->size.y = pen.y;
	maxWidth = eemax( maxWidth, curParagraph->size.x );
	result.size = { maxWidth, std::ceil( pen.y ) };
	result.hasMixedDirection = !!gdc.ltr + !!gdc.rtl + !!gdc.ttb + !!gdc.btt + !!gdc.other > 1;

	if ( wrapMode != LineWrapMode::NoWrap && wrapWidth ) {
		wrapLayout( string, result, wrapMode, wrapWidth, vspace, keepIndentation, font,
					characterSize, style, tabWidth, outlineThickness, hspace );
	}

	sLayoutCache.put( hash, resultPtr );
	return resultPtr;
}

TextLayout::Cache TextLayout::layout( const String& string, Font* font, const Uint32& fontSize,
									  const Uint32& style, const Uint32& tabWidth,
									  const Float& outlineThickness, std::optional<Float> tabOffset,
									  Uint32 textDrawHints, TextDirection baseDirection,
									  LineWrapMode wrapMode, Uint32 wrapWidth, bool keepIndentation,
									  Float initialXOffset ) {
	return TextLayout::layout( string.view(), font, fontSize, style, tabWidth, outlineThickness,
							   tabOffset, textDrawHints, baseDirection, wrapMode, wrapWidth,
							   keepIndentation, initialXOffset );
}

std::vector<Float> TextLayout::getLinesWidth() const {
	std::vector<Float> lw;
	lw.reserve( paragraphs.size() );
	for ( const auto& sp : paragraphs )
		lw.push_back( sp.size.x );
	return lw;
}

void TextLayout::wrapLayout( const String::View& string, TextLayout& result,
							 LineWrapMode lineWrapMode, Float wrapWidth, Float vspace,
							 bool keepIndentation, Font* font, const Uint32& characterSize,
							 const Uint32& fontStyle, const Uint32& tabWidth,
							 const Float& outlineThickness, Float hspace ) {
	std::size_t paragraphCount = result.paragraphs.size();

	for ( std::size_t paragraphIdx = 0; paragraphIdx < paragraphCount; paragraphIdx++ ) {
		ShapedTextParagraph& sp = result.paragraphs[paragraphIdx];
		std::size_t shapedGlyphCount = sp.shapedGlyphs.size();
		std::size_t lastSpace = std::string::npos;
		std::size_t lastSpaceStringIdx = std::string::npos;
		Vector2f currentOffset( 0.f, 0.f );
		Sizef maxSize{ 0, vspace };

		if ( keepIndentation && shapedGlyphCount ) {
			sp.wrapInfo.paddingStart = LineWrap::computeOffsets(
				string.substr( sp.shapedGlyphs[0].stringIndex ), font, characterSize, fontStyle,
				outlineThickness, tabWidth, eemax( wrapWidth - hspace, hspace ) );
		}

		for ( std::size_t idx = 0; idx < shapedGlyphCount; idx++ ) {
			ShapedGlyph& sg = sp.shapedGlyphs[idx];
			auto curChar = string[sg.stringIndex];

			sg.position += currentOffset;

			if ( sg.position.x + sg.advance.x > wrapWidth ) {
				std::size_t breakIndex = idx;
				std::size_t breakStringIdx = sg.stringIndex;

				bool performWordWrap =
					( lineWrapMode == LineWrapMode::Word && lastSpace != std::string::npos );

				if ( performWordWrap ) {
					ShapedGlyph& prevBreakGlyph = sp.shapedGlyphs[lastSpace];
					maxSize.x =
						std::max( prevBreakGlyph.position.x + prevBreakGlyph.advance.x, maxSize.x );
				}

				// Break after the last space (start of the current word)
				if ( performWordWrap ) {
					breakIndex = lastSpace + 1;
					breakStringIdx = lastSpaceStringIdx + 1;
				}

				if ( breakIndex > idx )
					breakIndex = idx;

				if ( breakStringIdx > string.size() )
					breakStringIdx = string.size();

				sp.wrapInfo.wraps.push_back( breakStringIdx );

				ShapedGlyph& breakGlyph = sp.shapedGlyphs[breakIndex];
				Vector2f adjustment( -breakGlyph.position.x + sp.wrapInfo.paddingStart, vspace );

				for ( std::size_t k = breakIndex; k <= idx; ++k )
					sp.shapedGlyphs[k].position += adjustment;

				maxSize.y = std::max( breakGlyph.position.y + vspace, maxSize.y );

				currentOffset += adjustment;
				lastSpace = std::string::npos;
				lastSpaceStringIdx = std::string::npos;
			} else if ( LineWrap::isWrapChar( curChar ) ) {
				lastSpace = idx;
				lastSpaceStringIdx = sg.stringIndex;
			}
		}

		sp.size = maxSize;
	}

	result.size = result.paragraphs[result.paragraphs.size() - 1].size;
}

} // namespace EE::Graphics
