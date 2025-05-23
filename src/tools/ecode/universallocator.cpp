#include "ecode.hpp"
#include "pathhelper.hpp"
#include "settingsmenu.hpp"
#include "universallocator.hpp"

#include <algorithm>

namespace ecode {

class LSPSymbolInfoModel : public Model {
  public:
	static std::shared_ptr<LSPSymbolInfoModel> create( UISceneNode* uiSceneNode,
													   const std::string& query,
													   const LSPSymbolInformationList& data,
													   bool displayLine = false ) {
		return std::make_shared<LSPSymbolInfoModel>( uiSceneNode, query, data, displayLine );
	}

	explicit LSPSymbolInfoModel( UISceneNode* uiSceneNode, const std::string& query,
								 const LSPSymbolInformationList& info, bool displayLine ) :
		mUISceneNode( uiSceneNode ), mQuery( query ), mInfo( info ), mDisplayLine( displayLine ) {}

	size_t rowCount( const ModelIndex& ) const override { return mInfo.size(); };

	size_t columnCount( const ModelIndex& ) const override { return 2; }

	Variant data( const ModelIndex& index, ModelRole role ) const override {
		if ( index.row() >= (Int64)mInfo.size() )
			return {};
		if ( role == ModelRole::Display ) {
			switch ( index.column() ) {
				case 0:
					return Variant( mInfo[index.row()].name.c_str() );
				case 1: {
					if ( mDisplayLine ) {
						std::string detail( !mInfo[index.row()].detail.empty()
												? mInfo[index.row()].detail + " (" +
													  mInfo[index.row()].range.toString() + ")"
												: mInfo[index.row()].range.toString() );
						return Variant( detail );
					} else {
						return Variant( mInfo[index.row()].url.getFSPath() );
					}
				}
			}
		} else if ( role == ModelRole::Icon && index.column() == 0 ) {
			return mUISceneNode->findIcon(
				LSPSymbolKindHelper::toIconString( mInfo[index.row()].kind ) );
		} else if ( role == ModelRole::Custom && index.column() == 1 ) {
			return Variant( mInfo[index.row()].range.toString() );
		}
		return {};
	}

	const LSPSymbolInformationList& getInfo() const { return mInfo; }

	LSPSymbolInformation at( const size_t& idx ) {
		eeASSERT( idx < mInfo.size() );
		return mInfo[idx];
	}

	const std::string& getQuery() const { return mQuery; }

	void append( const LSPSymbolInformationList& res ) {
		mInfo.insert( mInfo.end(), res.begin(), res.end() );
		std::sort( mInfo.begin(), mInfo.end(),
				   []( const LSPSymbolInformation& l, const LSPSymbolInformation& r ) {
					   return l.score > r.score;
				   } );
		onModelUpdate();
	}

	void setQuery( const std::string& query ) {
		if ( mQuery != query ) {
			clear();
			mQuery = query;
			onModelUpdate();
		}
	}

	void clear() { mInfo.clear(); }

  protected:
	UISceneNode* mUISceneNode{ nullptr };
	std::string mQuery;
	LSPSymbolInformationList mInfo;
	bool mDisplayLine{ false };
};

LSPSymbolInformationList fuzzyMatchTextDocumentSymbol( const LSPSymbolInformationList& list,
													   const std::string& query,
													   const size_t& limit ) {
	LSPSymbolInformationList nl;
	std::map<int, LSPSymbolInformation, std::greater<int>> matchesMap;

	for ( const auto& l : list ) {
		int score = String::fuzzyMatch( query, l.name );
		if ( score > std::numeric_limits<int>::min() )
			matchesMap.insert( { score, l } );
	}

	for ( auto& m : matchesMap ) {
		m.second.score = m.first;
		nl.emplace_back( std::move( m.second ) );
	}
	return nl;
}

static constexpr auto LOCATEBAR_MAX_VISIBLE_ITEMS = 18;
static constexpr auto LOCATEBAR_MAX_RESULTS = 100;

UniversalLocator::UniversalLocator( UICodeEditorSplitter* editorSplitter, UISceneNode* sceneNode,
									App* app ) :
	mSplitter( editorSplitter ),
	mUISceneNode( sceneNode ),
	mApp( app ),
	mCommandPalette( mApp->getThreadPool() ) {

	mLocatorProviders.push_back(
		{ ">", mUISceneNode->i18n( "search_in_command_palette", "Search in Command Palette" ),
		  [this]( auto ) {
			  showCommandPalette();
			  return true;
		  },
		  [this]( const Variant&, const ModelEvent* modelEvent ) {
			  ModelIndex idx(
				  modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 2 ) );
			  if ( idx.isValid() ) {
				  String cmd = modelEvent->getModel()->data( idx, ModelRole::Display ).toString();
				  mApp->runCommand( cmd );
				  if ( mSplitter->getCurWidget()->isType( UI_TYPE_CODEEDITOR ) &&
					   mSplitter->curEditorIsNotNull() &&
					   mSplitter->getCurEditor()->getDocument().hasCommand( cmd ) &&
					   !mUISceneNode->getRoot()->getLastChild()->isType( UI_TYPE_WINDOW ) )
					  mSplitter->getCurEditor()->setFocus();
				  if ( cmd != "open-locatebar" && cmd != "open-workspace-symbol-search" &&
					   cmd != "open-document-symbol-search" && cmd != "go-to-line" &&
					   cmd != "show-open-documents" && cmd != "open-locatebar-glob-search" ) {
					  hideLocateBar();
				  } else {
					  mLocateInput->setFocus();
				  }
			  }
		  },
		  nullptr, false } );

	mLocatorProviders.push_back(
		{ ":", mUISceneNode->i18n( "search_for_workspace_symbols", "Search for Workspace Symbols" ),
		  [this]( auto ) {
			  showWorkspaceSymbol();
			  return true;
		  },
		  nullptr } );

	mLocatorProviders.push_back(
		{ ".",
		  mUISceneNode->i18n( "search_for_symbols_in_current_document",
							  "Search for Symbols in Current Document" ),
		  [this]( auto ) {
			  showDocumentSymbol();
			  return true;
		  },
		  [this]( const Variant&, const ModelEvent* modelEvent ) {
			  Variant rangeStr( modelEvent->getModel()->data(
				  modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 1 ),
				  ModelRole::Custom ) );
			  auto range = rangeStr.isValid() ? TextRange::fromString( rangeStr.asStdString() )
											  : TextRange();
			  if ( !range.isValid() )
				  return;
			  UITab* tab = mSplitter->isDocumentOpen( URI( mCurDocURI ), true );
			  if ( tab ) {
				  tab->getTabWidget()->setTabSelected( tab );
				  UICodeEditor* editor = tab->getOwnedWidget()->asType<UICodeEditor>();
				  editor->goToLine( range.start() );
				  mSplitter->addEditorPositionToNavigationHistory( editor );
				  mLocateBarLayout->execute( "close-locatebar" );
			  }
		  } } );

	mLocatorProviders.push_back(
		{ "l",
		  mUISceneNode->i18n( "go_to_line_in_current_document", "Go To Line in Current Document" ),
		  [this]( const String& inputTxt ) {
			  if ( mSplitter->curEditorExistsAndFocused() ) {
				  String lineColInput( inputTxt.substr( 2 ) );
				  std::vector<String> parts = lineColInput.split( ':' );
				  Int64 line = 0;
				  if ( parts.size() > 0 && String::fromString( line, parts[0] ) && line - 1 >= 0 )
					  mLocateTable->setVisible( false );
			  }
			  return true;
		  },
		  nullptr,
		  [this]( const String& inputTxt ) {
			  if ( mSplitter->curEditorExistsAndFocused() ) {
				  String lineColInput( inputTxt.substr( 2 ) );
				  std::vector<String> parts = lineColInput.split( ':' );
				  Int64 line, column = 0;
				  if ( parts.size() > 0 && String::fromString( line, parts[0] ) && line - 1 >= 0 ) {
					  if ( parts.size() > 1 ) {
						  String::fromString( column, parts[1] );
						  if ( column < 0 ) {
							  column = 0;
						  }
					  }

					  if ( mSplitter->curEditorExistsAndFocused() ) {
						  mSplitter->getCurEditor()->goToLine( { line - 1, column } );
						  mSplitter->addCurrentPositionToNavigationHistory();
					  }

					  mLocateBarLayout->execute( "close-locatebar" );
					  return true;
				  }
			  }
			  return false;
		  },
		  false } );

	mLocatorProviders.push_back( { "o", mUISceneNode->i18n( "open_documents", "Open Documents" ),
								   [this]( auto ) {
									   showOpenDocuments();
									   return true;
								   },
								   nullptr, nullptr, false } );

	mLocatorProviders.push_back(
		{ "g",
		  mUISceneNode->i18n( "search_files_with_glob_match", "Search files with glob matching" ),
		  [this]( auto ) {
			  showLocateTableGlob();
			  return true;
		  },
		  nullptr, nullptr, false } );

	// clang-format off
	mLocatorProviders.push_back( { "sb", mUISceneNode->i18n( "switch_build", "Switch Build" ),
		   [this](auto) {
			showSwitchBuild();
			return true;
		   },
		   [this]( const Variant& vName, const ModelEvent* ) {
			   auto pbm = mApp->getProjectBuildManager();
			   if ( nullptr == pbm )
				   return;
			   auto cfg = pbm->getConfig();
			   std::string buildName = vName.toString();
			   if ( pbm->hasBuild( buildName ) ) {
				   cfg.buildName = buildName;
				   pbm->setConfig( cfg );
				   mLocateBarLayout->execute( "close-locatebar" );
			   }
		   } } );
	// clang-format on

	mLocatorProviders.push_back(
		{ "sbt", mUISceneNode->i18n( "switch_build_type", "Switch Build Type" ),
		  [this]( auto ) {
			  showSwitchBuildType();
			  return true;
		  },
		  [this]( const Variant& vName, const ModelEvent* ) {
			  auto pbm = mApp->getProjectBuildManager();
			  if ( nullptr == pbm )
				  return;
			  auto cfg = pbm->getConfig();
			  auto build = pbm->getBuild( cfg.buildName );
			  if ( build != nullptr ) {
				  std::string buildType = vName.toString();
				  if ( build->buildTypes().find( buildType ) != build->buildTypes().end() ) {
					  cfg.buildType = buildType;
					  pbm->setConfig( cfg );
					  mLocateBarLayout->execute( "close-locatebar" );
				  }
			  }
		  } } );

	mLocatorProviders.push_back(
		{ "srt", mUISceneNode->i18n( "switch_run_target", "Switch Run Target" ),
		  [this]( auto ) {
			  showSwitchRunTarget();
			  return true;
		  },
		  [this]( const Variant& vName, const ModelEvent* ) {
			  auto pbm = mApp->getProjectBuildManager();
			  if ( nullptr == pbm )
				  return;
			  auto cfg = pbm->getConfig();
			  auto build = pbm->getBuild( cfg.buildName );
			  if ( build != nullptr ) {
				  std::string runTarget = vName.toString();
				  auto it =
					  std::find_if( build->runConfigs().begin(), build->runConfigs().end(),
									[&runTarget]( const std::unique_ptr<ProjectBuildStep>& run ) {
										return run->name == runTarget;
									} );
				  if ( it != build->runConfigs().end() ) {
					  cfg.runName = runTarget;
					  pbm->setConfig( cfg );
					  mLocateBarLayout->execute( "close-locatebar" );
				  }
			  }
		  } } );

	mLocatorProviders.push_back(
		{ "ft", mUISceneNode->i18n( "switch_file_type", "Switch File Type" ),
		  [this]( auto ) {
			  showSwitchFileType();
			  return true;
		  },
		  [this]( const Variant& vName, const ModelEvent* ) {
			  if ( mSplitter->getCurEditor() ) {
				  const auto& df =
					  SyntaxDefinitionManager::instance()->getByLanguageName( vName.toString() );
				  mSplitter->getCurEditor()->setSyntaxDefinition( df );
				  mApp->getSettingsMenu()->updateCurrentFileType();
				  mLocateBarLayout->execute( "close-locatebar" );
			  }
		  } } );

	mApp->getPluginManager()->subscribeMessages(
		"universallocator", [this]( const PluginMessage& msg ) -> PluginRequestHandle {
			return processResponse( msg );
		} );
}

void UniversalLocator::hideLocateBar() {
	mLocateBarLayout->setVisible( false );
	mLocateTable->setVisible( false );
	mApp->getStatusBar()->updateState();
}

void UniversalLocator::toggleLocateBar() {
	if ( mLocateBarLayout->isVisible() ) {
		mLocateBarLayout->execute( "close-locatebar" );
	} else {
		showLocateBar();
	}
}

void UniversalLocator::updateFilesTable( bool useGlob ) {
	auto text = mLocateInput->getText();

	if ( pathHasPosition( text ) ) {
		auto pathAndPos = getPathAndPosition( text );
		text = pathAndPos.first;
	}

	if ( useGlob && String::startsWith( text, "g " ) )
		text = text.substr( 2 );

	if ( !mApp->isDirTreeReady() ) {
		mLocateTable->setModel(
			ProjectDirectoryTree::emptyModel( getLocatorCommands(), mApp->getCurrentProject() ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	} else if ( !mLocateInput->getText().empty() ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		mApp->getDirTree()->asyncMatchTree(
			useGlob ? ProjectDirectoryTree::MatchType::Glob
					: ProjectDirectoryTree::MatchType::Fuzzy,
			text, LOCATEBAR_MAX_RESULTS,
			[this, text]( auto res ) {
				mUISceneNode->runOnMainThread( [this, res] {
					mLocateTable->setModel( res );
					mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
					mLocateTable->scrollToTop();
					updateLocateBarSync();
				} );
			},
			mApp->getCurrentProject() );
#else
		mLocateTable->setModel(
			useGlob ? mApp->getDirTree()->globMatchTree( text, LOCATEBAR_MAX_RESULTS,
														 mApp->getCurrentProject() )
					: mApp->getDirTree()->fuzzyMatchTree( text, LOCATEBAR_MAX_RESULTS,
														  mApp->getCurrentProject() ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
		mLocateTable->scrollToTop();
#endif
	} else {
		mLocateTable->setModel( mApp->getDirTree()->asModel(
			LOCATEBAR_MAX_RESULTS, getLocatorCommands(), mApp->getCurrentProject(),
			Image::getImageExtensionsSupported() ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	}
}

void UniversalLocator::updateCommandPaletteTable() {
	if ( !mCommandPalette.isSet() )
		mCommandPalette.setCommandPalette( mApp->getMainLayout()->getCommandList(),
										   mApp->getMainLayout()->getKeyBindings() );

	if ( !mCommandPalette.isEditorSet() && mSplitter->curEditorIsNotNull() )
		mCommandPalette.setEditorCommandPalette(
			mSplitter->getCurEditor()->getDocumentRef()->getCommandList(),
			mSplitter->getCurEditor()->getKeyBindings() );

	mCommandPalette.setCurModel( mSplitter->curEditorIsNotNull() ? mCommandPalette.getEditorModel()
																 : mCommandPalette.getBaseModel() );

	auto txt( mLocateInput->getText() );
	txt.trim();

	if ( txt.size() > 1 ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		mCommandPalette.asyncFuzzyMatch( txt.substr( 1 ).trim(), 10000, [this]( auto res ) {
			mUISceneNode->runOnMainThread( [this, res] {
				mLocateTable->setModel( res );
				if ( mLocateTable->getModel()->hasChilds() )
					mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
				mLocateTable->scrollToTop();
			} );
		} );
#else
		mLocateTable->setModel( mCommandPalette.fuzzyMatch( txt.substr( 1 ).trim(), 10000 ) );
		if ( mLocateTable->getModel()->hasChilds() )
			mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
		mLocateTable->scrollToTop();
#endif
	} else if ( mCommandPalette.getCurModel() ) {
		mLocateTable->setModel( mCommandPalette.getCurModel() );
		if ( mLocateTable->getModel()->hasChilds() )
			mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	}
}

void UniversalLocator::showLocateTable() {
	mLocateTable->setVisible( true );
	Vector2f pos( mLocateInput->convertToWorldSpace( { 0, 0 } ) );
	pos.y -= mLocateTable->getPixelsSize().getHeight();
	mLocateTable->setPixelsPosition( pos );
	updateFilesTable();
}

void UniversalLocator::showLocateTableGlob() {
	mLocateTable->setVisible( true );
	Vector2f pos( mLocateInput->convertToWorldSpace( { 0, 0 } ) );
	pos.y -= mLocateTable->getPixelsSize().getHeight();
	mLocateTable->setPixelsPosition( pos );
	updateFilesTable( true );
}

void UniversalLocator::goToLine() {
	showLocateBar();
	mLocateInput->setText( "l " );
}

bool UniversalLocator::isCommand( const std::string& filename ) {
	const auto isLocator = [this]( const std::string& filename ) {
		return std::find_if( mLocatorProviders.begin(), mLocatorProviders.end(),
							 [&filename]( const LocatorProvider& provider ) {
								 return filename == provider.symbolTrigger;
							 } ) != mLocatorProviders.end();
	};

	return !filename.empty() && isLocator( filename );
}

std::optional<UniversalLocator::LocatorProvider> UniversalLocator::getLocator( const String& txt ) {
	for ( const auto& locator : mLocatorProviders )
		if ( String::startsWith( txt, locator.symbolTrigger ) )
			return locator;
	return {};
}

bool UniversalLocator::isLocator( const String& txt ) {
	if ( !txt.empty() ) {
		for ( const auto& locator : mLocatorProviders )
			if ( String::startsWith( txt, locator.symbolTrigger ) )
				return true;
	}
	return false;
}

bool UniversalLocator::tryLocator( const String& txt ) {
	if ( txt.empty() )
		return false;
	for ( const auto& locator : mLocatorProviders ) {
		if ( String::startsWith( txt, locator.symbolTrigger ) && locator.switchFn( txt ) )
			return true;
	}
	return false;
}

bool UniversalLocator::openLocator( const String& txt, const Variant& vName,
									const ModelEvent* modelEvent ) {
	for ( const auto& locator : mLocatorProviders ) {
		if ( String::startsWith( txt, locator.symbolTrigger ) && locator.openFn ) {
			locator.openFn( vName, modelEvent );
			return true;
		}
	}
	return false;
}

bool UniversalLocator::pressEnterLocator( const String& txt ) {
	for ( const auto& locator : mLocatorProviders ) {
		if ( String::startsWith( txt, locator.symbolTrigger ) && locator.pressEnterFn &&
			 locator.pressEnterFn( txt ) )
			return true;
	}
	return false;
}

void UniversalLocator::initLocateBar( UILocateBar* locateBar, UITextInput* locateInput ) {
	mLocateBarLayout = locateBar;
	mLocateInput = locateInput;
	auto addClickListener = [this]( UIWidget* widget, std::string cmd ) {
		widget->on( Event::MouseClick, [this, cmd]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
				mLocateBarLayout->execute( cmd );
			}
		} );
	};
	mLocateTable = UITableView::New();
	mLocateTable->setId( "locate_bar_table" );
	mLocateTable->setParent( mUISceneNode->getRoot() );
	mLocateTable->setHeadersVisible( false );
	mLocateTable->setVisible( false );
	mLocateInput->on( Event::OnTextChanged, [this]( const Event* ) {
		const String& inputTxt = mLocateInput->getText();
		if ( !tryLocator( inputTxt ) )
			showLocateTable();
		updateLocateBarSync();
	} );
	mLocateInput->on( Event::OnPressEnter, [this]( const Event* ) {
		KeyEvent keyEvent( mLocateTable, Event::KeyDown, KEY_RETURN, SCANCODE_UNKNOWN, 0, 0 );
		const String& inputTxt = mLocateInput->getText();
		if ( pressEnterLocator( inputTxt ) )
			return;
		mLocateTable->forceKeyDown( keyEvent );
	} );
	mLocateInput->on( Event::KeyDown, [this]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		mLocateTable->forceKeyDown( *keyEvent );
	} );
	mLocateBarLayout->setCommand( "close-locatebar", [this] {
		hideLocateBar();
		if ( mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
	} );
	mLocateBarLayout->getKeyBindings().addKeybindsString( {
		{ "escape", "close-locatebar" },
	} );
	mLocateTable->on( Event::KeyDown, [this]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_ESCAPE )
			mLocateBarLayout->execute( "close-locatebar" );
	} );
	addClickListener( mLocateBarLayout->find<UIWidget>( "locatebar_close" ), "close-locatebar" );
	mLocateTable->on( Event::OnModelEvent, [this]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			Variant vName( modelEvent->getModel()->data(
				modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 0 ),
				ModelRole::Display ) );

			if ( isCommand( vName.toString() ) ) {
				mLocateInput->setText( vName.toString() );
				return;
			}

			if ( openLocator( mLocateInput->getText(), vName, modelEvent ) )
				return;

			Variant vPath( modelEvent->getModel()->data(
				modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 1 ),
				ModelRole::Display ) );

			if ( !vPath.isValid() )
				return;

			std::string path( vPath.toString() );
			if ( path.empty() )
				return;

			Variant rangeStr( modelEvent->getModel()->data(
				modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 1 ),
				ModelRole::Custom ) );
			auto range =
				rangeStr.isValid() ? TextRange::fromString( rangeStr.toString() ) : TextRange();

			if ( FileSystem::isRelativePath( path ) )
				path = mApp->getCurrentProject() + path;

			if ( !range.isValid() && !FileSystem::isRelativePath( path ) &&
				 pathHasPosition( mLocateInput->getText() ) &&
				 String::startsWith( mLocateInput->getText().toUtf8(), path ) ) {
				auto pathAndPos = getPathAndPosition( mLocateInput->getText() );
				range = { pathAndPos.second, pathAndPos.second };
			}

			mApp->focusOrLoadFile( path, range );
			mLocateBarLayout->execute( "close-locatebar" );
		}
	} );
}

void UniversalLocator::updateLocateBarSync() {
	Float width = eeceil( mLocateInput->getPixelsSize().getWidth() );
	mLocateTable->setPixelsSize( width,
								 mLocateTable->getRowHeight() * LOCATEBAR_MAX_VISIBLE_ITEMS );
	width -= mLocateTable->getVerticalScrollBar()->getPixelsSize().getWidth();
	if ( mLocateTable->getModel() && mLocateTable->getModel()->columnCount() >= 3 ) {
		mLocateTable->setColumnsVisible( { 0, 1, 2 } );
		mLocateTable->setColumnWidth( 0, eefloor( width * 0.5 ) );
		mLocateTable->setColumnWidth( 1, eefloor( width * 0.25 ) );
		mLocateTable->setColumnWidth( 2, eefloor( width * 0.25 ) );
	} else if ( mLocateTable->getModel() && mLocateTable->getModel()->columnCount() >= 2 ) {
		mLocateTable->setColumnsVisible( { 0, 1 } );
		mLocateTable->setColumnWidth( 0, eeceil( width * 0.5 ) );
		mLocateTable->setColumnWidth( 1, width - mLocateTable->getColumnWidth( 0 ) );
	} else {
		mLocateTable->setColumnsVisible( { 0 } );
		mLocateTable->setColumnWidth( 0, width );
	}
	Vector2f pos( mLocateInput->convertToWorldSpace( { 0, 0 } ) );
	pos.y -= mLocateTable->getPixelsSize().getHeight();
	mLocateTable->setPixelsPosition( pos );
}

void UniversalLocator::updateLocateBar() {
	mLocateBarLayout->runOnMainThread( [this] { updateLocateBarSync(); } );
}

void UniversalLocator::showBar() {
	mApp->hideGlobalSearchBar();
	mApp->hideSearchBar();
	mApp->hideStatusTerminal();
	mApp->hideStatusBuildOutput();
	mApp->hideStatusAppOutput();
	mApp->getStatusBar()->hideAllElements();

	mLocateBarLayout->setVisible( true );
	mLocateInput->setFocus();
	mLocateTable->setVisible( true );
	const String& text = mLocateInput->getText();

	if ( isLocator( text ) ) {
		auto locator = *getLocator( text );
		mLocateInput->getDocument().setSelection(
			{ { 0, mLocateInput->getDocument().endOfLine( { 0, 0 } ).column() },
			  { 0, static_cast<Int64>( locator.symbolTrigger.size() ) } } );
	} else {
		mLocateInput->getDocument().selectAll();
	}

	mLocateInput->on( Event::OnSizeChange, [this]( const Event* ) { updateLocateBar(); } );
}

void UniversalLocator::showLocateBar( bool useGlob ) {
	showBar();

	if ( isLocator( mLocateInput->getText() ) )
		mLocateInput->setText( useGlob ? "g " : "" );

	if ( mApp->getDirTree() && !mLocateTable->getModel() ) {
		mLocateTable->setModel( mApp->getDirTree()->asModel(
			LOCATEBAR_MAX_RESULTS, getLocatorCommands(), mApp->getCurrentProject(),
			Image::getImageExtensionsSupported() ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	} else if ( !mLocateTable->getModel() ) {
		mLocateTable->setModel(
			ProjectDirectoryTree::emptyModel( getLocatorCommands(), mApp->getCurrentProject() ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	}

	updateLocateBar();
	mApp->getStatusBar()->updateState();
}

void UniversalLocator::showCommandPalette() {
	showBar();

	if ( mLocateInput->getText().empty() || mLocateInput->getText()[0] != '>' )
		mLocateInput->setText( "> " );

	updateCommandPaletteTable();
	updateLocateBar();
	mApp->getStatusBar()->updateState();
}

void UniversalLocator::showWorkspaceSymbol() {
	showBar();

	if ( mLocateInput->getText().empty() || mLocateInput->getText()[0] != ':' )
		mLocateInput->setText( ": " );

	requestWorkspaceSymbol();
	updateLocateBar();
	mApp->getStatusBar()->updateState();
}

void UniversalLocator::showDocumentSymbol() {
	showBar();

	if ( mLocateInput->getText().empty() || mLocateInput->getText()[0] != '.' )
		mLocateInput->setText( ". " );

	requestDocumentSymbol();
	updateLocateBar();
	mApp->getStatusBar()->updateState();
}

void UniversalLocator::showOpenDocuments() {
	showBar();

	if ( mLocateInput->getText().empty() || mLocateInput->getText()[0] != 'o' )
		mLocateInput->setText( "o " );

	if ( mLocateInput->getText().size() >= 2 )
		updateOpenDocumentsTable();
	updateLocateBar();
	mApp->getStatusBar()->updateState();
}

std::shared_ptr<FileListModel> UniversalLocator::openDocumentsModel( const std::string& pattern ) {
	std::vector<std::string> docs;

	mApp->getSplitter()->forEachDoc( [&docs]( TextDocument& doc ) {
		if ( doc.hasFilepath() &&
			 std::find( docs.begin(), docs.end(), doc.getFilePath() ) == docs.end() ) {
			docs.push_back( doc.getFilePath() );
		}
	} );

	std::sort( docs.begin(), docs.end(), []( const auto& left, const auto& right ) {
		return FileSystem::fileNameFromPath( left ) < FileSystem::fileNameFromPath( right );
	} );

	std::vector<std::string> files;
	std::vector<std::string> names;
	files.reserve( docs.size() );
	names.reserve( docs.size() );

	for ( const auto& doc : docs ) {
		names.emplace_back( FileSystem::fileNameFromPath( doc ) );
		files.emplace_back( std::move( doc ) );
	}

	if ( pattern.empty() )
		return std::make_shared<FileListModel>( std::move( files ), std::move( names ) );

	std::multimap<int, int, std::greater<int>> matchesMap;

	for ( size_t i = 0; i < names.size(); i++ ) {
		int matchName = String::fuzzyMatch( pattern, names[i] );
		int matchPath = String::fuzzyMatch( pattern, files[i] );
		int matchScore = std::max( matchName, matchPath );
		if ( matchScore > std::numeric_limits<int>::min() )
			matchesMap.insert( { std::max( matchName, matchPath ), i } );
	}

	std::vector<std::string> ffiles;
	std::vector<std::string> fnames;
	ffiles.reserve( matchesMap.size() );
	fnames.reserve( matchesMap.size() );

	for ( auto& res : matchesMap ) {
		fnames.emplace_back( std::move( names[res.second] ) );
		ffiles.emplace_back( std::move( files[res.second] ) );
	}

	return std::make_shared<FileListModel>( std::move( ffiles ), std::move( fnames ) );
}

void UniversalLocator::updateOpenDocumentsTable() {
	mLocateTable->setModel(
		openDocumentsModel( mLocateInput->getText().substr( 2 ).trim().toUtf8() ) );
	if ( mLocateTable->getModel()->hasChilds() )
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	mLocateTable->scrollToTop();
}

void UniversalLocator::showSwitchBuild() {
	showBar();

	if ( mLocateInput->getText().empty() || !String::startsWith( mLocateInput->getText(), "sb " ) )
		mLocateInput->setText( "sb " );

	if ( mLocateInput->getText().size() >= 3 )
		updateSwitchBuildTable();
	updateLocateBar();
	mApp->getStatusBar()->updateState();
}

std::shared_ptr<ItemListOwnerModel<std::string>>
UniversalLocator::openBuildModel( const std::string& pattern ) {
	if ( nullptr == mApp->getProjectBuildManager() )
		return ItemListOwnerModel<std::string>::create( {} );
	const auto& builds = mApp->getProjectBuildManager()->getBuilds();
	std::vector<std::string> buildNames;
	if ( builds.empty() )
		return ItemListOwnerModel<std::string>::create( {} );
	buildNames.reserve( builds.size() );
	for ( const auto& build : builds ) {
		if ( pattern.empty() ||
			 String::startsWith( String::toLower( build.first ), String::toLower( pattern ) ) )
			buildNames.push_back( build.first );
	}
	std::sort( buildNames.begin(), buildNames.end() );
	return ItemListOwnerModel<std::string>::create( buildNames );
}

void UniversalLocator::updateSwitchBuildTable() {
	mLocateTable->setModel( openBuildModel( mLocateInput->getText().substr( 3 ).trim().toUtf8() ) );
	if ( mLocateTable->getModel()->hasChilds() ) {
		ModelIndex idx =
			mLocateTable->findRowWithText( mApp->getProjectBuildManager()->getConfig().buildName );
		mLocateTable->getSelection().set( idx.isValid() ? idx
														: mLocateTable->getModel()->index( 0 ) );
	}
	mLocateTable->scrollToTop();
}

void UniversalLocator::showSwitchBuildType() {
	showBar();

	if ( mLocateInput->getText().empty() || !String::startsWith( mLocateInput->getText(), "sbt " ) )
		mLocateInput->setText( "sbt " );

	if ( mLocateInput->getText().size() >= 4 )
		updateSwitchBuildTypeTable();
	updateLocateBar();
	mApp->getStatusBar()->updateState();
}

void UniversalLocator::showSwitchRunTarget() {
	showBar();

	if ( mLocateInput->getText().empty() || !String::startsWith( mLocateInput->getText(), "srt " ) )
		mLocateInput->setText( "srt " );

	if ( mLocateInput->getText().size() >= 4 )
		updateSwitchRunTargetTable();
	updateLocateBar();
	mApp->getStatusBar()->updateState();
}

std::shared_ptr<ItemListOwnerModel<std::string>>
UniversalLocator::openBuildTypeModel( const std::string& pattern ) {
	if ( nullptr == mApp->getProjectBuildManager() )
		return ItemListOwnerModel<std::string>::create( {} );
	const auto& builds = mApp->getProjectBuildManager()->getBuilds();
	const auto& cfg = mApp->getProjectBuildManager()->getConfig();

	auto buildIt = builds.find( cfg.buildName );
	if ( buildIt == builds.end() )
		return ItemListOwnerModel<std::string>::create( {} );
	const auto& build = buildIt->second;
	const auto& buildTypes = build.buildTypes();
	if ( buildTypes.empty() )
		return ItemListOwnerModel<std::string>::create( {} );

	std::vector<std::string> buildTypeNames;
	buildTypeNames.reserve( buildTypes.size() );
	for ( const auto& build : buildTypes ) {
		if ( pattern.empty() ||
			 String::startsWith( String::toLower( build ), String::toLower( pattern ) ) )
			buildTypeNames.push_back( build );
	}
	std::sort( buildTypeNames.begin(), buildTypeNames.end() );
	return ItemListOwnerModel<std::string>::create( buildTypeNames );
}

std::shared_ptr<ItemListOwnerModel<std::string>>
UniversalLocator::openRunTargetModel( const std::string& pattern ) {
	if ( nullptr == mApp->getProjectBuildManager() )
		return ItemListOwnerModel<std::string>::create( {} );
	const auto& builds = mApp->getProjectBuildManager()->getBuilds();
	const auto& cfg = mApp->getProjectBuildManager()->getConfig();
	auto buildIt = builds.find( cfg.buildName );
	if ( buildIt == builds.end() )
		return ItemListOwnerModel<std::string>::create( {} );
	const auto& build = buildIt->second;
	const auto& runs = build.runConfigs();
	if ( runs.empty() )
		return ItemListOwnerModel<std::string>::create( {} );

	std::vector<std::string> runTargetNames;
	runTargetNames.reserve( runs.size() );
	for ( const auto& run : runs ) {
		if ( pattern.empty() ||
			 String::startsWith( String::toLower( run->name ), String::toLower( pattern ) ) )
			runTargetNames.push_back( run->name );
	}
	std::sort( runTargetNames.begin(), runTargetNames.end() );
	return ItemListOwnerModel<std::string>::create( runTargetNames );
}

void UniversalLocator::updateSwitchBuildTypeTable() {
	mLocateTable->setModel(
		openBuildTypeModel( mLocateInput->getText().substr( 4 ).trim().toUtf8() ) );
	if ( mLocateTable->getModel()->hasChilds() && nullptr != mApp->getProjectBuildManager() ) {
		ModelIndex idx =
			mLocateTable->findRowWithText( mApp->getProjectBuildManager()->getConfig().buildType );
		mLocateTable->getSelection().set( idx.isValid() ? idx
														: mLocateTable->getModel()->index( 0 ) );
	}
	mLocateTable->scrollToTop();
	mLocateTable->setColumnsVisible( { 0 } );
}

void UniversalLocator::updateSwitchRunTargetTable() {
	mLocateTable->setModel(
		openRunTargetModel( mLocateInput->getText().substr( 4 ).trim().toUtf8() ) );
	if ( mLocateTable->getModel()->hasChilds() && nullptr != mApp->getProjectBuildManager() ) {
		ModelIndex idx =
			mLocateTable->findRowWithText( mApp->getProjectBuildManager()->getConfig().runName );
		mLocateTable->getSelection().set( idx.isValid() ? idx
														: mLocateTable->getModel()->index( 0 ) );
	}
	mLocateTable->scrollToTop();
	mLocateTable->setColumnsVisible( { 0 } );
}

void UniversalLocator::showSwitchFileType() {
	showBar();

	if ( mLocateInput->getText().empty() || !String::startsWith( mLocateInput->getText(), "ft " ) )
		mLocateInput->setText( "ft " );

	if ( mLocateInput->getText().size() >= 3 )
		updateSwitchFileTypeTable();
	updateLocateBar();
	mApp->getStatusBar()->updateState();
}

std::shared_ptr<ItemListOwnerModel<std::string>>
UniversalLocator::openFileTypeModel( const std::string& pattern ) {
	if ( nullptr == mApp->getSplitter()->getCurEditor() )
		return ItemListOwnerModel<std::string>::create( {} );
	const auto& defs = SyntaxDefinitionManager::instance()->getDefinitions();
	std::vector<std::string> fileTypeNames;
	fileTypeNames.reserve( defs.size() );
	for ( const auto& def : defs ) {
		if ( !def.isVisible() )
			continue;
		if ( pattern.empty() || String::startsWith( String::toLower( def.getLanguageName() ),
													String::toLower( pattern ) ) )
			fileTypeNames.push_back( def.getLanguageName() );
	}
	std::sort( fileTypeNames.begin(), fileTypeNames.end() );
	return ItemListOwnerModel<std::string>::create( fileTypeNames );
}

void UniversalLocator::updateSwitchFileTypeTable() {
	mLocateTable->setModel(
		openFileTypeModel( mLocateInput->getText().substr( 3 ).trim().toUtf8() ) );
	if ( mLocateTable->getModel()->hasChilds() && mApp->getSplitter()->getCurEditor() ) {
		ModelIndex idx = mLocateTable->findRowWithText( mApp->getSplitter()
															->getCurEditor()
															->getDocumentRef()
															->getSyntaxDefinition()
															.getLanguageName() );
		mLocateTable->setSelection( idx.isValid() ? idx : mLocateTable->getModel()->index( 0 ) );
	} else {
		mLocateTable->scrollToTop();
	}
	mLocateTable->setColumnsVisible( { 0 } );
}

void UniversalLocator::onCodeEditorFocusChange( UICodeEditor* editor ) {
	if ( !mLocateTable || !mLocateTable->isVisible() )
		return;

	if ( String::startsWith( mLocateInput->getText(), ". " ) &&
		 editor->getDocument().getURI().toString() != mCurDocURI )
		showDocumentSymbol();
}

std::shared_ptr<LSPSymbolInfoModel> UniversalLocator::emptyModel( const String& defTxt,
																  const std::string& query ) {
	LSPSymbolInformation info;
	info.name = defTxt.toUtf8();
	info.url = "";
	return LSPSymbolInfoModel::create( mUISceneNode, query, { info } );
}

bool UniversalLocator::findCapability( PluginCapability capability ) {
	nlohmann::json capa;
	capa["capability"] = capability;
	capa["uri"] = getCurDocURI();
	PluginRequestHandle resp = mApp->getPluginManager()->sendRequest(
		PluginMessageType::QueryPluginCapability, PluginMessageFormat::JSON, &capa );
	if ( resp.isResponse() && resp.getResponse().data.is_boolean() )
		return resp.getResponse().data.get<bool>();
	return false;
}

String UniversalLocator::getDefQueryText( PluginCapability capability ) {
	bool hasCapability = findCapability( capability );
	if ( !hasCapability )
		return mUISceneNode->i18n( "no_running_lsp_server", "No running LSP server" );
	return mUISceneNode->i18n( "insert_search_query", "Insert search query" );
}

nlohmann::json UniversalLocator::pluginID( const PluginIDType& id ) {
	nlohmann::json r;
	r["uri"] = getCurDocURI();
	if ( id.isInteger() )
		r["id"] = id.asInt();
	else
		r["id"] = id.asString();
	return r;
}

void UniversalLocator::requestWorkspaceSymbol() {
	if ( mLocateInput->getText().empty() )
		return;
	auto txt( mLocateInput->getText().substr( 1 ).trim() );
	if ( mWorkspaceSymbolQuery != txt.toUtf8() || mWorkspaceSymbolQuery.empty() ) {
		mWorkspaceSymbolQuery = txt.toUtf8();

		if ( !mWorkspaceSymbolModel ) {
			auto defTxt = getDefQueryText( PluginCapability::WorkspaceSymbol );
			mWorkspaceSymbolModel = emptyModel( defTxt, mWorkspaceSymbolQuery );
		}
		mLocateTable->setModel( mWorkspaceSymbolModel );

		if ( mQueryWorkspaceLastId.isValid() ) {
			nlohmann::json r( pluginID( mQueryWorkspaceLastId ) );
			mApp->getPluginManager()->sendBroadcast( PluginMessageType::CancelRequest,
													 PluginMessageFormat::JSON, &r );
		}

		mApp->getThreadPool()->run( [this] {
			nlohmann::json j;
			j["query"] = mWorkspaceSymbolQuery;
			auto hdl = mApp->getPluginManager()->sendRequest( PluginMessageType::WorkspaceSymbol,
															  PluginMessageFormat::JSON, &j );
			mQueryWorkspaceLastId = hdl.id();
		} );
	} else if ( mWorkspaceSymbolModel && mWorkspaceSymbolModel.get() != mLocateTable->getModel() ) {
		mLocateTable->setModel( mWorkspaceSymbolModel );
	}
}

void UniversalLocator::updateWorkspaceSymbol( const LSPSymbolInformationList& res ) {
	mUISceneNode->runOnMainThread( [this, res] {
		if ( !mWorkspaceSymbolModel ) {
			mWorkspaceSymbolModel =
				LSPSymbolInfoModel::create( mApp->getUISceneNode(), mWorkspaceSymbolQuery, res );
		} else {
			mWorkspaceSymbolModel->setQuery( mWorkspaceSymbolQuery );
			mWorkspaceSymbolModel->append( res );
		}
		mLocateTable->setModel( mWorkspaceSymbolModel );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
		mLocateTable->scrollToTop();
	} );
}

void UniversalLocator::requestDocumentSymbol() {
	if ( mLocateInput->getText().empty() )
		return;
	auto txt( mLocateInput->getText().substr( 1 ).trim() );
	bool needsUpdate = false;
	if ( txt != mCurDocQuery || getCurDocURI() != mCurDocURI ) {
		mCurDocQuery = txt;
		mCurDocURI = getCurDocURI();
		needsUpdate = true;
	}

	mLocateTable->setModel( mTextDocumentSymbolModel );

	if ( mSplitter->curEditorIsNotNull() ) {
		if ( needsUpdate ) {
			mTextDocumentSymbolModel =
				emptyModel( getDefQueryText( PluginCapability::TextDocumentSymbol ) );
			mLocateTable->setModel( mTextDocumentSymbolModel );

			nlohmann::json j;
			j["uri"] = mCurDocURI = getCurDocURI();
			auto hdl = mApp->getPluginManager()->sendRequest(
				PluginMessageType::TextDocumentFlattenSymbol, PluginMessageFormat::JSON, &j );
		} else {
			if ( !mTextDocumentSymbolModel ) {
				mTextDocumentSymbolModel =
					emptyModel( getDefQueryText( PluginCapability::TextDocumentSymbol ) );
			}
			mLocateTable->setModel( mTextDocumentSymbolModel );
		}
	} else {
		mTextDocumentSymbolModel =
			emptyModel( mUISceneNode->i18n( "no_open_document", "No open document" ) );
		mLocateTable->setModel( mTextDocumentSymbolModel );
	}
}

void UniversalLocator::updateDocumentSymbol( const LSPSymbolInformationList& res ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
	if ( mCurDocQuery.empty() ) {
		mTextDocumentSymbolModel =
			LSPSymbolInfoModel::create( mApp->getUISceneNode(), mCurDocQuery, res, true );
		mLocateTable->setModel( mTextDocumentSymbolModel );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
		mLocateTable->scrollToTop();
	} else {
		asyncFuzzyMatchTextDocumentSymbol( res, mCurDocQuery, 100, [this]( const auto model ) {
			mTextDocumentSymbolModel = model;
			mUISceneNode->runOnMainThread( [this] {
				mLocateTable->setModel( mTextDocumentSymbolModel );
				mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
				mLocateTable->scrollToTop();
			} );
		} );
	}
#else
	mUISceneNode->runOnMainThread( [this, res] {
		if ( mCurDocQuery.empty() ) {
			mTextDocumentSymbolModel =
				LSPSymbolInfoModel::create( mApp->getUISceneNode(), mCurDocQuery, res, true );
		} else {
			mTextDocumentSymbolModel = LSPSymbolInfoModel::create(
				mApp->getUISceneNode(), mCurDocQuery,
				fuzzyMatchTextDocumentSymbol( res, mCurDocQuery, 100 ), true );
		}
		mLocateTable->setModel( mTextDocumentSymbolModel );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
		mLocateTable->scrollToTop();
	} );
#endif
}

std::string UniversalLocator::getCurDocURI() {
	return mSplitter->curEditorIsNotNull()
			   ? mSplitter->getCurEditor()->getDocument().getURI().toString()
			   : "";
}

PluginRequestHandle UniversalLocator::processResponse( const PluginMessage& msg ) {
	if ( msg.isResponse() && msg.type == PluginMessageType::WorkspaceSymbol &&
		 msg.format == PluginMessageFormat::SymbolInformation ) {
		mQueryWorkspaceLastId = PluginIDType();
		updateWorkspaceSymbol( msg.asSymbolInformation() );
	} else if ( msg.isResponse() && msg.type == PluginMessageType::TextDocumentFlattenSymbol &&
				msg.format == PluginMessageFormat::SymbolInformation ) {
		if ( String::startsWith( mLocateInput->getText(), ". " ) && msg.responseID.isString() &&
			 mCurDocURI == msg.responseID.asString() )
			updateDocumentSymbol( msg.asSymbolInformation() );
	}
	return {};
}

void UniversalLocator::asyncFuzzyMatchTextDocumentSymbol(
	const LSPSymbolInformationList& list, const std::string& query, const size_t& limit,
	std::function<void( std::shared_ptr<LSPSymbolInfoModel> )> cb ) {
	mApp->getThreadPool()->run( [this, list, query, limit, cb] {
		cb( LSPSymbolInfoModel::create(
			mUISceneNode, query, fuzzyMatchTextDocumentSymbol( list, query, limit ), true ) );
	} );
}

std::vector<ProjectDirectoryTree::CommandInfo> UniversalLocator::getLocatorCommands() const {
	std::vector<ProjectDirectoryTree::CommandInfo> vec;
	UIIcon* icon = mUISceneNode->findIcon( "chevron-right" );
	bool isOpenFolder = !mApp->getCurrentProject().empty();
	for ( const auto& locator : mLocatorProviders ) {
		if ( !isOpenFolder && locator.projectNeeded )
			continue;
		vec.push_back( { locator.symbolTrigger, locator.description, icon } );
	}
	return vec;
}

} // namespace ecode
