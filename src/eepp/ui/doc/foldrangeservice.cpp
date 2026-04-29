#include "eepp/ui/doc/syntaxhighlighter.hpp"
#include <eepp/system/log.hpp>
#include <eepp/ui/doc/foldrangeservice.hpp>
#include <eepp/ui/doc/textdocument.hpp>

#include <gumbo-parser/gumbo.h>

#include <stack>

namespace EE { namespace UI { namespace Doc {

static void walkGumboASTForFolding( GumboNode* node, std::vector<TextRange>& regions ) {
	if ( node->type != GUMBO_NODE_ELEMENT && node->type != GUMBO_NODE_DOCUMENT ) {
		return;
	}

	if ( node->type == GUMBO_NODE_ELEMENT ) {
		// 1. Check if both the opening and closing tags physically exist in the source text.
		// Gumbo will sometimes synthesize missing tags (like <html> or <tbody>) to fix bad HTML.
		// We only want to fold tags the user actually typed.
		if ( node->v.element.original_tag.length > 0 &&
			 node->v.element.original_end_tag.length > 0 ) {

			// 2. Gumbo's source positions are 1-indexed. eepp TextPositions are 0-indexed.
			Int64 startLine = static_cast<Int64>( node->v.element.start_pos.line ) - 1;
			Int64 endLine = static_cast<Int64>( node->v.element.end_pos.line ) - 1;

			// 3. Only create a fold region if the tag spans multiple lines.
			if ( endLine > startLine ) {
				// We create a range starting from the `<` of the opening tag
				// to the `<` of the closing tag.
				Int64 startCol = static_cast<Int64>( node->v.element.start_pos.column ) - 1;
				Int64 endCol = static_cast<Int64>( node->v.element.end_pos.column ) - 1;

				regions.emplace_back( TextPosition( startLine, startCol ),
									  TextPosition( endLine, endCol ) );
			}
		}

		// 4. Recursively walk children
		GumboVector* children = &node->v.element.children;
		for ( unsigned int i = 0; i < children->length; ++i ) {
			walkGumboASTForFolding( static_cast<GumboNode*>( children->data[i] ), regions );
		}
	} else if ( node->type == GUMBO_NODE_DOCUMENT ) {
		// Root document node, just process children
		GumboVector* children = &node->v.document.children;
		for ( unsigned int i = 0; i < children->length; ++i ) {
			walkGumboASTForFolding( static_cast<GumboNode*>( children->data[i] ), regions );
		}
	}
}

static std::vector<TextRange> findFoldingRangesTag( TextDocument* doc ) {
	Clock c;
	std::vector<TextRange> regions;

	if ( doc->linesCount() <= 2 )
		return regions;

	// Extract the full document text as a UTF-8 string for Gumbo
	std::string fullText = doc->toUtf8String();
	Log::debug( "findFoldingRangesTag for \"%s\"  doc to string took: %s", doc->getFilePath(),
				c.getElapsedTime().toString() );

	// Parse the text
	GumboOutput* output = gumbo_parse( fullText.c_str() );

	// Walk the AST and populate the folding regions
	walkGumboASTForFolding( output->root, regions );

	// Clean up
	gumbo_destroy_output( &kGumboDefaultOptions, output );

	Log::debug( "findFoldingRangesTag for \"%s\" took %s", doc->getFilePath(),
				c.getElapsedTime().toString() );

	return regions;
}

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
	static const String codeFence( "```" );
	Clock c;
	std::vector<TextRange> regions;
	auto lineCount = doc->linesCount();

	if ( lineCount <= 2 )
		return regions;

	// Stack to track open heading sections: (line number, heading level)
	std::vector<std::pair<Int64, int>> sectionStack;
	bool inCodeBlock = false;
	Int64 codeBlockStart = -1;

	for ( size_t lineIdx = 0; lineIdx < lineCount; lineIdx++ ) {
		const String& lineText = doc->line( lineIdx ).getText();
		String::View trimmed = String::trim( lineText.view() );

		if ( inCodeBlock ) {
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
				// Check if the line contains a closing ``` on the same line.
				// We search starting from index 3 (after the opening ```).
				bool isSingleLineBlock =
					trimmed.size() > 3 && trimmed.find( codeFence.view(), 3 ) != String::View::npos;

				if ( !isSingleLineBlock ) {
					// Start a new code block
					inCodeBlock = true;
					codeBlockStart = static_cast<Int64>( lineIdx );
				}
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

FoldRangeService::FoldRangeService( TextDocument* doc ) : mDoc( doc ) {}

bool FoldRangeService::canFold() const {
	if ( !mEnabled )
		return false;
	if ( mProvider && mProvider->foldingRangeProvider() )
		return true;
	return mDoc->getSyntaxDefinition().getFoldRangeType() != FoldRangeType::Undefined;
}

void FoldRangeService::findRegions() {
	if ( !mEnabled || mDoc == nullptr || !canFold() )
		return;

	if ( mProvider && mProvider->foldingRangeProvider() ) {
		mProvider->requestFoldRange();
		return;
	}

	findRegionsNative();
}

void FoldRangeService::findRegionsNative() {
	if ( !mEnabled || mDoc == nullptr || !canFold() )
		return;

	switch ( mDoc->getSyntaxDefinition().getFoldRangeType() ) {
		case FoldRangeType::Braces:
			setFoldingRegions( findFoldingRangesBraces( mDoc ) );
			break;
		case FoldRangeType::Indentation:
			setFoldingRegions( findFoldingRangesIndentation( mDoc ) );
			break;
		case FoldRangeType::Markdown:
			setFoldingRegions( findFoldingRangesMarkdown( mDoc ) );
			break;
		case FoldRangeType::Tag:
			setFoldingRegions( findFoldingRangesTag( mDoc ) );
			break;
		case FoldRangeType::Undefined:
			break;
	}
}

void FoldRangeService::clear() {
	Lock l( mMutex );
	mFoldingRegions.clear();
}

bool FoldRangeService::empty() {
	Lock l( mMutex );
	return mFoldingRegions.empty();
}

std::optional<TextRange> FoldRangeService::find( Int64 docIdx ) {
	Lock l( mMutex );
	auto foldRegionIt = mFoldingRegions.find( docIdx );
	if ( foldRegionIt == mFoldingRegions.end() )
		return {};
	return foldRegionIt->second;
}

void FoldRangeService::addFoldRegions( std::vector<TextRange> regions ) {
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

bool FoldRangeService::isFoldingRegionInLine( Int64 docIdx ) {
	Lock l( mMutex );
	auto foldRegionIt = mFoldingRegions.find( docIdx );
	return foldRegionIt != mFoldingRegions.end();
}

void FoldRangeService::shiftFoldingRegions( Int64 fromLine, Int64 numLines ) {
	Lock l( mMutex );
	FoldingRegions foldingRegions;

	if ( numLines < 0 ) {
		Int64 removedLines = -numLines;
		Int64 toLine = fromLine + removedLines;

		for ( auto& foldingRegion : mFoldingRegions ) {
			auto& range = foldingRegion.second;

			if ( range.start().line() >= toLine ) {
				range.start().setLine( range.start().line() + numLines );
				range.end().setLine( range.end().line() + numLines );
				foldingRegions[range.start().line()] = range;
			} else if ( range.start().line() <= fromLine ) {
				if ( range.end().line() >= toLine ) {
					range.end().setLine( range.end().line() + numLines );
					foldingRegions[range.start().line()] = range;
				} else if ( range.end().line() > fromLine ) {
					range.end().setLine( fromLine );
					if ( range.start().line() < range.end().line() )
						foldingRegions[range.start().line()] = range;
				} else {
					foldingRegions[range.start().line()] = range;
				}
			} else {
				if ( range.end().line() >= toLine ) {
					range.start().setLine( fromLine );
					range.end().setLine( range.end().line() + numLines );
					if ( range.start().line() < range.end().line() )
						foldingRegions[range.start().line()] = range;
				}
			}
		}
	} else {
		for ( auto& foldingRegion : mFoldingRegions ) {
			auto& range = foldingRegion.second;
			if ( range.start().line() >= fromLine ) {
				range.start().setLine( range.start().line() + numLines );
				range.end().setLine( range.end().line() + numLines );
			} else if ( range.end().line() >= fromLine ) {
				range.end().setLine( range.end().line() + numLines );
			}
			foldingRegions[range.start().line()] = range;
		}
	}

	mFoldingRegions = std::move( foldingRegions );
}

void FoldRangeService::setFoldingRegions( std::vector<TextRange> regions ) {
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

FoldRangeProvider* FoldRangeService::getProvider() const {
	return mProvider;
}

bool FoldRangeService::hasProvider() const {
	return mProvider != nullptr;
}

void FoldRangeService::setProvider( FoldRangeProvider* provider ) {
	mProvider = provider;
	if ( provider == nullptr ) {
		mFoldingRegions.clear();
	}
}

bool FoldRangeService::isEnabled() const {
	return mEnabled;
}

void FoldRangeService::setEnabled( bool enabled ) {
	mEnabled = enabled;
}

}}} // namespace EE::UI::Doc
