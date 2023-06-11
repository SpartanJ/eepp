#include "statusbuildoutputcontroller.hpp"
#include "ecode.hpp"
#include "widgetcommandexecuter.hpp"

namespace ecode {

StatusBuildOutputController::StatusBuildOutputController( UISplitter* mainSplitter,
														  UISceneNode* uiSceneNode, App* app ) :
	mMainSplitter( mainSplitter ),
	mUISceneNode( uiSceneNode ),
	mApp( app ),
	mSplitter( mApp->getSplitter() ) {}

void StatusBuildOutputController::toggle() {
	if ( nullptr == mContainer ) {
		show();
		return;
	}

	if ( mMainSplitter->getLastWidget() != nullptr ) {
		if ( mMainSplitter->getLastWidget() == mContainer ) {
			hide();
		} else {
			show();
		}
	} else {
		show();
	}
}

void StatusBuildOutputController::hide() {
	if ( mContainer && mContainer->isVisible() ) {
		mContainer->setParent( mUISceneNode );
		mContainer->setVisible( false );
		mApp->getStatusBar()->updateState();
		if ( mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
	}
}

void StatusBuildOutputController::show() {
	if ( nullptr == mContainer ) {
		mMainSplitter->updateLayout();
		createContainer();
	}

	if ( !mContainer->isVisible() ) {
		mApp->hideLocateBar();
		mApp->hideSearchBar();
		mApp->hideGlobalSearchBar();
		if ( mMainSplitter->getLastWidget() != nullptr ) {
			mMainSplitter->getLastWidget()->setVisible( false );
			mMainSplitter->getLastWidget()->setParent( mUISceneNode );
		}
		mContainer->setParent( mMainSplitter );
		mContainer->setVisible( true );
		mContainer->getFirstChild()->setFocus();
		mApp->getStatusBar()->updateState();
	}
}

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

UIPushButton* StatusBuildOutputController::getBuildButton( App* app ) {
	if ( app->getSidePanel() ) {
		UIWidget* tab = app->getSidePanel()->find<UIWidget>( "build_tab" );
		if ( tab )
			return tab->find<UIPushButton>( "build_button" );
	}
	return nullptr;
}

UIPushButton* StatusBuildOutputController::getCleanButton( App* app ) {
	if ( app->getSidePanel() ) {
		UIWidget* tab = app->getSidePanel()->find<UIWidget>( "build_tab" );
		if ( tab )
			return tab->find<UIPushButton>( "clean_button" );
	}
	return nullptr;
}

bool StatusBuildOutputController::searchFindAndAddStatusResult(
	const std::vector<PatternHolder>& patterns, const std::string& text,
	const ProjectBuildCommand* cmd ) {
	LuaPattern::Range matches[12];
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

				std::string subtxt = text.substr( matches[i].start, matches[i].end );
				if ( pattern.config.patternOrder.message == i ) {
					auto nl = subtxt.find_first_of( '\n' );
					if ( nl == std::string::npos ) {
						status.message = std::move( subtxt );
					} else {
						status.message = subtxt.substr( 0, nl );
					}
				} else if ( pattern.config.patternOrder.file == i ) {
					status.file = FileSystem::getRealPath( cmd->workingDir + subtxt );
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
			return true;
		}
	}
	return false;
}

void StatusBuildOutputController::runBuild( const std::string& buildName,
											const std::string& buildType,
											const ProjectBuildOutputParser& outputParser ) {
	if ( !mApp->getProjectBuildManager() )
		return;

	auto pbm = mApp->getProjectBuildManager();

	show();
	showBuildOutput();

	mStatusResults.clear();
	if ( mTableIssues )
		mTableIssues->getSelection().clear();
	mBuildOutput->getDocument().reset();
	mBuildOutput->setScrollY( mBuildOutput->getMaxScroll().y );

	std::vector<SyntaxPattern> patterns;

	mPatternHolder.clear();
	mCurLineBuffer.clear();

	auto configs = { outputParser.getPresetConfig(), outputParser.getConfig() };
	for ( const auto& config : configs ) {
		for ( const auto& parser : config ) {
			mPatternHolder.push_back( { LuaPattern( parser.pattern ), parser } );

			SyntaxPattern ptn( { parser.pattern },
							   getProjectOutputParserTypeToString( parser.type ) );

			patterns.emplace_back( std::move( ptn ) );
		}
	}

	patterns.emplace_back(
		SyntaxPattern( { "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:.*error.*[^\n]+" }, "error" ) );
	patterns.emplace_back( SyntaxPattern(
		{ "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:.*warning.*[^\n]+" }, "warning" ) );
	patterns.emplace_back(
		SyntaxPattern( { "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:[^\n]+" }, "notice" ) );

	SyntaxDefinition synDef( "custom_build", {}, patterns );

	mBuildOutput->getDocument().setSyntaxDefinition( synDef );
	mBuildOutput->getVScrollBar()->setValue( 1.f );
	mBuildOutput->getDocument().getHighlighter()->setMaxTokenizationLength( 2048 );

	UIPushButton* buildButton = getBuildButton( mApp );
	if ( buildButton )
		buildButton->setText( mApp->i18n( "cancel_build", "Cancel Build" ) );
	UIPushButton* cleanButton = getCleanButton( mApp );
	bool enableCleanButton = false;
	if ( cleanButton && cleanButton->isEnabled() ) {
		cleanButton->setEnabled( false );
		enableCleanButton = true;
	}

	auto res = pbm->build(
		buildName, [this]( const auto& key, const auto& def ) { return mApp->i18n( key, def ); },
		buildType,
		[this]( auto, std::string buffer, const ProjectBuildCommand* cmd ) {
			mBuildOutput->runOnMainThread( [this, buffer]() {
				bool scrollToBottom = mBuildOutput->getVScrollBar()->getValue() == 1.f;
				mBuildOutput->getDocument().textInput( buffer );
				if ( scrollToBottom )
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
		[this, enableCleanButton]( auto exitCode, const ProjectBuildCommand* cmd ) {
			if ( !mCurLineBuffer.empty() && nullptr != cmd )
				searchFindAndAddStatusResult( mPatternHolder, mCurLineBuffer, cmd );
			String buffer;

			if ( EXIT_SUCCESS == exitCode ) {
				buffer = Sys::getDateTimeStr() + ": " +
						 mApp->i18n( "build_successful", "Build run successfully\n" );
			} else {
				buffer = Sys::getDateTimeStr() + ": " +
						 mApp->i18n( "build_failed", "Build run with errors\n" );
			}

			mBuildOutput->runOnMainThread( [this, buffer]() {
				bool scrollToBottom = mBuildOutput->getVScrollBar()->getValue() == 1.f;
				mBuildOutput->getDocument().textInput( buffer );
				if ( scrollToBottom )
					mBuildOutput->setScrollY( mBuildOutput->getMaxScroll().y );
			} );

			UIPushButton* buildButton = getBuildButton( mApp );
			if ( buildButton )
				buildButton->setText( mApp->i18n( "build", "Build" ) );

			if ( enableCleanButton ) {
				UIPushButton* cleanButton = getCleanButton( mApp );
				if ( cleanButton )
					cleanButton->setEnabled( true );
			}
		} );

	if ( !res.isValid() ) {
		mApp->getNotificationCenter()->addNotification( res.errorMsg );
	}
}

void StatusBuildOutputController::runClean( const std::string& buildName,
											const std::string& buildType,
											const ProjectBuildOutputParser& outputParser ) {
	if ( !mApp->getProjectBuildManager() )
		return;

	auto pbm = mApp->getProjectBuildManager();

	show();

	mBuildOutput->getDocument().reset();
	mBuildOutput->setScrollY( mBuildOutput->getMaxScroll().y );

	std::vector<SyntaxPattern> patterns;

	for ( const auto& parser : outputParser.getConfig() ) {
		SyntaxPattern ptn( { parser.pattern }, getProjectOutputParserTypeToString( parser.type ) );
		patterns.emplace_back( std::move( ptn ) );
	}

	patterns.emplace_back(
		SyntaxPattern( { "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:.*error.*[^\n]+" }, "error" ) );
	patterns.emplace_back( SyntaxPattern(
		{ "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:.*warning.*[^\n]+" }, "warning" ) );
	patterns.emplace_back(
		SyntaxPattern( { "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:[^\n]+" }, "notice" ) );

	SyntaxDefinition synDef( "custom_build", {}, patterns );

	mBuildOutput->getDocument().setSyntaxDefinition( synDef );
	mBuildOutput->getVScrollBar()->setValue( 1.f );

	UIPushButton* buildButton = getBuildButton( mApp );
	bool enableBuildButton = false;
	if ( buildButton && buildButton->isEnabled() ) {
		buildButton->setEnabled( false );
		enableBuildButton = true;
	}
	UIPushButton* cleanButton = getCleanButton( mApp );
	if ( cleanButton )
		cleanButton->setText( mApp->i18n( "cancel_clean", "Cancel Clean" ) );

	auto res = pbm->clean(
		buildName, [this]( const auto& key, const auto& def ) { return mApp->i18n( key, def ); },
		buildType,
		[this]( auto, auto buffer, auto ) {
			mBuildOutput->runOnMainThread( [this, buffer]() {
				bool scrollToBottom = mBuildOutput->getVScrollBar()->getValue() == 1.f;
				mBuildOutput->getDocument().textInput( buffer );
				if ( scrollToBottom )
					mBuildOutput->setScrollY( mBuildOutput->getMaxScroll().y );
			} );
		},
		[this, enableBuildButton]( auto exitCode, auto ) {
			String buffer;

			if ( EXIT_SUCCESS == exitCode ) {
				buffer = Sys::getDateTimeStr() + ": " +
						 mApp->i18n( "clean_successful", "Clean run successfully\n" );
			} else {
				buffer = Sys::getDateTimeStr() + ": " +
						 mApp->i18n( "clean_failed", "Clean run with errors\n" );
			}

			mBuildOutput->runOnMainThread( [this, buffer]() {
				bool scrollToBottom = mBuildOutput->getVScrollBar()->getValue() == 1.f;
				mBuildOutput->getDocument().textInput( buffer );
				if ( scrollToBottom )
					mBuildOutput->setScrollY( mBuildOutput->getMaxScroll().y );
			} );

			UIPushButton* cleanButton = getCleanButton( mApp );
			if ( cleanButton )
				cleanButton->setText( mApp->i18n( "clean", "Clean" ) );

			if ( enableBuildButton ) {
				UIPushButton* buildButton = getBuildButton( mApp );
				if ( buildButton )
					buildButton->setEnabled( true );
			}
		} );

	if ( !res.isValid() ) {
		mApp->getNotificationCenter()->addNotification( res.errorMsg );
	}
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

	virtual void update() { onModelUpdate(); }

	virtual std::string columnName( const size_t& idx ) const {
		switch ( idx ) {
			case 2:
				return mSceneNode->i18n( "message", "Message" );
			case 1:
				return mSceneNode->i18n( "file", "File" );
			case 0:
				return mSceneNode->i18n( "line", "Line" );
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
	}
}

void StatusBuildOutputController::setHeaderWidth() {
	auto totWidth = mTableIssues->getPixelsSize().getWidth() -
					( mTableIssues->getVerticalScrollBar()->isVisible()
						  ? mTableIssues->getVerticalScrollBar()->getPixelsSize().getWidth()
						  : 0.f );
	mTableIssues->setColumnWidth( 0, totWidth * 0.80f );
	mTableIssues->setColumnWidth( 1, totWidth * 0.15f );
	mTableIssues->setColumnWidth( 2, totWidth * 0.05f );
}

void StatusBuildOutputController::createContainer() {
	if ( mContainer )
		return;
	const auto XML = R"xml(
<RelativeLayout id="build_output" lw="mp" lh="mp" visible="false">
	<rellayce class="status_build_output_cont" lw="mp" lh="mp">
		<CodeEditor id="build_output_output" lw="mp" lh="mp" />
		<TableView id="build_output_issues" lw="mp" lh="mp" visible="false" />
		<SelectButton id="but_build_output_issues" text="@string(issues, Issues)" lg="bottom|right" margin-right="1dp" margin-bottom="18dp" margin-right="18dp" />
		<SelectButton id="but_build_output_output" text="@string(output, Output)" layout-to-left-of="but_build_output_issues" selected="true" />
	</rellayce>
</RelativeLayout>
	)xml";
	mContainer = mApp->getUISceneNode()
					 ->loadLayoutFromString( XML, mMainSplitter )
					 ->asType<UIRelativeLayoutCommandExecuter>();
	auto editor = mContainer->find<UICodeEditor>( "build_output_output" );
	editor->setLocked( true );
	editor->setLineBreakingColumn( 0 );
	editor->setShowLineNumber( false );
	editor->getDocument().reset();
	editor->getDocument().textInput(
		mApp->i18n( "no_build_has_been_run", "No build has been run" ) );
	editor->setScrollY( editor->getMaxScroll().y );
	mButOutput = mContainer->find<UISelectButton>( "but_build_output_output" );
	mButIssues = mContainer->find<UISelectButton>( "but_build_output_issues" );
	mTableIssues = mContainer->find<UITableView>( "build_output_issues" );
	mTableIssues->setHeadersVisible( true );
	mTableIssues->setModel( StatusMessageModel::create( mStatusResults, mApp->getUISceneNode() ) );
	setHeaderWidth();
	mTableIssues->on( Event::OnSizeChange, [this]( auto ) { setHeaderWidth(); } );
	mTableIssues->on( Event::OnModelEvent, [this]( const Event* event ) {
		auto modelEvent = static_cast<const ModelEvent*>( event );
		auto idx = modelEvent->getModelIndex();
		if ( modelEvent->getModel() && modelEvent->getModelEventType() == ModelEventType::Open &&
			 idx.isValid() ) {
			auto model = modelEvent->getModel();
			Variant vPath( model->data( idx, ModelRole::Custom ) );
			if ( vPath.isValid() && vPath.is( Variant::Type::cstr ) ) {
				std::string path( vPath.asCStr() );
				UITab* tab = mSplitter->isDocumentOpen( path );
				Variant lineNum( model->data( model->index( modelEvent->getModelIndex().row(), 1 ),
											  ModelRole::Custom ) );
				Variant colNum( model->data( model->index( modelEvent->getModelIndex().row(), 2 ),
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
	mBuildOutput = editor;
	mContainer->setVisible( false );
	auto cont = mContainer->getFirstChild()->asType<UIRelativeLayoutCommandExecuter>();
	cont->setCommand( "build-output-show-build-output", [this]() { showBuildOutput(); } );
	cont->setCommand( "build-output-show-build-issues", [this]() { showIssues(); } );
	cont->getKeyBindings().addKeybind( { KEY_1, KeyMod::getDefaultModifier() },
									   "build-output-show-build-output" );
	cont->getKeyBindings().addKeybind( { KEY_2, KeyMod::getDefaultModifier() },
									   "build-output-show-build-issues" );
	mButOutput->onClick( [this]( auto ) { showBuildOutput(); } );
	mButIssues->onClick( [this]( auto ) { showIssues(); } );
	mButOutput->setTooltipText(
		cont->getKeyBindings().getCommandKeybindString( "build-output-show-build-output" ) );
	mButIssues->setTooltipText(
		cont->getKeyBindings().getCommandKeybindString( "build-output-show-build-issues" ) );
}

} // namespace ecode
