#include "uibuildsettings.hpp"
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uilinearlayout.hpp>

namespace ecode {

class UIBuildStep : public UILinearLayout {
  public:
	static UIBuildStep* New( bool isBuildStep, UIBuildSettings* buildSettings, size_t stepNum,
							 ProjectBuildStep& buildStep ) {
		return eeNew( UIBuildStep, ( isBuildStep, buildSettings, stepNum, buildStep ) );
	}

  protected:
	UIBuildStep( bool isBuildStep, UIBuildSettings* buildSettings, size_t stepNum,
				 ProjectBuildStep& buildStep ) :
		UILinearLayout( "buildstep", UIOrientation::Vertical ),
		mIsBuildStep( isBuildStep ),
		mBuildSettings( buildSettings ),
		mStepNum( stepNum ),
		mStep( buildStep ) {
		setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
		addClass( "build_step" );

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
		findByClass<UITextView>( "step_name" )
			->setText( String::format( mBuildSettings->getUISceneNode()
										   ->i18n( "build_step_num", "Step %u" )
										   .toUtf8()
										   .c_str(),
									   stepNum + 1 ) );
		mDataBindHolder += UIDataBindBool::New( &mStep.enabled, findByClass( "enabled_checkbox" ) );
		mDataBindHolder += UIDataBindString::New( &mStep.cmd, findByClass( "input_cmd" ) );
		mDataBindHolder += UIDataBindString::New( &mStep.args, findByClass( "input_args" ) );
		mDataBindHolder +=
			UIDataBindString::New( &mStep.workingDir, findByClass( "input_working_dir" ) );
		findByClass( "details_but" )
			->addMouseClickListener(
				[this]( const MouseEvent* event ) {
					auto me = event->getNode()->asType<UIPushButton>();
					findByClass( "details" )->setVisible( me->hasClass( "contracted" ) );
					me->toggleClass( "contracted" );
				},
				MouseButton::EE_BUTTON_LEFT );
	}

	bool mIsBuildStep{ true };
	UIBuildSettings* mBuildSettings{ nullptr };
	size_t mStepNum{ 0 };
	ProjectBuildStep& mStep;
	UIDataBindHolder mDataBindHolder;
};

static const auto SETTINGS_PANEL_XML = R"xml(
<ScrollView lw="mp" lh="mp">
		<vbox lw="mp" lh="wc" class="settings_panel" id="build_settings_panel">
			<TextView class="title" text="@string(build_settings, Build Settings)" />
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
				<PushButton class="add_build_step" text="@string(add_build_step, Add Build Step)" />
			</vbox>

			<vbox lw="mp" lh="wc" class="clean_steps">
				<TextView class="subtitle" text="@string(clean_steps, Clean Steps)" />
				<vbox id="build_clean_steps_cont" lw="mp" lh="wc"></vbox>
				<PushButton class="add_build_step" text="@string(add_clean_step, Add Clean Step)" />
			</vbox>

			<vbox lw="mp" lh="wc" class="build_types">
				<TextView class="subtitle" text="@string(build_types, Build Types)" />
				<TextView lw="mp" lh="wc" word-wrap="true" text="@string(build_types_desc, Build types can be used as a dynamic build option represented by the special key ${build_type}. The build type can be switch easily from the editor.)" />
				<StackLayout class="build_types_cont span" lw="mp" lh="wc">
					<DropDownList id="build_type_list" layout_width="200dp" layout_height="wc" />
					<PushButton lh="mp" id="build_type_del" text="@string(delete_selected, Delete Selected)" text-as-fallback="true" icon="icon(delete-bin, 12dp)" tooltip="@string(delete_selected, Delete Selected)" />
				</StackLayout>
				<PushButton id="build_type_add" class="span" text="@string(add_build_type, Add Build Type)" />
			</vbox>

			<vbox class="advanced_options" lw="mp" lh="wc">
				<hbox class="title advanced_options_title" lw="mp" lh="wc">
					<TextView enabled="false" lw="0" lw8="1" lh="wc" class="advance_opt" text="@string(advanced_options, Advanced Options)" />
					<Image enabled="false" lg="center" />
				</hbox>
				<vbox class="inner_box" lw="mp" lh="wc">
					<vbox lw="mp" lh="wc" class="build_environment">
						<TextView class="subtitle" text="@string(build_environment, Build Environment)" />
						<CheckBox text="@string(clear_system_enviroment, Clear System Environment)" />
					</vbox>

					<vbox lw="mp" lh="wc" class="output_parser">
						<TextView class="subtitle" text="@string(output_parser, Output Parser)" />
						<TextView lw="mp" lh="wc" word-wrap="true" text="@string(output_parser_desc, Custom output parsers scan command line output for user-provided error patterns to create entries in Issues and highlight those errors on the Build Output)" />
						<TextView lw="mp" lh="wc" word-wrap="true" text='@string(output_parser_preset, "Presets are provided as generic output parsers, you can select one below, by default a \"generic\" preset will be selected:")' />
						<DropDownList id="output_parsers_presets_list" layout_width="200dp" layout_height="wc" selected-text="generic">
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
						<TableView lw="mp" lh="150dp" />
						<TextView class="span" lw="mp" lh="wc" word-wrap="true" text='@string(custom_variables_desc_3, There are predefined custom variables available to use:&#10;${project_root}: The folder / project root directory.&#10;${build_type}: The build type selected to build the project.&#10;${os}: The current operating system name.&#10;${nproc}: The number of logical processing units.)' />
					</vbox>
				</vbox>

			</vbox>

			<TextView class="build_settings_clarification span" word-wrap="true" lw="mp" lh="wc" text='@string(build_settings_save_clarification, * All changes are automatically saved)' />
		</vbox>
	</ScrollView>
)xml";

UIBuildSettings* UIBuildSettings::New( ProjectBuild& build ) {
	return eeNew( UIBuildSettings, ( build ) );
}

UIBuildSettings::UIBuildSettings( ProjectBuild& build ) : mBuild( build ) {
	mUISceneNode->loadLayoutFromString( SETTINGS_PANEL_XML, this,
										String::hash( "build_settings" ) );
	mDataBindHolder += UIDataBindString::New( &mBuild.mName, find<UITextInput>( "build_name" ) );

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
		auto bs = UIBuildStep::New( true, this, step, mBuild.mBuild[step] );
		bs->setParent( buildStepsParent );
	}

	auto buildCleanStepsParent = find( "build_clean_steps_cont" );
	for ( size_t step = 0; step < mBuild.mClean.size(); ++step ) {
		auto bs = UIBuildStep::New( false, this, step, mBuild.mClean[step] );
		bs->setParent( buildCleanStepsParent );
	}

	auto buildTypeDropDown = find<UIDropDownList>( "build_type_list" );
	std::vector<String> buildTypes;
	for ( const auto& type : mBuild.mBuildTypes )
		buildTypes.push_back( type );
	buildTypeDropDown->getListBox()->addListBoxItems( buildTypes );
	buildTypeDropDown->getListBox()->setSelected( 0 );

	auto advTitle = querySelector( ".settings_panel > .advanced_options > .title" );
	advTitle->addMouseClickListener(
		[this]( const MouseEvent* event ) {
			auto img = event->getNode()->findByType( UI_TYPE_IMAGE )->asType<UIWidget>();
			findByClass( "inner_box" )->toggleClass( "visible" );
			img->toggleClass( "expanded" );
		},
		MouseButton::EE_BUTTON_LEFT );
}

void UIBuildSettings::updateOS() {
	mBuild.mOS.clear();
	auto oses = find<UIWidget>( "os_select" )->querySelectorAll( "CheckBox" );
	for ( const auto os : oses ) {
		if ( os->asType<UICheckBox>()->isChecked() )
			mBuild.mOS.insert( os->getId() );
	}
}

} // namespace ecode
