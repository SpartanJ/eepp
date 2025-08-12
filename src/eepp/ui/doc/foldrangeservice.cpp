#include "eepp/ui/doc/syntaxhighlighter.hpp"
#include <eepp/system/log.hpp>
#include <eepp/ui/doc/foldrangeservice.hpp>
#include <eepp/ui/doc/textdocument.hpp>

#include <stack>

namespace EE { namespace UI { namespace Doc {

static std::vector<TextRange> findFoldingRangesBraces( TextDocument* doc ) {
	Clock c;
	std::vector<TextRange> regions;
	auto lineCount = doc->linesCount();
	if ( lineCount <= 2 )
		return regions;
	const auto& braces = doc->getSyntaxDefinition().getFoldBraces();
	std::stack<TextPosition> braceStack;
	auto highlighter = doc->getHighlighter();
	for ( size_t lineIdx = 0; lineIdx < lineCount; lineIdx++ ) {
		const auto& line = doc->line( lineIdx ).getText();
		size_t lineLength = line.length();
		for ( size_t colIdx = 0; colIdx < lineLength; colIdx++ ) {
			for ( const auto& bracePair : braces ) {
				String::StringBaseType curChar = line[colIdx];
				if ( curChar == bracePair.first ) {
					auto textPosition = TextPosition( lineIdx, colIdx );
					auto tokenType = highlighter->getTokenTypeAt( textPosition );
					if ( tokenType != SyntaxStyleTypes::String &&
						 tokenType != SyntaxStyleTypes::Comment ) {
						braceStack.push( TextPosition( lineIdx, colIdx ) );
					}
				} else if ( curChar == bracePair.second ) {
					if ( !braceStack.empty() ) {
						auto textPosition = TextPosition( lineIdx, colIdx );
						auto tokenType = highlighter->getTokenTypeAt( textPosition );
						if ( tokenType != SyntaxStyleTypes::String &&
							 tokenType != SyntaxStyleTypes::Comment ) {
							auto start = braceStack.top();
							braceStack.pop();
							if ( start.line() != static_cast<Int64>( lineIdx ) )
								regions.emplace_back( start, TextPosition( lineIdx, colIdx ) );
						}
					}
				}
			}
		}
	}
	Log::debug( "findFoldingRangesBraces for \"%s\" took %s", doc->getFilePath(),
				c.getElapsedTime().toString() );
	return regions;
}

static int countLeadingSpaces( const String& line ) {
	int count = 0;
	for ( auto ch : line ) {
		if ( ch != ' ' && ch != '\t' )
			break;
		++count;
	}
	return count;
}

static std::vector<TextRange> findFoldingRangesIndentation( TextDocument* doc ) {
	Clock c;
	std::stack<TextPosition> indentStack;
	std::vector<TextRange> regions;
	auto lineCount = doc->linesCount();
	if ( lineCount <= 2 )
		return regions;
	const auto& braces = doc->getSyntaxDefinition().getFoldBraces();
	int currentIndent = 0;

	for ( size_t lineIdx = 0; lineIdx < lineCount; lineIdx++ ) {
		const auto& line = doc->line( lineIdx ).getText();
		int newIndent = countLeadingSpaces( line );
		if ( newIndent > currentIndent ) {
			// Block starts at the previous line
			indentStack.push( { static_cast<Int64>( lineIdx - 1 ), 0 } );
		} else if ( newIndent < currentIndent && !indentStack.empty() ) {
			while ( !indentStack.empty() && indentStack.top().column() >= newIndent ) {
				auto top = indentStack.top();
				indentStack.pop();
				// End at the previous line
				regions.emplace_back( TextPosition( top.line(), 0 ),
									  TextPosition( static_cast<Int64>( lineIdx ) - 1, 0 ) );
			}
		}
		currentIndent = newIndent;
	}

	// Close any remaining open blocks
	while ( !indentStack.empty() ) {
		auto top = indentStack.top();
		indentStack.pop();
		regions.emplace_back( TextPosition( top.line() + 1, 0 ),
							  TextPosition( static_cast<Int64>( lineCount ) - 1, 0 ) );
	}

	Log::debug( "findFoldingRangesIndentation for \"%s\" took %s", doc->getFilePath(),
				c.getElapsedTime().toString() );
	return regions;
}

static std::vector<TextRange> findFoldingRangesMarkdown( TextDocument* doc ) {
	Clock c; // For performance logging, consistent with existing functions
	std::vector<TextRange> regions;
	auto lineCount = doc->linesCount();

	// Early return for small documents, as in other folding functions
	if ( lineCount <= 2 )
		return regions;

	// Stack to track open heading sections: (line number, heading level)
	std::vector<std::pair<Int64, int>> sectionStack;

	// State for code blocks
	bool inCodeBlock = false;
	Int64 codeBlockStart = -1;

	// Process each line
	for ( size_t lineIdx = 0; lineIdx < lineCount; lineIdx++ ) {
		const String& lineText = doc->line( lineIdx ).getText();
		String::View trimmed =
			String::trim( lineText.view() ); // Remove leading and trailing whitespace

		if ( inCodeBlock ) {
			// Check for code block end
			if ( String::startsWith( trimmed, "```" ) ) {
				// Ensure there's content to fold between start and end
				if ( codeBlockStart != -1 && codeBlockStart <= static_cast<Int64>( lineIdx ) - 1 ) {
					regions.emplace_back( TextPosition( codeBlockStart, 0 ), // Start at opening ```
										  TextPosition( lineIdx - 1, 0 ) // End before closing ```
					);
				}
				inCodeBlock = false;
				codeBlockStart = -1;
			}
			// Continue to next line if still in code block
		} else {
			if ( String::startsWith( trimmed, "```" ) ) {
				// Start a new code block
				inCodeBlock = true;
				codeBlockStart = static_cast<Int64>( lineIdx );
			} else if ( String::startsWith( trimmed, "#" ) ) {
				// Check if it's a valid heading
				size_t hashCount = 0;
				while ( hashCount < trimmed.size() && trimmed[hashCount] == '#' )
					hashCount++;
				// Valid heading: 1-6 # symbols followed by a space
				if ( hashCount <= 6 && hashCount < trimmed.size() && trimmed[hashCount] == ' ' ) {
					int level = static_cast<int>( hashCount );

					// Close sections with equal or greater level
					while ( !sectionStack.empty() && sectionStack.back().second >= level ) {
						auto [headingLine, _] = sectionStack.back();
						sectionStack.pop_back();
						// Create folding range if there's content to fold
						if ( headingLine < static_cast<Int64>( lineIdx ) ) {
							regions.emplace_back(
								TextPosition( headingLine, 0 ), // Start at heading
								TextPosition( lineIdx - 1, 0 )	// End before next heading
							);
						}
					}
					// Open new section
					sectionStack.emplace_back( static_cast<Int64>( lineIdx ), level );
				}
			}
		}
	}

	// Close remaining open heading sections
	while ( !sectionStack.empty() ) {
		auto [headingLine, _] = sectionStack.back();
		sectionStack.pop_back();
		if ( headingLine < static_cast<Int64>( lineCount ) ) {
			regions.emplace_back( TextPosition( headingLine, 0 ),  // Start at heading
								  TextPosition( lineCount - 1, 0 ) // End at document end
			);
		}
	}

	// Close any unterminated code block
	if ( inCodeBlock && codeBlockStart != -1 && codeBlockStart < static_cast<Int64>( lineCount ) ) {
		regions.emplace_back( TextPosition( codeBlockStart, 0 ), // Start at opening ```
							  TextPosition( lineCount - 1, 0 )	 // End at document end
		);
	}

	// Log performance, matching style of existing functions
	Log::debug( "findFoldingRangesMarkdown for \"%s\" took %s", doc->getFilePath(),
				c.getElapsedTime().toString() );

	return regions;
}

FoldRangeServive::FoldRangeServive( TextDocument* doc ) : mDoc( doc ) {}

bool FoldRangeServive::canFold() const {
	if ( !mEnabled )
		return false;
	if ( mProvider && mProvider->foldingRangeProvider() )
		return true;
	auto type = mDoc->getSyntaxDefinition().getFoldRangeType();
	return type == FoldRangeType::Braces || type == FoldRangeType::Indentation ||
		   type == FoldRangeType::Markdown;
}

void FoldRangeServive::findRegions() {
	if ( !mEnabled || mDoc == nullptr || !canFold() )
		return;

	if ( mProvider && mProvider->foldingRangeProvider() ) {
		mProvider->requestFoldRange();
		return;
	}

	findRegionsNative();
}

void FoldRangeServive::findRegionsNative() {
	if ( !mEnabled || mDoc == nullptr || !canFold() )
		return;

	switch ( mDoc->getSyntaxDefinition().getFoldRangeType() ) {
		case FoldRangeType::Braces:
			setFoldingRegions( findFoldingRangesBraces( mDoc ) );
			break;
		case FoldRangeType::Indentation:
			setFoldingRegions( findFoldingRangesIndentation( mDoc ) );
		case FoldRangeType::Markdown:
			setFoldingRegions( findFoldingRangesMarkdown( mDoc ) );
		case FoldRangeType::Tag:
		case FoldRangeType::Undefined:
			break;
	}
}

void FoldRangeServive::clear() {
	Lock l( mMutex );
	mFoldingRegions.clear();
}

bool FoldRangeServive::empty() {
	Lock l( mMutex );
	return mFoldingRegions.empty();
}

std::optional<TextRange> FoldRangeServive::find( Int64 docIdx ) {
	Lock l( mMutex );
	auto foldRegionIt = mFoldingRegions.find( docIdx );
	if ( foldRegionIt == mFoldingRegions.end() )
		return {};
	return foldRegionIt->second;
}

void FoldRangeServive::addFoldRegions( std::vector<TextRange> regions ) {
	size_t newCount;
	size_t oldCount;
	{
		Lock l( mMutex );
		oldCount = mFoldingRegions.size();
		for ( auto& region : regions ) {
			region.normalize();
			mFoldingRegions[region.start().line()] = std::move( region );
		}
		newCount = mFoldingRegions.size();
	}
	mDoc->notifyFoldRegionsUpdated( oldCount, newCount );
}

bool FoldRangeServive::isFoldingRegionInLine( Int64 docIdx ) {
	Lock l( mMutex );
	auto foldRegionIt = mFoldingRegions.find( docIdx );
	return foldRegionIt != mFoldingRegions.end();
}

void FoldRangeServive::shiftFoldingRegions( Int64 fromLine, Int64 numLines ) {
	// TODO: Optimize this
	Lock l( mMutex );
	std::unordered_map<Int64, TextRange> foldingRegions;
	for ( auto& foldingRegion : mFoldingRegions ) {
		if ( foldingRegion.second.start().line() > fromLine ) {
			foldingRegion.second.start().setLine( foldingRegion.second.start().line() + numLines );
			foldingRegion.second.end().setLine( foldingRegion.second.end().line() + numLines );
			foldingRegions[foldingRegion.second.start().line()] = foldingRegion.second;
		}
		foldingRegions[foldingRegion.second.start().line()] = foldingRegion.second;
	}
	mFoldingRegions = foldingRegions;
}

void FoldRangeServive::setFoldingRegions( std::vector<TextRange> regions ) {
	size_t newCount = regions.size();
	size_t oldCount;
	{
		Lock l( mMutex );
		oldCount = mFoldingRegions.size();
		mFoldingRegions.clear();
		std::sort( regions.begin(), regions.end() );
		for ( auto& range : regions ) {
			auto line = range.start().line();
			mFoldingRegions[line] = std::move( range );
		}
	}
	mDoc->notifyFoldRegionsUpdated( oldCount, newCount );
}

FoldRangeProvider* FoldRangeServive::getProvider() const {
	return mProvider;
}

bool FoldRangeServive::hasProvider() const {
	return mProvider != nullptr;
}

void FoldRangeServive::setProvider( FoldRangeProvider* provider ) {
	mProvider = provider;
	if ( provider == nullptr ) {
		mFoldingRegions.clear();
	}
}

bool FoldRangeServive::isEnabled() const {
	return mEnabled;
}

void FoldRangeServive::setEnabled( bool enabled ) {
	mEnabled = enabled;
}

}}} // namespace EE::UI::Doc
