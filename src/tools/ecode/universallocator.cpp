#include "universallocator.hpp"
#include "ecode.hpp"

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

	void update() override { onModelUpdate(); }

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
		int matchName = String::fuzzyMatch( l.name, query );
		matchesMap.insert( { matchName, l } );
	}

	while ( matchesMap.size() > limit )
		matchesMap.erase( std::prev( matchesMap.end() ) );

	for ( auto& m : matchesMap ) {
		m.second.score = m.first;
		nl.emplace_back( std::move( m.second ) );
	}
	return nl;
}

static int LOCATEBAR_MAX_VISIBLE_ITEMS = 18;
static int LOCATEBAR_MAX_RESULTS = 100;

UniversalLocator::UniversalLocator( UICodeEditorSplitter* editorSplitter, UISceneNode* sceneNode,
									App* app ) :
	mSplitter( editorSplitter ),
	mUISceneNode( sceneNode ),
	mApp( app ),
	mCommandPalette( mApp->getThreadPool() ) {
	mApp->getPluginManager()->subscribeMessages(
		"universallocator",
		[&]( const PluginMessage& msg ) -> PluginRequestHandle { return processResponse( msg ); } );
}

void UniversalLocator::hideLocateBar() {
	mLocateBarLayout->setVisible( false );
	mLocateTable->setVisible( false );
}

void UniversalLocator::updateFilesTable() {
	if ( !mLocateInput->getText().empty() ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		mApp->getDirTree()->asyncFuzzyMatchTree(
			mLocateInput->getText(), LOCATEBAR_MAX_RESULTS, [&]( auto res ) {
				mUISceneNode->runOnMainThread( [&, res] {
					mLocateTable->setModel( res );
					mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
					mLocateTable->scrollToTop();
				} );
			} );
#else
		mLocateTable->setModel(
			mApp->getDirTree()->fuzzyMatchTree( mLocateInput->getText(), LOCATEBAR_MAX_RESULTS ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
		mLocateTable->scrollToTop();
#endif
	} else {
		mLocateTable->setModel(
			mApp->getDirTree()->asModel( LOCATEBAR_MAX_RESULTS, getLocatorCommands() ) );
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
		mCommandPalette.asyncFuzzyMatch( txt.substr( 1 ), 1000, [&]( auto res ) {
			mUISceneNode->runOnMainThread( [&, res] {
				mLocateTable->setModel( res );
				mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
				mLocateTable->scrollToTop();
			} );
		} );
#else
		mLocateTable->setModel( mCommandPalette.fuzzyMatch( txt.substr(), 1000 ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
		mLocateTable->scrollToTop();
#endif
	} else if ( mCommandPalette.getCurModel() ) {
		mLocateTable->setModel( mCommandPalette.getCurModel() );

		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	}

	mLocateTable->setColumnsVisible( { 0, 1 } );
}

void UniversalLocator::showLocateTable() {
	mLocateTable->setVisible( true );
	Vector2f pos( mLocateInput->convertToWorldSpace( { 0, 0 } ) );
	pos.y -= mLocateTable->getPixelsSize().getHeight();
	mLocateTable->setPixelsPosition( pos );
}

void UniversalLocator::goToLine() {
	showLocateBar();
	mLocateInput->setText( "l " );
}

static bool isCommand( const std::string& filename ) {
	return !filename.empty() &&
		   ( filename == "> " || filename == ": " || filename == "l " || filename == ". " );
}

void UniversalLocator::initLocateBar( UILocateBar* locateBar, UITextInput* locateInput ) {
	mLocateBarLayout = locateBar;
	mLocateInput = locateInput;
	auto addClickListener = [&]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::MouseClick, [this, cmd]( const Event* event ) {
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
	mLocateInput->addEventListener( Event::OnTextChanged, [&]( const Event* ) {
		const String& txt = mLocateInput->getText();
		if ( mSplitter->curEditorExistsAndFocused() && String::startsWith( txt, String( "l " ) ) ) {
			String number( txt.substr( 2 ) );
			Int64 val;
			if ( String::fromString( val, number ) && val - 1 >= 0 ) {
				if ( mSplitter->curEditorExistsAndFocused() )
					mSplitter->getCurEditor()->goToLine( { val - 1, 0 } );
				mLocateTable->setVisible( false );
			}
		} else if ( !txt.empty() && mLocateInput->getText()[0] == '>' ) {
			showCommandPalette();
		} else if ( !txt.empty() && mLocateInput->getText()[0] == ':' ) {
			showWorkspaceSymbol();
		} else if ( String::startsWith( txt, ". " ) ) {
			showDocumentSymbol();
		} else {
			showLocateTable();
			if ( !mApp->isDirTreeReady() )
				return;
			updateFilesTable();
		}
	} );
	mLocateInput->addEventListener( Event::OnPressEnter, [&]( const Event* ) {
		KeyEvent keyEvent( mLocateTable, Event::KeyDown, KEY_RETURN, SCANCODE_UNKNOWN, 0, 0 );
		mLocateTable->forceKeyDown( keyEvent );
	} );
	mLocateInput->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		mLocateTable->forceKeyDown( *keyEvent );
	} );
	mLocateBarLayout->setCommand( "close-locatebar", [&] {
		hideLocateBar();
		if ( mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
	} );
	mLocateBarLayout->getKeyBindings().addKeybindsString( {
		{ "escape", "close-locatebar" },
	} );
	mLocateTable->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_ESCAPE )
			mLocateBarLayout->execute( "close-locatebar" );
	} );
	addClickListener( mLocateBarLayout->find<UIWidget>( "locatebar_close" ), "close-locatebar" );
	mLocateTable->addEventListener( Event::OnModelEvent, [&]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			// Keep it simple for now, command palette has 3 columns
			if ( modelEvent->getModel()->columnCount() == 3 ) {
				ModelIndex idx(
					modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 2 ) );
				if ( idx.isValid() ) {
					auto cmd = modelEvent->getModel()->data( idx, ModelRole::Display ).toString();
					mApp->runCommand( cmd );
					if ( !mSplitter->getCurWidget()->isType( UI_TYPE_TERMINAL ) ) {
						if ( mSplitter->curEditorIsNotNull() &&
							 mSplitter->getCurEditor()->getDocument().hasCommand( cmd ) )
							mSplitter->getCurEditor()->setFocus();
					}
				}
				hideLocateBar();
			} else {
				Variant vName( modelEvent->getModel()->data(
					modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 0 ),
					ModelRole::Display ) );
				if ( isCommand( vName.toString() ) ) {
					mLocateInput->setText( vName.toString() );
					return;
				}
				Variant vPath( modelEvent->getModel()->data(
					modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 1 ),
					ModelRole::Display ) );
				if ( vPath.isValid() && !String::startsWith( mLocateInput->getText(), ". " ) ) {
					std::string path( vPath.is( Variant::Type::cstr ) ? vPath.asCStr()
																	  : vPath.asStdString() );
					if ( path.empty() )
						return;

					Variant rangeStr( modelEvent->getModel()->data(
						modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 1 ),
						ModelRole::Custom ) );
					auto range = rangeStr.isValid()
									 ? TextRange::fromString( rangeStr.asStdString() )
									 : TextRange();
					UITab* tab = mSplitter->isDocumentOpen( path, true );
					if ( !tab ) {
						FileInfo fileInfo( path );
						if ( fileInfo.exists() && fileInfo.isRegularFile() )
							mApp->loadFileFromPath( path, true, nullptr,
													[range]( UICodeEditor* editor, auto ) {
														if ( range.isValid() )
															editor->goToLine( range.start() );
													} );
					} else {
						tab->getTabWidget()->setTabSelected( tab );
						if ( range.isValid() ) {
							UICodeEditor* editor = tab->getOwnedWidget()->asType<UICodeEditor>();
							editor->goToLine( range.start() );
						}
					}
					mLocateBarLayout->execute( "close-locatebar" );
				} else {
					Variant rangeStr( modelEvent->getModel()->data(
						modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 1 ),
						ModelRole::Custom ) );
					auto range = rangeStr.isValid()
									 ? TextRange::fromString( rangeStr.asStdString() )
									 : TextRange();
					if ( !range.isValid() )
						return;
					UITab* tab = mSplitter->isDocumentOpen( URI( mCurDocURI ), true );
					if ( tab ) {
						tab->getTabWidget()->setTabSelected( tab );
						UICodeEditor* editor = tab->getOwnedWidget()->asType<UICodeEditor>();
						editor->goToLine( range.start() );
						mLocateBarLayout->execute( "close-locatebar" );
					}
				}
			}
		}
	} );
}

void UniversalLocator::updateLocateBar() {
	mLocateBarLayout->runOnMainThread( [&] {
		Float width = eeceil( mLocateInput->getPixelsSize().getWidth() );
		mLocateTable->setPixelsSize( width,
									 mLocateTable->getRowHeight() * LOCATEBAR_MAX_VISIBLE_ITEMS );
		width -= mLocateTable->getVerticalScrollBar()->getPixelsSize().getWidth();
		mLocateTable->setColumnsVisible( { 0, 1 } );
		mLocateTable->setColumnWidth( 0, eeceil( width * 0.5 ) );
		mLocateTable->setColumnWidth( 1, width - mLocateTable->getColumnWidth( 0 ) );
		Vector2f pos( mLocateInput->convertToWorldSpace( { 0, 0 } ) );
		pos.y -= mLocateTable->getPixelsSize().getHeight();
		mLocateTable->setPixelsPosition( pos );
	} );
}

void UniversalLocator::showBar() {
	mApp->hideGlobalSearchBar();
	mApp->hideSearchBar();

	mLocateBarLayout->setVisible( true );
	mLocateInput->setFocus();
	mLocateTable->setVisible( true );
	const String& text = mLocateInput->getText();

	if ( !text.empty() && ( text[0] == '>' || text[0] == ':' ) ) {
		Int64 selectFrom = 1;
		if ( text.size() >= 2 && text[1] == ' ' )
			selectFrom = 2;

		mLocateInput->getDocument().setSelection(
			{ { 0, selectFrom },
			  { 0, mLocateInput->getDocument().endOfLine( { 0, 0 } ).column() } } );
	} else {
		mLocateInput->getDocument().selectAll();
	}

	mLocateInput->addEventListener( Event::OnSizeChange,
									[&]( const Event* ) { updateLocateBar(); } );
}

void UniversalLocator::showLocateBar() {
	showBar();

	if ( !mLocateInput->getText().empty() &&
		 ( mLocateInput->getText()[0] == '>' || mLocateInput->getText()[0] == ':' ) )
		mLocateInput->setText( "" );

	if ( mApp->getDirTree() && !mLocateTable->getModel() ) {
		mLocateTable->setModel(
			mApp->getDirTree()->asModel( LOCATEBAR_MAX_RESULTS, getLocatorCommands() ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	}

	updateLocateBar();
}

void UniversalLocator::showCommandPalette() {
	showBar();

	if ( mLocateInput->getText().empty() || mLocateInput->getText()[0] != '>' )
		mLocateInput->setText( "> " );

	updateCommandPaletteTable();
	updateLocateBar();
}

void UniversalLocator::showWorkspaceSymbol() {
	showBar();

	if ( mLocateInput->getText().empty() || mLocateInput->getText()[0] != ':' )
		mLocateInput->setText( ": " );

	requestWorkspaceSymbol();
	updateLocateBar();
}

void UniversalLocator::showDocumentSymbol() {
	showBar();

	if ( mLocateInput->getText().empty() || mLocateInput->getText()[0] != '.' )
		mLocateInput->setText( ". " );

	requestDocumentSymbol();
	updateLocateBar();
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

void UniversalLocator::requestWorkspaceSymbol() {
	if ( mLocateInput->getText().empty() )
		return;
	auto txt( mLocateInput->getText().substr( 1 ).trim() );
	if ( mWorkspaceSymbolQuery != txt.toUtf8() || mWorkspaceSymbolQuery.empty() ) {
		mWorkspaceSymbolQuery = txt.toUtf8();
		if ( !mWorkspaceSymbolModel ) {
			auto defTxt = mUISceneNode->i18n( "no_running_lsp_server", "No running LSP server" );
			mWorkspaceSymbolModel = emptyModel( defTxt, mWorkspaceSymbolQuery );
		}
		mLocateTable->setModel( mWorkspaceSymbolModel );

		json j = json{ json{ "query", mWorkspaceSymbolQuery } };
		mApp->getPluginManager()->sendRequest( PluginMessageType::WorkspaceSymbol,
											   PluginMessageFormat::JSON, &j );
	} else if ( mWorkspaceSymbolModel && mWorkspaceSymbolModel.get() != mLocateTable->getModel() ) {
		mLocateTable->setModel( mWorkspaceSymbolModel );
	}
}

void UniversalLocator::updateWorkspaceSymbol( const LSPSymbolInformationList& res ) {
	mUISceneNode->runOnMainThread( [&, res] {
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
			mTextDocumentSymbolModel = emptyModel(
				mUISceneNode->i18n( "no_running_lsp_server", "No running LSP server" ) );
			json j;
			j["uri"] = mCurDocURI = getCurDocURI();
			mApp->getPluginManager()->sendRequest( PluginMessageType::TextDocumentFlattenSymbol,
												   PluginMessageFormat::JSON, &j );
		} else {
			if ( !mTextDocumentSymbolModel ) {
				mTextDocumentSymbolModel = emptyModel(
					mUISceneNode->i18n( "no_running_lsp_server", "No running LSP server" ) );
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
		asyncFuzzyMatchTextDocumentSymbol( res, mCurDocQuery, 100, [&]( const auto model ) {
			mTextDocumentSymbolModel = model;
			mUISceneNode->runOnMainThread( [&] {
				mLocateTable->setModel( mTextDocumentSymbolModel );
				mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
				mLocateTable->scrollToTop();
			} );
		} );
	}
#else
	mUISceneNode->runOnMainThread( [&, res] {
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
	vec.push_back( { "> ",
					 mUISceneNode->i18n( "search_in_command_palette", "Search in Command Palette" ),
					 icon } );
	vec.push_back(
		{ ": ",
		  mUISceneNode->i18n( "search_for_workspace_symbols", "Search for Workspace Symbols" ),
		  icon } );
	vec.push_back( { ". ",
					 mUISceneNode->i18n( "search_for_document_symbols",
										 "Search for Symbols in Current Document" ),
					 icon } );
	vec.push_back(
		{ "l ",
		  mUISceneNode->i18n( "go_to_line_in_current_document", "Go To Line in Current Document" ),
		  icon } );
	return vec;
}

} // namespace ecode
