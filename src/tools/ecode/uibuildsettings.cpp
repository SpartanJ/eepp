#include "uibuildsettings.hpp"
#include <algorithm>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uitableview.hpp>

using namespace EE::UI::Models;

namespace ecode {

class UIBuildStep : public UILinearLayout {
  public:
	static UIBuildStep* New( bool isBuildStep, UIBuildSettings* buildSettings, size_t stepNum,
							 ProjectBuildStep* buildStep ) {
		return eeNew( UIBuildStep, ( isBuildStep, buildSettings, stepNum, buildStep ) );
	}

	void clearBindings() { mDataBindHolder.clear(); }

	void updateStep( size_t stepNum, ProjectBuildStep* buildStep ) {
		clearBindings();

		removeClass( String::toString( mStepNum ) );
		mStepNum = stepNum;
		mStep = buildStep;
		addClass( String::toString( mStepNum ) );

		findByClass<UITextView>( "step_name" )
			->setText( String::format( mBuildSettings->getUISceneNode()
										   ->i18n( "build_step_num", "Step %u: %s" )
										   .toUtf8()
										   .c_str(),
									   mStepNum + 1, mStep->cmd.c_str() ) );

		mDataBindHolder +=
			UIDataBindBool::New( &mStep->enabled, findByClass( "enabled_checkbox" ) );
		auto placeholder = UIDataBindString::New( &mStep->cmd, findByClass( "input_cmd" ) );
		mDataBindHolder += UIDataBindString::New( &mStep->args, findByClass( "input_args" ) );
		mDataBindHolder +=
			UIDataBindString::New( &mStep->workingDir, findByClass( "input_working_dir" ) );
	}

  protected:
	UIBuildStep( bool isBuildStep, UIBuildSettings* buildSettings, size_t stepNum,
				 ProjectBuildStep* buildStep ) :
		UILinearLayout( "buildstep", UIOrientation::Vertical ),
		mIsBuildStep( isBuildStep ),
		mBuildSettings( buildSettings ),
		mStepNum( stepNum ),
		mStep( buildStep ) {
		setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
		addClass( "build_step" );
		addClass( String::toString( stepNum ) );

		static const auto BUILD_STEP_XML = R"xml(
<!-- <vbox lw="mp" lh="wc" class="build_step"> -->
	<hbox class="header" lw="mp" lh="wc">
		<TextView lw="0" lw8="1" class="step_name" />
		<CheckBox class="enabled_checkbox" checked="true" layout_gravity="center_vertical" tooltip="@string(enabled_question, Enabled?)" />
		<PushButton class="move_up" text="@string(move_up, Move Up)" text-as-fallback="true" icon="icon(arrow-up-s, 12dp)" tooltip="@string(move_up, Move Up)" />
		<PushButton class="move_down" text="@string(move_down, Move Down)" text-as-fallback="true" icon="icon(arrow-down-s, 12dp)"  tooltip="@string(move_down, Move Down)" />
		<PushButton class="remove_item" text="@string(remove item, Remove Item)" text-as-fallback="true" icon="icon(delete-bin, 12dp)" tooltip="@string(remove_item, Remove Item)" />
		<PushButton class="details_but" text='@string(details_nbsp, "Details ")' />
	</hbox>
	<vbox class="details" lw="mp" lh="wc">
		<hbox lw="mp">
			<TextView lh="mp" min-width="100dp" text="@string(command, Command)" />
			<Input class="input_cmd" lw="0" lw8="1" />
		</hbox>
		<hbox lw="mp">
			<TextView lh="mp" min-width="100dp" text="@string(arguments, Arguments)" />
			<Input class="input_args" lw="0" lw8="1" />
		</hbox>
		<hbox lw="mp">
			<TextView lh="mp" min-width="100dp" text="@string(working_dir, Working Directory)" />
			<Input class="input_working_dir" lw="0" lw8="1" />
		</hbox>
	</vbox>
<!-- </vbox> -->
)xml";

		getUISceneNode()->loadLayoutFromString( BUILD_STEP_XML, this );

		findByClass( "details_but" )->onClick( [this]( const MouseEvent* event ) {
			auto me = event->getNode()->asType<UIPushButton>();
			findByClass( "details" )->setVisible( me->hasClass( "contracted" ) );
			me->toggleClass( "contracted" );
		} );

		findByClass( "move_down" )->onClick( [this]( auto ) {
			mBuildSettings->moveStepDown( mStepNum, !mIsBuildStep );
		} );

		findByClass( "move_up" )->onClick( [this]( auto ) {
			mBuildSettings->moveStepUp( mStepNum, !mIsBuildStep );
		} );

		findByClass( "remove_item" )->onClick( [this]( auto ) {
			mBuildSettings->deleteStep( mStepNum, !mIsBuildStep );
		} );

		updateStep( mStepNum, mStep );
	}

	bool mIsBuildStep{ true };
	UIBuildSettings* mBuildSettings{ nullptr };
	size_t mStepNum{ 0 };
	ProjectBuildStep* mStep;
	UIDataBindHolder mDataBindHolder;
};

static const auto SETTINGS_PANEL_XML = R"xml(
<ScrollView lw="mp" lh="mp">
		<vbox lw="mp" lh="wc" class="settings_panel" id="build_settings_panel">
			<hbox lw="mp" lh="wc">
				<TextView lw="0" lw8="1" lh="wc" class="title" text="@string(build_settings, Build Settings)" />
				<PushButton id="build_del" lh="mp" text="@string(delete_setting, Delete Setting)" text-as-fallback="true" icon="icon(delete-bin, 12dp)" tooltip="@string(delete_setting, Delete Setting)" />
			</hbox>
			<Widget class="separator" lw="mp" lh="1dp" />
			<TextView class="subtitle" text="@string(build_name, Build Name)" />
			<Input id="build_name" lw="mp" lh="wc" text="new_name" />
			<TextView class="subtitle" text="@string(supported_platforms, Supported Platforms)" />
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
				<TextView class="subtitle" text="@string(build_steps, Build Steps)" />
				<vbox id="build_steps_cont" lw="mp" lh="wc"></vbox>
				<PushButton id="add_build_step" class="add_build_step" text="@string(add_build_step, Add Build Step)" />
			</vbox>

			<vbox lw="mp" lh="wc" class="clean_steps">
				<TextView class="subtitle" text="@string(clean_steps, Clean Steps)" />
				<vbox id="build_clean_steps_cont" lw="mp" lh="wc"></vbox>
				<PushButton id="add_clean_step" class="add_build_step" text="@string(add_clean_step, Add Clean Step)" />
			</vbox>

			<vbox lw="mp" lh="wc" class="build_types">
				<TextView class="subtitle" text="@string(build_types, Build Types)" />
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
					<vbox lw="mp" lh="wc" class="build_environment">
						<TextView class="subtitle" text="@string(build_environment, Build Environment)" />
						<CheckBox id="clear_sys_env" text="@string(clear_system_enviroment, Clear System Environment)" />
					</vbox>

					<vbox lw="mp" lh="wc" class="output_parser">
						<TextView class="subtitle" text="@string(output_parser, Output Parser)" />
						<TextView lw="mp" lh="wc" word-wrap="true" text="@string(output_parser_desc, Custom output parsers scan command line output for user-provided error patterns to create entries in Issues and highlight those errors on the Build Output)" />
						<TextView lw="mp" lh="wc" word-wrap="true" text='@string(output_parser_preset, "Presets are provided as generic output parsers, you can select one below, by default a \"generic\" preset will be selected:")' />
						<DropDownList id="output_parsers_presets_list" layout_width="200dp" layout_height="wc">
							<item></item>
							<item>generic</item>
						</DropDownList>
						<PushButton class="output_parser_custom_rule span" text="@string(output_parser_custom_rule, Add Custom Rule)" />
						<hbox class="output_parser_rules" lw="mp" lh="wc">
							<TextView lw="0" lw8="0.2" lh="wc" class="type" text="@string(type, Type)" />
							<TextView lw="0" lw8="0.8" lh="wc" class="pattern" class="rule" text="@string(pattern, Pattern)" />
							<PushButton class="remove_item" text="@string(remove item, Remove Item)" text-as-fallback="true" icon="icon(delete-bin, 12dp)" tooltip="@string(remove_item, Remove Item)" />
						</hbox>
					</vbox>

					<vbox lw="mp" lh="wc" class="custom_vars">
						<TextView class="subtitle" text="@string(custom_variables, Custom Variables)" />
						<TextView lw="mp" lh="wc" word-wrap="true" text='@string(custom_variables_desc, "Custom Variables allow to simplify the build commands steps adding custom variables that can be used over the build settings in commands, arguments, and working directories.")' />
						<TextView lw="mp" lh="wc" word-wrap="true" text='@string(custom_variables_desc_2, "Custom Variables can be invoked using ${variable_name} in any of the commands.)' />
						<TableView id="table_vars" lw="mp" lh="150dp" />
						<TextView class="span" lw="mp" lh="wc" word-wrap="true" text='@string(custom_variables_desc_3, There are predefined custom variables available to use:&#10;${project_root}: The folder / project root directory.&#10;${build_type}: The build type selected to build the project.&#10;${os}: The current operating system name.&#10;${nproc}: The number of logical processing units.)' />
					</vbox>
				</vbox>

			</vbox>

			<TextView class="build_settings_clarification span" word-wrap="true" lw="mp" lh="wc" text='@string(build_settings_save_clarification, * All changes are automatically saved)' />
		</vbox>
	</ScrollView>
)xml";

UIBuildSettings* UIBuildSettings::New( ProjectBuild& build, ProjectBuildConfiguration& config ) {
	return eeNew( UIBuildSettings, ( build, config ) );
}

UIBuildSettings::UIBuildSettings( ProjectBuild& build, ProjectBuildConfiguration& config ) :
	mBuild( build ), mConfig( config ), mOldName( mBuild.getName() ) {
	mUISceneNode->loadLayoutFromString( SETTINGS_PANEL_XML, this,
										String::hash( "build_settings" ) );
	auto buildNameInput = find<UITextInput>( "build_name" );
	mDataBindHolder += UIDataBindString::New( &mBuild.mName, buildNameInput );

	auto panelBuildNameDDL = getUISceneNode()
								 ->getRoot()
								 ->querySelector( " #build_tab #build_list" )
								 ->asType<UIDropDownList>();

	buildNameInput->on( Event::OnValueChange, [this, panelBuildNameDDL]( auto ) {
		refreshTab();
		if ( panelBuildNameDDL ) {
			auto idx = panelBuildNameDDL->getListBox()->getItemIndex( mOldName );
			if ( idx != eeINDEX_NOT_FOUND )
				panelBuildNameDDL->getListBox()->setItemText( idx, mBuild.getName() );
		}
		mOldName = mBuild.getName();
	} );

	auto oses = find<UIWidget>( "os_select" )->querySelectorAll( "CheckBox" );
	for ( const auto os : oses ) {
		if ( mBuild.mOS.find( os->getId() ) != mBuild.mOS.end() )
			os->asType<UICheckBox>()->setChecked( true );
		os->on( Event::OnValueChange, [this]( const Event* ) { updateOS(); } );
	}

	if ( mBuild.mBuild.empty() )
		mBuild.mBuild.push_back( {} );

	if ( mBuild.mClean.empty() )
		mBuild.mClean.push_back( {} );

	auto buildStepsParent = find( "build_steps_cont" );
	for ( size_t step = 0; step < mBuild.mBuild.size(); ++step ) {
		auto bs = UIBuildStep::New( true, this, step, &mBuild.mBuild[step] );
		bs->setParent( buildStepsParent );
	}

	find( "add_build_step" )->onClick( [this, buildStepsParent]( const Event* ) {
		mBuild.mBuild.push_back( {} );
		auto step = mBuild.mBuild.size() - 1;
		UIBuildStep::New( true, this, step, &mBuild.mBuild[step] )->setParent( buildStepsParent );
	} );

	auto buildCleanStepsParent = find( "build_clean_steps_cont" );
	for ( size_t step = 0; step < mBuild.mClean.size(); ++step ) {
		UIBuildStep::New( false, this, step, &mBuild.mClean[step] )
			->setParent( buildCleanStepsParent );
	}

	find( "add_clean_step" )->onClick( [this, buildCleanStepsParent]( const Event* ) {
		mBuild.mClean.push_back( {} );
		auto step = mBuild.mClean.size() - 1;
		UIBuildStep::New( false, this, step, &mBuild.mClean[step] )
			->setParent( buildCleanStepsParent );
	} );

	auto buildTypeDropDown = find<UIDropDownList>( "build_type_list" );
	auto panelBuildTypeDDL = getUISceneNode()
								 ->getRoot()
								 ->querySelector( " #build_tab #build_type_list" )
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
	if ( panelBuildTypeDDL )
		panelBuildTypeDDL->on(
			Event::OnItemSelected, [this, buildTypeDropDown, panelBuildTypeDDL]( const Event* ) {
				mConfig.buildType = panelBuildTypeDDL->getListBox()->getItemSelectedText().toUtf8();
				if ( buildTypeDropDown )
					buildTypeDropDown->getListBox()->setSelected( mConfig.buildType );
			} );

	auto advTitle = querySelector( ".settings_panel > .advanced_options > .title" );
	advTitle->onClick( [this]( const MouseEvent* event ) {
		auto img = event->getNode()->findByType( UI_TYPE_IMAGE )->asType<UIWidget>();
		findByClass( "inner_box" )->toggleClass( "visible" );
		img->toggleClass( "expanded" );
	} );

	mDataBindHolder +=
		UIDataBindBool::New( &mBuild.mConfig.clearSysEnv, find<UIWidget>( "clear_sys_env" ) );

	UITableView* tableVars = find<UITableView>( "table_vars" );
	auto model = ItemPairListModel<std::string, std::string>::create( mBuild.mVars );
	model->setColumnName( 0, getTranslatorString( "var_name", "Name" ) );
	model->setColumnName( 1, getTranslatorString( "var_value", "Value" ) );
	tableVars->setAutoColumnsWidth( true );
	tableVars->setModel( model );

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

void UIBuildSettings::refreshTab() {
	if ( !mTab )
		return;
	mTab->setText(
		String::format( ( i18n( "build_seetings", "Build Settings" ) + ": %s" ).toUtf8().c_str(),
						mBuild.mName.c_str() ) );
	mTab->setId( "build_settings_" + mBuild.mName );
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
	auto bs1 = cont->findByClass<UIBuildStep>( String::toString( stepNum ) );
	auto bs2 = cont->findByClass<UIBuildStep>( String::toString( newStep ) );
	bs1->updateStep( stepNum, &steps[stepNum] );
	bs2->updateStep( newStep, &steps[newStep] );
}

void UIBuildSettings::deleteStep( size_t stepNum, bool isClean ) {
	ProjectBuildSteps& steps = isClean ? mBuild.mClean : mBuild.mBuild;
	UIWidget* cont =
		isClean ? find<UIWidget>( "build_clean_steps_cont" ) : find<UIWidget>( "build_steps_cont" );
	for ( auto step = stepNum; step < steps.size(); step++ )
		cont->findByClass<UIBuildStep>( String::toString( step ) )->clearBindings();
	// cppcheck-suppress mismatchingContainerIterator
	steps.erase( steps.begin() + stepNum );
	cont->findByClass<UIBuildStep>( String::toString( stepNum ) )->close();
	for ( auto step = stepNum + 1; step < steps.size(); step++ )
		cont->findByClass<UIBuildStep>( String::toString( step ) )
			->updateStep( step, &steps[step] );
}

} // namespace ecode
