#include "globalsearchcontroller.hpp"
#include "ecode.hpp"
#include "uitreeviewglobalsearch.hpp"

namespace ecode {
static int LOCATEBAR_MAX_VISIBLE_ITEMS = 18;

GlobalSearchController::GlobalSearchController( UICodeEditorSplitter* editorSplitter,
												UISceneNode* sceneNode, App* app ) :
	mSplitter( editorSplitter ), mUISceneNode( sceneNode ), mApp( app ) {
	mApp->getPluginManager()->subscribeMessages(
		"GlobalSearchController", [this]( const PluginMessage& msg ) -> PluginRequestHandle {
			return processMessage( msg );
		} );
}

static bool replaceInFile( const std::string& path, const std::string& replaceText,
						   const std::vector<std::pair<Int64, Int64>>& replacements ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return false;

	Int64 diff = 0;

	for ( const auto& range : replacements ) {
		data.replace( range.first + diff, range.second - range.first, replaceText );
		diff += replaceText.size() - ( range.second - range.first );
	}

	if ( !FileSystem::fileWrite( path, (const Uint8*)data.c_str(), data.size() ) )
		return false;

	return true;
}

size_t GlobalSearchController::replaceInFiles( const std::string& replaceText,
											   std::shared_ptr<ProjectSearch::ResultModel> model ) {
	size_t count = 0;
	if ( model->isResultFromSymbolReference() ) {
		// TODO Implement replacement from result from symbol reference
		return count;
	}

	const ProjectSearch::Result& res = model.get()->getResult();

	for ( const auto& fileResult : res ) {
		std::vector<std::pair<Int64, Int64>> replacements;

		for ( const auto& result : fileResult.results )
			if ( result.selected )
				replacements.push_back( { result.start, result.end } );

		if ( replaceInFile( fileResult.file, replaceText, replacements ) )
			count += replacements.size();
	}

	return count;
}

void GlobalSearchController::showGlobalSearch() {
	showGlobalSearch( isUsingSearchReplaceTree() );
}

void GlobalSearchController::initGlobalSearchBar(
	UIGlobalSearchBar* globalSearchBar, const GlobalSearchBarConfig& globalSearchBarConfig,
	std::unordered_map<std::string, std::string> keybindings ) {
	mGlobalSearchBarLayout = globalSearchBar;
	mGlobalSearchBarLayout->setVisible( false )->setEnabled( false );
	auto addClickListener = [this]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::MouseClick, [this, cmd]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
				mGlobalSearchBarLayout->execute( cmd );
		} );
	};
	auto& kbind = mGlobalSearchBarLayout->getKeyBindings();
	kbind.addKeybindsString( { { mApp->getKeybind( "find-replace" ), "find-replace" } } );
	kbind.addKeybindsStringUnordered( keybindings );

	UIPushButton* searchButton = mGlobalSearchBarLayout->find<UIPushButton>( "global_search" );

	UIPushButton* searchReplaceButton =
		mGlobalSearchBarLayout->find<UIPushButton>( "global_search_replace" );
	searchReplaceButton->setTooltipText(
		kbind.getCommandKeybindString( "search-replace-in-files" ) );

	UIPushButton* searchClearHistory =
		mGlobalSearchBarLayout->find<UIPushButton>( "global_search_clear_history" );
	searchClearHistory->setTooltipText(
		kbind.getCommandKeybindString( "global-search-clear-history" ) );

	UICheckBox* caseSensitiveChk = mGlobalSearchBarLayout->find<UICheckBox>( "case_sensitive" );
	caseSensitiveChk->setTooltipText( kbind.getCommandKeybindString( "change-case" ) );

	UICheckBox* wholeWordChk = mGlobalSearchBarLayout->find<UICheckBox>( "whole_word" );
	wholeWordChk->setTooltipText( kbind.getCommandKeybindString( "change-whole-word" ) );

	UICheckBox* luaPatternChk = mGlobalSearchBarLayout->find<UICheckBox>( "lua_pattern" );
	luaPatternChk->setTooltipText( kbind.getCommandKeybindString( "toggle-lua-pattern" ) );

	UICheckBox* escapeSequenceChk = mGlobalSearchBarLayout->find<UICheckBox>( "escape_sequence" );
	std::string kbindEscape = kbind.getCommandKeybindString( "change-escape-sequence" );
	if ( !kbindEscape.empty() )
		escapeSequenceChk->setTooltipText( escapeSequenceChk->getTooltipText() + " (" +
										   kbindEscape + ")" );

	UIWidget* searchBarClose = mGlobalSearchBarLayout->find<UIWidget>( "global_searchbar_close" );

	caseSensitiveChk->setChecked( globalSearchBarConfig.caseSensitive );
	luaPatternChk->setChecked( globalSearchBarConfig.luaPattern );
	wholeWordChk->setChecked( globalSearchBarConfig.wholeWord );
	escapeSequenceChk->setChecked( globalSearchBarConfig.escapeSequence );

	mGlobalSearchInput = mGlobalSearchBarLayout->find<UITextInput>( "global_search_find" );

	mGlobalSearchHistoryList =
		mGlobalSearchBarLayout->find<UIDropDownList>( "global_search_history" );
	mGlobalSearchBarLayout->setCommand( "global-search-clear-history", [this] { clearHistory(); } );
	mGlobalSearchBarLayout->setCommand(
		"search-in-files", [&, caseSensitiveChk, wholeWordChk, luaPatternChk, escapeSequenceChk] {
			doGlobalSearch( mGlobalSearchInput->getText(), caseSensitiveChk->isChecked(),
							wholeWordChk->isChecked(), luaPatternChk->isChecked(),
							escapeSequenceChk->isChecked(), false );
		} );
	mGlobalSearchBarLayout->setCommand(
		"search-again", [&, caseSensitiveChk, wholeWordChk, luaPatternChk, escapeSequenceChk] {
			auto listBox = mGlobalSearchHistoryList->getListBox();
			if ( listBox->getItemSelectedIndex() < mGlobalSearchHistory.size() ) {
				doGlobalSearch( mGlobalSearchHistory[mGlobalSearchHistory.size() - 1 -
													 listBox->getItemSelectedIndex()]
									.first,
								caseSensitiveChk->isChecked(), wholeWordChk->isChecked(),
								luaPatternChk->isChecked(), escapeSequenceChk->isChecked(),
								mGlobalSearchTreeReplace == mGlobalSearchTree, true );
			}
		} );
	mGlobalSearchBarLayout->setCommand( "search-set-string", [this] {
		auto listBox = mGlobalSearchHistoryList->getListBox();
		mGlobalSearchInput->setText(
			mGlobalSearchHistory[mGlobalSearchHistory.size() - 1 - listBox->getItemSelectedIndex()]
				.first );
		;
	} );
	mGlobalSearchBarLayout->setCommand( "close-global-searchbar", [this] {
		hideGlobalSearchBar();
		if ( mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
	} );
	mGlobalSearchBarLayout->setCommand( "expand-all", [this] {
		mGlobalSearchTree->expandAll();
		mGlobalSearchTree->setFocus();
	} );
	mGlobalSearchBarLayout->setCommand( "collapse-all", [this] {
		mGlobalSearchTree->collapseAll();
		mGlobalSearchTree->setFocus();
	} );
	mGlobalSearchBarLayout->setCommand( "change-case", [&, caseSensitiveChk] {
		caseSensitiveChk->setChecked( !caseSensitiveChk->isChecked() );
	} );
	mGlobalSearchBarLayout->setCommand( "change-whole-word", [&, wholeWordChk] {
		wholeWordChk->setChecked( !wholeWordChk->isChecked() );
	} );
	mGlobalSearchBarLayout->setCommand( "toggle-lua-pattern", [&, luaPatternChk] {
		luaPatternChk->setChecked( !luaPatternChk->isChecked() );
	} );
	mGlobalSearchBarLayout->setCommand( "change-escape-sequence", [&, escapeSequenceChk] {
		escapeSequenceChk->setChecked( !escapeSequenceChk->isChecked() );
	} );
	mGlobalSearchBarLayout->setCommand( "find-replace", [this] { mApp->showFindView(); } );
	mGlobalSearchInput->addEventListener( Event::OnPressEnter, [this]( const Event* ) {
		if ( mGlobalSearchInput->hasFocus() ) {
			mGlobalSearchBarLayout->execute( "search-in-files" );
		} else {
			KeyEvent keyEvent( mGlobalSearchTree, Event::KeyDown, KEY_RETURN, SCANCODE_UNKNOWN, 0,
							   0 );
			mGlobalSearchTree->forceKeyDown( keyEvent );
		}
	} );
	auto switchInputToTree = [this]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		Uint32 keyCode = keyEvent->getKeyCode();
		if ( ( keyCode == KEY_UP || keyCode == KEY_DOWN || keyCode == KEY_PAGEUP ||
			   keyCode == KEY_PAGEDOWN || keyCode == KEY_HOME || keyCode == KEY_END ) &&
			 mGlobalSearchTree->forceKeyDown( *keyEvent ) && !mGlobalSearchTree->hasFocus() ) {
			mGlobalSearchTree->setFocus();
		}
	};
	mGlobalSearchInput->addEventListener( Event::KeyDown, switchInputToTree );
	mGlobalSearchInput->addEventListener( Event::OnSizeChange, [this]( const Event* ) {
		if ( mGlobalSearchBarLayout->isVisible() )
			updateGlobalSearchBar();
	} );
	addClickListener( searchButton, "search-in-files" );
	addClickListener( searchReplaceButton, "search-replace-in-files" );
	addClickListener( searchBarClose, "close-global-searchbar" );
	addClickListener( searchClearHistory, "global-search-clear-history" );
	mGlobalSearchLayout = mUISceneNode
							  ->loadLayoutFromString( R"xml(
		<vbox id="global_search_layout" layout_width="fixed" layout_height="wrap_content" visible="false">
			<hbox class="status_box" layout_width="match_parent" layout_height="wrap_content" visible="false">
				<TextView layout_width="wrap_content" layout_height="match_parent" text="Searched for:" margin-right="4dp" />
				<TextView class="search_str" layout_width="wrap_content" layout_height="match_parent" />
				<PushButton id="global_search_again" layout_width="wrap_content" layout_height="18dp" text="Search Again" margin-left="8dp" />
				<PushButton id="global_search_set_search" layout_width="wrap_content" layout_height="18dp" text="Set Search String" margin-left="8dp" />
				<PushButton id="global_search_collapse" layout_width="wrap_content" layout_height="18dp" tooltip='@string(collapse_all, "Collapse All")' margin-left="8dp" icon="menu-fold" />
				<PushButton id="global_search_expand" layout_width="wrap_content" layout_height="18dp" tooltip='@string(expand_all, "Expand All")' margin-left="8dp" icon="menu-unfold" />
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
	searchAgainBtn->setTooltipText( kbind.getCommandKeybindString( "search-again" ) );

	UIPushButton* searchSetSearch =
		mGlobalSearchLayout->find<UIPushButton>( "global_search_set_search" );
	UITextInput* replaceInput =
		mGlobalSearchLayout->find<UITextInput>( "global_search_replace_input" );
	UIPushButton* replaceButton =
		mGlobalSearchLayout->find<UIPushButton>( "global_search_replace_button" );

	UIPushButton* searchExpandButton =
		mGlobalSearchLayout->find<UIPushButton>( "global_search_expand" );
	searchExpandButton->setTooltipText( kbind.getCommandKeybindString( "expand-all" ) );

	UIPushButton* searchCollapseButton =
		mGlobalSearchLayout->find<UIPushButton>( "global_search_collapse" );
	searchCollapseButton->setTooltipText( kbind.getCommandKeybindString( "collapse-all" ) );

	addClickListener( searchAgainBtn, "search-again" );
	addClickListener( searchSetSearch, "search-set-string" );
	addClickListener( replaceButton, "replace-in-files" );
	addClickListener( searchExpandButton, "expand-all" );
	addClickListener( searchCollapseButton, "collapse-all" );
	replaceInput->addEventListener( Event::OnPressEnter, [&, replaceInput]( const Event* ) {
		if ( replaceInput->hasFocus() )
			mGlobalSearchBarLayout->execute( "replace-in-files" );
	} );
	replaceInput->addEventListener( Event::KeyDown, switchInputToTree );
	mGlobalSearchLayout->addEventListener( Event::KeyDown, [this]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_ESCAPE ) {
			mGlobalSearchBarLayout->execute( "close-global-searchbar" );
			return;
		}
		mGlobalSearchBarLayout->forceKeyDown( *keyEvent );
	} );
	mGlobalSearchBarLayout->setCommand(
		"search-replace-in-files",
		[&, caseSensitiveChk, wholeWordChk, luaPatternChk, escapeSequenceChk, replaceInput] {
			if ( mGlobalSearchTreeReplace == mGlobalSearchTree ) {
				replaceInput->setFocus();
				replaceInput->getDocument().selectAll();
				return;
			}

			// TODO Implement replacement from result from symbol reference
			/*if ( mGlobalSearchHistory.back().second->isResultFromSymbolReference() ) {
				mGlobalSearchTreeReplace->setModel( mGlobalSearchHistory.back().second );
				showGlobalSearch( true );
				updateGlobalSearchBarResults( mGlobalSearchHistory.back().first,
											  mGlobalSearchHistory.back().second, true, false );
			} else*/
			{
				doGlobalSearch( mGlobalSearchInput->getText(), caseSensitiveChk->isChecked(),
								wholeWordChk->isChecked(), luaPatternChk->isChecked(),
								escapeSequenceChk->isChecked(), true );
			}
		} );
	mGlobalSearchBarLayout->setCommand( "replace-in-files", [&, replaceInput, escapeSequenceChk] {
		auto listBox = mGlobalSearchHistoryList->getListBox();
		if ( listBox->getItemSelectedIndex() < mGlobalSearchHistory.size() ) {
			const auto& replaceData = mGlobalSearchHistory[mGlobalSearchHistory.size() - 1 -
														   listBox->getItemSelectedIndex()];
			String text( replaceInput->getText() );
			if ( escapeSequenceChk->isChecked() )
				text.unescape();
			size_t count = replaceInFiles( text.toUtf8(), replaceData.second );
			mGlobalSearchBarLayout->execute( "search-again" );
			mGlobalSearchBarLayout->execute( "close-global-searchbar" );
			mApp->getNotificationCenter()->addNotification(
				String::format( "Replaced %zu occurrences.", count ) );
		}
	} );
	mGlobalSearchTreeSearch =
		UITreeViewGlobalSearch::New( mSplitter->getCurrentColorScheme(), false );
	mGlobalSearchTreeReplace =
		UITreeViewGlobalSearch::New( mSplitter->getCurrentColorScheme(), true );
	initGlobalSearchTree( mGlobalSearchTreeSearch );
	initGlobalSearchTree( mGlobalSearchTreeReplace );
	mGlobalSearchTreeReplace->addEventListener( Event::KeyDown, [this]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getSanitizedMod() == KeyMod::getDefaultModifier() &&
			 keyEvent->getKeyCode() == KEY_RETURN )
			mGlobalSearchBarLayout->execute( "replace-in-files" );
	} );
	mGlobalSearchTree = mGlobalSearchTreeSearch;
}

void GlobalSearchController::showGlobalSearch( bool searchReplace ) {
	mApp->hideLocateBar();
	mApp->hideSearchBar();
	mApp->hideStatusTerminal();
	mApp->hideStatusBuildOutput();
	bool wasReplaceTree = mGlobalSearchTreeReplace == mGlobalSearchTree;
	mGlobalSearchTree = searchReplace ? mGlobalSearchTreeReplace : mGlobalSearchTreeSearch;
	mGlobalSearchTreeSearch->setVisible( !searchReplace );
	mGlobalSearchTreeReplace->setVisible( searchReplace );
	mGlobalSearchBarLayout->setVisible( true )->setEnabled( true );
	mGlobalSearchInput->setFocus();
	mGlobalSearchLayout->setVisible( true );
	UICheckBox* escapeSequenceChk = mGlobalSearchBarLayout->find<UICheckBox>( "escape_sequence" );
	if ( mSplitter->curEditorExistsAndFocused() &&
		 mSplitter->getCurEditor()->getDocument().hasSelection() ) {
		auto& doc = mSplitter->getCurEditor()->getDocument();
		String text = doc.getSelectedText();
		if ( !doc.getSelection().inSameLine() ) {
			text.escape();
			if ( !escapeSequenceChk->isChecked() )
				escapeSequenceChk->setChecked( true );
		}
		mGlobalSearchInput->setText( text );
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
										  searchReplace, escapeSequenceChk->isChecked() );
		}
	}
	updateGlobalSearchBar();
	mApp->getStatusBar()->updateState();
}

void GlobalSearchController::updateColorScheme( const SyntaxColorScheme& colorScheme ) {
	mGlobalSearchTreeSearch->updateColorScheme( colorScheme );
	mGlobalSearchTreeReplace->updateColorScheme( colorScheme );
}

bool GlobalSearchController::isUsingSearchReplaceTree() {
	return mGlobalSearchTreeReplace == mGlobalSearchTree;
}

void GlobalSearchController::clearHistory() {
	auto listBox = mGlobalSearchHistoryList->getListBox();
	listBox->clear();
	mGlobalSearchHistory.clear();
	mGlobalSearchHistoryOnItemSelectedCb = 0;
	mGlobalSearchTree->setSearchStr( "" );
	mGlobalSearchTree->setModel( nullptr );
	mGlobalSearchInput->setText( "" );
	mGlobalSearchLayout->findByClass( "status_box" )->setVisible( false );
	mGlobalSearchLayout->findByClass<UITextView>( "search_str" )->setText( "" );
	updateGlobalSearchBar();
}

GlobalSearchBarConfig GlobalSearchController::getGlobalSearchBarConfig() const {
	UICheckBox* caseSensitiveChk = mGlobalSearchBarLayout->find<UICheckBox>( "case_sensitive" );
	UICheckBox* wholeWordChk = mGlobalSearchBarLayout->find<UICheckBox>( "whole_word" );
	UICheckBox* luaPatternChk = mGlobalSearchBarLayout->find<UICheckBox>( "lua_pattern" );
	UICheckBox* escapeSequenceChk = mGlobalSearchBarLayout->find<UICheckBox>( "escape_sequence" );
	GlobalSearchBarConfig globalSeachBarConfig;
	globalSeachBarConfig.caseSensitive = caseSensitiveChk->isChecked();
	globalSeachBarConfig.luaPattern = luaPatternChk->isChecked();
	globalSeachBarConfig.wholeWord = wholeWordChk->isChecked();
	globalSeachBarConfig.escapeSequence = escapeSequenceChk->isChecked();
	return globalSeachBarConfig;
}

void GlobalSearchController::updateGlobalSearchBar() {
	mGlobalSearchBarLayout->runOnMainThread( [this] {
		Float width = eeceil( mGlobalSearchInput->getPixelsSize().getWidth() );
		Float rowHeight = mGlobalSearchTree->getRowHeight() * LOCATEBAR_MAX_VISIBLE_ITEMS;
		mGlobalSearchLayout->setPixelsSize( width, 0 );
		mGlobalSearchTree->setPixelsSize( width, rowHeight );
		width -= mGlobalSearchTree->shouldVerticalScrollBeVisible()
					 ? mGlobalSearchTree->getVerticalScrollBar()->getPixelsSize().getWidth()
					 : 0;
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
	mApp->getStatusBar()->updateState();
}

void GlobalSearchController::toggleGlobalSearchBar() {
	if ( mGlobalSearchBarLayout->isVisible() ) {
		mGlobalSearchBarLayout->execute( "close-global-searchbar" );
	} else {
		showGlobalSearch();
	}
}

void GlobalSearchController::updateGlobalSearchBarResults(
	const std::string& search, std::shared_ptr<ProjectSearch::ResultModel> model,
	bool searchReplace, bool isEscaped ) {
	updateGlobalSearchBar();
	mGlobalSearchTree->hAsCPP = mApp->getProjectDocConfig().hAsCPP;
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
		replaceInput->setText( isEscaped ? String( search ).escape().toUtf8() : search );
		replaceInput->getDocument().selectAll();
		replaceInput->setFocus();
	}
}

void GlobalSearchController::updateGlobalSearchHistory(
	std::shared_ptr<ProjectSearch::ResultModel> model, const std::string& search,
	bool searchReplace, bool searchAgain, bool escapeSequence ) {
	auto listBox = mGlobalSearchHistoryList->getListBox();

	if ( !searchAgain ) {
		mGlobalSearchHistory.push_back( std::make_pair( search, model ) );
		if ( mGlobalSearchHistory.size() > 10 )
			mGlobalSearchHistory.pop_front();

		std::vector<String> items;
		for ( auto item = mGlobalSearchHistory.rbegin(); item != mGlobalSearchHistory.rend();
			  item++ ) {
			items.push_back( item->first );
		}

		listBox->clear();
		listBox->addListBoxItems( items );
		if ( mGlobalSearchHistoryOnItemSelectedCb )
			mGlobalSearchHistoryList->removeEventListener( mGlobalSearchHistoryOnItemSelectedCb );
		listBox->setSelected( 0 );
		mGlobalSearchHistoryOnItemSelectedCb = mGlobalSearchHistoryList->addEventListener(
			Event::OnItemSelected, [&, searchReplace]( const Event* ) {
				auto idx = mGlobalSearchHistoryList->getListBox()->getItemSelectedIndex();
				auto idxItem = mGlobalSearchHistory.at( mGlobalSearchHistory.size() - 1 - idx );
				updateGlobalSearchBarResults( idxItem.first, idxItem.second, searchReplace,
											  escapeSequence );
			} );
	} else if ( listBox->getItemSelectedIndex() < mGlobalSearchHistory.size() ) {
		mGlobalSearchHistory[mGlobalSearchHistory.size() - 1 - listBox->getItemSelectedIndex()]
			.second = model;
	}
}

void GlobalSearchController::doGlobalSearch( String text, bool caseSensitive, bool wholeWord,
											 bool luaPattern, bool escapeSequence,
											 bool searchReplace, bool searchAgain ) {
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
		loader->setParent( mGlobalSearchLayout->getParent() );
		loader->setPosition( mGlobalSearchLayout->getPosition() +
							 mGlobalSearchLayout->getSize() * 0.5f - loader->getSize() * 0.5f );
		Clock* clock = eeNew( Clock, () );
		if ( escapeSequence )
			text.unescape();
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
				mUISceneNode->runOnMainThread( [&, loader, res, search, searchReplace, searchAgain,
												escapeSequence] {
					auto model = ProjectSearch::asModel( res );
					updateGlobalSearchHistory( model, search, searchReplace, searchAgain,
											   escapeSequence );
					updateGlobalSearchBarResults( search, model, searchReplace, escapeSequence );
					loader->setVisible( false );
					loader->close();
				} );
			},
			caseSensitive, wholeWord,
			luaPattern ? TextDocument::FindReplaceType::LuaPattern
					   : TextDocument::FindReplaceType::Normal );
	}
}

void GlobalSearchController::onLoadDone( const Variant& lineNum, const Variant& colNum ) {
	if ( mSplitter->curEditorExistsAndFocused() && lineNum.isValid() && colNum.isValid() &&
		 lineNum.is( Variant::Type::Int64 ) && colNum.is( Variant::Type::Int64 ) ) {
		TextPosition pos{ lineNum.asInt64(), colNum.asInt64() };
		mSplitter->getCurEditor()->getDocument().setSelection( pos );
		mSplitter->getCurEditor()->goToLine( pos );
		mSplitter->addCurrentPositionToNavigationHistory();
		hideGlobalSearchBar();
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
	searchTree->addEventListener( Event::OnModelEvent, [this]( const Event* event ) {
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
										ModelRole::Custom ) );
			if ( vPath.isValid() && vPath.isString() ) {
				std::string path( vPath.toString() );
				UITab* tab = mSplitter->isDocumentOpen( path );
				Variant lineNum(
					model->data( model->index( modelEvent->getModelIndex().row(),
											   ProjectSearch::ResultModel::FileOrPosition,
											   modelEvent->getModelIndex().parent() ),
								 ModelRole::Custom ) );
				Variant colNum( model->data( model->index( modelEvent->getModelIndex().row(),
														   ProjectSearch::ResultModel::ColumnStart,
														   modelEvent->getModelIndex().parent() ),
											 ModelRole::Custom ) );
				if ( !tab ) {
					FileInfo fileInfo( path );
					if ( fileInfo.exists() && fileInfo.isRegularFile() )
						mApp->loadFileFromPath(
							path, true, nullptr,
							[&, lineNum, colNum]( UICodeEditor*, const std::string& ) {
								onLoadDone( lineNum, colNum );
							} );
				} else {
					tab->getTabWidget()->setTabSelected( tab );
					onLoadDone( lineNum, colNum );
				}
			}
		}
	} );
}

PluginRequestHandle GlobalSearchController::processMessage( const PluginMessage& msg ) {
	if ( msg.type != PluginMessageType::SymbolReference || !msg.isBroadcast() )
		return {};

	const ProjectSearch::Result& res = msg.asProjectSearchResult();
	if ( res.empty() )
		return {};
	auto model = ProjectSearch::asModel( res );
	model->removeLastNewLineCharacter();
	model->setResultFromSymbolReference( true );
	ProjectSearch::ResultData::Result sample;

	for ( const auto& r : res ) {
		if ( !r.results.empty() ) {
			sample = r.results.front();
			break;
		}
	}

	if ( sample.line.empty() )
		return {};

	if ( sample.position.end().column() - sample.position.start().column() <= 0 )
		return {};

	auto search =
		sample.line.substr( sample.position.start().column(),
							sample.position.end().column() - sample.position.start().column() );

	mUISceneNode->runOnMainThread( [this, search, model] {
		showGlobalSearch( false );
		mGlobalSearchInput->setText( search );
		updateGlobalSearchHistory( model, search, false, false, false );
		updateGlobalSearchBarResults( search, model, false, false );
	} );

	return {};
}

} // namespace ecode
