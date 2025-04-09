#include "uibuildsettings.hpp"
#include <algorithm>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uitableview.hpp>

using namespace EE::UI::Models;

namespace ecode {

class OutputParserModel final : public Model {
  public:
	static std::shared_ptr<OutputParserModel>
	create( std::vector<ProjectBuildOutputParserConfig>& data,
			std::function<String( const std::string&, const String& )> i18n ) {
		return std::make_shared<OutputParserModel>( data, i18n );
	}

	explicit OutputParserModel( std::vector<ProjectBuildOutputParserConfig>& data,
								std::function<String( const std::string&, const String& )> i18n ) :
		mData( data ), i18n( i18n ) {
		mColumnNames.push_back( i18n( "type", "Type" ) );
		mColumnNames.push_back( i18n( "pattern", "Pattern" ) );
	}

	virtual ~OutputParserModel() {}

	virtual size_t rowCount( const ModelIndex& ) const { return mData.size(); }

	virtual size_t columnCount( const ModelIndex& ) const { return 2; }

	virtual std::string columnName( const size_t& index ) const {
		eeASSERT( index < 2 );
		return mColumnNames[index];
	}

	virtual void setColumnName( const size_t& index, const std::string& name ) {
		eeASSERT( index < 2 );
		mColumnNames[index] = name;
	}

	virtual ModelIndex index( int row, int column, const ModelIndex& parent = ModelIndex() ) const {
		if ( row >= (int)rowCount( parent ) || column >= (int)columnCount( parent ) )
			return {};
		return Model::index( row, column, parent );
	}

	virtual Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const {
		if ( role == ModelRole::Display ) {
			switch ( index.column() ) {
				case 0: {
					std::string val =
						ProjectBuildOutputParserConfig::typeToString( mData[index.row()].type );
					return Variant( i18n( val, String::capitalize( val ) ) );
				}
				case 1:
				default:
					return Variant( mData[index.row()].pattern );
			}
		}
		return {};
	}

  private:
	std::vector<ProjectBuildOutputParserConfig>& mData;
	std::function<String( const std::string&, const String& )> i18n;
	std::vector<std::string> mColumnNames;
};

class UICustomOutputParserWindow : public UIWindow {
  public:
	static UICustomOutputParserWindow* New( ProjectBuildOutputParserConfig& cfg ) {
		return eeNew( UICustomOutputParserWindow, ( cfg ) );
	}

	explicit UICustomOutputParserWindow( ProjectBuildOutputParserConfig& cfg ) :
		UIWindow( SIMPLE_LAYOUT, { UI_WIN_CLOSE_BUTTON | UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS |
								   UI_WIN_SHARE_ALPHA_WITH_CHILDS | UI_WIN_MODAL } ),
		mTmpCfg( cfg ),
		mCfg( cfg ) {
		static const auto CUSTOM_OUTPUT_PARSER_XML = R"xml(
<vbox class="custom_output_parser_cont" lw="400dp" lh="wc">
	<TextView text="@string(type, Type)" focusable="false" />
	<DropDownList lw="mp" lh="wc" id="custom_parser_type" selectedIndex="0">
		<item>@string("error", "Error")</item>
		<item>@string("warning", "Warning")</item>
		<item>@string("notice", "Notice")</item>
	</DropDownList>
	<TextView text="@string(message_capture_pattern, Message Capture Pattern)" focusable="false" />
	<TextInput lw="mp" lh="wc" id="custom_parser_pattern" hint="@string(lua_pattern, Lua Pattern)" />
	<TextView text="@string(capture_positions, Capture Positions)" focusable="false" />
	<hbox lw="mp" lh="wc" class="capture_positions_cont">
		<vbox lw="0" lw8="0.25" lh="wc">
			<TextView lw="mp" lh="wc" text="@string(file_name, File Name)" focusable="false" />
			<SpinBox id="file_name_pos" lw="mp" lh="wc" min-value="0" max-value="4" value="1" />
		</vbox>
		<vbox lw="0" lw8="0.25" lh="wc">
			<TextView lw="mp" lh="wc" text="@string(line_number, Line Number)" focusable="false" />
			<SpinBox id="line_number_pos" lw="mp" lh="wc" min-value="0" max-value="4" value="2" />
		</vbox>
		<vbox lw="0" lw8="0.25" lh="wc">
			<TextView lw="mp" lh="wc" text="@string(column_position, Column Position)" focusable="false" />
			<SpinBox id="column_pos" lw="mp" lh="wc" min-value="0" max-value="4" value="3" />
		</vbox>
		<vbox lw="0" lw8="0.25" lh="wc">
			<TextView lw="mp" lh="wc" text="@string(message, Message)" focusable="false" />
			<SpinBox id="message_pos" lw="mp" lh="wc" min-value="0" max-value="4" value="4" />
		</vbox>
	</hbox>
	<hbox lw="wc" lh="wc" layout_gravity="right">
		<PushButton id="but_ok" text="@string(msg_box_ok, Ok)" icon="ok" margin-right="4dp" />
		<PushButton id="but_cancel" text="@string(camsg_box_cancel, Cancel)" icon="cancel" />
	</hbox>
</vbox>
)xml";

		mLayoutCont =
			getUISceneNode()->loadLayoutFromString( CUSTOM_OUTPUT_PARSER_XML, mContainer );

		setTitle( i18n( "custom_output_parser", "Custom Output Parser" ) );

		auto patternInput = find<UITextInput>( "custom_parser_pattern" );

		mDataBindHolder += UIDataBindString::New( &mTmpCfg.pattern, patternInput );

		UIDropDownList* cpTypeddl = find<UIDropDownList>( "custom_parser_type" );
		UIDataBind<ProjectOutputParserTypes>::Converter projectOutputParserTypesConverter(
			[]( const UIDataBind<ProjectOutputParserTypes>* databind, ProjectOutputParserTypes& val,
				const std::string& str ) -> bool {
				auto v = StyleSheetProperty( databind->getPropertyDefinition(), str ).asString();
				Uint32 idx;
				if ( String::fromString( idx, v ) && idx >= 0 && idx <= 2 ) {
					val = (ProjectOutputParserTypes)idx;
					return true;
				}
				return false;
			},
			[cpTypeddl]( const UIDataBind<ProjectOutputParserTypes>*, std::string& str,
						 const ProjectOutputParserTypes& val ) -> bool {
				str = cpTypeddl->getListBox()->getItem( (Uint32)val )->getText();
				return true;
			} );

		mDataBindHolder += UIDataBind<ProjectOutputParserTypes>::New(
			&mTmpCfg.type, cpTypeddl, projectOutputParserTypesConverter, "selected-index" );
		mDataBindHolder +=
			UIDataBind<int>::New( &mTmpCfg.patternOrder.file, find<UIWidget>( "file_name_pos" ) );
		mDataBindHolder +=
			UIDataBind<int>::New( &mTmpCfg.patternOrder.line, find<UIWidget>( "line_number_pos" ) );
		mDataBindHolder +=
			UIDataBind<int>::New( &mTmpCfg.patternOrder.col, find<UIWidget>( "column_pos" ) );
		mDataBindHolder +=
			UIDataBind<int>::New( &mTmpCfg.patternOrder.message, find<UIWidget>( "message_pos" ) );

		auto butOK = find<UIPushButton>( "but_ok" );
		butOK->setEnabled( !patternInput->getText().empty() );

		patternInput->on( Event::OnTextChanged, [butOK, patternInput]( auto ) {
			butOK->setEnabled( !patternInput->getText().empty() );
		} );

		butOK->onClick( [this]( auto ) {
			mCfg.pattern = mTmpCfg.pattern;
			mCfg.patternOrder = mTmpCfg.patternOrder;
			mCfg.type = mTmpCfg.type;
			sendCommonEvent( Event::OnConfirm );
			closeWindow();
		} );

		find( "but_cancel" )->onClick( [this]( auto ) { closeWindow(); } );
	}

	virtual ~UICustomOutputParserWindow() {}

  protected:
	UIWidget* mLayoutCont{ nullptr };
	ProjectBuildOutputParserConfig mTmpCfg;
	ProjectBuildOutputParserConfig& mCfg;
	UIDataBindHolder<int, std::string, ProjectOutputParserTypes> mDataBindHolder;

	virtual void onWindowReady() {
		forcedApplyStyle();

		Sizef size( mLayoutCont->getSize() );
		setMinWindowSize( size );
		center();

		if ( mShowWhenReady ) {
			mShowWhenReady = false;
			show();
		}

		sendCommonEvent( Event::OnWindowReady );
	}
};

class UIBuildStep : public UILinearLayout {
  public:
	enum class StepType { Build, Clean, Run };

	static UIBuildStep* New( StepType stepType, UIBuildSettings* buildSettings, size_t stepNum,
							 ProjectBuildStep* buildStep ) {
		return eeNew( UIBuildStep, ( stepType, buildSettings, stepNum, buildStep ) );
	}

	void clearBindings() { mDataBindHolder.clear(); }

	void updateStep( size_t stepNum, ProjectBuildStep* buildStep ) {
		clearBindings();

		removeClass( String::toString( (Uint64)mStepNum ) );
		mStepNum = stepNum;
		mStep = buildStep;
		addClass( String::toString( (Uint64)mStepNum ) );

		forEachChild( [buildStep]( Node* node ) { node->setEnabled( buildStep != nullptr ); } );
		if ( buildStep == nullptr )
			return;

		auto stepName = findByClass<UITextView>( "step_name" );
		if ( isBuildOrClean() ) {
			stepName->setText( String::format( mBuildSettings->getUISceneNode()
												   ->i18n( "build_step_num", "Step %u: %s" )
												   .toUtf8()
												   .c_str(),
											   mStepNum + 1, mStep->cmd.c_str() ) );
		} else {
			stepName->setText( getUISceneNode()->i18n( "executable_to_run", "Executable to Run" ) );
		}

		mDataBindHolder +=
			UIDataBindBool::New( &mStep->enabled, findByClass( "enabled_checkbox" ) );
		mDataBindHolder += UIDataBindString::New( &mStep->cmd, findByClass( "input_cmd" ) );
		mDataBindHolder += UIDataBindString::New( &mStep->args, findByClass( "input_args" ) );
		mDataBindHolder +=
			UIDataBindString::New( &mStep->workingDir, findByClass( "input_working_dir" ) );
		mDataBindHolder +=
			UIDataBindBool::New( &mStep->runInTerminal, findByClass( "run_in_terminal" ) );
	}

  protected:
	UIBuildStep( StepType stepType, UIBuildSettings* buildSettings, size_t stepNum,
				 ProjectBuildStep* buildStep ) :
		UILinearLayout( "buildstep", UIOrientation::Vertical ),
		mStepType( stepType ),
		mBuildSettings( buildSettings ),
		mStepNum( stepNum ),
		mStep( buildStep ) {
		setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
		addClass( "build_step" );
		addClass( String::toString( (Uint64)stepNum ) );

		static const auto BUILD_STEP_XML = R"xml(
	<hbox class="header" lw="mp" lh="wc">
		<TextView lw="0" lw8="1" class="step_name" focusable="false" />
		<CheckBox class="enabled_checkbox" checked="true" layout_gravity="center_vertical" tooltip="@string(enabled_question, Enabled?)" />
		<PushButton class="move_up" text="@string(move_up, Move Up)" text-as-fallback="true" icon="icon(arrow-up-s, 12dp)" tooltip="@string(move_up, Move Up)" />
		<PushButton class="move_down" text="@string(move_down, Move Down)" text-as-fallback="true" icon="icon(arrow-down-s, 12dp)"  tooltip="@string(move_down, Move Down)" />
		<PushButton class="remove_item" text="@string(remove item, Remove Item)" text-as-fallback="true" icon="icon(delete-bin, 12dp)" tooltip="@string(remove_item, Remove Item)" />
		<PushButton class="details_but" text='@string(details_nbsp, "Details ")' />
	</hbox>
	<vbox class="details" lw="mp" lh="wc">
		<hbox lw="mp">
			<TextView lh="mp" min-width="100dp" text="@string(command, Command)" focusable="false" />
			<Input class="input_cmd" lw="0" lw8="1" />
		</hbox>
		<hbox lw="mp">
			<TextView lh="mp" min-width="100dp" text="@string(arguments, Arguments)" focusable="false" />
			<Input class="input_args" lw="0" lw8="1" />
		</hbox>
		<hbox lw="mp">
			<TextView lh="mp" min-width="100dp" text="@string(working_dir, Working Directory)" focusable="false" />
			<Input class="input_working_dir" lw="0" lw8="1" />
		</hbox>
		<CheckBox class="run_in_terminal" text="@string(run_in_terminal, Run in terminal)" visible="false" />
	</vbox>
)xml";

		getUISceneNode()->loadLayoutFromString( BUILD_STEP_XML, this );

		if ( !isBuildOrClean() ) {
			findByClass( "enabled_checkbox" )->setVisible( false );
			auto runInTerminal = findByClass( "run_in_terminal" )->asType<UICheckBox>();
			runInTerminal->setVisible( true );
			runInTerminal->setChecked( buildStep->runInTerminal );
		}

		findByClass( "details_but" )->onClick( [this]( const MouseEvent* event ) {
			auto me = event->getNode()->asType<UIPushButton>();
			findByClass( "details" )->setVisible( me->hasClass( "contracted" ) );
			me->toggleClass( "contracted" );
		} );

		auto moveDown = findByClass( "move_down" );
		if ( isBuildOrClean() ) {
			moveDown->onClick( [this]( auto ) {
				mBuildSettings->moveStepDown( mStepNum, mStepType == StepType::Clean );
			} );
		} else {
			moveDown->setVisible( false );
		}

		auto moveUp = findByClass( "move_up" );
		if ( isBuildOrClean() ) {
			moveUp->onClick( [this]( auto ) {
				mBuildSettings->moveStepUp( mStepNum, mStepType == StepType::Clean );
			} );
		} else {
			moveUp->setVisible( false );
		}

		auto removeItem = findByClass( "remove_item" );

		if ( isBuildOrClean() ) {
			removeItem->onClick( [this]( auto ) {
				mBuildSettings->deleteStep( mStepNum, mStepType == StepType::Clean );
			} );
		} else {
			removeItem->setVisible( false );
		}

		updateStep( mStepNum, mStep );
	}

	bool isBuildOrClean() { return mStepType == StepType::Build || mStepType == StepType::Clean; }

	StepType mStepType{ true };
	UIBuildSettings* mBuildSettings{ nullptr };
	size_t mStepNum{ 0 };
	ProjectBuildStep* mStep;
	UIDataBindHolder<int, bool, std::string> mDataBindHolder;
};

static const auto SETTINGS_PANEL_XML = R"xml(
<ScrollView lw="mp" lh="mp">
	<vbox lw="mp" lh="wc" class="settings_panel" id="build_settings_panel">
		<hbox lw="mp" lh="wc">
			<TextView lw="0" lw8="1" lh="wc" class="title" text="@string(build_settings, Build Settings)" />
			<PushButton id="build_clone" lh="mp" text="@string(clone_setting, Clone Setting)" text-as-fallback="true" icon="icon(file-copy, 12dp)" tooltip="@string(clone_setting, Clone Setting)" margin-right="4dp" />
			<PushButton id="build_del" lh="mp" text="@string(delete_setting, Delete Setting)" text-as-fallback="true" icon="icon(delete-bin, 12dp)" tooltip="@string(delete_setting, Delete Setting)" />
		</hbox>
		<Widget class="separator" lw="mp" lh="1dp" />
		<TextView class="subtitle" text="@string(build_name, Build Name)" focusable="false" />
		<Input id="build_name" lw="mp" lh="wc" text="new_name" />
		<TextView class="subtitle" text="@string(supported_platforms, Supported Platforms)" focusable="false" />
		<TextView lw="mp" lh="wc" word-wrap="true" text="@string(supported_platforms_desc, Selecting none means that the build settings will work and be available on any Operating System)" />
		<StackLayout id="os_select" class="os_select" lw="wc" lh="wc">
			<CheckBox id="linux" text="Linux" />
			<CheckBox id="macos" text="macOS" />
			<CheckBox id="windows" text="Windows" />
			<CheckBox id="android" text="Android" />
			<CheckBox id="ios" text="iOS" />
			<CheckBox id="haiku" text="Haiku" />
			<CheckBox id="freebsd" text="FreeBSD" />
		</StackLayout>

		<vbox lw="mp" lh="wc" class="build_steps">
			<TextView class="subtitle" text="@string(build_steps, Build Steps)" focusable="false" />
			<vbox id="build_steps_cont" lw="mp" lh="wc"></vbox>
			<PushButton id="add_build_step" class="add_build_step" text="@string(add_build_step, Add Build Step)" />
		</vbox>

		<vbox lw="mp" lh="wc" class="clean_steps">
			<TextView class="subtitle" text="@string(clean_steps, Clean Steps)" focusable="false" />
			<vbox id="build_clean_steps_cont" lw="mp" lh="wc"></vbox>
			<PushButton id="add_clean_step" class="add_build_step" text="@string(add_clean_step, Add Clean Step)" />
		</vbox>

		<vbox lw="mp" lh="wc" class="run_step">
			<TextView class="subtitle" text="@string(run, Run)" focusable="false" />
			<StackLayout id="run_select" lw="mp" lh="wc" class="stack_margins">
				<TextView text="@string(run_configuration_colon, Run configuration:)" focusable="false" />
				<DropDownList id="run_list" layout_width="200dp" layout_height="19dp" />
				<PushButton id="run_add" text="@string(add_ellipsis, Add...)" />
				<PushButton id="run_remove" text="@string(remove, Remove)" />
				<PushButton id="run_remove_all" text="@string(remove_all, Remove All)" />
				<PushButton id="run_rename" text="@string(rename_ellipsis, Rename...)" />
				<PushButton id="run_clone" text="@string(clone_ellipsis, Clone...)" />
			</StackLayout>
			<vbox id="run_cont" lw="mp" lh="wc"></vbox>
		</vbox>

		<vbox lw="mp" lh="wc" class="build_types">
			<TextView class="subtitle" text="@string(build_types, Build Types)" focusable="false" />
			<TextView lw="mp" lh="wc" word-wrap="true" text="@string(build_types_desc, Build types can be used as a dynamic build option represented by the special key ${build_type}. The build type can be switch easily from the editor.)" />
			<StackLayout class="build_types_cont span" lw="mp" lh="wc">
				<DropDownList id="build_type_list" layout_width="200dp" layout_height="wc" />
				<PushButton id="build_type_add" lh="mp" text="@string(add_build_type, Add Build Type)" tooltip="@string(add_build_type, Add Build Type)" text-as-fallback="true" icon="icon(add, 12dp)" />
				<PushButton id="build_type_del" lh="mp" text="@string(delete_selected, Delete Selected)" text-as-fallback="true" icon="icon(delete-bin, 12dp)" tooltip="@string(delete_selected, Delete Selected)" />
			</StackLayout>
		</vbox>

		<vbox class="advanced_options" lw="mp" lh="wc">
			<hbox class="title advanced_options_title" lw="mp" lh="wc">
				<TextView enabled="false" lw="0" lw8="1" lh="wc" class="advance_opt" text="@string(advanced_options, Advanced Options)" />
				<Image enabled="false" lg="center" />
			</hbox>
			<vbox class="inner_box" lw="mp" lh="wc">
				<vbox lw="mp" lh="wc" class="custom_vars">
					<TextView class="subtitle" text="@string(custom_variables, Custom Variables)" focusable="false" />
					<TextView lw="mp" lh="wc" word-wrap="true" text='@string(custom_variables_desc, "Custom Variables allow to simplify the build commands steps adding custom variables that can be used over the build settings in commands, arguments, and working directories.")' />
					<TextView lw="mp" lh="wc" word-wrap="true" text='@string(custom_variables_desc_2, "Custom Variables can be invoked using ${variable_name} in any of the commands.)' />
					<hbox lw="mp" lh="wc">
						<TableView id="table_vars" lw="0" lw8="1" lh="150dp" />
						<vbox lw="wc" lh="mp" class="buttons_box">
							<PushButton id="custom_var_add" icon="icon(add, 12dp)" min-width="20dp" tooltip="@string(add_custom_variable, Add Custom Variable)" lg="center" />
							<PushButton id="custom_var_del" icon="icon(delete-bin, 12dp)" min-width="20dp" tooltip="@string(del_custom_variable, Delete Selected Variable)" lg="center" />
						</vbox>
					</hbox>
					<TextView class="span" lw="mp" lh="wc" word-wrap="true" text='@string(custom_variables_desc_3, "There are predefined custom variables available to use:&#10;${project_root}: The folder / project root directory.&#10;${build_type}: The build type selected to build the project.&#10;${os}: The current operating system name.&#10;${arch}: The current operating architecture.&#10;${nproc}: The number of logical processing units.&#10;${current_doc}: The last or current focused document path.&#10;${current_doc_name}: The last or current focused document name without extension.&#10;${current_doc_dir}: The last or current focused document directory.")' />
				</vbox>

				<vbox lw="mp" lh="wc" class="build_environment">
					<TextView class="subtitle" text="@string(build_environment, Build Environment)" focusable="false" />
					<CheckBox id="clear_sys_env" text="@string(clear_system_enviroment, Clear System Environment)" />
					<TextView class="subtitle" text="@string(custom_environment_variables, Custom Environment Variables)" focusable="false" />
					<hbox lw="mp" lh="wc">
						<TableView id="table_envs" lw="0" lw8="1" lh="150dp" />
						<vbox lw="wc" lh="mp" class="buttons_box">
							<PushButton id="custom_env_add" icon="icon(add, 12dp)" min-width="20dp" tooltip="@string(add_custom_variable, Add Custom Environment Variable)" lg="center" />
							<PushButton id="custom_env_del" icon="icon(delete-bin, 12dp)" min-width="20dp" tooltip="@string(del_custom_variable, Delete Selected Environment Variable)" lg="center" />
						</vbox>
					</hbox>
				</vbox>

				<vbox lw="mp" lh="wc" class="output_parser">
					<TextView class="subtitle" text="@string(output_parser, Output Parser)" focusable="false" />
					<TextView lw="mp" lh="wc" word-wrap="true" text="@string(output_parser_desc, Custom output parsers scan command line output for user-provided error patterns to create entries in Build Issues and highlight those errors on the Build Output)" focusable="false" />
					<TextView lw="mp" lh="wc" word-wrap="true" text='@string(output_parser_preset, "Presets are provided as generic output parsers, you can select one below, by default a \"generic\" preset will be selected:")' focusable="false" />
					<DropDownList id="output_parsers_presets_list" layout_width="200dp" layout_height="wc">
						<item></item>
						<item>generic</item>
					</DropDownList>
					<hbox lw="mp" lh="wc">
						<TableView id="table_output_parsers" lw="0" lw8="1" lh="150dp" focusable="false" />
						<vbox lw="wc" lh="mp" class="buttons_box">
							<PushButton id="custom_op_add" icon="icon(add, 12dp)" min-width="20dp" tooltip="@string(add_custom_output_parser, Add Custom Output Parser)" lg="center" />
							<PushButton id="custom_op_edit" icon="icon(file-edit, 12dp)" min-width="20dp" tooltip="@string(edit_custom_output_parser, Edit Selected Custom Output Parser)" lg="center" />
							<PushButton id="custom_op_del" icon="icon(delete-bin, 12dp)" min-width="20dp" tooltip="@string(del_custom_output_parser, Delete Selected Custom Output Parser)" lg="center" />
						</vbox>
					</hbox>
				</vbox>

			</vbox>

		</vbox>

		<TextView class="build_settings_clarification span" word-wrap="true" lw="mp" lh="wc" text='@string(build_settings_save_clarification, * All changes are automatically saved)' focusable="false" />
	</vbox>
</ScrollView>
)xml";

UIBuildSettings* UIBuildSettings::New(
	ProjectBuild& build, ProjectBuildConfiguration& config, bool isNew,
	const std::function<void( const std::string&, const std::string& )> onBuildNameChange ) {
	return eeNew( UIBuildSettings, ( build, config, isNew, onBuildNameChange ) );
}

UIBuildSettings::~UIBuildSettings() {
	for ( const auto& cbs : mCbs ) {
		for ( const auto& cb : cbs.second )
			cbs.first->removeEventListener( cb );
	}
	if ( !mCanceled )
		sendCommonEvent( Event::OnConfirm );
}

UIBuildSettings::UIBuildSettings(
	ProjectBuild& build, ProjectBuildConfiguration& config, bool isNew,
	const std::function<void( const std::string&, const std::string& )> onBuildNameChange ) :
	mBuild( build ),
	mConfig( config ),
	mOldName( mBuild.getName() ),
	mIsNew( isNew ),
	mNewNameFn( onBuildNameChange ) {
	addClass( "build_settings" );
	mUISceneNode->loadLayoutFromString( SETTINGS_PANEL_XML, this,
										String::hash( "build_settings" ) );
	auto buildNameInput = find<UITextInput>( "build_name" );
	mDataBindHolder += UIDataBindString::New( &mBuild.mName, buildNameInput );

	auto panelBuildNameDDL = getUISceneNode()
								 ->getRoot()
								 ->querySelector( "#build_tab_view #build_list" )
								 ->asType<UIDropDownList>();

	buildNameInput->on( Event::OnValueChange, [this, panelBuildNameDDL]( auto ) {
		refreshTab();
		if ( panelBuildNameDDL ) {
			auto idx = panelBuildNameDDL->getListBox()->getItemIndex( mOldName );
			if ( idx != eeINDEX_NOT_FOUND )
				panelBuildNameDDL->getListBox()->setItemText( idx, mBuild.getName() );
		}
		if ( mOldName == mConfig.buildName )
			mConfig.buildName = mBuild.getName();
		if ( mNewNameFn )
			mNewNameFn( mOldName, mBuild.getName() );
		mOldName = mBuild.getName();
	} );

	auto oses = find<UIWidget>( "os_select" )->querySelectorAll( "CheckBox" );
	for ( const auto os : oses ) {
		if ( mBuild.mOS.find( os->getId() ) != mBuild.mOS.end() )
			os->asType<UICheckBox>()->setChecked( true );
		os->on( Event::OnValueChange, [this]( const Event* ) { updateOS(); } );
	}

	if ( mBuild.mBuild.empty() )
		mBuild.mBuild.push_back( std::make_unique<ProjectBuildStep>() );

	if ( mBuild.mClean.empty() )
		mBuild.mClean.push_back( std::make_unique<ProjectBuildStep>() );

	auto buildStepsParent = find( "build_steps_cont" );
	for ( size_t step = 0; step < mBuild.mBuild.size(); ++step ) {
		auto bs =
			UIBuildStep::New( UIBuildStep::StepType::Build, this, step, mBuild.mBuild[step].get() );
		bs->setParent( buildStepsParent );
	}

	find( "add_build_step" )->onClick( [this, buildStepsParent]( const Event* ) {
		mBuild.mBuild.push_back( std::make_unique<ProjectBuildStep>() );
		auto step = mBuild.mBuild.size() - 1;
		UIBuildStep::New( UIBuildStep::StepType::Build, this, step, mBuild.mBuild[step].get() )
			->setParent( buildStepsParent );
	} );

	auto buildCleanStepsParent = find( "build_clean_steps_cont" );
	for ( size_t step = 0; step < mBuild.mClean.size(); ++step ) {
		UIBuildStep::New( UIBuildStep::StepType::Clean, this, step, mBuild.mClean[step].get() )
			->setParent( buildCleanStepsParent );
	}

	find( "add_clean_step" )->onClick( [this, buildCleanStepsParent]( const Event* ) {
		mBuild.mClean.push_back( std::make_unique<ProjectBuildStep>() );
		auto step = mBuild.mClean.size() - 1;
		UIBuildStep::New( UIBuildStep::StepType::Clean, this, step, mBuild.mClean[step].get() )
			->setParent( buildCleanStepsParent );
	} );

	auto buildTypeDropDown = find<UIDropDownList>( "build_type_list" );
	auto panelBuildTypeDDL = getUISceneNode()
								 ->getRoot()
								 ->querySelector( "#build_tab_view #build_type_list" )
								 ->asType<UIDropDownList>();

	std::vector<String> buildTypes;
	for ( const auto& type : mBuild.mBuildTypes )
		buildTypes.push_back( type );
	buildTypeDropDown->getListBox()->addListBoxItems( buildTypes );
	buildTypeDropDown->getListBox()->setSelected( mConfig.buildType );
	buildTypeDropDown->on(
		Event::OnItemSelected, [this, buildTypeDropDown, panelBuildTypeDDL]( const Event* ) {
			mConfig.buildType = buildTypeDropDown->getListBox()->getItemSelectedText().toUtf8();
			if ( panelBuildTypeDDL )
				panelBuildTypeDDL->getListBox()->setSelected( mConfig.buildType );
		} );
	if ( panelBuildTypeDDL ) {
		mCbs[panelBuildTypeDDL].push_back( panelBuildTypeDDL->on(
			Event::OnItemSelected, [this, buildTypeDropDown, panelBuildTypeDDL]( const Event* ) {
				mConfig.buildType = panelBuildTypeDDL->getListBox()->getItemSelectedText().toUtf8();
				if ( buildTypeDropDown )
					buildTypeDropDown->getListBox()->setSelected( mConfig.buildType );
			} ) );
		mCbs[panelBuildTypeDDL].push_back(
			panelBuildTypeDDL->on( Event::OnClose, [this, panelBuildTypeDDL]( auto ) {
				mCbs.erase( panelBuildTypeDDL );
			} ) );
	}

	auto advTitle = querySelector( ".settings_panel > .advanced_options > .title" );
	advTitle->onClick( [this, advTitle]( const MouseEvent* event ) {
		if ( getEventDispatcher()->getMouseDownNode() == advTitle ) {
			auto img = event->getNode()->findByType( UI_TYPE_IMAGE )->asType<UIWidget>();
			findByClass( "inner_box" )->toggleClass( "visible" );
			img->toggleClass( "expanded" );
		}
	} );

	mDataBindHolder +=
		UIDataBindBool::New( &mBuild.mConfig.clearSysEnv, find<UIWidget>( "clear_sys_env" ) );

	bindTable( "table_envs", "env", mBuild.mEnvs );
	bindTable( "table_vars", "var", mBuild.mVars );

	find( "build_del" )->onClick( [this]( auto ) {
		UIMessageBox* msgBox =
			UIMessageBox::New( UIMessageBox::OK_CANCEL,
							   i18n( "confirm_build_delete",
									 "Are you sure you want to delete the build configuration?" ) );
		msgBox->setTitle( i18n( "build_settings", "Build Settings" ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->showWhenReady();
		msgBox->addEventListener( Event::OnConfirm, [this, msgBox]( const Event* ) {
			mCanceled = true;
			sendTextEvent( Event::OnClear, mBuild.getName() );
			msgBox->closeWindow();
			if ( mTab )
				mTab->removeTab();
		} );
	} );

	find( "build_clone" )->onClick( [this]( auto ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::INPUT, i18n( "build_cloned_name", "New Cloned Build Name:" ) );
		msgBox->setTitle( i18n( "build_settings", "Build Settings" ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->showWhenReady();
		msgBox->on( Event::OnWindowReady, [this, msgBox]( auto ) {
			msgBox->getTextInput()->setText( mBuild.getName() );
			msgBox->getTextInput()->getDocument().selectAll();
		} );
		msgBox->addEventListener( Event::OnConfirm, [msgBox, this]( const Event* ) {
			const auto& newBuildName = msgBox->getTextInput()->getText();
			sendTextEvent( Event::OnCopy, newBuildName );
			msgBox->closeWindow();
		} );
	} );

	find( "build_type_add" )->onClick( [this, buildTypeDropDown, panelBuildTypeDDL]( auto ) {
		UIMessageBox* msgBox =
			UIMessageBox::New( UIMessageBox::INPUT, i18n( "build_type_name", "Build Type Name:" ) );
		msgBox->setTitle( i18n( "build_settings", "Build Settings" ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->showWhenReady();
		msgBox->addEventListener(
			Event::OnConfirm, [this, msgBox, buildTypeDropDown, panelBuildTypeDDL]( const Event* ) {
				const auto& buildType = msgBox->getTextInput()->getText();
				mBuild.mBuildTypes.insert( buildType.toUtf8() );
				buildTypeDropDown->getListBox()->addListBoxItem( buildType );
				buildTypeDropDown->getListBox()->setSelected( buildType );
				if ( panelBuildTypeDDL ) {
					panelBuildTypeDDL->getListBox()->addListBoxItem( buildType );
					panelBuildTypeDDL->getListBox()->setSelected( buildType );
					panelBuildTypeDDL->setEnabled( true );
				}
				msgBox->closeWindow();
			} );
	} );

	find( "build_type_del" )->onClick( [this, buildTypeDropDown, panelBuildTypeDDL]( auto ) {
		const auto& txt = buildTypeDropDown->getListBox()->getItemSelectedText();
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			String::format(
				i18n( "build_type_name_del", "Delete Build Type: %s?" ).toUtf8().c_str(),
				txt.toUtf8().c_str() ) );
		msgBox->setTitle( i18n( "build_settings", "Build Settings" ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->showWhenReady();
		msgBox->addEventListener( Event::OnConfirm, [this, msgBox, buildTypeDropDown,
													 panelBuildTypeDDL, txt]( const Event* ) {
			mBuild.mBuildTypes.erase( txt.toUtf8() );
			buildTypeDropDown->getListBox()->removeListBoxItem( txt );
			if ( panelBuildTypeDDL ) {
				panelBuildTypeDDL->getListBox()->removeListBoxItem( txt );
				if ( panelBuildTypeDDL->getListBox()->isEmpty() )
					panelBuildTypeDDL->setEnabled( false );
			}
			msgBox->closeWindow();
		} );
	} );

	auto outputParserPresetsDDL = find<UIDropDownList>( "output_parsers_presets_list" );
	outputParserPresetsDDL->getListBox()->setSelected( mBuild.mOutputParser.mPreset );
	outputParserPresetsDDL->on( Event::OnItemSelected, [this]( const Event* event ) {
		std::string txt( event->getNode()
							 ->asType<UIDropDownList>()
							 ->getListBox()
							 ->getItemSelectedText()
							 .toUtf8() );
		mBuild.mOutputParser.mPreset = txt;
		if ( ProjectBuildOutputParser::existsPreset( txt ) ) {
			mBuild.mOutputParser.mPresetConfig =
				ProjectBuildOutputParser::getPresets()[mBuild.mOutputParser.mPreset].mConfig;
		}
	} );

	UITableView* tableOP = find<UITableView>( "table_output_parsers" );
	tableOP->setMainColumn( 1 );
	tableOP->setAutoColumnsWidth( true );
	tableOP->setFitAllColumnsToWidget( true );
	auto modelOP = OutputParserModel::create( mBuild.mOutputParser.mConfig,
											  [this]( auto s, auto s2 ) { return i18n( s, s2 ); } );
	tableOP->setModel( modelOP );

	find( "custom_op_add" )->onClick( [this, modelOP]( auto ) {
		mTmpOpCfg = {};
		auto ret = UICustomOutputParserWindow::New( mTmpOpCfg );
		ret->showWhenReady();
		ret->on( Event::OnConfirm, [this, modelOP]( auto ) {
			mBuild.mOutputParser.mConfig.push_back( mTmpOpCfg );
			modelOP->invalidate();
		} );
	} );

	find( "custom_op_edit" )->onClick( [this, tableOP, modelOP]( auto ) {
		if ( !tableOP->getSelection().isEmpty() && tableOP->getSelection().first().row() >= 0 &&
			 tableOP->getSelection().first().row() < (int)mBuild.mOutputParser.mConfig.size() ) {
			auto ret = UICustomOutputParserWindow::New(
				mBuild.mOutputParser.mConfig[tableOP->getSelection().first().row()] );
			ret->showWhenReady();
			ret->on( Event::OnConfirm, [modelOP]( auto ) { modelOP->invalidate(); } );
		}
	} );

	find( "custom_op_del" )->onClick( [this, tableOP, modelOP]( auto ) {
		if ( !tableOP->getSelection().isEmpty() && tableOP->getSelection().first().row() >= 0 &&
			 tableOP->getSelection().first().row() < (int)mBuild.mOutputParser.mConfig.size() ) {
			mBuild.mOutputParser.mConfig.erase( mBuild.mOutputParser.mConfig.begin() +
												tableOP->getSelection().first().row() );
			modelOP->invalidate();
		}
	} );

	runSetup();
}

void UIBuildSettings::updateOS() {
	mBuild.mOS.clear();
	auto oses = find<UIWidget>( "os_select" )->querySelectorAll( "CheckBox" );
	for ( const auto os : oses ) {
		if ( os->asType<UICheckBox>()->isChecked() )
			mBuild.mOS.insert( os->getId() );
	}
}

void UIBuildSettings::setTab( UITab* tab ) {
	if ( tab != mTab ) {
		mTab = tab;
		refreshTab();
	}
}

UITab* UIBuildSettings::getTab() const {
	return mTab;
}

void UIBuildSettings::refreshTab() {
	if ( !mTab )
		return;
	mTab->setText(
		String::format( ( i18n( "build_seetings", "Build Settings" ) + ": %s" ).toUtf8().c_str(),
						mBuild.mName.c_str() ) );
	std::string hashName = String::toString( String::hash( mIsNew ? "new_name" : mBuild.mName ) );
	mTab->setId( "build_settings_" + hashName );
}

void UIBuildSettings::bindTable( const std::string& name, const std::string& key,
								 ProjectBuildKeyVal& data ) {

	UITableView* table = find<UITableView>( name );
	auto model = ItemPairListModel<std::string, std::string>::create( data );
	const auto createInputDelegate = [table]( const ModelIndex& ) -> ModelEditingDelegate* {
		auto delegate = StringModelEditingDelegate::New();
		delegate->onWillBeginEditing = [delegate, table]() {
			delegate->getWidget()->asType<UITextInput>()->on( Event::OnFocusLoss,
															  [delegate, table]( auto ) {
																  delegate->onCommit();
																  table->recalculateColumnsWidth();
															  } );
		};
		return delegate;
	};
	model->setColumnName( 0, i18n( key + "_name", "Name" ) );
	model->setColumnName( 1, i18n( key + "_value", "Value" ) );
	model->setIsEditable( true );
	table->setMainColumn( 1 );
	table->setAutoColumnsWidth( true );
	table->setFitAllColumnsToWidget( true );
	table->setModel( model );
	table->setEditable( true );
	table->setSelectionType( UIAbstractView::SelectionType::Cell );
	table->setEditTriggers( UIAbstractView::EditTrigger::DoubleClicked |
							UIAbstractTableView::EditTrigger::EditKeyPressed );
	table->onCreateEditingDelegate = createInputDelegate;

	find<UIPushButton>( "custom_" + key + "_add" )->onClick( [this, model, &data]( auto ) {
		data.push_back( { i18n( "new_name", "New Name" ), i18n( "new_value", "New Value" ) } );
		model->invalidate();
	} );

	find<UIPushButton>( "custom_" + key + "_del" )->onClick( [model, table, &data]( auto ) {
		if ( !table->getSelection().isEmpty() ) {
			data.erase( data.begin() + table->getSelection().first().row() );
			model->invalidate();
		}
	} );
}

void UIBuildSettings::moveStepUp( size_t stepNum, bool isClean ) {
	moveStepDir( stepNum, isClean, -1 );
}

void UIBuildSettings::moveStepDown( size_t stepNum, bool isClean ) {
	moveStepDir( stepNum, isClean, 1 );
}

void UIBuildSettings::moveStepDir( size_t stepNum, bool isClean, int dir ) {
	ProjectBuildSteps& steps = isClean ? mBuild.mClean : mBuild.mBuild;
	UIWidget* cont =
		isClean ? find<UIWidget>( "build_clean_steps_cont" ) : find<UIWidget>( "build_steps_cont" );
	int newStep = (int)stepNum + dir;
	std::swap( steps[stepNum], steps[newStep] );
	auto bs1 = cont->findByClass<UIBuildStep>( String::toString( (Uint64)stepNum ) );
	auto bs2 = cont->findByClass<UIBuildStep>( String::toString( newStep ) );
	bs1->updateStep( stepNum, steps[stepNum].get() );
	bs2->updateStep( newStep, steps[newStep].get() );
}

void UIBuildSettings::deleteStep( size_t stepNum, bool isClean ) {
	ProjectBuildSteps& steps = isClean ? mBuild.mClean : mBuild.mBuild;
	UIWidget* cont =
		isClean ? find<UIWidget>( "build_clean_steps_cont" ) : find<UIWidget>( "build_steps_cont" );
	for ( auto step = stepNum; step < steps.size(); step++ )
		cont->findByClass<UIBuildStep>( String::toString( (Uint64)step ) )->clearBindings();
	steps.erase( steps.begin() + stepNum );
	cont->findByClass<UIBuildStep>( String::toString( (Uint64)stepNum ) )->close();
	for ( auto step = stepNum + 1; step <= steps.size(); step++ )
		cont->findByClass<UIBuildStep>( String::toString( (Uint64)step ) )
			->updateStep( step - 1, steps[step - 1].get() );
}

void UIBuildSettings::runSetup() {
	if ( mBuild.mRun.empty() ) {
		mBuild.mRun.push_back( std::make_unique<ProjectBuildStep>() );
		mBuild.mRun.back()->name = i18n( "custom_executable", "Custom Executable" );
	}

	UIBuildStep::New( UIBuildStep::StepType::Run, this, 0, mBuild.mRun[runIndex()].get() )
		->setParent( find( "run_cont" ) );

	UIDropDownList* runList = find( "run_list" )->asType<UIDropDownList>();
	auto panelRunListDDL = getUISceneNode()
							   ->getRoot()
							   ->querySelector( "#build_tab_view #run_config_list" )
							   ->asType<UIDropDownList>();

	runUpdate( true, runList, panelRunListDDL );

	runList->getListBox()->setSelected( runIndex() );
	if ( panelRunListDDL )
		panelRunListDDL->getListBox()->setSelected( runIndex() );

	runSelect( runIndex() );

	runList->on( Event::OnItemSelected, [this, runList, panelRunListDDL]( auto ) {
		mConfig.runName = runList->getListBox()->getItemSelectedText().toUtf8();
		runSelect( runIndex() );
		if ( panelRunListDDL )
			panelRunListDDL->getListBox()->setSelected( mConfig.runName );
	} );

	if ( panelRunListDDL ) {
		mCbs[panelRunListDDL].push_back( panelRunListDDL->on(
			Event::OnItemSelected, [this, runList, panelRunListDDL]( const Event* ) {
				mConfig.runName = panelRunListDDL->getListBox()->getItemSelectedText().toUtf8();
				if ( runList )
					runList->getListBox()->setSelected( mConfig.runName );
			} ) );
		mCbs[panelRunListDDL].push_back( panelRunListDDL->on(
			Event::OnClose, [this, panelRunListDDL]( auto ) { mCbs.erase( panelRunListDDL ); } ) );
	}

	find( "run_add" )->onClick( [this, runList, panelRunListDDL]( auto ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::INPUT, i18n( "run_configuration_name", "Run configuration name:" ) );
		msgBox->setTitle( i18n( "run_settings", "Run Settings" ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->showWhenReady();
		msgBox->on( Event::OnConfirm, [this, msgBox, runList, panelRunListDDL]( auto ) {
			bool unique = true;
			auto newName = msgBox->getTextInput()->getText().toUtf8();
			for ( const auto& run : mBuild.mRun ) {
				if ( newName == run->name ) {
					unique = false;
					break;
				}
			}

			if ( unique ) {
				mBuild.mRun.push_back( std::make_unique<ProjectBuildStep>() );
				mBuild.mRun.back()->name = newName;

				runList->getListBox()->addListBoxItem( newName );
				runList->getListBox()->setSelected( newName );

				if ( panelRunListDDL ) {
					panelRunListDDL->getListBox()->addListBoxItem( newName );
					panelRunListDDL->getListBox()->setSelected( newName );
				}

				runUpdate( false, runList, panelRunListDDL );
			} else {
				UIMessageBox* uniqueAlert = UIMessageBox::New(
					UIMessageBox::OK, i18n( "run_configuration_name_must_be_unique",
											"Run configuration name must be unique!" ) );
				uniqueAlert->setTitle( i18n( "run_settings", "Run Settings" ) );
				uniqueAlert->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
				uniqueAlert->showWhenReady();
			}

			msgBox->close();
		} );
	} );

	find( "run_remove" )->onClick( [this, runList, panelRunListDDL]( auto ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			i18n( "run_configuration_delete_confirm",
				  "Are you sure you want to delete the currently selected run configuration?" ) );
		msgBox->setTitle( i18n( "run_configuration", "Run Configuration" ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->showWhenReady();
		msgBox->on( Event::OnConfirm, [this, runList, panelRunListDDL]( auto ) {
			runRemove( false, runList, panelRunListDDL );
		} );
	} );

	find( "run_remove_all" )->onClick( [this, runList, panelRunListDDL]( auto ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			i18n( "run_configuration_delete_all_confirm",
				  "Are you sure you want to delete all the run configurations?" ) );
		msgBox->setTitle( i18n( "run_configuration", "Run Configuration" ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->showWhenReady();
		msgBox->on( Event::OnConfirm, [this, runList, panelRunListDDL]( auto ) {
			runRemove( true, runList, panelRunListDDL );
		} );
	} );

	find( "run_rename" )->onClick( [this, runList, panelRunListDDL]( auto ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::INPUT, i18n( "run_configuration_rename", "Rename configuration name:" ) );
		msgBox->setTitle( i18n( "run_settings", "Run Settings" ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->getTextInput()->setText( runList->getText() );
		msgBox->getTextInput()->getDocument().selectAll();
		msgBox->showWhenReady();
		auto selectedIndex = runList->getListBox()->getItemSelectedIndex();
		msgBox->on(
			Event::OnConfirm, [this, msgBox, selectedIndex, runList, panelRunListDDL]( auto ) {
				bool unique = true;
				auto newName = msgBox->getTextInput()->getText().toUtf8();
				for ( size_t i = 0; i < mBuild.mRun.size(); i++ ) {
					const auto& run = mBuild.mRun[i];
					if ( newName == run->name && i != selectedIndex ) {
						unique = false;
						break;
					}
				}

				if ( unique && selectedIndex < mBuild.mRun.size() ) {
					if ( mBuild.mRun[selectedIndex]->name == mConfig.runName )
						mConfig.runName = newName;
					mBuild.mRun[selectedIndex]->name = newName;
					runList->getListBox()->setItemText( selectedIndex, newName );
					if ( panelRunListDDL )
						panelRunListDDL->getListBox()->setItemText( selectedIndex, newName );
				} else {
					UIMessageBox* uniqueAlert = UIMessageBox::New(
						UIMessageBox::OK, i18n( "run_configuration_name_must_be_unique",
												"Run configuration name must be unique!" ) );
					uniqueAlert->setTitle( i18n( "run_settings", "Run Settings" ) );
					uniqueAlert->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
					uniqueAlert->showWhenReady();
				}

				msgBox->close();
			} );
	} );

	find( "run_clone" )->onClick( [this, runList, panelRunListDDL]( auto ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::INPUT, i18n( "run_configuration_name", "Run configuration name:" ) );
		msgBox->setTitle( i18n( "run_settings", "Run Settings" ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->getTextInput()->setText( runList->getText() );
		msgBox->getTextInput()->getDocument().selectAll();
		msgBox->showWhenReady();
		msgBox->on( Event::OnConfirm, [this, msgBox, runList, panelRunListDDL]( auto ) {
			bool unique = true;
			auto newName = msgBox->getTextInput()->getText().toUtf8();
			for ( const auto& run : mBuild.mRun ) {
				if ( newName == run->name ) {
					unique = false;
					break;
				}
			}

			if ( unique ) {
				auto selectedIndex = runList->getListBox()->getItemSelectedIndex();
				if ( selectedIndex < mBuild.mRun.size() ) {
					mBuild.mRun.push_back(
						std::make_unique<ProjectBuildStep>( *mBuild.mRun[selectedIndex].get() ) );
				} else {
					mBuild.mRun.push_back( std::make_unique<ProjectBuildStep>() );
				}
				mBuild.mRun.back()->name = newName;

				runList->getListBox()->addListBoxItem( newName );
				runList->getListBox()->setSelected( newName );
				if ( panelRunListDDL ) {
					panelRunListDDL->getListBox()->addListBoxItem( newName );
					panelRunListDDL->getListBox()->setSelected( newName );
				}
			} else {
				UIMessageBox* uniqueAlert = UIMessageBox::New(
					UIMessageBox::OK, i18n( "run_configuration_name_must_be_unique",
											"Run configuration name must be unique!" ) );
				uniqueAlert->setTitle( i18n( "run_settings", "Run Settings" ) );
				uniqueAlert->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
				uniqueAlert->showWhenReady();
			}

			msgBox->close();
		} );
	} );
}

void UIBuildSettings::runSelect( Uint32 index ) {
	UIWidget* cont = find<UIWidget>( "run_cont" );
	auto bs = cont->findByClass<UIBuildStep>( String::toString( 0 ) );
	if ( index < mBuild.mRun.size() ) {
		bs->updateStep( 0, mBuild.mRun[index].get() );
	} else {
		if ( mBuild.mRun.empty() ) {
			mBuild.mRun.push_back( std::make_unique<ProjectBuildStep>() );
			mBuild.mRun.back()->name = i18n( "custom_executable", "Custom Executable" );
		}
		bs->updateStep( 0, mBuild.mRun[0].get() );
	}
}

void UIBuildSettings::runRemove( bool all, UIDropDownList* runList,
								 UIDropDownList* panelRunListDDL ) {

	if ( runList->getListBox()->isEmpty() )
		return;

	if ( all ) {
		runList->getListBox()->clear();
		if ( panelRunListDDL )
			panelRunListDDL->getListBox()->clear();
		mBuild.mRun.clear();
	} else {
		auto name = runList->getListBox()->getItemSelectedText();
		mBuild.mRun.erase( mBuild.mRun.begin() + runList->getListBox()->getItemSelectedIndex() );
		runList->getListBox()->removeListBoxItem( name );
		if ( panelRunListDDL )
			panelRunListDDL->getListBox()->removeListBoxItem( name );
	}

	runUpdate( false, runList, panelRunListDDL );

	if ( all )
		runSelect();
}

void UIBuildSettings::runUpdate( bool recreateList, UIDropDownList* runList,
								 UIDropDownList* panelRunListDDL ) {
	if ( recreateList ) {
		runList->getListBox()->clear();
		if ( panelRunListDDL )
			panelRunListDDL->getListBox()->clear();

		size_t i = 1;
		for ( const auto& run : mBuild.mRun ) {
			auto name =
				run->name.empty()
					? String::format(
						  i18n( "custom_executable_num", "Custom Executable %d" ).toUtf8(), i )
					: run->name;
			runList->getListBox()->addListBoxItem( name );
			if ( panelRunListDDL )
				panelRunListDDL->getListBox()->addListBoxItem( name );
			i++;
		}
	}

	bool runButEnabled = !runList->getListBox()->isEmpty();
	runList->setEnabled( runButEnabled );
	if ( panelRunListDDL )
		panelRunListDDL->setEnabled( runButEnabled );
	find( "run_cont" )->setEnabled( runButEnabled )->setVisible( runButEnabled );
	find( "run_remove" )->setEnabled( runButEnabled );
	find( "run_remove_all" )->setEnabled( runButEnabled );
	find( "run_rename" )->setEnabled( runButEnabled );
	find( "run_clone" )->setEnabled( runButEnabled );
}

Uint32 UIBuildSettings::runIndex() const {
	for ( size_t i = 0; i < mBuild.mRun.size(); i++ ) {
		if ( mBuild.mRun[i]->name == mConfig.runName )
			return i;
	}
	return 0;
}

} // namespace ecode
