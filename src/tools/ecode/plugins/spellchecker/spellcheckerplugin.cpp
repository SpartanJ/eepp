#include "spellcheckerplugin.hpp"
#include "eepp/ui/abstract/uiabstractview.hpp"
#include "eepp/ui/models/itemlistmodel.hpp"
#include "eepp/window/engine.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/uipopupmenu.hpp>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ecode {

static constexpr auto SPELL_CHECKER_CMD = "typos";
static constexpr auto SPELL_CHECKER_ARGS = "--format=brief";

Plugin* SpellCheckerPlugin::New( PluginManager* pluginManager ) {
	return eeNew( SpellCheckerPlugin, ( pluginManager, false ) );
}

Plugin* SpellCheckerPlugin::NewSync( PluginManager* pluginManager ) {
	return eeNew( SpellCheckerPlugin, ( pluginManager, true ) );
}

SpellCheckerPlugin::SpellCheckerPlugin( PluginManager* pluginManager, bool sync ) :
	PluginBase( pluginManager ) {
	if ( sync ) {
		load( pluginManager );
	} else {
		mThreadPool->run( [this, pluginManager] { load( pluginManager ); } );
	}
}

SpellCheckerPlugin::~SpellCheckerPlugin() {
	waitUntilLoaded();
	mShuttingDown = true;

	if ( mWorkersCount != 0 ) {
		std::unique_lock<std::mutex> lock( mWorkMutex );
		mWorkerCondition.wait( lock, [this]() { return mWorkersCount <= 0; } );
	}
}

void SpellCheckerPlugin::load( PluginManager* pluginManager ) {
	Clock clock;
	AtomicBoolScopedOp loading( mLoading, true );
	std::string path = pluginManager->getPluginsPath() + "spellchecker.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite( path, "{\n  \"config\":{},\n  \"keybindings\":{}\n}\n" ) ) {
		mConfigPath = path;
	}
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	mConfigHash = String::hash( data );

	json j;
	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error(
			"SpellCheckerPlugin::load - Error parsing config from path %s, error: %s, config "
			"file content:\n%s",
			path.c_str(), e.what(), data.c_str() );
		// Recreate it
		j = json::parse( "{\n  \"config\":{},\n  \"keybindings\":{},\n}\n", nullptr, true, true );
	}

	bool updateConfigFile = false;

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];
		if ( config.contains( "delay_time" ) )
			setDelayTime( Time::fromString( config["delay_time"].get<std::string>() ) );
		else {
			config["delay_time"] = getDelayTime().toString();
			updateConfigFile = true;
		}
	}

	if ( mKeyBindings.empty() ) {
		mKeyBindings["spellchecker-fix-typo"] = "alt+shift+return";
	}

	if ( j.contains( "keybindings" ) ) {
		auto& kb = j["keybindings"];
		auto list = { "spellchecker-fix-typo", "spellchecker-go-to-next-error",
					  "spellchecker-go-to-previous-error" };
		for ( const auto& key : list ) {
			if ( kb.contains( key ) ) {
				if ( !kb[key].empty() )
					mKeyBindings[key] = kb[key];
			} else {
				kb[key] = mKeyBindings[key];
				updateConfigFile = true;
			}
		}
	}

	if ( updateConfigFile ) {
		std::string newData = j.dump( 2 );
		if ( newData != data ) {
			FileSystem::fileWrite( path, newData );
			mConfigHash = String::hash( newData );
		}
	}

	subscribeFileSystemListener();
	mTyposFound = !Sys::which( SPELL_CHECKER_CMD ).empty();
	mReady = true;
	fireReadyCbs();
	setReady( clock.getElapsedTime() );
}

void SpellCheckerPlugin::onDocumentLoaded( TextDocument* doc ) {
	setDocDirty( doc );
}

void SpellCheckerPlugin::onRegisterDocument( TextDocument* doc ) {
	doc->setCommand( "spellchecker-fix-typo", [this]( TextDocument::Client* client ) {
		createSpellCheckAlternativesView( static_cast<UICodeEditor*>( client ) );
	} );

	doc->setCommand( "spellchecker-go-to-next-error", [this]( TextDocument::Client* client ) {
		goToNextError( static_cast<UICodeEditor*>( client ) );
	} );

	doc->setCommand( "spellchecker-go-to-previous-error", [this]( TextDocument::Client* client ) {
		goToPrevError( static_cast<UICodeEditor*>( client ) );
	} );

	setDocDirty( doc );
}

void SpellCheckerPlugin::goToNextError( UICodeEditor* editor ) {
	if ( nullptr == editor || !editor->hasDocument() )
		return;
	TextDocument* doc = &editor->getDocument();
	auto pos = doc->getSelection().start();

	Lock l( mMatchesMutex );
	auto fMatch = mMatches.find( doc );
	if ( fMatch == mMatches.end() )
		return;
	const auto& matches = fMatch->second;
	if ( matches.empty() )
		return;

	const SpellCheckerMatch* matched = nullptr;
	for ( const auto& match : matches ) {
		if ( match.first >= pos.line() ) {
			for ( const auto& m : match.second ) {
				if ( pos < m.range.normalized().start() ) {
					matched = &m;
					break;
				}
			}
			if ( matched )
				break;
		}
	}

	if ( matched != nullptr ) {
		editor->goToLine( matched->range.start() );
		mManager->getSplitter()->addCurrentPositionToNavigationHistory();
		return;
	} else if ( matches.begin()->second.front().range.start().line() != pos.line() ) {
		editor->goToLine( matches.begin()->second.front().range.start() );
		mManager->getSplitter()->addCurrentPositionToNavigationHistory();
		return;
	}
}

void SpellCheckerPlugin::goToPrevError( UICodeEditor* editor ) {
	if ( nullptr == editor || !editor->hasDocument() )
		return;
	TextDocument* doc = &editor->getDocument();
	auto pos = doc->getSelection().start();

	Lock l( mMatchesMutex );
	auto fMatch = mMatches.find( doc );
	if ( fMatch == mMatches.end() )
		return;
	auto& matches = fMatch->second;
	if ( matches.empty() )
		return;

	const SpellCheckerMatch* matched = nullptr;
	for ( auto match = matches.rbegin(); match != matches.rend(); ++match ) {
		if ( match->first <= pos.line() ) {
			for ( auto it = match->second.rbegin(); it != match->second.rend(); ++it ) {
				const auto& m = *it;
				if ( m.range.normalized().start() < pos ) {
					matched = &m;
					break;
				}
			}
			if ( matched )
				break;
		}
	}

	if ( matched != nullptr ) {
		editor->goToLine( matched->range.start() );
		mManager->getSplitter()->addCurrentPositionToNavigationHistory();
		return;
	} else if ( matches.rbegin()->second.front().range.start().line() != pos.line() ) {
		editor->goToLine( matches.rbegin()->second.front().range.start() );
		mManager->getSplitter()->addCurrentPositionToNavigationHistory();
		return;
	}
}

void SpellCheckerPlugin::onUnregisterDocument( TextDocument* doc ) {
	mDirtyDoc.erase( doc );
}

void SpellCheckerPlugin::onDocumentChanged( UICodeEditor*, TextDocument* oldDoc ) {
	mDirtyDoc.erase( oldDoc );
}

void SpellCheckerPlugin::onRegisterListeners( UICodeEditor* editor,
											  std::vector<Uint32>& listeners ) {
	listeners.push_back( editor->addEventListener(
		Event::OnTextChanged, [this, editor]( const Event* ) { setDocDirty( editor ); } ) );
}

void SpellCheckerPlugin::setDocDirty( TextDocument* doc ) {
	mDirtyDoc[doc] = std::make_unique<Clock>();
}

void SpellCheckerPlugin::setDocDirty( UICodeEditor* editor ) {
	mDirtyDoc[editor->getDocumentRef().get()] = std::make_unique<Clock>();
}

void SpellCheckerPlugin::update( UICodeEditor* editor ) {
	std::shared_ptr<TextDocument> doc = editor->getDocumentRef();
	auto it = mDirtyDoc.find( doc.get() );
	if ( it != mDirtyDoc.end() && it->second->getElapsedTime() >= mDelayTime ) {
		mDirtyDoc.erase( doc.get() );
		mThreadPool->run( [this, doc] { spellCheckDoc( doc ); } );
	}
}

void SpellCheckerPlugin::spellCheckDoc( std::shared_ptr<TextDocument> doc ) {
	if ( ( !mLanguagesDisabled.empty() &&
		   mLanguagesDisabled.find( doc->getSyntaxDefinition().getLSPName() ) !=
			   mLanguagesDisabled.end() ) || doc->isHuge() )
		return;

	ScopedOp op(
		[this, doc]() {
			std::lock_guard l( mWorkMutex );
			mWorkersCount++;
		},
		[this]() {
			{
				std::lock_guard l( mWorkMutex );
				mWorkersCount--;
			}
			mWorkerCondition.notify_all();
		} );
	if ( !mReady )
		return;

	mTyposFound = !Sys::which( SPELL_CHECKER_CMD ).empty();

	if ( !mTyposFound )
		return;

	IOStreamString fileString;
	mClock.restart();
	if ( doc->isDirty() || !doc->hasFilepath() ) {
		std::string tmpPath =
			Sys::getTempPath() + ".ecode-" + doc->getFilename() + "." + String::randString( 8 );

		doc->save( fileString, true );
		FileSystem::fileWrite( tmpPath, (Uint8*)fileString.getStreamPointer(),
							   fileString.getSize() );
		FileSystem::fileHide( tmpPath );
		runSpellChecker( doc, tmpPath );
		FileSystem::fileRemove( tmpPath );
	} else {
		runSpellChecker( doc, doc->getFilePath() );
	}
}

void SpellCheckerPlugin::runSpellChecker( std::shared_ptr<TextDocument> doc,
										  const std::string& path ) {
	std::string args( String::format( "%s \"%s\"", SPELL_CHECKER_ARGS, path ) );
	std::unique_ptr<Process> process = std::make_unique<Process>();
	TextDocument* docPtr = doc.get();
	ScopedOp op(
		[this, &process, &docPtr] {
			std::lock_guard l( mRunningProcessesMutex );
			auto found = mRunningProcesses.find( docPtr );
			if ( found != mRunningProcesses.end() ) {
				found->second->kill();
				eeASSERT( found->second != process.get() );
			}
			mRunningProcesses[docPtr] = process.get();
		},
		[this, &docPtr] {
			std::lock_guard l( mRunningProcessesMutex );
			mRunningProcesses.erase( docPtr );
		} );

	if ( process->create( SPELL_CHECKER_CMD, args,
						  Process::getDefaultOptions() | Process::CombinedStdoutStderr, {},
						  mManager->getWorkspaceFolder() ) ) {
		int returnCode = 0;
		std::string data;
		process->readAllStdOut( data, Seconds( 30 ) );

		if ( mShuttingDown ) {
			process->kill();
			return;
		}

		if ( process->isAlive() ) {
			process->join( &returnCode );
			process->destroy();
		} else if ( process->killed() ) {
			return;
		}

		if ( 0 != returnCode && 2 != returnCode ) {
			Lock matchesLock( mMatchesMutex );
			std::map<Int64, std::vector<SpellCheckerMatch>> empty;
			setMatches( doc.get(), std::move( empty ) );
			return;
		}

		std::map<Int64, std::vector<SpellCheckerMatch>> matches;
		size_t totalMatches = 0;

		String::readBySeparator( data, [doc, &matches, &totalMatches]( std::string_view chunk ) {
			LuaPattern pattern( "[^:]:(%d+):(%d+):%s`(.-)`%s?%-%>%s?([^\n]*)" );
			std::string data{ chunk };
			for ( auto& match : pattern.gmatch( data ) ) {
				SpellCheckerMatch spellCheckerMatch;
				std::string lineStr = match.group( 1 );
				std::string colStr = match.group( 2 );
				spellCheckerMatch.text = match.group( 3 );
				String::trimInPlace( spellCheckerMatch.text );
				String::trimInPlace( spellCheckerMatch.text, '\n' );
				Int64 line;
				Int64 col = 1;
				if ( !spellCheckerMatch.text.empty() && !lineStr.empty() &&
					 String::fromString( line, lineStr ) ) {
					if ( !colStr.empty() )
						String::fromString( col, colStr );
					spellCheckerMatch.range.setStart(
						{ line > 0 ? line - 1 : 0, col > 0 ? col - 1 : 0 } );
					spellCheckerMatch.range.setEnd(
						{ spellCheckerMatch.range.start().line(),
						  spellCheckerMatch.range.start().column() +
							  (Int64)String( spellCheckerMatch.text ).size() } );
					spellCheckerMatch.range = spellCheckerMatch.range.normalized();
					spellCheckerMatch.range = doc->sanitizeRange( spellCheckerMatch.range );
					spellCheckerMatch.lineHash =
						doc->getLineHash( spellCheckerMatch.range.start().line() );

					std::vector<std::string> alternatives = String::split( match.group( 4 ), ',' );
					if ( !alternatives.empty() ) {
						for ( auto& alt : alternatives ) {
							String::trimInPlace( alt, ' ' );
							String::trimInPlace( alt, '`' );
						}
						spellCheckerMatch.alternatives = std::move( alternatives );
						matches[line - 1].emplace_back( std::move( spellCheckerMatch ) );
						totalMatches++;
					}
				}
			}
		} );

		// Log::debug( "SpellChecker result:\n%s", data.c_str() );

		setMatches( doc.get(), std::move( matches ) );

		Log::debug( "SpellCheckerPlugin::runSpellChecker with binary %s for %s took %.2fms. Found: "
					"%d matches.",
					SPELL_CHECKER_CMD, path, mClock.getElapsedTime().asMilliseconds(),
					totalMatches );
	}
}

void SpellCheckerPlugin::invalidateEditors( TextDocument* doc ) {
	Lock l( mMutex );
	for ( auto& it : mEditorDocs ) {
		if ( it.second == doc )
			it.first->invalidateDraw();
	}
}

void SpellCheckerPlugin::setMatches( TextDocument* doc,
									 std::map<Int64, std::vector<SpellCheckerMatch>>&& matches ) {
	Lock matchesLock( mMatchesMutex );
	mMatches[doc] = std::move( matches );
	invalidateEditors( doc );
}

void SpellCheckerPlugin::drawAfterLineText( UICodeEditor* editor, const Int64& index,
											Vector2f position, const Float& /*fontSize*/,
											const Float& lineHeight ) {
	Lock l( mMatchesMutex );
	auto matchIt = mMatches.find( editor->getDocumentRef().get() );
	if ( matchIt == mMatches.end() )
		return;

	std::map<Int64, std::vector<SpellCheckerMatch>>& map = matchIt->second;
	auto lineIt = map.find( index );
	if ( lineIt == map.end() )
		return;
	TextDocument* doc = matchIt->first;
	std::vector<SpellCheckerMatch>& matches = lineIt->second;

	for ( size_t i = 0; i < matches.size(); ++i ) {
		auto& match = matches[i];

		if ( match.lineHash != doc->getLineHash( index ) )
			return;

		Text line( "", editor->getFont(), editor->getFontSize() );
		Color color( editor->getColorScheme().getEditorSyntaxStyle( "notice"_sst ).color );
		line.setTabWidth( editor->getTabWidth() );
		line.setStyleConfig( editor->getFontStyleConfig() );
		line.setColor( color );

		auto rects = editor->getTextRangeRectangles( { match.range }, editor->getScreenScroll(), {},
													 lineHeight );
		if ( rects.empty() )
			continue;

		Int64 strSize = match.range.end().column() - match.range.start().column();
		Vector2f pos = rects[0].getPosition();

		if ( strSize <= 0 ) {
			strSize = 1;
			pos = { position.x, position.y };
		}

		std::string str( strSize, '~' );
		String string( str );
		line.setString( string );
		Rectf box( pos - editor->getScreenPos(), { editor->getTextWidth( string ), lineHeight } );
		match.box[editor] = box;
		line.draw( pos.x, pos.y + lineHeight * 0.5f );
	}
}

void SpellCheckerPlugin::minimapDrawBefore(
	UICodeEditor* editor, const DocumentLineRange& docLineRange, const DocumentViewLineRange&,
	const Vector2f& /*linePos*/, const Vector2f& /*lineSize*/, const Float& /*charWidth*/,
	const Float& /*gutterWidth*/, const DrawTextRangesFn& drawTextRanges ) {
	Lock l( mMatchesMutex );
	auto matchIt = mMatches.find( editor->getDocumentRef().get() );
	if ( matchIt == mMatches.end() )
		return;

	TextDocument* doc = matchIt->first;
	for ( const auto& matches : matchIt->second ) {
		for ( const auto& match : matches.second ) {
			if ( match.range.intersectsLineRange( docLineRange ) ) {
				if ( match.lineHash != doc->getLineHash( match.range.start().line() ) )
					return;
				Color col( editor->getColorScheme().getEditorSyntaxStyle( "notice"_sst ).color );
				col.blendAlpha( 100 );
				drawTextRanges( match.range, col, true );
			}
		}
	}
}

std::optional<SpellCheckerMatch>
SpellCheckerPlugin::getMatchFromTextPosition( UICodeEditor* editor, TextPosition textPos ) {

	Mutex mMatchesMutex;
	auto docMatch = mMatches.find( &editor->getDocument() );
	if ( docMatch == mMatches.end() )
		return {};
	auto lineMatch = docMatch->second.find( textPos.line() );
	if ( lineMatch == docMatch->second.end() )
		return {};
	const auto& spellCheckerMatches = lineMatch->second;
	auto checkerMatch = std::find_if(
		spellCheckerMatches.begin(), spellCheckerMatches.end(),
		[&textPos]( const SpellCheckerMatch& match ) { return match.range.contains( textPos ); } );
	if ( checkerMatch == spellCheckerMatches.end() )
		return {};
	return *checkerMatch;
}

std::optional<SpellCheckerMatch> SpellCheckerPlugin::getMatchFromScreenPos( UICodeEditor* editor,
																			Vector2f pos ) {
	return getMatchFromTextPosition( editor, editor->resolveScreenPosition( pos ) );
}

void SpellCheckerPlugin::replaceMatchWithText( const TextRange& range, const std::string& newText,
											   UICodeEditor* editor ) {
	{
		// If dirty we must cancel for safety
		Lock l( mMutex );
		if ( mDirtyDoc.find( &editor->getDocument() ) != mDirtyDoc.end() )
			return;
	}

	auto doc = editor->getDocumentRef();
	;
	auto sels = doc->getSelections();

	doc->resetSelection( { range } );
	doc->textInput( newText );

	doc->resetSelection( sels );
}

bool SpellCheckerPlugin::onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu,
											  const Vector2i& position, const Uint32& /*flags*/ ) {

	auto addFn = [this]( UIPopUpMenu* subMenu, const std::string& txtKey, const std::string& txtVal,
						 const std::string& icon = "" ) {
		subMenu
			->add( i18n( txtKey, txtVal ),
				   !icon.empty() ? findIcon( icon )->getSize( PixelDensity::dpToPxI( 12 ) )
								 : nullptr,
				   KeyBindings::keybindFormat( mKeyBindings[txtKey] ) )
			->setId( txtKey );
	};

	if ( !mTyposFound ) {
		menu->addSeparator();
		auto* subMenu = UIPopUpMenu::New();
		subMenu->addClass( "spellchecker_menu" );
		subMenu->on( Event::OnItemClicked, []( const Event* ) {
			Engine::instance()->openURI( "https://github.com/crate-ci/typos/" );
		} );
		addFn( subMenu, "spell-checker-not-installed",
			   "Install the typos tool to have spell-checking (click to open typos site)" );
		menu->addSubMenu( i18n( "spell-checker", "Spell Checker" ), nullptr, subMenu );
		return false;
	}

	auto pickedMatch = getMatchFromScreenPos( editor, position.asFloat() );
	if ( !pickedMatch )
		return false;

	menu->addSeparator();

	auto range = pickedMatch->range;
	auto* subMenu = UIPopUpMenu::New();
	subMenu->addClass( "spellchecker_menu" );
	subMenu->on( Event::OnItemClicked, [range, this, editor]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string id( item->getId() );
		replaceMatchWithText( range, id, editor );
	} );

	for ( const auto& alt : pickedMatch->alternatives )
		addFn( subMenu, alt, alt );

	menu->addSubMenu( i18n( "spell-checker", "Spell Checker" ), nullptr, subMenu );

	return false;
}

void SpellCheckerPlugin::createSpellCheckAlternativesView( UICodeEditor* editor ) {
	auto match = getMatchFromTextPosition( editor, editor->getDocument().getSelection().start() );
	if ( !match || match->alternatives.empty() )
		return;

	auto range = match->range;
	createListView( editor, ItemListOwnerModel<std::string>::create( match->alternatives ),
					[this, range, editor]( const ModelEvent* modelEvent ) {
						if ( modelEvent->getModelEventType() != ModelEventType::Open )
							return;
						auto str =
							modelEvent->getModel()->data( modelEvent->getModelIndex() ).toString();
						replaceMatchWithText( range, str, editor );
						modelEvent->getNode()->close();
					} );
}

const Time& SpellCheckerPlugin::getDelayTime() const {
	return mDelayTime;
}

void SpellCheckerPlugin::setDelayTime( const Time& delayTime ) {
	mDelayTime = delayTime;
}

} // namespace ecode
