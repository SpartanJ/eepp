#include <eepp/graphics/text.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/parsermatcher.hpp>
#include <eepp/system/regex.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/doc/syntaxtokenizer.hpp>

#include <array>
#include <memory_resource>
#include <variant>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

static constexpr auto REGEX_FLAGS = RegEx::Options::Utf | RegEx::Options::AllowFallback;

struct PatternStackItem {
	const std::vector<SyntaxPattern>* patterns{ nullptr };
	size_t index = 0;
	SyntaxSyateHolderType repositoryIdx = 0;
};

// This tokenizer was a direct conversion to C++ from the lite (https://github.com/rxi/lite)
// tokenizer. This allows eepp to support the same color schemes and syntax definitions from
// lite. Making much easier to implement a complete code editor. Currently some improvements
// has been made. It's still compatible with lite tokenizer.

// Max token size limits the length of each token for cases where there's a single token for a very
// large line. This will help the editor to cull the rendering only for the visible tokens
#define MAX_TOKEN_SIZE ( 512 )

#define MAX_MATCHES ( 64 )

#define MAX_PATTERN_STACK_SIZE ( 16 )

#define MAX_TRIED_PATTERNS ( 64 )

static int isInMultiByteCodePoint( const char* text, const size_t& textSize, const size_t& pos ) {
	// current char is a multybyte codepoint
	if ( ( text[pos] & 0xC0 ) == 0x80 ) {
		int nextCodePoint = 1;
		while ( pos + nextCodePoint < textSize ) {
			// search the start of the next codepoint
			if ( ( text[pos + nextCodePoint] & 0xC0 ) != 0x80 )
				return nextCodePoint;
			++nextCodePoint;
		}
	}
	return 0;
}

template <typename T>
static void pushToken( std::vector<T>& tokens, const SyntaxStyleType& type,
					   const std::string_view& text ) {
	if ( text.empty() )
		return;
	if ( !tokens.empty() && ( tokens[tokens.size() - 1].type == type ) ) {
		size_t tokenPos = tokens.size() - 1;
		tokens[tokenPos].type = type;
		if constexpr ( std::is_same_v<T, SyntaxTokenComplete> )
			tokens[tokenPos].text += text;
		tokens[tokenPos].len += String::utf8Length( text );
	} else {
		if ( text.size() > MAX_TOKEN_SIZE ) {
			size_t textSize = text.size();
			size_t pos = 0;

			while ( textSize > 0 ) {
				size_t chunkSize = textSize > MAX_TOKEN_SIZE ? MAX_TOKEN_SIZE : textSize;
				int multiByteCodePointPos = 0;
				if ( ( multiByteCodePointPos = isInMultiByteCodePoint( text.data(), text.size(),
																	   pos + chunkSize ) ) > 0 ) {
					chunkSize = eemin( textSize, chunkSize + multiByteCodePointPos );
				}
				std::string_view substr = text.substr( pos, chunkSize );
				SyntaxStyleType len = String::utf8Length( substr );
				if constexpr ( std::is_same_v<T, SyntaxTokenComplete> ) {
					tokens.push_back( { type, std::string{ substr }, len } );
				} else if constexpr ( std::is_same_v<T, SyntaxTokenPosition> ) {
					SyntaxStyleType tokenPos = tokens.empty() ? 0
															  : tokens[tokens.size() - 1].pos +
																	tokens[tokens.size() - 1].len;
					tokens.push_back( { type, tokenPos, len } );
				} else {
					tokens.push_back( { type, len } );
				}
				textSize -= chunkSize;
				pos += chunkSize;
			}
		} else {
			if constexpr ( std::is_same_v<T, SyntaxTokenComplete> ) {
				tokens.push_back( { type, std::string{ text },
									static_cast<SyntaxTokenLen>( String::utf8Length( text ) ) } );
			} else if constexpr ( std::is_same_v<T, SyntaxTokenPosition> ) {
				SyntaxStyleType tokenPos =
					tokens.empty() ? 0
								   : tokens[tokens.size() - 1].pos + tokens[tokens.size() - 1].len;
				tokens.push_back(
					{ type, tokenPos, static_cast<SyntaxTokenLen>( String::utf8Length( text ) ) } );
			} else {
				tokens.push_back(
					{ type, static_cast<SyntaxTokenLen>( String::utf8Length( text ) ) } );
			}
		}
	}
}

static bool isScaped( const std::string& text, const size_t& startIndex,
					  const std::string& escapeStr ) {
	char escapeByte = escapeStr.empty() ? '\\' : escapeStr[0];
	int count = 0;
	for ( int i = startIndex - 1; i >= 0; i-- ) {
		if ( text[i] != escapeByte )
			break;
		count++;
	}
	return count % 2 == 1;
}

struct NonEscapedMatch {
	std::pair<int, int> range{ -1, -1 };
	PatternMatcher::Range matches[6];
	int numMatches{ 0 };
};

static NonEscapedMatch findNonEscaped( const std::string& text, const std::string& pattern,
									   int offset, const std::string& escapeStr,
									   SyntaxPatternMatchType matchType, bool anchored ) {
	eeASSERT( !pattern.empty() );
	if ( pattern.empty() )
		return {};
	std::variant<RegEx, LuaPattern, ParserMatcher> wordsVar =
		matchType == SyntaxPatternMatchType::LuaPattern
			? std::variant<RegEx, LuaPattern, ParserMatcher>( LuaPattern(
				  pattern, anchored ? LuaPattern::Options::Anchored : LuaPattern::Options::None ) )
			: ( matchType == SyntaxPatternMatchType::RegEx
					? std::variant<RegEx, LuaPattern, ParserMatcher>( RegEx(
						  pattern, REGEX_FLAGS | ( anchored ? RegEx::Options::Anchored : 0 ) ) )
					: std::variant<RegEx, LuaPattern, ParserMatcher>( ParserMatcher( pattern ) ) );
	PatternMatcher& words =
		std::visit( []( auto& patternType ) -> PatternMatcher& { return patternType; }, wordsVar );
	int start, end;
	PatternMatcher::Range matches[6];
	while ( words.find( text, start, end, offset, 0, matches ) ) {
		if ( !escapeStr.empty() && isScaped( text, start, escapeStr ) ) {
			offset = end;
		} else {
			NonEscapedMatch res;
			res.range = { start, end };
			res.numMatches = words.getNumMatches();
			std::memcpy( res.matches, matches, sizeof( matches ) );
			return res;
		}
	}
	return {};
}

SyntaxStateRestored SyntaxTokenizer::retrieveSyntaxState( const SyntaxDefinition& syntax,
														  const SyntaxState& state ) {
	SyntaxStateRestored syntaxState{ &syntax, nullptr, state.state[0], 0 };
	const SyntaxPattern* curPattern = nullptr;
	for ( size_t i = 0; i < MAX_SUB_SYNTAXS - 1; ++i ) {
		curPattern = syntaxState.currentSyntax->getPatternFromState( state.state[i] );
		if ( curPattern && state.state[i].state != SYNTAX_TOKENIZER_STATE_NONE ) {
			if ( curPattern->hasSyntaxOrContentScope() ) {
				syntaxState.subsyntaxInfo = curPattern;
				auto langIndex = state.langStack[i];

				if ( langIndex ) {
					syntaxState.currentSyntax =
						langIndex != 0
							? &SyntaxDefinitionManager::instance()->getByLanguageIndex( langIndex )
							: &SyntaxDefinitionManager::instance()->getByLanguageName(
								  syntaxState.subsyntaxInfo->syntax );
				}

				if ( curPattern->hasContentScope() ) {
					syntaxState.currentPatternIdx = state.state[i];
					syntaxState.currentLevel = i;
				} else {
					syntaxState.currentPatternIdx = {};
					syntaxState.currentLevel++;
				}
			} else {
				syntaxState.currentPatternIdx = state.state[i];
				syntaxState.currentLevel = i;
			}
		} else {
			break;
		}
	}
	return syntaxState;
}

static inline void setSubsyntaxPatternIdx( SyntaxStateRestored& curState, SyntaxState& retState,
										   const SyntaxStateType& patternIndex ) {
	curState.currentPatternIdx = patternIndex;
	retState.state[curState.currentLevel] = patternIndex;
}

static inline void pushStack( SyntaxStateRestored& curState, SyntaxState& retState,
							  const SyntaxPattern& enteringSubsyntax,
							  const SyntaxStateType& patternIndex,
							  std::string_view patternTextStr ) {
	if ( curState.currentLevel == MAX_SUB_SYNTAXS - 1 )
		return;

	if ( !enteringSubsyntax.hasSyntax() ) {
		if ( retState.langStack[curState.currentLevel] != 0 ||
			 retState.state[curState.currentLevel].state != 0 )
			curState.currentLevel++;
		curState.subsyntaxInfo = &enteringSubsyntax;
		retState.langStack[curState.currentLevel] = curState.currentSyntax->getLanguageIndex();
		setSubsyntaxPatternIdx( curState, retState, patternIndex );
		return;
	}

	setSubsyntaxPatternIdx( curState, retState, patternIndex );

	curState.subsyntaxInfo = &enteringSubsyntax;
	curState.currentSyntax = &SyntaxDefinitionManager::instance()->getByLanguageName(
		curState.subsyntaxInfo->dynSyntax
			? curState.subsyntaxInfo->dynSyntax( enteringSubsyntax, patternTextStr )
			: curState.subsyntaxInfo->syntax );

	retState.langStack[curState.currentLevel] = curState.currentSyntax->getLanguageIndex();
	curState.currentLevel++;

	setSubsyntaxPatternIdx( curState, retState, SyntaxStateType{} );
}

static inline void popStack( SyntaxStateRestored& curState, SyntaxState& retState,
							 const SyntaxDefinition& syntax, const SyntaxPattern& fromPattern ) {
	if ( curState.currentLevel == 0 && retState.state[0].state == SYNTAX_TOKENIZER_STATE_NONE ) {
		Log::debug( "Attempted to pop base stack level or already at an empty base." );
		return;
	}

	auto languageIndex = retState.langStack[curState.currentLevel];

	retState.langStack[curState.currentLevel] = 0;
	setSubsyntaxPatternIdx( curState, retState, SyntaxStateType{} );

	if ( fromPattern.isSimpleRangedMatch() ) {
		curState = SyntaxTokenizer::retrieveSyntaxState( syntax, retState );
		return;
	}

	if ( curState.currentLevel > 0 )
		curState.currentLevel--;

	if ( retState.langStack[curState.currentLevel] != 0 &&
		 retState.langStack[curState.currentLevel] != syntax.getLanguageIndex() ) {
		retState.langStack[curState.currentLevel] = 0;
		setSubsyntaxPatternIdx( curState, retState, SyntaxStateType{} );

		// Remove all the removed language stack
		while ( retState.langStack[curState.currentLevel] == languageIndex &&
				retState.langStack[curState.currentLevel] != 0 ) {
			if ( curState.currentLevel > 0 )
				curState.currentLevel--;
			retState.langStack[curState.currentLevel] = 0;
			setSubsyntaxPatternIdx( curState, retState, SyntaxStateType{} );
		}
	}

	curState = SyntaxTokenizer::retrieveSyntaxState( syntax, retState );
}

template <typename T>
static inline void pushTokensToOpenCloseSubsyntax( int i, std::string_view textv,
												   const SyntaxPattern* subsyntaxInfo,
												   const NonEscapedMatch& rangeSubsyntax,
												   std::vector<T>& tokens, bool isClose = false ) {
	const auto& types = isClose && !subsyntaxInfo->endTypes.empty() ? subsyntaxInfo->endTypes
																	: subsyntaxInfo->types;
	if ( rangeSubsyntax.numMatches > 1 ) {
		int patternMatchStart = rangeSubsyntax.matches[0].start;
		int patternMatchEnd = rangeSubsyntax.matches[0].end;

		if ( patternMatchStart == patternMatchEnd )
			return;

		auto patternType = types[0];
		int lastStart = patternMatchStart;
		int lastEnd = patternMatchEnd;

		if ( i < patternMatchStart )
			pushToken( tokens, patternType, textv.substr( i, patternMatchStart - i ) );

		int start;
		int end;

		for ( int sidx = 1; sidx < rangeSubsyntax.numMatches; sidx++ ) {
			start = rangeSubsyntax.matches[sidx].start;
			end = rangeSubsyntax.matches[sidx].end;

			if ( start == -1 || end == -1 )
				continue;

			if ( sidx == 1 && start > lastStart ) {
				pushToken( tokens, patternType,
						   textv.substr( patternMatchStart, start - patternMatchStart ) );
			} else if ( start > lastEnd ) {
				pushToken( tokens, patternType, textv.substr( lastEnd, start - lastEnd ) );
			}

			auto ss{ textv.substr( start, end - start ) };

			pushToken( tokens, sidx < static_cast<int>( types.size() ) ? types[sidx] : types[0],
					   ss );

			if ( sidx == rangeSubsyntax.numMatches - 1 && end < patternMatchEnd ) {
				pushToken( tokens, patternType, textv.substr( end, patternMatchEnd - end ) );
			}

			lastStart = start;
			lastEnd = end;
		}
	} else {
		pushToken( tokens, types[0], textv.substr( i, rangeSubsyntax.range.second - i ) );
	}
}

template <typename T>
static inline std::pair<std::vector<T>, SyntaxState>
_tokenize( const SyntaxDefinition& syntax, const std::string& text, const SyntaxState& state,
		   const size_t& startIndex, bool skipSubSyntaxSeparator ) {
	std::vector<T> tokens;

	if ( syntax.getPatterns().empty() ) {
		pushToken( tokens, SyntaxStyleTypes::Normal, text );
		return std::make_pair( std::move( tokens ), SyntaxState{} );
	}

	std::array<PatternMatcher::Range, MAX_MATCHES> matches;
	const std::string_view textv{ text };
	SyntaxState retState = state;
	SyntaxStateRestored curState = SyntaxTokenizer::retrieveSyntaxState( syntax, state );
	std::vector<size_t> priorityMap;
	std::string_view patternText;
	size_t numMatches = 0;
	std::optional<NonEscapedMatch> shouldCloseSubSyntax;

	static constexpr auto TRIED_PATTERNS_BUFFER = MAX_TRIED_PATTERNS * sizeof( SyntaxStateType );
	std::array<std::byte, TRIED_PATTERNS_BUFFER> triedPatternsBuffer;
	std::pmr::monotonic_buffer_resource triedPatternsRes(
		triedPatternsBuffer.data(), triedPatternsBuffer.size(), std::pmr::null_memory_resource() );
	std::pmr::vector<SyntaxStateType> triedPatterns( &triedPatternsRes );

	const auto matchPattern = [&]( const SyntaxPattern& pattern, size_t& startIdx,
								   SyntaxStateType patternIndex,
								   NonEscapedMatch* endRange = nullptr ) -> bool {
		int start = 0, end = 0;
		std::variant<RegEx, LuaPattern, ParserMatcher> wordsVar =
			pattern.matchType == SyntaxPatternMatchType::LuaPattern
				? std::variant<RegEx, LuaPattern, ParserMatcher>(
					  LuaPattern( pattern.patterns[0], LuaPattern::Options::Anchored ) )
				: ( pattern.matchType == SyntaxPatternMatchType::RegEx
						? std::variant<RegEx, LuaPattern, ParserMatcher>(
							  RegEx( pattern.patterns[0], REGEX_FLAGS | RegEx::Options::Anchored ) )
						: std::variant<RegEx, LuaPattern, ParserMatcher>(
							  ParserMatcher( pattern.patterns[0] ) ) );
		PatternMatcher& words = std::visit(
			[]( auto& patternType ) -> PatternMatcher& { return patternType; }, wordsVar );
		if ( !words.isValid() ) // Skip invalid patterns
			return false;
		if ( words.matches( text, matches.data(), startIdx ) &&
			 ( numMatches = words.getNumMatches() ) > 0 ) {
			if ( shouldCloseSubSyntax ) {
				if ( shouldCloseSubSyntax->range.second >= matches[0].end ) {
					if ( !skipSubSyntaxSeparator ) {
						pushTokensToOpenCloseSubsyntax( startIdx, textv, curState.subsyntaxInfo,
														*shouldCloseSubSyntax, tokens );
					}
					popStack( curState, retState, syntax, *curState.subsyntaxInfo );
					startIdx = shouldCloseSubSyntax->range.second;
					shouldCloseSubSyntax = {};
					return true;
				}
				shouldCloseSubSyntax = {};
			}

			eeASSERT( numMatches <= MAX_MATCHES );
			if ( numMatches > MAX_MATCHES )
				numMatches = MAX_MATCHES;

			size_t previousStartIdx = startIdx; // Track position before match

			if ( numMatches > 1 ) {
				int fullMatchStart = matches[0].start;
				int fullMatchEnd = matches[0].end;

				if ( pattern.matchType == SyntaxPatternMatchType::RegEx ||
					 pattern.matchType == SyntaxPatternMatchType::Parser ||
					 std::find_if( matches.begin(), matches.end(), []( auto match ) {
						 return match.start == match.end;
					 } ) != matches.end() ) {
					priorityMap.clear();
					priorityMap.resize( fullMatchEnd - fullMatchStart, 0 );

					for ( size_t captureIndex = 1; captureIndex < numMatches; ++captureIndex ) {
						int capStart = matches[captureIndex].start;
						int capEnd = matches[captureIndex].end;

						if ( endRange && capEnd >= endRange->range.first ) {
							capEnd = endRange->range.first;
						}

						if ( capStart < fullMatchStart || capEnd > fullMatchEnd ||
							 capStart >= capEnd )
							continue;

						if ( captureIndex >= pattern.types.size() )
							break;

						for ( int k = capStart; k < capEnd; ++k )
							priorityMap[k - fullMatchStart] = captureIndex;
					}

					int currentBytePos = fullMatchStart;
					while ( currentBytePos < fullMatchEnd ) {
						size_t currentCaptureIndex = priorityMap[currentBytePos - fullMatchStart];
						SyntaxStyleType currentType =
							currentCaptureIndex < pattern.types.size()
								? pattern.types[currentCaptureIndex]
								: ( pattern.types.empty() ? SyntaxStyleTypes::Normal
														  : pattern.types[0] );

						int segmentEndBytePos = currentBytePos + 1;
						while ( segmentEndBytePos < fullMatchEnd &&
								priorityMap[segmentEndBytePos - fullMatchStart] ==
									currentCaptureIndex ) {
							segmentEndBytePos++;
						}

						std::string_view segmentText =
							textv.substr( currentBytePos, segmentEndBytePos - currentBytePos );

						if ( currentType == SyntaxStyleTypes::Symbol ||
							 currentType == SyntaxStyleTypes::Normal ) {
							SyntaxStyleType symbolType =
								curState.currentSyntax->getSymbol( segmentText );
							if ( symbolType != SyntaxStyleEmpty() ) {
								currentType = symbolType;
							} else if ( currentType == SyntaxStyleTypes::Symbol ) {
								currentType = SyntaxStyleTypes::Normal;
							}
						}

						if ( !( skipSubSyntaxSeparator && pattern.hasSyntaxOrContentScope() ) )
							pushToken( tokens, currentType, segmentText );

						currentBytePos = segmentEndBytePos;
					}

					if ( pattern.isRangedMatch() ) {
						pushStack( curState, retState, pattern, patternIndex,
								   textv.substr( fullMatchStart, fullMatchEnd - fullMatchStart ) );
					}

					startIdx = fullMatchEnd;

					if ( startIdx == previousStartIdx ) {
						triedPatterns.push_back( patternIndex );
					} else {
						triedPatterns.clear();
					}
					return true;
				} else {
					auto patternType = pattern.types[0];
					int lastStart = fullMatchStart;
					int lastEnd = fullMatchEnd;

					for ( size_t curMatch = 1; curMatch < numMatches; curMatch++ ) {
						start = matches[curMatch].start;
						end = matches[curMatch].end;

						if ( !( start == end || start < 0 || end < 0 ) &&
							 !( pattern.patterns.size() >= 3 && startIdx > 0 &&
								text[startIdx - 1] == pattern.patterns[2][0] ) ) {
							Uint8 lead = ( 0xff & ( text[start] ) );
							if ( !( lead < 0x80 ) ) {
								char* strStart = const_cast<char*>( text.c_str() + start );
								char* strEnd = strStart;
								String::utf8Next( strEnd );
								end = start + ( strEnd - strStart );
							}
							if ( curMatch == 1 && start > lastStart ) {
								pushToken( tokens, patternType,
										   textv.substr( fullMatchStart, start - fullMatchStart ) );
							} else if ( start > lastEnd ) {
								pushToken( tokens, patternType,
										   textv.substr( lastEnd, start - lastEnd ) );
							}

							patternText = textv.substr( start, end - start );
							SyntaxStyleType type =
								curMatch < pattern.types.size() &&
										( pattern.types[curMatch] == SyntaxStyleTypes::Symbol ||
										  pattern.types[curMatch] == SyntaxStyleTypes::Normal )
									? curState.currentSyntax->getSymbol( patternText )
									: SyntaxStyleEmpty();

							if ( !skipSubSyntaxSeparator || !pattern.hasSyntaxOrContentScope() ) {
								pushToken( tokens,
										   type == SyntaxStyleEmpty()
											   ? ( curMatch < pattern.types.size()
													   ? pattern.types[curMatch]
													   : pattern.types[0] )
											   : type,
										   patternText );
							}

							if ( pattern.isRangedMatch() && curMatch == numMatches - 1 &&
								 end == fullMatchEnd ) {
								pushStack(
									curState, retState, pattern, patternIndex,
									textv.substr( fullMatchStart, fullMatchEnd - fullMatchStart ) );
							}

							startIdx = end;

							if ( curMatch == numMatches - 1 && end < fullMatchEnd ) {
								pushToken( tokens, patternType,
										   textv.substr( end, fullMatchEnd - end ) );
								startIdx = fullMatchEnd;
								end = fullMatchEnd;

								if ( pattern.isRangedMatch() && curMatch == numMatches - 1 ) {
									pushStack( curState, retState, pattern, patternIndex,
											   textv.substr( fullMatchStart,
															 fullMatchEnd - fullMatchStart ) );
								}
							}

							lastStart = start;
							lastEnd = end;
						}
					}

					if ( lastEnd < fullMatchEnd ) {
						pushToken( tokens, patternType,
								   textv.substr( lastEnd, fullMatchEnd - lastEnd ) );
						startIdx = fullMatchEnd;
						if ( pattern.isRangedMatch() ) {
							pushStack(
								curState, retState, pattern, patternIndex,
								textv.substr( fullMatchStart, fullMatchEnd - fullMatchStart ) );
						}
						startIdx = fullMatchEnd;
					}

					if ( startIdx == previousStartIdx ) {
						triedPatterns.push_back( patternIndex );
					} else {
						triedPatterns.clear();
					}
					return true;
				}
			} else {
				start = matches[0].start;
				end = matches[0].end;
				if ( pattern.patterns.size() >= 3 && startIdx > 0 &&
					 text[startIdx - 1] == pattern.patterns[2][0] )
					return false;
				Uint8 lead = ( 0xff & ( text[start] ) );
				if ( !( lead < 0x80 ) ) {
					char* strStart = const_cast<char*>( text.c_str() + start );
					char* strEnd = strStart;
					String::utf8Next( strEnd );
					end = start + ( strEnd - strStart );
				}

				if ( endRange && end >= endRange->range.first ) {
					end = endRange->range.first;
					if ( start == end )
						return false;
				}

				patternText = textv.substr( start, end - start );
				SyntaxStyleType type =
					( !pattern.types.empty() && ( pattern.types[0] == SyntaxStyleTypes::Symbol ||
												  pattern.types[0] == SyntaxStyleTypes::Normal ) )
						? curState.currentSyntax->getSymbol( patternText )
						: SyntaxStyleEmpty();

				if ( !skipSubSyntaxSeparator || !pattern.hasSyntaxOrContentScope() ) {
					pushToken( tokens,
							   !pattern.types.empty() && type == SyntaxStyleEmpty()
								   ? pattern.types[0]
								   : type,
							   patternText );
				}

				if ( pattern.isRangedMatch() ) {
					pushStack( curState, retState, pattern, patternIndex, patternText );
				}

				startIdx = end;

				if ( startIdx == previousStartIdx ) {
					triedPatterns.push_back( patternIndex );
				} else {
					triedPatterns.clear();
				}
				return true;
			}
		}

		return false;
	};

	size_t size = text.size();
	size_t startIdx = startIndex;
	const SyntaxPattern* activePattern = nullptr;

	static constexpr auto PATTERN_STACK_BUFFER =
		MAX_PATTERN_STACK_SIZE * sizeof( PatternStackItem );
	std::array<std::byte, PATTERN_STACK_BUFFER> patternStackBuffer;
	std::pmr::monotonic_buffer_resource patternStackRes(
		patternStackBuffer.data(), patternStackBuffer.size(), std::pmr::null_memory_resource() );
	std::pmr::vector<PatternStackItem> patternStack( &patternStackRes );
	std::string emptyStr;

	while ( startIdx < size ) {
		bool matched = false;
		patternStack.clear();
		activePattern = nullptr;

		if ( curState.currentPatternIdx.state != SYNTAX_TOKENIZER_STATE_NONE ) {
			activePattern =
				curState.currentSyntax->getPatternFromState( curState.currentPatternIdx );
			eeASSERT( activePattern );
		}

		if ( activePattern ) {
			auto endRange = findNonEscaped(
				text, activePattern->patterns[1], startIdx,
				activePattern->patterns.size() >= 3 ? activePattern->patterns[2] : emptyStr,
				activePattern->matchType, false );

			if ( activePattern->hasContentScope() ) {
				if ( endRange.range.first == static_cast<Int64>( startIdx ) ) {
					pushTokensToOpenCloseSubsyntax( startIdx, textv, activePattern, endRange,
													tokens, true );
					popStack( curState, retState, syntax, *activePattern );
					startIdx = endRange.range.second;
					triedPatterns.clear(); // Position advanced
					continue;
				}

				const auto& contentScopeRepository =
					curState.currentSyntax->getRepository( activePattern->contentScopeRepoHash );

				auto contentScopeRepoGlobalIndex = curState.currentSyntax->getRepositoryIndex(
					activePattern->contentScopeRepoHash );

				patternStack.push_back(
					{ &contentScopeRepository.patterns, 0,
					  static_cast<SyntaxSyateHolderType>( contentScopeRepoGlobalIndex ) } );

				while ( !patternStack.empty() && !matched ) {
					PatternStackItem& current = patternStack.back();
					if ( current.index >= current.patterns->size() ) {
						patternStack.pop_back();
						continue;
					}
					const SyntaxPattern* innerPtrn = &current.patterns->data()[current.index];
					SyntaxStateType patternState = {
						static_cast<SyntaxSyateHolderType>( current.index + 1 ),
						current.repositoryIdx };
					current.index++;

					if ( innerPtrn->isRepositoryInclude() ) {
						if ( patternStack.size() + 1 >= MAX_PATTERN_STACK_SIZE )
							break;
						const auto& targetRepo =
							curState.currentSyntax->getRepository( innerPtrn->getRepositoryName() );
#ifdef EE_DEBUG
						const auto repoIndex = curState.currentSyntax->getRepositoryIndex(
							innerPtrn->getRepositoryName() );
						eeASSERT( repoIndex == innerPtrn->repositoryIdx );
#endif
						patternStack.push_back(
							{ &targetRepo.patterns, 0,
							  static_cast<SyntaxSyateHolderType>( innerPtrn->repositoryIdx ) } );
						continue;
					} else if ( innerPtrn->isRootSelfInclude() ) {
						if ( patternStack.size() + 1 >= MAX_PATTERN_STACK_SIZE )
							break;
						patternStack.push_back( { &curState.currentSyntax->getPatterns(), 0, 0 } );
						continue;
					} else if ( innerPtrn->isSourceInclude() ) {
						if ( patternStack.size() + 1 >= MAX_PATTERN_STACK_SIZE )
							break;
						const auto& targetRepo =
							curState.currentSyntax->getRepository( innerPtrn->getRepositoryName() );

						const auto repoIndex = curState.currentSyntax->getRepositoryIndex(
							innerPtrn->getRepositoryName() );

						patternStack.push_back(
							{ &targetRepo.patterns, 0,
							  static_cast<SyntaxSyateHolderType>( repoIndex ) } );
						continue;
					}

					if ( startIdx != 0 &&
						 innerPtrn->matchType == SyntaxPatternMatchType::LuaPattern &&
						 innerPtrn->patterns[0][0] == '^' )
						continue;

					if ( patternStack.size() > 1 && innerPtrn->isAutomaticallyAdded() )
						continue;

					if ( std::find( triedPatterns.begin(), triedPatterns.end(), patternState ) !=
						 triedPatterns.end() )
						continue;

					if ( matchPattern( *innerPtrn, startIdx, patternState,
									   endRange.numMatches ? &endRange : nullptr ) ) {
						matched = true;
					}
				}

				if ( matched )
					continue;

				if ( !matched && startIdx < text.size() ) {
					char* strStart = const_cast<char*>( text.c_str() + startIdx );
					char* strEnd = strStart;
					String::utf8Next( strEnd );
					int dist = strEnd - strStart;
					if ( dist > 0 ) {
						pushToken( tokens, activePattern->types[0], text.substr( startIdx, dist ) );
						startIdx += dist;
						triedPatterns.clear(); // Position advanced
					} else {
						Log::error( "Error parsing \"%s\" using syntax: %s", text.c_str(),
									syntax.getLSPName().c_str() );
						break;
					}
					continue;
				}
			} else {
				bool skip = false;

				if ( curState.subsyntaxInfo != nullptr &&
					 curState.subsyntaxInfo->patterns.size() > 1 &&
					 curState.currentSyntax->getLanguageIndex() != syntax.getLanguageIndex() ) {
					auto rangeSubsyntax =
						findNonEscaped( text, curState.subsyntaxInfo->patterns[1], startIdx,
										curState.subsyntaxInfo->patterns.size() >= 3
											? curState.subsyntaxInfo->patterns[2]
											: emptyStr,
										activePattern->matchType, false );

					if ( rangeSubsyntax.range.first != -1 &&
						 ( endRange.range.first == -1 ||
						   rangeSubsyntax.range.first < endRange.range.first ) ) {
						if ( !skipSubSyntaxSeparator ) {
							pushTokensToOpenCloseSubsyntax( startIdx, textv, curState.subsyntaxInfo,
															rangeSubsyntax, tokens, true );
						}
						popStack( curState, retState, syntax, *curState.subsyntaxInfo );
						startIdx = rangeSubsyntax.range.second;
						triedPatterns.clear(); // Position advanced
						skip = true;
					}
				}

				if ( !skip ) {
					if ( endRange.range.first != -1 ) {
						pushTokensToOpenCloseSubsyntax( startIdx, textv, activePattern, endRange,
														tokens, true );
						popStack( curState, retState, syntax, *activePattern );
						startIdx = endRange.range.second;
						triedPatterns.clear(); // Position advanced
						continue;
					} else {
						pushToken( tokens, activePattern->types[0], textv.substr( startIdx ) );
						break;
					}
				}
			}
		}

		if ( curState.subsyntaxInfo != nullptr && curState.subsyntaxInfo->patterns.size() > 1 ) {
			auto rangeSubsyntax = findNonEscaped(
				text, curState.subsyntaxInfo->patterns[1], startIdx,
				curState.subsyntaxInfo->patterns.size() >= 3 ? curState.subsyntaxInfo->patterns[2]
															 : emptyStr,
				curState.subsyntaxInfo->matchType, true );

			if ( rangeSubsyntax.range.first != -1 ) {
				shouldCloseSubSyntax = rangeSubsyntax;
			}
		}

		patternStack.push_back( { &curState.currentSyntax->getPatterns(), 0, 0 } );

		while ( !patternStack.empty() && !matched ) {
			PatternStackItem& current = patternStack.back();
			if ( current.index >= current.patterns->size() ) {
				patternStack.pop_back();
				continue;
			}
			const SyntaxPattern* pattern = &current.patterns->data()[current.index];
			current.index++;

			if ( pattern->isRepositoryInclude() ) {
				if ( patternStack.size() + 1 >= MAX_PATTERN_STACK_SIZE )
					break;

				const auto& repo =
					curState.currentSyntax->getRepository( pattern->getRepositoryName() );
				patternStack.push_back(
					{ &repo.patterns, 0,
					  static_cast<SyntaxSyateHolderType>( pattern->repositoryIdx ) } );
			} else if ( pattern->isRootSelfInclude() ) {
				if ( patternStack.size() + 1 >= MAX_PATTERN_STACK_SIZE )
					break;

				patternStack.push_back( { &curState.currentSyntax->getPatterns(), 0, 0 } );
			} else if ( pattern->isSourceInclude() ) {
				if ( patternStack.size() + 1 >= MAX_PATTERN_STACK_SIZE )
					break;

				const auto& repo =
					curState.currentSyntax->getRepository( pattern->getRepositoryName() );

				auto repoIndex =
					curState.currentSyntax->getRepositoryIndex( pattern->getRepositoryName() );

				patternStack.push_back(
					{ &repo.patterns, 0, static_cast<SyntaxSyateHolderType>( repoIndex ) } );
			} else {
				if ( startIdx != 0 && pattern->matchType == SyntaxPatternMatchType::LuaPattern &&
					 pattern->patterns[0][0] == '^' )
					continue;

				SyntaxStateType patternIndex = {
					static_cast<SyntaxSyateHolderType>( current.index ), current.repositoryIdx };

				if ( std::find( triedPatterns.begin(), triedPatterns.end(), patternIndex ) !=
					 triedPatterns.end() )
					continue;

				if ( matchPattern( *pattern, startIdx, patternIndex ) ) {
					matched = true;
					break;
				}
			}
		}

		if ( !matched && shouldCloseSubSyntax ) {
			if ( !skipSubSyntaxSeparator ) {
				pushTokensToOpenCloseSubsyntax( startIdx, textv, curState.subsyntaxInfo,
												*shouldCloseSubSyntax, tokens );
			}
			popStack( curState, retState, syntax, *curState.subsyntaxInfo );
			startIdx = shouldCloseSubSyntax->range.second;
			triedPatterns.clear(); // Position advanced
			matched = true;
			shouldCloseSubSyntax = {};
			continue;
		}

		if ( !matched && startIdx < text.size() ) {
			char* strStart = const_cast<char*>( text.c_str() + startIdx );
			char* strEnd = strStart;
			String::utf8Next( strEnd );
			int dist = strEnd - strStart;
			if ( dist > 0 ) {
				pushToken( tokens, SyntaxStyleTypes::Normal, text.substr( startIdx, dist ) );
				startIdx += dist;
				triedPatterns.clear(); // Position advanced
			} else {
				Log::error( "Error parsing \"%s\" using syntax: %s", text.c_str(),
							syntax.getLSPName().c_str() );
				break;
			}
		}
	}

	return std::make_pair( std::move( tokens ), retState );
}

std::pair<std::vector<SyntaxToken>, SyntaxState>
SyntaxTokenizer::tokenize( const SyntaxDefinition& syntax, const std::string& text,
						   const SyntaxState& state, const size_t& startIndex,
						   bool skipSubSyntaxSeparator ) {
	return _tokenize<SyntaxToken>( syntax, text, state, startIndex, skipSubSyntaxSeparator );
}

std::pair<std::vector<SyntaxTokenPosition>, SyntaxState>
SyntaxTokenizer::tokenizePosition( const SyntaxDefinition& syntax, const std::string& text,
								   const SyntaxState& state, const size_t& startIndex,
								   bool skipSubSyntaxSeparator ) {
	return _tokenize<SyntaxTokenPosition>( syntax, text, state, startIndex,
										   skipSubSyntaxSeparator );
}

std::pair<std::vector<SyntaxTokenComplete>, SyntaxState>
SyntaxTokenizer::tokenizeComplete( const SyntaxDefinition& syntax, const std::string& text,
								   const SyntaxState& state, const size_t& startIndex,
								   bool skipSubSyntaxSeparator ) {
	return _tokenize<SyntaxTokenComplete>( syntax, text, state, startIndex,
										   skipSubSyntaxSeparator );
}

Text* SyntaxTokenizer::tokenizeText( const SyntaxDefinition& syntax,
									 const SyntaxColorScheme& colorScheme, Text* text,
									 const size_t& startIndex, const size_t& endIndex,
									 bool skipSubSyntaxSeparator, const std::string& trimChars ) {

	auto tokens = SyntaxTokenizer::tokenizeComplete( syntax, text->getString(), SyntaxState{},
													 startIndex, skipSubSyntaxSeparator )
					  .first;

	if ( skipSubSyntaxSeparator || !trimChars.empty() ) {
		String txt;
		size_t c = 0;
		for ( auto& token : tokens ) {
			if ( c == 0 ) {
				auto f = token.text.find_first_not_of( trimChars );
				if ( f == std::string::npos ) {
					token.text.clear();
					token.len = 0;
				} else if ( f > 0 ) {
					token.text = token.text.substr( f );
					token.len = String::utf8Length( token.text );
				}
			} else if ( c == tokens.size() - 1 ) {
				auto f = token.text.find_last_not_of( trimChars );
				if ( f == std::string::npos ) {
					token.text.clear();
					token.len = 0;
				} else if ( f >= 0 && f + 1 <= token.text.size() ) {
					token.text = token.text.substr( 0, f + 1 );
					token.len = String::utf8Length( token.text );
				}
			}
			if ( !token.text.empty() )
				txt += token.text;
			++c;
		}
		text->setString( txt );
	}

	size_t start = startIndex;
	for ( const auto& token : tokens ) {
		if ( start < endIndex ) {
			if ( token.len > 0 )
				text->setFillColor( colorScheme.getSyntaxStyle( token.type ).color, start,
									std::min( start + token.len, endIndex ) );
			start += token.len;
		} else {
			break;
		}
	}

	return text;
}

}}} // namespace EE::UI::Doc
