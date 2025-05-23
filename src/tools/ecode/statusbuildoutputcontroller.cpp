#include "statusbuildoutputcontroller.hpp"
#include "notificationcenter.hpp"
#include "plugins/plugincontextprovider.hpp"
#include "projectdirectorytree.hpp"
#include "widgetcommandexecuter.hpp"
#include <eepp/ui/uicodeeditor.hpp>

namespace ecode {

StatusBuildOutputController::StatusBuildOutputController( UISplitter* mainSplitter,
														  UISceneNode* uiSceneNode,
														  PluginContextProvider* pluginContext ) :
	StatusBarElement( mainSplitter, uiSceneNode, pluginContext ) {}

static std::string getProjectOutputParserTypeToString( const ProjectOutputParserTypes& type ) {
	switch ( type ) {
		case ProjectOutputParserTypes::Error:
			return "error";
		case ProjectOutputParserTypes::Warning:
			return "warning";
		case ProjectOutputParserTypes::Notice:
			return "notice";
	}
	return "notice";
}

UIPushButton* StatusBuildOutputController::getBuildButton() {
	if ( mContext->getSidePanel() ) {
		UIWidget* tab = mContext->getSidePanel()->find<UIWidget>( "build_tab_view" );
		if ( tab )
			return tab->find<UIPushButton>( "build_button" );
	}
	return nullptr;
}

UIPushButton* StatusBuildOutputController::getBuildAndRunButton() {
	if ( mContext->getSidePanel() ) {
		UIWidget* tab = mContext->getSidePanel()->find<UIWidget>( "build_tab_view" );
		if ( tab )
			return tab->find<UIPushButton>( "build_and_run_button" );
	}
	return nullptr;
}

UIPushButton* StatusBuildOutputController::getCleanButton() {
	if ( mContext->getSidePanel() ) {
		UIWidget* tab = mContext->getSidePanel()->find<UIWidget>( "build_tab_view" );
		if ( tab )
			return tab->find<UIPushButton>( "clean_button" );
	}
	return nullptr;
}

bool StatusBuildOutputController::searchFindAndAddStatusResult(
	const std::vector<PatternHolder>& patterns, const std::string& text,
	const ProjectBuildCommand* cmd ) {
	PatternMatcher::Range matches[12];
	for ( const auto& pattern : patterns ) {
		if ( pattern.pattern.matches( text, matches ) ) {
			StatusMessage status;
			status.type = pattern.config.type;

			for ( int i = 0; i < (int)pattern.pattern.getNumMatches(); ++i ) {
				if ( !matches[i].isValid() )
					break;

				if ( i == 0 ) {
					status.output = text;
					continue;
				}

				std::string subtxt =
					text.substr( matches[i].start, matches[i].end - matches[i].start );
				if ( pattern.config.patternOrder.message == i ) {
					auto nl = subtxt.find_first_of( '\n' );
					if ( nl == std::string::npos ) {
						status.message = std::move( subtxt );
					} else {
						status.message = subtxt.substr( 0, nl );
					}
				} else if ( pattern.config.patternOrder.file == i ) {
					bool isRelativePath = FileSystem::isRelativePath( subtxt );
					status.file = !subtxt.empty() && isRelativePath
									  ? FileSystem::getRealPath( cmd->workingDir + subtxt )
									  : FileSystem::getRealPath( subtxt );
					if ( isRelativePath ) {
						FileInfo file( status.file );
						if ( !file.exists() || !file.isRegularFile() )
							status.file = subtxt;
					}
					status.fileName = FileSystem::fileNameFromPath( status.file );
				} else if ( pattern.config.patternOrder.line == i ) {
					int l;
					if ( String::fromString( l, subtxt ) )
						status.line = l;
				} else if ( pattern.config.patternOrder.col == i ) {
					int c;
					if ( String::fromString( c, subtxt ) )
						status.col = c;
				}
			}

			mStatusResults.emplace_back( status );
			if ( mTableIssues->getModel() )
				mTableIssues->getModel()->invalidate();
			return true;
		}
	}
	return false;
}

static void safeInsertBuffer( TextDocument& doc, const std::string& buffer ) {
	doc.insert( 0, doc.endOfDoc(), buffer );
}

void StatusBuildOutputController::runBuild( const std::string& buildName,
											const std::string& buildType,
											const ProjectBuildOutputParser& outputParser,
											bool isClean,
											std::function<void( int exitStatus )> doneFn ) {
	if ( nullptr == mContext->getProjectBuildManager() )
		return;

	auto pbm = mContext->getProjectBuildManager();

	show();
	showBuildOutput();

	mStatusResults.clear();
	if ( !isClean && mTableIssues )
		mTableIssues->getSelection().clear();
	mBuildOutput->getDocument().reset();
	mBuildOutput->invalidateLongestLineWidth();
	mBuildOutput->setScrollY( mBuildOutput->getMaxScroll().y );

	std::vector<SyntaxPattern> patterns;

	mPatternHolder.clear();
	mCurLineBuffer.clear();

	auto configs = { outputParser.getPresetConfig(), outputParser.getConfig() };
	for ( const auto& config : configs ) {
		for ( const auto& parser : config ) {
			mPatternHolder.push_back( { LuaPatternStorage( parser.pattern ), parser } );

			SyntaxPattern ptn( { parser.pattern },
							   getProjectOutputParserTypeToString( parser.type ) );

			patterns.emplace_back( std::move( ptn ) );
		}
	}

	patterns.emplace_back( SyntaxPattern(
		{ "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:.*[Ee][Rr][Rr][Oo][Rr].*[^\n]+" }, "error" ) );
	patterns.emplace_back( SyntaxPattern(
		{ "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:.*[Ww][Aa][Rr][Nn][Ii][Nn][Gg].*[^\n]+" },
		"warning" ) );
	patterns.emplace_back(
		SyntaxPattern( { "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:[^\n]+" }, "notice" ) );

	SyntaxDefinition synDef( "custom_build", {}, std::move( patterns ) );

	mBuildOutput->getDocument().setSyntaxDefinition( synDef );
	mBuildOutput->getVScrollBar()->setValue( 1.f );
	mBuildOutput->getDocument().getHighlighter()->setMaxTokenizationLength( 2048 );
	mBuildOutput->setLineWrapMode( LineWrapMode::Word );

	mScrollLocked = true;

	UIPushButton* buildButton = getBuildButton();
	UIPushButton* buildAndRunButton = getBuildAndRunButton();
	UIPushButton* cleanButton = getCleanButton();

	bool enableBuildButton = false;
	bool enableCleanButton = false;

	if ( isClean && buildButton && buildButton->isEnabled() ) {
		buildButton->setEnabled( false );
		enableBuildButton = true;
	}

	if ( !isClean && cleanButton && cleanButton->isEnabled() ) {
		cleanButton->setEnabled( false );
		enableCleanButton = true;
	}

	if ( isClean ) {
		if ( cleanButton )
			cleanButton->setText( mContext->i18n( "cancel_clean", "Cancel Clean" ) );
	} else {
		if ( buildButton )
			buildButton->setText( mContext->i18n( "cancel_build", "Cancel Build" ) );
	}

	buildAndRunButton->setEnabled( false );

	mBuildButton->setEnabled( false );

	mStopButton->setEnabled( true );

	const auto updateBuildButton = [this, isClean, enableBuildButton, enableCleanButton]() {
		UIPushButton* buildButton = getBuildButton();
		UIPushButton* buildAndRunButton = getBuildAndRunButton();
		UIPushButton* cleanButton = getCleanButton();

		if ( !isClean && buildButton ) {
			buildButton->runOnMainThread( [this, buildButton] {
				buildButton->setText( mContext->i18n( "build", "Build" ) );
			} );
		}

		if ( isClean && cleanButton ) {
			cleanButton->runOnMainThread( [this, cleanButton] {
				cleanButton->setText( mContext->i18n( "clean", "Clean" ) );
			} );
		}

		if ( enableBuildButton && buildButton )
			buildButton->setEnabled( true );

		if ( enableCleanButton && cleanButton )
			cleanButton->runOnMainThread( [cleanButton] { cleanButton->setEnabled( true ); } );

		buildAndRunButton->setEnabled( true );
		mBuildButton->setEnabled( true );
		mStopButton->setEnabled( false );
	};

	auto res = pbm->build(
		buildName,
		[this]( const auto& key, const auto& def ) { return mContext->i18n( key, def ); },
		buildType,
		[this]( auto, std::string buffer, const ProjectBuildCommand* cmd ) {
			mBuildOutput->runOnMainThread( [this, buffer]() {
				safeInsertBuffer( mBuildOutput->getDocument(), buffer );
				if ( mScrollLocked )
					mBuildOutput->setScrollY( mBuildOutput->getMaxScroll().y );
			} );

			if ( nullptr == cmd )
				return;

			do {
				auto nl = buffer.find_first_of( '\n' );
				if ( nl != std::string::npos ) {
					mCurLineBuffer += buffer.substr( 0, nl );
					searchFindAndAddStatusResult( mPatternHolder, mCurLineBuffer, cmd );
					buffer = buffer.substr( nl + 1 );
					mCurLineBuffer.clear();
				} else {
					mCurLineBuffer += buffer;
					buffer.clear();
				}
			} while ( !buffer.empty() );
		},
		[this, updateBuildButton, isClean, doneFn]( auto exitCode,
													const ProjectBuildCommand* cmd ) {
			if ( !mCurLineBuffer.empty() && nullptr != cmd )
				searchFindAndAddStatusResult( mPatternHolder, mCurLineBuffer, cmd );
			String buffer;

			if ( EXIT_SUCCESS == exitCode ) {
				buffer =
					Sys::getDateTimeStr() + ": " +
					( isClean ? mContext->i18n( "clean_successful", "Clean run successfully\n" )
							  : mContext->i18n( "build_successful", "Build run successfully\n" ) );
			} else {
				buffer = Sys::getDateTimeStr() + ": " +
						 ( isClean ? mContext->i18n( "clean_failed", "Clean run with errors\n" )
								   : mContext->i18n( "build_failed", "Build run with errors\n" ) );
			}

			mBuildOutput->runOnMainThread( [this, buffer]() {
				safeInsertBuffer( mBuildOutput->getDocument(), buffer );
				if ( mScrollLocked )
					mBuildOutput->setScrollY( mBuildOutput->getMaxScroll().y );
			} );

			updateBuildButton();

			if ( !mContext->getWindow()->hasFocus() ) {
				mContext->getUISceneNode()->runOnMainThread( [this] {
					mContext->getWindow()->flash( WindowFlashOperation::UntilFocused );
				} );
			}

			if ( doneFn )
				doneFn( exitCode );
		},
		isClean );

	if ( !res.isValid() ) {
		mContext->getNotificationCenter()->addNotification( res.errorMsg );
		updateBuildButton();
		if ( doneFn )
			doneFn( EXIT_FAILURE );
	}
}

UIWidget* StatusBuildOutputController::getWidget() {
	return mContainer;
}

UIWidget* StatusBuildOutputController::createWidget() {
	if ( nullptr == mContainer )
		createContainer();
	return mContainer;
}

UICodeEditor* StatusBuildOutputController::getContainer() {
	return mBuildOutput;
}

void StatusBuildOutputController::showIssues() {
	mBuildOutput->setVisible( false );
	mTableIssues->setVisible( true );
	mButOutput->setSelected( false );
	mButIssues->setSelected( true );
	mTableIssues->setFocus();
}

void StatusBuildOutputController::showBuildOutput() {
	mBuildOutput->setVisible( true );
	mTableIssues->setVisible( false );
	mButOutput->setSelected( true );
	mButIssues->setSelected( false );
	mBuildOutput->setFocus();
}

class StatusMessageModel : public Model {
  public:
	static std::shared_ptr<StatusMessageModel> create( std::vector<StatusMessage>& data,
													   UISceneNode* sceneNode ) {
		return std::make_shared<StatusMessageModel>( data, sceneNode );
	}

	explicit StatusMessageModel( std::vector<StatusMessage>& data, UISceneNode* sceneNode ) :
		mData( data ), mSceneNode( sceneNode ) {}

	virtual size_t rowCount( const ModelIndex& ) const { return mData.size(); }

	virtual size_t columnCount( const ModelIndex& ) const { return 3; }

	virtual Variant data( const ModelIndex& index, ModelRole role ) const {
		eeASSERT( index.row() < (Int64)mData.size() );
		static UIIcon* errorIcon = mSceneNode->findIcon( "error" );
		static UIIcon* warnIcon = mSceneNode->findIcon( "warning" );
		if ( role == ModelRole::Display ) {
			switch ( index.column() ) {
				case 2:
					return Variant( mData[index.row()].line );
				case 1:
					return Variant( mData[index.row()].fileName.c_str() );
				case 0:
				default:
					return Variant( mData[index.row()].message );
			}
		} else if ( role == ModelRole::Icon && index.column() == 0 ) {
			return Variant( mData[index.row()].type == ProjectOutputParserTypes::Error ? errorIcon
																					   : warnIcon );
		} else if ( role == ModelRole::Class ) {
			return Variant( mData[index.row()].type == ProjectOutputParserTypes::Error
								? "theme-error"
								: "theme-warning" );
		} else if ( role == ModelRole::Custom ) {
			switch ( index.column() ) {
				case 0:
					return Variant( mData[index.row()].file.c_str() );
				case 1:
					return Variant( mData[index.row()].line );
				case 2:
					return Variant( mData[index.row()].col );
			}
		}
		return {};
	}

	virtual std::string columnName( const size_t& idx ) const {
		switch ( idx ) {
			case 2:
				return mSceneNode->i18n( "line", "Line" );
			case 1:
				return mSceneNode->i18n( "file", "File" );
			case 0:
				return mSceneNode->i18n( "message", "Message" );
		}
		return "";
	}

	virtual bool classModelRoleEnabled() { return true; }

  protected:
	std::vector<StatusMessage>& mData;
	UISceneNode* mSceneNode;
};

void StatusBuildOutputController::onLoadDone( const Variant& lineNum, const Variant& colNum ) {
	if ( mSplitter->curEditorExistsAndFocused() && lineNum.isValid() && colNum.isValid() &&
		 lineNum.is( Variant::Type::Int64 ) && colNum.is( Variant::Type::Int64 ) ) {
		TextPosition pos{ lineNum.asInt64() > 0 ? lineNum.asInt64() - 1 : 0, colNum.asInt64() };
		mSplitter->getCurEditor()->getDocument().setSelection( pos );
		mSplitter->getCurEditor()->goToLine( pos );
		mSplitter->addCurrentPositionToNavigationHistory();
	}
}

void StatusBuildOutputController::setHeaderWidth() {
	auto totWidth =
		eefloor( mTableIssues->getPixelsSize().getWidth() -
				 ( mTableIssues->getVerticalScrollBar()->isVisible()
					   ? mTableIssues->getVerticalScrollBar()->getPixelsSize().getWidth()
					   : 0.f ) );
	Float col1 = eefloor( totWidth * 0.80f );
	Float col2 = eefloor( totWidth * 0.15f );
	Float col3 = totWidth - col1 - col2;
	mTableIssues->setColumnWidth( 0, col1 );
	mTableIssues->setColumnWidth( 1, col2 );
	mTableIssues->setColumnWidth( 2, col3 );
}

static void removeRelativeSubPaths( std::string& path ) {
	while ( String::startsWith( path, "../" ) || String::startsWith( path, "..\\" ) )
		path = path.substr( 3 );
	while ( String::startsWith( path, "./" ) || String::startsWith( path, ".\\" ) )
		path = path.substr( 2 );
}

void StatusBuildOutputController::createContainer() {
	if ( mContainer )
		return;
	const auto XML = R"xml(
	<hbox id="build_output" class="vertical_bar" lw="mp" lh="mp" visible="false">
		<rellayce id="build_output_command_executer" lw="0" lw8="1" lh="mp" class="status_build_output_cont">
			<CodeEditor id="build_output_output" lw="mp" lh="mp" />
			<TableView id="build_output_issues" lw="mp" lh="mp" visible="false" />
			<SelectButton id="but_build_output_issues" text="@string(issues_capitalized, Issues)" lg="bottom|right" margin-right="1dp" margin-bottom="18dp" margin-right="19dp" />
			<SelectButton id="but_build_output_output" text="@string(output_capitalized, Output)" layout-to-left-of="but_build_output_issues" selected="true" />
		</rellayce>
		<vbox lw="16dp" lh="mp">
			<PushButton id="build_output_clear" lw="mp" icon="icon(eraser, 12dp)" tooltip="@string(clear, Clear)" />
			<PushButton id="build_output_build" lw="mp" icon="icon(hammer, 12dp)" tooltip="@string(build, Build)" />
			<PushButton id="build_output_stop" lw="mp" icon="icon(stop, 12dp)" enabled="false" />
			<PushButton id="build_output_find" lw="mp" icon="icon(search, 12dp)" tooltip="@string(find, Find)" />
			<PushButton id="build_output_configure" lw="mp" icon="icon(settings, 12dp)" tooltip="@string(configure_ellipsis, Configure...)" />
		</vbox>
	</hbox>
	)xml";

	if ( mMainSplitter->getLastWidget() != nullptr ) {
		mMainSplitter->getLastWidget()->setVisible( false );
		mMainSplitter->getLastWidget()->setParent( mUISceneNode );
	}

	mContainer = mContext->getUISceneNode()
					 ->loadLayoutFromString( XML, mMainSplitter )
					 ->asType<UILinearLayout>();
	mRelLayCE = mContainer->find( "build_output_command_executer" )
					->asType<UIRelativeLayoutCommandExecuter>();
	auto editor = mContainer->find<UICodeEditor>( "build_output_output" );
	editor->setLocked( true );
	editor->setLineBreakingColumn( 0 );
	editor->setShowLineNumber( false );
	editor->getDocument().reset();
	editor->getDocument().textInput(
		mContext->i18n( "no_build_has_been_run", "No build has been run" ), false );
	editor->setScrollY( editor->getMaxScroll().y );
	mButOutput = mContainer->find<UISelectButton>( "but_build_output_output" );
	mButIssues = mContainer->find<UISelectButton>( "but_build_output_issues" );
	mTableIssues = mContainer->find<UITableView>( "build_output_issues" );
	mTableIssues->setHeadersVisible( true );
	mTableIssues->setModel(
		StatusMessageModel::create( mStatusResults, mContext->getUISceneNode() ) );
	setHeaderWidth();
	mTableIssues->on( Event::OnSizeChange, [this]( auto ) { setHeaderWidth(); } );
	mTableIssues->onModelEvent( [this]( const ModelEvent* modelEvent ) {
		auto model = modelEvent->getModel();
		auto idx = modelEvent->getModelIndex();
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			Variant vPath( model->data( idx, ModelRole::Custom ) );
			if ( vPath.isValid() && vPath.isString() ) {
				std::string path( vPath.toString() );
				UITab* tab = mSplitter->isDocumentOpen( path );
				Variant lineNum( model->data( model->index( modelEvent->getModelIndex().row(), 1 ),
											  ModelRole::Custom ) );
				Variant colNum( model->data( model->index( modelEvent->getModelIndex().row(), 2 ),
											 ModelRole::Custom ) );
				if ( !tab ) {
					FileInfo fileInfo( path );
					if ( fileInfo.exists() && fileInfo.isRegularFile() ) {
						mContext->loadFileFromPath(
							path, true, nullptr,
							[this, lineNum, colNum]( UICodeEditor*, const std::string& ) {
								onLoadDone( lineNum, colNum );
							} );
					} else {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
						removeRelativeSubPaths( path );
						mContext->getDirTree()->asyncMatchTree(
							ProjectDirectoryTree::MatchType::Fuzzy, path, 1,
							[this, colNum, lineNum]( std::shared_ptr<FileListModel> res ) {
								if ( res->rowCount( {} ) == 0 )
									return;
								auto data = res->data( res->index( 0, 1 ) );
								if ( !data.isValid() )
									return;
								std::string path = data.toString();
								mUISceneNode->runOnMainThread( [this, path, lineNum, colNum] {
									UITab* tab = mSplitter->isDocumentOpen( path );
									if ( !tab ) {
										mContext->loadFileFromPath(
											path, true, nullptr,
											[this, lineNum, colNum]( auto, auto ) {
												onLoadDone( lineNum, colNum );
											} );
									} else {
										tab->getTabWidget()->setTabSelected( tab );
										onLoadDone( lineNum, colNum );
									}
								} );
							} );
#endif
					}
				} else {
					tab->getTabWidget()->setTabSelected( tab );
					onLoadDone( lineNum, colNum );
				}
			}
		} else if ( modelEvent->getModelEventType() == ModelEventType::OpenMenu ) {
			UIPopUpMenu* menu = UIPopUpMenu::New();
			menu->add( mContext->i18n( "copy_error_message", "Copy Error Message" ),
					   mContext->findIcon( "copy" ) )
				->setId( "copy-error-message" );
			menu->add( mContext->i18n( "copy_file_path", "Copy File Path" ),
					   mContext->findIcon( "copy" ) )
				->setId( "copy-file-path" );
			menu->on( Event::OnItemClicked, [this, model, idx]( const Event* event ) {
				UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
				std::string id( item->getId() );
				if ( id == "copy-error-message" ) {
					Variant msg( model->data( model->index( idx.row(), 0 ), ModelRole::Display ) );
					mContext->getWindow()->getClipboard()->setText( msg.toString() );
				} else if ( id == "copy-file-path" ) {
					Variant msg( model->data( idx, ModelRole::Custom ) );
					mContext->getWindow()->getClipboard()->setText( msg.toString() );
				}
			} );
			UITableCell* cell = mTableIssues->getCellFromIndex( idx );
			if ( modelEvent->getTriggerEvent()->getType() == Event::MouseClick ||
				 cell == nullptr ) {
				Vector2f pos( mContext->getWindow()->getInput()->getMousePos().asFloat() );
				menu->nodeToWorldTranslation( pos );
				UIMenu::findBestMenuPos( pos, menu );
				menu->setPixelsPosition( pos );
			} else {
				Vector2f pos( 0, cell->getPixelsSize().getHeight() );
				cell->nodeToWorldTranslation( pos );
				UIMenu::findBestMenuPos( pos, menu );
				menu->setPixelsPosition( pos );
			}
			menu->show();
		}
	} );

	mBuildOutput = editor;
	mBuildOutput->on( Event::OnScrollChange, [this]( auto ) {
		mScrollLocked = mBuildOutput->getMaxScroll().y == mBuildOutput->getScroll().y;
	} );
	mContainer->setVisible( false );
	mRelLayCE->setCommand( "build-output-show-build-output", [this]() { showBuildOutput(); } );
	mRelLayCE->setCommand( "build-output-show-build-issues", [this]() { showIssues(); } );
	mRelLayCE->getKeyBindings().addKeybind( { KEY_1, KeyMod::getDefaultModifier() },
											"build-output-show-build-output" );
	mRelLayCE->getKeyBindings().addKeybind( { KEY_2, KeyMod::getDefaultModifier() },
											"build-output-show-build-issues" );
	mButOutput->onClick( [this]( auto ) { showBuildOutput(); } );
	mButIssues->onClick( [this]( auto ) { showIssues(); } );
	mButOutput->setTooltipText(
		mRelLayCE->getKeyBindings().getCommandKeybindString( "build-output-show-build-output" ) );
	mButIssues->setTooltipText(
		mRelLayCE->getKeyBindings().getCommandKeybindString( "build-output-show-build-issues" ) );

	mContainer->bind( "build_output_clear", mClearButton );
	mContainer->bind( "build_output_build", mBuildButton );
	mContainer->bind( "build_output_stop", mStopButton );
	mContainer->bind( "build_output_find", mFindButton );
	mContainer->bind( "build_output_configure", mConfigureButton );

	mClearButton->onClick( [this]( auto ) {
		mBuildOutput->getDocument().reset();
		mBuildOutput->invalidateLongestLineWidth();
		mBuildOutput->setScrollY( mBuildOutput->getMaxScroll().y );
	} );

	mBuildButton->onClick( [this]( auto ) {
		auto pbm = mContext->getProjectBuildManager();
		if ( nullptr == pbm )
			return;
		if ( !pbm->hasBuildConfig() ) {
			UIMessageBox::New( UIMessageBox::OK,
							   mContext->i18n( "must_configure_build_config",
											   "You must first add a build configuration" ) )
				->setCloseShortcut( { KEY_ESCAPE } )
				->setTitle( mContext->getWindowTitle() )
				->showWhenReady()
				->on( Event::OnConfirm, [this]( auto ) {
					auto pbm = mContext->getProjectBuildManager();
					if ( nullptr != pbm )
						pbm->selectTab();
				} );
			return;
		}
		pbm->buildCurrentConfig( this );
	} );

	mStopButton->onClick( [this]( auto ) {
		auto pbm = mContext->getProjectBuildManager();
		if ( nullptr == pbm )
			return;
		pbm->cancelBuild();
	} );

	mFindButton->onClick( [this]( auto ) {
		if ( mBuildOutput->getFindReplace() == nullptr ||
			 !mBuildOutput->getFindReplace()->isVisible() )
			mBuildOutput->showFindReplace();
		else if ( mBuildOutput->getFindReplace() ) {
			mBuildOutput->getFindReplace()->hide();
			mBuildOutput->setFocus();
		}
	} );

	mConfigureButton->onClick( [this]( auto ) {
		auto pbm = mContext->getProjectBuildManager();
		if ( nullptr == pbm )
			return;
		pbm->editCurrentBuild();
		hide();
	} );
}

} // namespace ecode
