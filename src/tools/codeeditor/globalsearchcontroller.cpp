#include "globalsearchcontroller.hpp"
#include "codeeditor.hpp"
#include "uitreeviewglobalsearch.hpp"

static int LOCATEBAR_MAX_VISIBLE_ITEMS = 18;

GlobalSearchController::GlobalSearchController( UICodeEditorSplitter* editorSplitter,
												UISceneNode* sceneNode, App* app ) :
	mEditorSplitter( editorSplitter ), mUISceneNode( sceneNode ), mApp( app ) {}

static String replaceInText( String text, const String& replaceText,
							 std::vector<TextRange> replacements ) {
	Int64 diff = 0;
	String oldText( text );
	for ( const auto& range : replacements ) {
		Int64 len = range.end().column() - range.start().column();
		auto before = text.substr( 0, range.start().column() + diff );
		auto after = !text.empty() ? text.substr( range.end().column() + diff ) : "";
		text = before + replaceText + after;
		diff += replaceText.size() - len;
	}
	return text;
}

size_t GlobalSearchController::replaceInFiles( const String& replaceText,
											   std::shared_ptr<ProjectSearch::ResultModel> model ) {
	const ProjectSearch::Result& res = model.get()->getResult();
	size_t count = 0;

	for ( const auto& fileResult : res ) {
		std::map<Int64, std::pair<String, std::vector<TextRange>>> replaceRangeMap;
		std::map<Int64, String> replaceStringMap;
		for ( const auto& result : fileResult.results ) {
			if ( result.selected ) {
				replaceRangeMap[result.position.start().line()].first = result.line;
				replaceRangeMap[result.position.start().line()].second.push_back( result.position );
				count++;
			}
		}

		for ( const auto& replace : replaceRangeMap )
			replaceStringMap[replace.second.second.front().start().line()] =
				replaceInText( replace.second.first, replaceText, replace.second.second );

		std::shared_ptr<TextDocument> doc = mEditorSplitter->findDocFromPath( fileResult.file );
		bool loaded = doc ? true : false;
		bool save = false;
		bool inMemoryAndNotDirty = doc ? !doc->isDirty() : false;

		if ( !doc ) {
			doc = std::make_shared<TextDocument>();
			loaded = doc->loadFromFile( fileResult.file );
			save = true;
		}

		if ( doc && loaded ) {
			for ( const auto& replaceString : replaceStringMap )
				doc->replaceLine( replaceString.first, replaceString.second );

			if ( save || inMemoryAndNotDirty )
				doc->save();
		}
	}

	return count;
}

void GlobalSearchController::initGlobalSearchBar( UIGlobalSearchBar* globalSearchBar ) {
	mGlobalSearchBarLayout = globalSearchBar;
	mGlobalSearchBarLayout->setVisible( false )->setEnabled( false );
	auto addClickListener = [&]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::MouseClick, [this, cmd]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
				mGlobalSearchBarLayout->execute( cmd );
		} );
	};
	UIPushButton* searchButton = mGlobalSearchBarLayout->find<UIPushButton>( "global_search" );
	UIPushButton* searchReplaceButton =
		mGlobalSearchBarLayout->find<UIPushButton>( "global_search_replace" );
	UICheckBox* caseSensitiveChk = mGlobalSearchBarLayout->find<UICheckBox>( "case_sensitive" );
	UICheckBox* wholeWordChk = mGlobalSearchBarLayout->find<UICheckBox>( "whole_word" );
	UICheckBox* luaPatternChk = mGlobalSearchBarLayout->find<UICheckBox>( "lua_pattern" );
	UIWidget* searchBarClose = mGlobalSearchBarLayout->find<UIWidget>( "global_searchbar_close" );
	mGlobalSearchInput = mGlobalSearchBarLayout->find<UITextInput>( "global_search_find" );
	mGlobalSearchHistoryList =
		mGlobalSearchBarLayout->find<UIDropDownList>( "global_search_history" );
	mGlobalSearchBarLayout->addCommand(
		"search-in-files", [&, caseSensitiveChk, wholeWordChk, luaPatternChk] {
			doGlobalSearch( mGlobalSearchInput->getText(), caseSensitiveChk->isChecked(),
							wholeWordChk->isChecked(), luaPatternChk->isChecked(), false );
		} );
	mGlobalSearchBarLayout->addCommand(
		"search-replace-in-files", [&, caseSensitiveChk, wholeWordChk, luaPatternChk] {
			doGlobalSearch( mGlobalSearchInput->getText(), caseSensitiveChk->isChecked(),
							wholeWordChk->isChecked(), luaPatternChk->isChecked(), true );
		} );
	mGlobalSearchBarLayout->addCommand(
		"search-again", [&, caseSensitiveChk, wholeWordChk, luaPatternChk] {
			auto listBox = mGlobalSearchHistoryList->getListBox();
			if ( listBox->getItemSelectedIndex() < mGlobalSearchHistory.size() ) {
				doGlobalSearch( mGlobalSearchHistory[mGlobalSearchHistory.size() - 1 -
													 listBox->getItemSelectedIndex()]
									.first,
								caseSensitiveChk->isChecked(), wholeWordChk->isChecked(),
								luaPatternChk->isChecked(),
								mGlobalSearchTreeReplace == mGlobalSearchTree, true );
			}
		} );
	mGlobalSearchBarLayout->addCommand( "close-global-searchbar", [&] {
		hideGlobalSearchBar();
		if ( mEditorSplitter->getCurEditor() )
			mEditorSplitter->getCurEditor()->setFocus();
	} );
	mGlobalSearchBarLayout->getKeyBindings().addKeybindsString( {
		{ "escape", "close-global-searchbar" },
		{ "ctrl+s", "change-case" },
		{ "ctrl+w", "change-whole-word" },
		{ "ctrl+l", "toggle-lua-pattern" },
		{ "ctrl+r", "search-replace-in-files" },
		{ "ctrl+g", "search-again" },
	} );
	mGlobalSearchBarLayout->addCommand( "change-case", [&, caseSensitiveChk] {
		caseSensitiveChk->setChecked( !caseSensitiveChk->isChecked() );
	} );
	mGlobalSearchBarLayout->addCommand( "change-whole-word", [&, wholeWordChk] {
		wholeWordChk->setChecked( !wholeWordChk->isChecked() );
	} );
	mGlobalSearchBarLayout->addCommand( "toggle-lua-pattern", [&, luaPatternChk] {
		luaPatternChk->setChecked( !luaPatternChk->isChecked() );
	} );
	mGlobalSearchInput->addEventListener( Event::OnPressEnter, [&]( const Event* ) {
		if ( mGlobalSearchInput->hasFocus() ) {
			mGlobalSearchBarLayout->execute( "search-in-files" );
		} else {
			KeyEvent keyEvent( mGlobalSearchTree, Event::KeyDown, KEY_RETURN, 0, 0 );
			mGlobalSearchTree->forceKeyDown( keyEvent );
		}
	} );
	mGlobalSearchInput->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		Uint32 keyCode = keyEvent->getKeyCode();
		if ( ( keyCode == KEY_UP || keyCode == KEY_DOWN || keyCode == KEY_PAGEUP ||
			   keyCode == KEY_PAGEDOWN || keyCode == KEY_HOME || keyCode == KEY_END ) &&
			 mGlobalSearchTree->forceKeyDown( *keyEvent ) && !mGlobalSearchTree->hasFocus() ) {
			mGlobalSearchTree->setFocus();
		}
	} );
	mGlobalSearchInput->addEventListener( Event::OnSizeChange, [&]( const Event* ) {
		if ( mGlobalSearchBarLayout->isVisible() )
			updateGlobalSearchBar();
	} );
	addClickListener( searchButton, "search-in-files" );
	addClickListener( searchReplaceButton, "search-replace-in-files" );
	addClickListener( searchBarClose, "close-global-searchbar" );
	mGlobalSearchLayout = mUISceneNode
							  ->loadLayoutFromString( R"xml(
		<vbox id="global_search_layout" layout_width="wrap_content" layout_height="wrap_content" visible="false">
			<hbox class="status_box" layout_width="match_parent" layout_height="wrap_content" visible="false">
				<TextView layout_width="wrap_content" layout_height="match_parent" text="Searched for:" margin-right="4dp" />
				<TextView class="search_str" layout_width="wrap_content" layout_height="match_parent" />
				<PushButton id="global_search_again" layout_width="wrap_content" layout_height="18dp" text="Search Again" margin-left="8dp" />
				<Widget layout_width="0" layout_weight="1" layout_height="match_parent" />
				<TextView class="search_total" layout_width="wrap_content" layout_height="match_parent" margin-right="8dp" />
			</hbox>
			<hbox class="replace_box" layout_width="match_parent" layout_height="wrap_content" visible="false">
				<TextView layout_width="wrap_content" layout_height="wrap_content" text="Replace with:" margin-right="4dp" />
				<TextInput id="global_search_replace_input" class="small_input" layout_width="200dp" layout_height="18dp" padding="0" margin-right="4dp" />
				<PushButton id="global_search_replace_button" layout_width="wrap_content" layout_height="18dp" text="Replace" />
			</hbox>
		</vbox>
	)xml",
													  mUISceneNode->getRoot() )
							  ->asType<UILayout>();
	UIPushButton* searchAgainBtn = mGlobalSearchLayout->find<UIPushButton>( "global_search_again" );
	UITextInput* replaceInput =
		mGlobalSearchLayout->find<UITextInput>( "global_search_replace_input" );
	UIPushButton* replaceButton =
		mGlobalSearchLayout->find<UIPushButton>( "global_search_replace_button" );
	addClickListener( searchAgainBtn, "search-again" );
	addClickListener( replaceButton, "replace-in-files" );
	replaceInput->addEventListener( Event::OnPressEnter, [&, replaceInput]( const Event* ) {
		if ( replaceInput->hasFocus() )
			mGlobalSearchBarLayout->execute( "replace-in-files" );
	} );
	replaceInput->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_ESCAPE )
			mGlobalSearchBarLayout->execute( "close-global-searchbar" );
	} );
	mGlobalSearchBarLayout->addCommand( "replace-in-files", [&, replaceInput] {
		auto listBox = mGlobalSearchHistoryList->getListBox();
		if ( listBox->getItemSelectedIndex() < mGlobalSearchHistory.size() ) {
			const auto& replaceData = mGlobalSearchHistory[mGlobalSearchHistory.size() - 1 -
														   listBox->getItemSelectedIndex()];
			size_t count = replaceInFiles( replaceInput->getText(), replaceData.second );
			mGlobalSearchBarLayout->execute( "search-again" );
			mGlobalSearchBarLayout->execute( "close-global-searchbar" );
			mApp->getNotificationCenter()->addNotification(
				String::format( "Replaced %zu occurrences.", count ) );
		}
	} );
	mGlobalSearchTreeSearch =
		UITreeViewGlobalSearch::New( mEditorSplitter->getCurrentColorScheme(), false );
	mGlobalSearchTreeReplace =
		UITreeViewGlobalSearch::New( mEditorSplitter->getCurrentColorScheme(), true );
	initGlobalSearchTree( mGlobalSearchTreeSearch );
	initGlobalSearchTree( mGlobalSearchTreeReplace );
	mGlobalSearchTree = mGlobalSearchTreeSearch;
}

void GlobalSearchController::showGlobalSearch( bool searchReplace ) {
	mApp->hideLocateBar();
	mApp->hideSearchBar();
	bool wasReplaceTree = mGlobalSearchTreeReplace == mGlobalSearchTree;
	mGlobalSearchTree = searchReplace ? mGlobalSearchTreeReplace : mGlobalSearchTreeSearch;
	mGlobalSearchTreeSearch->setVisible( !searchReplace );
	mGlobalSearchTreeReplace->setVisible( searchReplace );
	mGlobalSearchBarLayout->setVisible( true )->setEnabled( true );
	mGlobalSearchInput->setFocus();
	mGlobalSearchLayout->setVisible( true );
	if ( mEditorSplitter->getCurEditor() &&
		 mEditorSplitter->getCurEditor()->getDocument().hasSelection() ) {
		mGlobalSearchInput->setText(
			mEditorSplitter->getCurEditor()->getDocument().getSelectedText() );
	}
	mGlobalSearchInput->getDocument().selectAll();
	auto* loader = mGlobalSearchTree->getParent()->find( "loader" );
	if ( loader )
		loader->setVisible( true );
	if ( !searchReplace ) {
		mGlobalSearchLayout->findByClass( "replace_box" )->setVisible( searchReplace );
		if ( wasReplaceTree ) {
			updateGlobalSearchBarResults( mGlobalSearchTreeReplace->getSearchStr(),
										  std::static_pointer_cast<ProjectSearch::ResultModel>(
											  mGlobalSearchTreeReplace->getModelShared() ),
										  searchReplace );
		}
	}
	updateGlobalSearchBar();
}

void GlobalSearchController::updateColorScheme( const SyntaxColorScheme& colorScheme ) {
	mGlobalSearchTreeSearch->updateColorScheme( colorScheme );
	mGlobalSearchTreeReplace->updateColorScheme( colorScheme );
}

bool GlobalSearchController::isUsingSearchReplaceTree() {
	return mGlobalSearchTreeReplace == mGlobalSearchTree;
}

void GlobalSearchController::updateGlobalSearchBar() {
	mGlobalSearchBarLayout->runOnMainThread( [&] {
		Float width = eeceil( mGlobalSearchInput->getPixelsSize().getWidth() );
		Float rowHeight = mGlobalSearchTree->getRowHeight() * LOCATEBAR_MAX_VISIBLE_ITEMS;
		mGlobalSearchLayout->setPixelsSize( width, 0 );
		mGlobalSearchTree->setPixelsSize( width, rowHeight );
		width -= mGlobalSearchTree->getVerticalScrollBar()->getPixelsSize().getWidth();
		mGlobalSearchTree->setColumnWidth( 0, eeceil( width ) );
		Vector2f pos( mGlobalSearchInput->convertToWorldSpace( { 0, 0 } ) );
		pos = PixelDensity::pxToDp( pos );
		mGlobalSearchLayout->setLayoutMarginLeft( pos.x );
		mGlobalSearchLayout->setLayoutMarginTop( pos.y -
												 mGlobalSearchLayout->getSize().getHeight() );
	} );
}

void GlobalSearchController::hideGlobalSearchBar() {
	mGlobalSearchBarLayout->setEnabled( false )->setVisible( false );
	mGlobalSearchLayout->setVisible( false );
	auto* loader = mGlobalSearchTree->getParent()->find( "loader" );
	if ( loader )
		loader->setVisible( false );
}

void GlobalSearchController::updateGlobalSearchBarResults(
	const std::string& search, std::shared_ptr<ProjectSearch::ResultModel> model,
	bool searchReplace ) {
	updateGlobalSearchBar();
	mGlobalSearchTree->setSearchStr( search );
	mGlobalSearchTree->setModel( model );
	if ( mGlobalSearchTree->getModel()->rowCount() < 50 )
		mGlobalSearchTree->expandAll();
	mGlobalSearchLayout->findByClass<UITextView>( "search_str" )->setText( search );
	mGlobalSearchLayout->findByClass<UITextView>( "search_total" )
		->setText( String::format( "%zu matches found.", model->resultCount() ) );
	mGlobalSearchLayout->findByClass( "status_box" )->setVisible( true );
	mGlobalSearchLayout->findByClass( "replace_box" )->setVisible( searchReplace );
	if ( searchReplace && mGlobalSearchBarLayout->isVisible() ) {
		auto* replaceInput =
			mGlobalSearchLayout->find<UITextInput>( "global_search_replace_input" );
		replaceInput->setText( search );
		replaceInput->setFocus();
	}
}

void GlobalSearchController::doGlobalSearch( const String& text, bool caseSensitive, bool wholeWord,
											 bool luaPattern, bool searchReplace,
											 bool searchAgain ) {
	if ( mApp->getDirTree() && mApp->getDirTree()->getFilesCount() > 0 && !text.empty() ) {
		mGlobalSearchTree = searchReplace ? mGlobalSearchTreeReplace : mGlobalSearchTreeSearch;
		mGlobalSearchTreeSearch->setVisible( !searchReplace );
		mGlobalSearchTreeReplace->setVisible( searchReplace );
		mGlobalSearchLayout->findByClass( "status_box" )->setVisible( true );
		mGlobalSearchLayout->findByClass( "replace_box" )->setVisible( false );
		mGlobalSearchLayout->findByClass<UITextView>( "search_str" )->setText( text );
		UILoader* loader = UILoader::New();
		loader->setId( "loader" );
		loader->setRadius( 48 );
		loader->setOutlineThickness( 6 );
		loader->setFillColor( Color::Red );
		loader->setParent( mGlobalSearchLayout->getParent() );
		loader->setPosition( mGlobalSearchLayout->getPosition() +
							 mGlobalSearchLayout->getSize() * 0.5f - loader->getSize() * 0.5f );
		Clock* clock = eeNew( Clock, () );
		std::string search( text.toUtf8() );
		ProjectSearch::find(
			mApp->getDirTree()->getFiles(), search,
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
			mApp->getThreadPool(),
#endif
			[&, clock, search, loader, searchReplace,
			 searchAgain]( const ProjectSearch::Result& res ) {
				Log::info( "Global search for \"%s\" took %.2fms", search.c_str(),
						   clock->getElapsedTime().asMilliseconds() );
				eeDelete( clock );
				mUISceneNode->runOnMainThread( [&, loader, res, search, searchReplace,
												searchAgain] {
					auto model = ProjectSearch::asModel( res );
					auto listBox = mGlobalSearchHistoryList->getListBox();

					if ( !searchAgain ) {
						mGlobalSearchHistory.push_back( std::make_pair( search, model ) );
						if ( mGlobalSearchHistory.size() > 10 )
							mGlobalSearchHistory.pop_front();

						std::vector<String> items;
						for ( auto item = mGlobalSearchHistory.rbegin();
							  item != mGlobalSearchHistory.rend(); item++ ) {
							items.push_back( item->first );
						}

						listBox->clear();
						listBox->addListBoxItems( items );
						if ( mGlobalSearchHistoryOnItemSelectedCb )
							mGlobalSearchHistoryList->removeEventListener(
								mGlobalSearchHistoryOnItemSelectedCb );
						listBox->setSelected( 0 );
						mGlobalSearchHistoryOnItemSelectedCb =
							mGlobalSearchHistoryList->addEventListener(
								Event::OnItemSelected, [&, searchReplace]( const Event* ) {
									auto idx = mGlobalSearchHistoryList->getListBox()
												   ->getItemSelectedIndex();
									auto idxItem = mGlobalSearchHistory.at(
										mGlobalSearchHistory.size() - 1 - idx );
									updateGlobalSearchBarResults( idxItem.first, idxItem.second,
																  searchReplace );
								} );
					} else if ( listBox->getItemSelectedIndex() < mGlobalSearchHistory.size() ) {
						mGlobalSearchHistory[mGlobalSearchHistory.size() - 1 -
											 listBox->getItemSelectedIndex()]
							.second = model;
					}

					updateGlobalSearchBarResults( search, model, searchReplace );
					loader->setVisible( false );
					loader->close();
				} );
			},
			caseSensitive, wholeWord,
			luaPattern ? TextDocument::FindReplaceType::LuaPattern
					   : TextDocument::FindReplaceType::Normal );
	}
}

void GlobalSearchController::initGlobalSearchTree( UITreeViewGlobalSearch* searchTree ) {
	searchTree->addClass( "search_tree" );
	searchTree->setParent( mGlobalSearchLayout );
	searchTree->setVisible( false );
	searchTree->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed );
	searchTree->setExpanderIconSize( PixelDensity::dpToPx( 20 ) );
	searchTree->setHeadersVisible( false );
	searchTree->setColumnsHidden(
		{ ProjectSearch::ResultModel::Line, ProjectSearch::ResultModel::ColumnStart }, true );
	searchTree->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_ESCAPE )
			mGlobalSearchBarLayout->execute( "close-global-searchbar" );
	} );
	searchTree->addEventListener( Event::OnModelEvent, [&]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			const Model* model = modelEvent->getModel();
			if ( !model )
				return;
			if ( mGlobalSearchTreeReplace == mGlobalSearchTree ) {
				if ( modelEvent->getTriggerEvent()->getType() == Event::KeyDown ) {
					const KeyEvent* keyEvent =
						static_cast<const KeyEvent*>( modelEvent->getTriggerEvent() );
					if ( keyEvent->getKeyCode() == KEY_SPACE &&
						 keyEvent->getNode()->isType( UI_TYPE_TREEVIEW_CELL ) ) {
						auto* cell =
							static_cast<UITreeViewCellGlobalSearch*>( keyEvent->getNode() );
						cell->toggleSelected();
						return;
					}
				}
			}

			Variant vPath( model->data( model->index( modelEvent->getModelIndex().internalId(),
													  ProjectSearch::ResultModel::FileOrPosition ),
										Model::Role::Custom ) );
			if ( vPath.isValid() && vPath.is( Variant::Type::cstr ) ) {
				std::string path( vPath.asCStr() );
				UITab* tab = mEditorSplitter->isDocumentOpen( path );
				if ( !tab ) {
					FileInfo fileInfo( path );
					if ( fileInfo.exists() && fileInfo.isRegularFile() )
						mApp->loadFileFromPath( path );
				} else {
					tab->getTabWidget()->setTabSelected( tab );
				}
				Variant lineNum(
					model->data( model->index( modelEvent->getModelIndex().row(),
											   ProjectSearch::ResultModel::FileOrPosition,
											   modelEvent->getModelIndex().parent() ),
								 Model::Role::Custom ) );
				Variant colNum( model->data( model->index( modelEvent->getModelIndex().row(),
														   ProjectSearch::ResultModel::ColumnStart,
														   modelEvent->getModelIndex().parent() ),
											 Model::Role::Custom ) );
				if ( mEditorSplitter->getCurEditor() && lineNum.isValid() && colNum.isValid() &&
					 lineNum.is( Variant::Type::Int64 ) && colNum.is( Variant::Type::Int64 ) ) {
					TextPosition pos{ lineNum.asInt64(), colNum.asInt64() };
					mEditorSplitter->getCurEditor()->getDocument().setSelection( pos );
					mEditorSplitter->getCurEditor()->goToLine( pos );
					hideGlobalSearchBar();
				}
			}
		}
	} );
}
