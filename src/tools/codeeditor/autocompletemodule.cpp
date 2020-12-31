#include "autocompletemodule.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/uiscenenode.hpp>
using namespace EE::Graphics;
using namespace EE::System;

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define AUTO_COMPLETE_THREADED 1
#else
#define AUTO_COMPLETE_THREADED 0
#endif

AutoCompleteModule::AutoCompleteModule() :
#if AUTO_COMPLETE_THREADED
	AutoCompleteModule( ThreadPool::createShared( eemin<int>( 2, Sys::getCPUCount() ) ) )
#else
	AutoCompleteModule( nullptr )
#endif
{
}

AutoCompleteModule::AutoCompleteModule( std::shared_ptr<ThreadPool> pool ) :
	mSymbolPattern( "[%a][%w_]*" ),
	mBoxPadding( PixelDensity::dpToPx( Rectf( 4, 4, 4, 4 ) ) ),
	mPool( pool ) {}

AutoCompleteModule::~AutoCompleteModule() {
	mClosing = true;
	Lock l( mDocMutex );
	Lock l2( mLangSymbolsMutex );
	Lock l3( mSuggestionsMutex );
	for ( const auto& editor : mEditors ) {
		for ( auto listener : editor.second )
			editor.first->removeEventListener( listener );
		editor.first->unregisterModule( this );
	}
}

void AutoCompleteModule::onRegister( UICodeEditor* editor ) {
	Lock l( mDocMutex );
	std::vector<Uint32> listeners;
	listeners.push_back( editor->addEventListener( Event::OnDocumentLoaded,
												   [&]( const Event* ) { mDirty = true; } ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentClosed, [&]( const Event* event ) {
			Lock l( mDocMutex );
			const DocEvent* docEvent = static_cast<const DocEvent*>( event );
			TextDocument* doc = docEvent->getDoc();
			mDocs.erase( doc );
			mDocCache.erase( doc );
			mDirty = true;
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentChanged, [&, editor]( const Event* ) {
			TextDocument* oldDoc = mEditorDocs[editor];
			TextDocument* newDoc = editor->getDocumentRef().get();
			Lock l( mDocMutex );
			mDocs.erase( oldDoc );
			mDocCache.erase( oldDoc );
			mEditorDocs[editor] = newDoc;
			mDirty = true;
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnCursorPosChange, [&, editor]( const Event* ) {
			if ( !mReplacing )
				resetSuggestions( editor );
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentSyntaxDefinitionChange, [&]( const Event* ev ) {
			const DocSyntaxDefEvent* event = static_cast<const DocSyntaxDefEvent*>( ev );
			std::string oldLang = event->getOldLang();
			std::string newLang = event->getNewLang();
#if AUTO_COMPLETE_THREADED
			mPool->run(
				[&, oldLang, newLang] {
					updateLangCache( oldLang );
					updateLangCache( newLang );
				},
				[] {} );
#else
			updateLangCache( oldLang );
			updateLangCache( newLang );
#endif
		} ) );

	mEditors.insert( { editor, listeners } );
	mDocs.insert( editor->getDocumentRef().get() );
	mEditorDocs[editor] = editor->getDocumentRef().get();
	mDirty = true;
}

void AutoCompleteModule::onUnregister( UICodeEditor* editor ) {
	if ( mClosing )
		return;
	if ( mSuggestionsEditor == editor )
		resetSuggestions( editor );
	Lock l( mDocMutex );
	TextDocument* doc = mEditorDocs[editor];
	auto cbs = mEditors[editor];
	for ( auto listener : cbs )
		editor->removeEventListener( listener );
	mEditors.erase( editor );
	mEditorDocs.erase( editor );
	for ( auto editor : mEditorDocs )
		if ( editor.second == doc )
			return;
	mDocs.erase( doc );
	mDocCache.erase( doc );
	mDirty = true;
}

bool AutoCompleteModule::onKeyDown( UICodeEditor* editor, const KeyEvent& event ) {
	if ( !mSuggestions.empty() ) {
		int max = eemin<int>( mSuggestionsMaxVisible, mSuggestions.size() );
		if ( event.getKeyCode() == KEY_DOWN ) {
			if ( mSuggestionIndex + 1 < max ) {
				mSuggestionIndex += 1;
			} else {
				mSuggestionIndex = 0;
			}
			editor->invalidateDraw();
			return true;
		} else if ( event.getKeyCode() == KEY_UP ) {
			if ( mSuggestionIndex - 1 < 0 ) {
				mSuggestionIndex = max - 1;
			} else {
				mSuggestionIndex -= 1;
			}
			editor->invalidateDraw();
			return true;
		} else if ( event.getKeyCode() == KEY_ESCAPE ) {
			resetSuggestions( editor );
			editor->invalidateDraw();
			return true;
		} else if ( event.getKeyCode() == KEY_HOME ) {
			mSuggestionIndex = 0;
			editor->invalidateDraw();
			return true;
		} else if ( event.getKeyCode() == KEY_END ) {
			mSuggestionIndex = max - 1;
			editor->invalidateDraw();
			return true;
		} else if ( event.getKeyCode() == KEY_PAGEUP ) {
			mSuggestionIndex = eemax<int>( mSuggestionIndex - (int)mSuggestionsMaxVisible, 0 );
			editor->invalidateDraw();
			return true;
		} else if ( event.getKeyCode() == KEY_PAGEDOWN ) {
			mSuggestionIndex =
				eemin<int>( mSuggestionIndex + (int)mSuggestionsMaxVisible, max - 1 );
			editor->invalidateDraw();
			return true;
		} else if ( event.getKeyCode() == KEY_TAB || event.getKeyCode() == KEY_RETURN ||
					event.getKeyCode() == KEY_KP_ENTER ) {
			pickSuggestion( editor );
			return true;
		}
	} else if ( event.getKeyCode() == KEY_SPACE && ( event.getMod() & KEYMOD_CTRL ) ) {
		std::string partialSymbol( getPartialSymbol( &editor->getDocument() ) );
		if ( partialSymbol.size() >= 3 ) {
			updateSuggestions( partialSymbol, editor );
			return true;
		}
	}
	return false;
}

bool AutoCompleteModule::onKeyUp( UICodeEditor*, const KeyEvent& ) {
	return false;
}

bool AutoCompleteModule::onTextInput( UICodeEditor* editor, const TextInputEvent& ) {
	std::string partialSymbol( getPartialSymbol( &editor->getDocument() ) );
	if ( partialSymbol.size() >= 3 ) {
		updateSuggestions( partialSymbol, editor );
	} else {
		resetSuggestions( editor );
	}
	return false;
}

void AutoCompleteModule::updateDocCache( TextDocument* doc ) {
	Lock l( mDocMutex );
	Clock clock;
	auto docCache = mDocCache.find( doc );
	if ( docCache == mDocCache.end() )
		return;
	auto& cache = docCache->second;
	cache.changeId = doc->getCurrentChangeId();
	cache.symbols = getDocumentSymbols( doc );
	std::string langName( doc->getSyntaxDefinition().getLanguageName() );
	auto& lang = mLangCache[langName];
	{
		Lock l( mLangSymbolsMutex );
		lang.clear();
		for ( const auto& d : mDocCache ) {
			if ( d.first->getSyntaxDefinition().getLanguageName() == langName )
				lang.insert( d.second.symbols.begin(), d.second.symbols.end() );
		}
	}
	Log::debug( "Dictionary for %s updated in: %.2fms", doc->getFilename().c_str(),
				clock.getElapsedTime().asMilliseconds() );
}

void AutoCompleteModule::updateLangCache( const std::string& langName ) {
	Clock clock;
	auto& lang = mLangCache[langName];
	Lock l( mLangSymbolsMutex );
	Lock l2( mDocMutex );
	lang.clear();
	for ( const auto& d : mDocCache ) {
		if ( d.first->getSyntaxDefinition().getLanguageName() == langName )
			lang.insert( d.second.symbols.begin(), d.second.symbols.end() );
	}
	Log::debug( "Lang dictionary for %s updated in: %.2fms", langName.c_str(),
				clock.getElapsedTime().asMilliseconds() );
}

void AutoCompleteModule::pickSuggestion( UICodeEditor* editor ) {
	mReplacing = true;
	editor->getDocument().execute( "delete-to-previous-word" );
	editor->getDocument().textInput( mSuggestions[mSuggestionIndex] );
	mReplacing = false;
	resetSuggestions( editor );
}

std::string AutoCompleteModule::getPartialSymbol( TextDocument* doc ) {
	TextPosition end = doc->getSelection().end();
	TextPosition start = doc->startOfWord( end );
	return doc->getText( { start, end } ).toUtf8();
}

void AutoCompleteModule::update( UICodeEditor* ) {
	if ( mClock.getElapsedTime() >= mUpdateFreq || mDirty ) {
		mClock.restart();
		mDirty = false;
		Lock l( mDocMutex );
		for ( auto& doc : mDocs ) {
			if ( mDocCache[doc].changeId != doc->getCurrentChangeId() ) {
#if AUTO_COMPLETE_THREADED
				mPool->run( [&, doc] { updateDocCache( doc ); }, [] {} );
#else
				updateDocCache( doc );
#endif
			}
		}
	}
}

void AutoCompleteModule::preDraw( UICodeEditor*, const Vector2f&, const Float&,
								  const TextPosition& ) {}

void AutoCompleteModule::postDraw( UICodeEditor* editor, const Vector2f& startScroll,
								   const Float& lineHeight, const TextPosition& cursor ) {
	std::vector<std::string> suggestions;
	{
		Lock l( mSuggestionsMutex );
		if ( mSuggestions.empty() || !mSuggestionsEditor || mSuggestionsEditor != editor )
			return;
		suggestions = mSuggestions;
	}

	Primitives primitives;
	TextPosition start =
		editor->getDocument().startOfWord( editor->getDocument().startOfWord( cursor ) );
	Vector2f cursorPos( startScroll.x + editor->getXOffsetCol( start ),
						startScroll.y + cursor.line() * lineHeight + lineHeight );
	size_t largestString = 0;
	size_t max = eemin<size_t>( mSuggestionsMaxVisible, suggestions.size() );
	const SyntaxColorScheme& scheme = editor->getColorScheme();
	mRowHeight = lineHeight + mBoxPadding.Top + mBoxPadding.Bottom;
	const auto& normalStyle = scheme.getEditorSyntaxStyle( "suggestion" );
	const auto& selectedStyle = scheme.getEditorSyntaxStyle( "suggestion_selected" );
	if ( cursorPos.y + mRowHeight * max > editor->getPixelsSize().getHeight() )
		cursorPos.y -= lineHeight + mRowHeight * max;
	for ( size_t i = 0; i < max; i++ )
		largestString = eemax<size_t>( largestString, editor->getTextWidth( suggestions[i] ) );

	mBoxRect =
		Rectf( Vector2f( cursorPos.x, cursorPos.y ) - editor->getScreenPos(),
			   Sizef( largestString + mBoxPadding.Left + mBoxPadding.Right, mRowHeight * max ) );

	for ( size_t i = 0; i < max; i++ ) {
		Text text( "", editor->getFont(), editor->getFontSize() );
		text.setFillColor( mSuggestionIndex == (int)i ? selectedStyle.color : normalStyle.color );
		text.setStyle( mSuggestionIndex == (int)i ? selectedStyle.style : normalStyle.style );
		text.setString( suggestions[i] );
		primitives.setColor(
			Color( mSuggestionIndex == (int)i ? selectedStyle.background : normalStyle.background )
				.blendAlpha( editor->getAlpha() ) );
		primitives.drawRectangle(
			Rectf( Vector2f( cursorPos.x, cursorPos.y + mRowHeight * i ),
				   Sizef( largestString + mBoxPadding.Left + mBoxPadding.Right, mRowHeight ) ) );
		text.draw( cursorPos.x + mBoxPadding.Left, cursorPos.y + mRowHeight * i + mBoxPadding.Top );
	}
}

bool AutoCompleteModule::onMouseDown( UICodeEditor* editor, const Vector2i& position,
									  const Uint32& flags ) {
	if ( mSuggestions.empty() || !mSuggestionsEditor || mSuggestionsEditor != editor ||
		 !( flags & EE_BUTTON_LMASK ) )
		return false;

	Vector2f localPos( editor->convertToNodeSpace( position.asFloat() ) );
	if ( mBoxRect.contains( localPos ) )
		return true;
	return false;
}

bool AutoCompleteModule::onMouseClick( UICodeEditor* editor, const Vector2i& position,
									   const Uint32& flags ) {
	if ( mSuggestions.empty() || !mSuggestionsEditor || mSuggestionsEditor != editor ||
		 !( flags & EE_BUTTON_LMASK ) )
		return false;

	Vector2f localPos( editor->convertToNodeSpace( position.asFloat() ) );
	if ( mBoxRect.contains( localPos ) ) {
		localPos -= { mBoxRect.Left, mBoxRect.Top };
		mSuggestionIndex = localPos.y / mRowHeight;
		editor->invalidateDraw();
		return true;
	}
	return false;
}

bool AutoCompleteModule::onMouseDoubleClick( UICodeEditor* editor, const Vector2i& position,
											 const Uint32& flags ) {
	if ( mSuggestions.empty() || !mSuggestionsEditor || mSuggestionsEditor != editor ||
		 !( flags & EE_BUTTON_LMASK ) )
		return false;

	Vector2f localPos( editor->convertToNodeSpace( position.asFloat() ) );
	if ( mBoxRect.contains( localPos ) ) {
		pickSuggestion( editor );
		return true;
	}
	return false;
}

bool AutoCompleteModule::onMouseMove( UICodeEditor* editor, const Vector2i& position,
									  const Uint32& ) {
	if ( mSuggestions.empty() || !mSuggestionsEditor || mSuggestionsEditor != editor )
		return false;

	Vector2f localPos( editor->convertToNodeSpace( position.asFloat() ) );
	if ( mBoxRect.contains( localPos ) )
		editor->getUISceneNode()->setCursor( Cursor::Hand );
	else
		editor->getUISceneNode()->setCursor( !editor->isLocked() ? Cursor::IBeam : Cursor::Arrow );
	return false;
}

const Rectf& AutoCompleteModule::getBoxPadding() const {
	return mBoxPadding;
}

void AutoCompleteModule::setBoxPadding( const Rectf& boxPadding ) {
	mBoxPadding = boxPadding;
}

const Uint32& AutoCompleteModule::getSuggestionsMaxVisible() const {
	return mSuggestionsMaxVisible;
}

void AutoCompleteModule::setSuggestionsMaxVisible( const Uint32& suggestionsMaxVisible ) {
	mSuggestionsMaxVisible = suggestionsMaxVisible;
}

const Time& AutoCompleteModule::getUpdateFreq() const {
	return mUpdateFreq;
}

void AutoCompleteModule::setUpdateFreq( const Time& updateFreq ) {
	mUpdateFreq = updateFreq;
}

const std::string& AutoCompleteModule::getSymbolPattern() const {
	return mSymbolPattern;
}

void AutoCompleteModule::setSymbolPattern( const std::string& symbolPattern ) {
	mSymbolPattern = symbolPattern;
}

bool AutoCompleteModule::isDirty() const {
	return mDirty;
}

void AutoCompleteModule::setDirty( bool dirty ) {
	mDirty = dirty;
}

void AutoCompleteModule::resetSuggestions( UICodeEditor* editor ) {
	Lock l( mSuggestionsMutex );
	mSuggestionIndex = 0;
	mSuggestionsEditor = nullptr;
	mSuggestions.clear();
	if ( editor && editor->hasFocus() )
		editor->getUISceneNode()->setCursor( !editor->isLocked() ? Cursor::IBeam : Cursor::Arrow );
}

AutoCompleteModule::SymbolsList AutoCompleteModule::getDocumentSymbols( TextDocument* doc ) {
	LuaPattern pattern( mSymbolPattern );
	AutoCompleteModule::SymbolsList symbols;
	Int64 lc = doc->linesCount();
	if ( lc == 0 )
		return symbols;
	std::string current( getPartialSymbol( doc ) );
	TextPosition end = doc->getSelection().end();
	for ( Int64 i = 0; i < lc; i++ ) {
		const auto& string = doc->line( i ).getText().toUtf8();
		for ( auto& match : pattern.gmatch( string ) ) {
			std::string matchStr( match[0] );
			// Ignore the symbol if is actually the current symbol being written
			if ( matchStr.size() < 3 || ( end.line() == i && current == matchStr ) )
				continue;
			symbols.insert( std::move( matchStr ) );
		}
	}
	return symbols;
}

static std::vector<std::string> fuzzyMatchSymbols( const AutoCompleteModule::SymbolsList& symbols,
												   const std::string& match, const size_t& max ) {
	std::multimap<int, std::string, std::greater<int>> matchesMap;
	std::vector<std::string> matches;
	int score;
	for ( const auto& symbol : symbols ) {
		if ( ( score = String::fuzzyMatch( symbol, match ) ) > 0 ) {
			matchesMap.insert( { score, symbol } );
		}
	}
	for ( auto& res : matchesMap ) {
		if ( matches.size() < max )
			matches.emplace_back( res.second );
	}
	return matches;
}

void AutoCompleteModule::runUpdateSuggestions( const std::string& symbol,
											   const SymbolsList& symbols, UICodeEditor* editor ) {
	Lock l( mLangSymbolsMutex );
	Lock l2( mSuggestionsMutex );
	mSuggestions = fuzzyMatchSymbols( symbols, symbol, mSuggestionsMaxVisible );
	mSuggestionsEditor = editor;
	editor->runOnMainThread( [editor] { editor->invalidateDraw(); } );
}

void AutoCompleteModule::updateSuggestions( const std::string& symbol, UICodeEditor* editor ) {
	const std::string& lang = editor->getDocument().getSyntaxDefinition().getLanguageName();
	auto langSuggestions = mLangCache.find( lang );
	if ( langSuggestions == mLangCache.end() )
		return;
	auto& symbols = langSuggestions->second;
	{
#if AUTO_COMPLETE_THREADED
		mPool->run(
			[this, symbol, symbols, editor] { runUpdateSuggestions( symbol, symbols, editor ); },
			[] {} );
#else
		runUpdateSuggestions( symbol, symbols, editor );
#endif
	}
}
