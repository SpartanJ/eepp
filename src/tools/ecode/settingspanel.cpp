#include "settingspanel.hpp"
#include "datetimecontroller.hpp"
#include "ecode.hpp"
#include "plugins/plugin.hpp"
#include "plugins/pluginmanager.hpp"
#include "settingsdocument.hpp"
#include "settingspage.hpp"
#include "uitreeviewfs.hpp"
#include <deque>
#include <limits>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

using namespace EE::UI::Models;

namespace ecode {

class SettingsCategoryModel final : public Model {
  public:
	struct Node {
		std::string id;
		std::string text;
		Node* parent{ nullptr };
		std::vector<Node*> children;
		std::vector<Node*> visibleChildren;
	};

	static std::shared_ptr<SettingsCategoryModel>
	create( const std::vector<std::pair<std::string, std::vector<std::string>>>& categories,
			const UnorderedMap<std::string, std::string>& ids ) {
		return std::make_shared<SettingsCategoryModel>( categories, ids );
	}

	SettingsCategoryModel(
		const std::vector<std::pair<std::string, std::vector<std::string>>>& categories,
		const UnorderedMap<std::string, std::string>& ids ) {
		mNodes.emplace_back();
		mRoot = &mNodes.back();
		for ( const auto& [parent, children] : categories ) {
			std::string parentId;
			if ( !children.empty() ) {
				auto id = ids.find( parent + '/' + children.front() );
				if ( id != ids.end() ) {
					auto separator = id->second.find( '.' );
					parentId = id->second.substr( 0, separator ) + ".*";
				}
			}
			mNodes.push_back( { std::move( parentId ), parent, mRoot } );
			auto* parentNode = &mNodes.back();
			mRoot->children.push_back( parentNode );
			for ( const auto& child : children ) {
				auto id = ids.find( parent + '/' + child );
				mNodes.push_back(
					{ id == ids.end() ? std::string{} : id->second, child, parentNode } );
				parentNode->children.push_back( &mNodes.back() );
			}
		}
		filter( {}, {} );
	}

	size_t rowCount( const ModelIndex& parent = {} ) const {
		auto* node = parent.isValid() ? static_cast<Node*>( parent.internalData() ) : mRoot;
		return node->visibleChildren.size();
	}

	size_t columnCount( const ModelIndex& = {} ) const { return 1; }

	ModelIndex index( int row, int column, const ModelIndex& parent = {} ) const {
		auto* node = parent.isValid() ? static_cast<Node*>( parent.internalData() ) : mRoot;
		if ( row < 0 || column != 0 || static_cast<size_t>( row ) >= node->visibleChildren.size() )
			return {};
		return createIndex( row, column, node->visibleChildren[row] );
	}

	ModelIndex parentIndex( const ModelIndex& index ) const {
		if ( !index.isValid() )
			return {};
		auto* node = static_cast<Node*>( index.internalData() );
		if ( !node->parent || node->parent == mRoot )
			return {};
		auto* parent = node->parent;
		auto found =
			std::find( mRoot->visibleChildren.begin(), mRoot->visibleChildren.end(), parent );
		return found == mRoot->visibleChildren.end()
				   ? ModelIndex{}
				   : createIndex( std::distance( mRoot->visibleChildren.begin(), found ), 0,
								  parent );
	}

	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const {
		if ( !index.isValid() || role != ModelRole::Display )
			return {};
		return Variant( static_cast<Node*>( index.internalData() )->text );
	}

	void filter( const std::string_view query,
				 const UnorderedSet<std::string>& matchingCategories ) {
		mRoot->visibleChildren.clear();
		for ( auto* parent : mRoot->children ) {
			parent->visibleChildren.clear();
			const bool parentMatches = query.empty() || String::icontains( parent->text, query );
			for ( auto* child : parent->children ) {
				if ( parentMatches || String::icontains( child->text, query ) ||
					 matchingCategories.contains( child->id ) )
					parent->visibleChildren.push_back( child );
			}
			if ( !parent->visibleChildren.empty() )
				mRoot->visibleChildren.push_back( parent );
		}
		invalidate( Model::UpdateFlag::InvalidateAllIndexes );
	}

  private:
	std::deque<Node> mNodes;
	Node* mRoot{ nullptr };
};

static constexpr const char* SETTINGS_PANEL_LAYOUT = R"xml(
<style>
<![CDATA[
.settings_panel #settings_sidebar {
	background-color: var(--list-back);
	padding: 10dp 8dp 8dp 8dp;
}
.settings_panel #settings_filter {
	margin-bottom: 10dp;
}
.settings_panel #settings_categories * {
	focusable: false;
}
.settings_panel #settings_categories {
	background-color: var(--list-back);
}
.settings_panel #settings_categories treeview::row {
	border-left: 0dp solid var(--primary);
	transition: border-left-width 0.1s;
}
.settings_panel #settings_categories treeview::row:selected {
	background-color: var(--tab-hover);
	border-left: 2dp solid var(--primary);
}
.settings_panel #settings_categories treeview::row:selected treeview::cell {
	color: var(--font);
}
.settings_panel #settings_rows {
	max-width: 820dp;
	padding: 20dp 28dp 28dp 28dp;
	layout-gravity: center_horizontal;
}
.settings_panel #settings_page_title {
	font-size: 18dp;
	margin-bottom: 18dp;
	font-weight: bold;
}
.settings_panel .settings_category_heading {
	font-size: 14dp;
	font-weight: bold;
	margin: 12dp 0dp 6dp 0dp;
	padding-bottom: 6dp;
	border-bottom: 1dp solid var(--tab-line);
}
.settings_panel .settings_subcategory_heading {
	font-size: 12dp;
	font-weight: bold;
	margin: 18dp 4dp 3dp 4dp;
	padding-bottom: 6dp;
	border-bottom: 1dp solid var(--tab-line);
}
.settings_panel .settings_option {
	border-bottom: 1dp solid var(--disabled-border);
	padding: 9dp 4dp 11dp 4dp;
	margin-bottom: 0dp;
}
.settings_panel .settings_option:disabled {
	opacity: 0.5;
}
.settings_panel .settings_option_name {
	font-style: normal;
}
.settings_panel .settings_option_description {
	color: var(--disabled-color);
	font-size: 10dp;
	margin-top: 3dp;
	word-wrap: true;
	font-style: normal;
}
@media (prefers-color-scheme: dark) {
	.settings_panel .settings_option_name,
	.settings_panel .settings_option_description {
		font-style: shadow;
	}
	.settings_panel .settings_option_description {
		color: var(--font-hint);
	}
}
.settings_panel .settings_boolean_option #setting_info,
.settings_panel .settings_boolean_option .settings_option_name,
.settings_panel .settings_boolean_option .settings_option_description {
	cursor: pointer;
}
.settings_panel .settings_option_control {
	layout-width: 210dp;
	gravity: right|center_vertical;
	margin-left: 20dp;
	layout-gravity: center_vertical;
}
.settings_panel .settings_bool,
.settings_panel .settings_action {
	layout-width: wrap_content;
	layout-height: wrap_content;
}
.settings_panel .settings_bool {
	check-mode: button;
}
.settings_panel .settings_choice {
	layout-width: 190dp;
	layout-height: wrap_content;
}
.settings_panel .settings_editable_choice {
	layout-width: 210dp;
	layout-height: wrap_content;
}
.settings_panel .settings_text {
	layout-width: 210dp;
	layout-height: wrap_content;
}
.settings_panel .settings_text.error {
	border-color: var(--theme-error);
}
.settings_panel .settings_integer {
	layout-width: 110dp;
	layout-height: wrap_content;
}
]]>
</style>
<vbox lw="mp" lh="mp" class="settings_panel">
	<Splitter id="settings_splitter" lw="mp" lh="mp" orientation="horizontal" splitter-partition="220dp">
		<vbox id="settings_sidebar" lw="0" lh="0" min-width="160dp">
			<TextInput id="settings_filter" lw="mp" lh="wc" hint="@string(search_settings, Search settings...)" />
			<TreeView id="settings_categories" lw="mp" lh="o" lw8="1" />
		</vbox>
		<ScrollView id="settings_scroll" lw="0" lw8="1" lh="mp" focusable="false">
			<vbox id="settings_rows" lw="mp" lh="wc">
				<TextView id="settings_page_title" lw="mp" lh="wc" focusable="false" />
			</vbox>
		</ScrollView>
	</Splitter>
</vbox>
)xml";

static std::string settingsRowLayout( std::string_view control ) {
	return R"xml(
<vbox lw="mp" lh="wc" class="settings_option">
	<hbox lw="mp" lh="wc" class="settings_option_content">
		<vbox id="setting_info" lw="0" lw8="1" lh="wc">
			<TextView id="setting_name" lw="mp" lh="wc" class="settings_option_name" focusable="false" />
			<TextView id="setting_description" lw="mp" lh="wc" class="settings_option_description" focusable="false" />
		</vbox>
		<hbox id="setting_control" lw="wc" lh="wc" class="settings_option_control">
)xml" + std::string( control ) +
		   R"xml(
		</hbox>
	</hbox>
</vbox>
)xml";
}

class SettingsLayoutTemplate {
  public:
	explicit SettingsLayoutTemplate( const std::string& layout ) {
		[[maybe_unused]] auto result =
			mDocument.load_string( layout.c_str(), pugi::parse_default | pugi::parse_ws_pcdata );
		eeASSERT( result );
	}

	pugi::xml_node root() const { return mDocument.first_child(); }

  private:
	pugi::xml_document mDocument;
};

static constexpr const char* SETTINGS_CATEGORY_HEADING_LAYOUT = R"xml(
<vbox lw="mp" lh="wc" visible="false">
	<TextView id="settings_category_heading" lw="mp" lh="wc" class="settings_category_heading" visible="false" focusable="false" />
	<vbox id="settings_category_rows" lw="mp" lh="wc" />
</vbox>
)xml";
static constexpr const char* SETTINGS_SUBCATEGORY_HEADING_LAYOUT = R"xml(
<TextView lw="mp" lh="wc" class="settings_subcategory_heading" visible="false" focusable="false" />
)xml";
static const SettingsLayoutTemplate SETTINGS_BOOL_ROW_LAYOUT( settingsRowLayout(
	R"xml(<CheckBox id="setting_control_widget" class="settings_bool" />)xml" ) );
static const SettingsLayoutTemplate SETTINGS_CHOICE_ROW_LAYOUT( settingsRowLayout(
	R"xml(<DropDownModelList id="setting_control_widget" class="settings_choice" />)xml" ) );
static const SettingsLayoutTemplate SETTINGS_EDITABLE_CHOICE_ROW_LAYOUT( settingsRowLayout(
	R"xml(<ComboBox id="setting_control_widget" class="settings_editable_choice" popup-to-root="true" />)xml" ) );
static const SettingsLayoutTemplate SETTINGS_INTEGER_ROW_LAYOUT( settingsRowLayout(
	R"xml(<SpinBox id="setting_control_widget" class="settings_integer" />)xml" ) );
static const SettingsLayoutTemplate SETTINGS_TEXT_ROW_LAYOUT( settingsRowLayout(
	R"xml(<TextInput id="setting_control_widget" class="settings_text" />)xml" ) );
static const SettingsLayoutTemplate SETTINGS_ACTION_ROW_LAYOUT( settingsRowLayout(
	R"xml(<PushButton id="setting_control_widget" class="settings_action" />)xml" ) );

static void disableTabFocusTree( Node* node ) {
	if ( node->isWidget() )
		node->asType<UIWidget>()->unsetTabFocusable();
	for ( auto* child = node->getFirstChild(); child; child = child->getNextNode() )
		disableTabFocusTree( child );
}

SettingsPanel::SettingsPanel( App* app ) : mApp( app ) {}

SettingsPanel::PanelState& SettingsPanel::state( Scope scope ) {
	return scope == Scope::User ? mUser : mProject;
}

void SettingsPanel::PanelState::reset() {
	bindingGroup.clear();
	connections.clear();
	window = nullptr;
	search = nullptr;
	categories = nullptr;
	scroll = nullptr;
	settings = nullptr;
	pageTitle = nullptr;
	categoryModel.reset();
	categoryItems.clear();
	categoryIds.clear();
	categorySearchText.clear();
	categoryTitles.clear();
	categoryHeadings.clear();
	categorySections.clear();
	categoryContainers.clear();
	materializedCategories.clear();
	subcategoryHeadings.clear();
	model.clear();
	documents.clear();
	settingViews.clear();
	selectedCategory.clear();
	categoryFilter.clear();
}

void SettingsPanel::show( Scope scope, const std::string& category ) {
	auto& panel = state( scope );
	if ( panel.window ) {
		panel.window->show();
		panel.window->toFront();
		selectCategory( panel, category );
		panel.search->runOnMainThread( [search = panel.search] { search->setFocus(); } );
		return;
	}
	Clock c;
	create( scope );
	selectCategory( panel, category );
	Log::info( "Settings Panel %s created in %s", scope == Scope::User ? "User" : "Project",
			   c.getElapsedTime().toString() );
}

void SettingsPanel::selectCategory( PanelState& panel, const std::string& category ) {
	if ( category.empty() || !panel.categories )
		return;
	String title;
	if ( String::endsWith( category, ".*" ) ) {
		const std::string prefix = category.substr( 0, category.size() - 1 );
		for ( const auto& [parent, children] : panel.categoryItems ) {
			if ( !children.empty() ) {
				auto id = panel.categoryIds.find( parent + '/' + children.front() );
				if ( id != panel.categoryIds.end() && String::startsWith( id->second, prefix ) ) {
					title = String::fromUtf8( parent );
					break;
				}
			}
		}
	} else if ( auto found = panel.categoryTitles.find( category );
				found != panel.categoryTitles.end() ) {
		title = found->second;
	}
	if ( title.empty() )
		return;
	panel.selectedCategory = category;
	auto index = panel.categories->findRowWithText(
		title.toUtf8(), true, UIAbstractView::FindRowWithTextMatchKind::Equals );
	if ( index.isValid() )
		panel.categories->setSelection( index );
	filter( panel );
	panel.scroll->getVerticalScrollBar()->setValue( 0, false );
}

void SettingsPanel::create( Scope scope ) {
	auto& panel = state( scope );
	UIWindow::StyleConfig config{ UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL };
	panel.window = UIWindow::NewOpt( UIWindow::SIMPLE_LAYOUT, config );
	panel.window->setId( scope == Scope::User ? "settings_panel" : "project_settings_panel" );
	panel.window->setTitle( scope == Scope::User
								? mApp->i18n( "settings", "Settings" )
								: mApp->i18n( "project_settings", "Project Settings" ) );
	const auto sceneSize = mApp->getUISceneNode()->getPixelsSize();
	panel.window->setPixelsSize( { eeclamp( sceneSize.getWidth() * 0.82f, 720.f, 1200.f ),
								   eeclamp( sceneSize.getHeight() * 0.82f, 520.f, 850.f ) } );
	panel.window->setMinWindowSize( 640, 440 );
	panel.window->setKeyBindingCommand( "closeWindow", [window = panel.window, this] {
		if ( !SceneManager::instance()->isShuttingDown() ) {
			window->closeWindow();
			if ( mApp->getSplitter() && mApp->getSplitter()->getCurWidget() )
				mApp->getSplitter()->getCurWidget()->setFocus();
		}
	} );
	panel.window->getKeyBindings().addKeybind( { KEY_ESCAPE }, "closeWindow" );
	auto* layout = mApp->getUISceneNode()->loadLayoutFromString( SETTINGS_PANEL_LAYOUT,
																 panel.window->getContainer() );
	panel.search = layout->find<UITextInput>( "settings_filter" );
	panel.categories = layout->find<UITreeView>( "settings_categories" );
	panel.settings = layout->find<UILinearLayout>( "settings_rows" );
	panel.pageTitle = layout->find<UITextView>( "settings_page_title" );
	panel.scroll = layout->find<UIScrollView>( "settings_scroll" );
	panel.scroll->setVerticalScrollMode( ScrollBarMode::Auto );
	panel.scroll->setHorizontalScrollMode( ScrollBarMode::AlwaysOff );
	disableTabFocusTree( panel.categories->getVerticalScrollBar() );
	disableTabFocusTree( panel.categories->getHorizontalScrollBar() );
	disableTabFocusTree( panel.scroll->getVerticalScrollBar() );
	disableTabFocusTree( panel.scroll->getHorizontalScrollBar() );
	panel.window->setKeyBindingCommand( "focusSettingsFilter", [&panel] {
		panel.search->setFocus();
		panel.search->getDocument().selectAll();
	} );
	panel.window->setKeyBindingCommand( "focusSettingsCategories", [&panel] {
		auto selected = panel.categories->getSelection().first();
		if ( selected.isValid() )
			panel.categories->setSelection( selected );
		panel.categories->setFocus();
	} );
	panel.window->getKeyBindings().addKeybind( { KEY_F, KeyMod::getDefaultModifier() },
											   "focusSettingsFilter" );
	panel.window->getKeyBindings().addKeybind(
		{ KEY_E, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "focusSettingsCategories" );
	panel.settings->beginAttributesTransaction();
	if ( scope == Scope::User ) {
		addUserSettings( panel );
		addPluginSettings( panel );
	} else {
		addProjectSettings( panel );
	}
	materializeCategory( panel, panel.selectedCategory );
	setupCategories( panel );
	panel.settings->endAttributesTransaction();
	filter( panel );
	panel.connections +=
		panel.search->connect( Event::OnTextChanged, [this, &panel]( const Event* ) {
			String query = panel.search->getText();
			query.trim();
			const UintPtr debounceTag = reinterpret_cast<UintPtr>( &panel );
			if ( query.size() < 2 ) {
				panel.search->removeActionsByTag( debounceTag );
				filter( panel );
				return;
			}
			panel.search->debounce( [this, &panel] { filter( panel ); }, Milliseconds( 150 ),
									debounceTag );
		} );
	panel.connections += panel.window->connect( Event::OnWindowReady, [&panel]( const Event* ) {
		auto title = panel.categoryTitles.find( panel.selectedCategory );
		if ( title != panel.categoryTitles.end() ) {
			auto index = panel.categories->findRowWithText(
				title->second.toUtf8(), true, UIAbstractView::FindRowWithTextMatchKind::Equals );
			if ( index.isValid() )
				panel.categories->setSelection( index );
		}
		panel.search->runOnMainThread( [search = panel.search] { search->setFocus(); } );
	} );
	panel.connections +=
		panel.window->connect( Event::OnWindowClose, [this, &panel, scope]( const Event* ) {
			for ( const auto& document : panel.documents ) {
				if ( !document->save() )
					Log::error( "Could not save settings document: %s", document->path() );
			}
			if ( scope == Scope::User )
				mApp->saveConfig();
			else
				mApp->saveProject();
			panel.reset();
		} );
	panel.window->center();
	panel.window->showWhenReady();
	panel.search->setFocus();
}

void SettingsPanel::addCategory( PanelState& panel, const std::string& id, const String& parent,
								 const String& name ) {
	panel.model.addCategory( { id, parent, name } );
	const auto parentText = parent.toUtf8();
	const auto nameText = name.toUtf8();
	auto parentItems =
		std::find_if( panel.categoryItems.begin(), panel.categoryItems.end(),
					  [&parentText]( const auto& item ) { return item.first == parentText; } );
	if ( parentItems == panel.categoryItems.end() ) {
		panel.categoryItems.emplace_back( parentText, std::vector<std::string>{ nameText } );
	} else {
		parentItems->second.emplace_back( nameText );
	}
	panel.categoryIds[parentText + '/' + nameText] = id;
	panel.categorySearchText[id] = parent + " " + name;
	panel.categoryTitles[id] = name;
	auto* section = mApp->getUISceneNode()->loadLayoutFromString( SETTINGS_CATEGORY_HEADING_LAYOUT,
																  panel.settings );
	auto* heading = section->find<UITextView>( "settings_category_heading" );
	heading->setText( name );
	heading->setId( "settings_category_" + id );
	panel.categoryHeadings[id] = heading;
	panel.categorySections[id] = section;
	panel.categoryContainers[id] = section->find<UILinearLayout>( "settings_category_rows" );
	if ( panel.selectedCategory.empty() )
		panel.selectedCategory = id;
}

void SettingsPanel::addSubcategoryHeading( PanelState& panel, const std::string& category,
										   const String& name ) {
	panel.model.addGroup( { category, name, panel.model.settings().size() } );
}

void SettingsPanel::setupCategories( PanelState& panel ) {
	auto model = SettingsCategoryModel::create( panel.categoryItems, panel.categoryIds );
	panel.categoryModel = model;
	panel.categories->setHeadersVisible( false );
	panel.categories->setAutoExpandOnSingleColumn( true );
	panel.categories->setFocusOnSelection( true );
	panel.categories->setModel( model );
	panel.categories->expandAll();
	panel.connections +=
		panel.categories->connect( Event::OnSelectionChanged, [this, &panel]( const Event* ) {
			auto index = panel.categories->getSelection().first();
			if ( !index.isValid() )
				return;
			auto* node = static_cast<SettingsCategoryModel::Node*>( index.internalData() );
			if ( !node || node->id.empty() )
				return;
			panel.selectedCategory = node->id;
			panel.pageTitle->setText( node->text );
			filter( panel );
			panel.scroll->getVerticalScrollBar()->setValue( 0, false );
		} );
}

UIWidget* SettingsPanel::createRow( PanelState& panel, SettingDefinition& setting,
									SettingView& view, pugi::xml_node layout ) {
	auto& binding = setting.descriptor;
	auto container = panel.categoryContainers.find( binding.category );
	eeASSERT( container != panel.categoryContainers.end() );
	auto* row = mApp->getUISceneNode()->loadLayoutNodes( layout, container->second, 0 );
	row->setId( "setting_" + binding.id );
	row->find<UITextView>( "setting_name" )->setText( binding.name );
	auto* description = row->find<UITextView>( "setting_description" );
	description->setText( binding.description );
	view.row = row;
	return row;
}

void SettingsPanel::addBool( PanelState& panel, SettingDescriptor binding, bool* value,
							 std::function<void( bool )> apply ) {
	panel.model.addSetting(
		{ std::move( binding ), BoolPointerSetting{ value, std::move( apply ) } } );
}

void SettingsPanel::addBool( PanelState& panel, SettingDescriptor binding,
							 std::function<bool()> get, std::function<void( bool )> set ) {
	panel.model.addSetting(
		{ std::move( binding ), BoolSetting{ std::move( get ), std::move( set ) } } );
}

UICheckBox* SettingsPanel::createBoolControl( PanelState& panel, SettingDefinition& setting,
											  SettingView& view ) {
	auto* row = createRow( panel, setting, view, SETTINGS_BOOL_ROW_LAYOUT.root() );
	row->addClass( "settings_boolean_option" );
	auto* check = row->find<UICheckBox>( "setting_control_widget" );
	auto toggle = [check]( const Event* event ) {
		if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
			check->setChecked( !check->isChecked() );
	};
	panel.connections +=
		row->find<UITextView>( "setting_name" )->connect( Event::MouseClick, toggle );
	panel.connections +=
		row->find<UITextView>( "setting_description" )->connect( Event::MouseClick, toggle );
	panel.connections += row->find<UILinearLayout>( "setting_info" )
							 ->connect( Event::MouseClick, std::move( toggle ) );
	return check;
}

void SettingsPanel::addChoice( PanelState& panel, SettingDescriptor binding,
							   const std::vector<String>& choices, std::function<size_t()> get,
							   std::function<void( size_t )> set,
							   std::vector<String> choiceDescriptions ) {
	panel.model.addSetting(
		{ std::move( binding ), ChoiceSetting{ choices, std::move( choiceDescriptions ),
											   std::move( get ), std::move( set ) } } );
}

void SettingsPanel::addEditableChoice( PanelState& panel, SettingDescriptor binding,
									   const std::vector<String>& choices,
									   std::function<String()> get,
									   std::function<bool( const String& )> set ) {
	panel.model.addSetting(
		{ std::move( binding ),
		  EditableChoiceSetting{ choices, std::move( get ), std::move( set ) } } );
}

void SettingsPanel::addInteger( PanelState& panel, SettingDescriptor binding, int min, int max,
								std::function<int()> get, std::function<void( int )> set ) {
	panel.model.addSetting(
		{ std::move( binding ), IntegerSetting{ min, max, std::move( get ), std::move( set ) } } );
}

void SettingsPanel::addText( PanelState& panel, SettingDescriptor binding,
							 std::function<std::string()> get,
							 std::function<bool( const std::string& )> set,
							 bool commitOnFocusLoss ) {
	panel.model.addSetting( { std::move( binding ), TextSetting{ std::move( get ), std::move( set ),
																 commitOnFocusLoss } } );
}

void SettingsPanel::addFloat( PanelState& panel, SettingDescriptor binding, double min, double max,
							  double step, std::function<double()> get,
							  std::function<void( double )> set ) {
	panel.model.addSetting( { std::move( binding ), FloatSetting{ min, max, step, std::move( get ),
																  std::move( set ) } } );
}

void SettingsPanel::addAction( PanelState& panel, SettingDescriptor binding,
							   const String& buttonText, std::function<void()> action ) {
	panel.model.addSetting(
		{ std::move( binding ), ActionSetting{ buttonText, std::move( action ) } } );
}

void SettingsPanel::refreshTextSetting( PanelState& panel, const std::string& id ) {
	const auto& settings = panel.model.settings();
	for ( size_t i = 0; i < settings.size(); ++i ) {
		if ( settings[i].descriptor.id != id || i >= panel.settingViews.size() ||
			 !panel.settingViews[i].row )
			continue;
		auto* value = std::get_if<TextSetting>( &settings[i].value );
		auto* input = panel.settingViews[i].row->find<UITextInput>( "setting_control_widget" );
		if ( value && input )
			input->setText( String::fromUtf8( value->get() ) );
		return;
	}
}

static void setNodeTreeEnabled( Node* node, bool enabled );

void SettingsPanel::materializeCategory( PanelState& panel, const std::string& category ) {
	if ( category.empty() || panel.materializedCategories.contains( category ) )
		return;
	auto container = panel.categoryContainers.find( category );
	if ( container == panel.categoryContainers.end() )
		return;
	auto& settings = panel.model.settings();
	panel.settingViews.resize( settings.size() );
	container->second->beginAttributesTransaction();
	for ( size_t i = 0; i < settings.size(); ++i ) {
		auto& setting = settings[i];
		if ( setting.descriptor.category != category )
			continue;
		for ( const auto& group : panel.model.groups() ) {
			if ( group.category != category || group.beforeSetting != i )
				continue;
			auto* heading =
				mApp->getUISceneNode()
					->loadLayoutFromString( SETTINGS_SUBCATEGORY_HEADING_LAYOUT, container->second )
					->asType<UITextView>();
			heading->setText( group.name );
			panel.subcategoryHeadings.push_back( { group.category, group.name, heading } );
		}
		auto& view = panel.settingViews[i];
		if ( auto* value = std::get_if<BoolPointerSetting>( &setting.value ) ) {
			auto* check = createBoolControl( panel, setting, view );
			auto binding = UIDataBind<bool>::New( value->value, check,
												  UIValueConverter<bool>::converterBool() );
			binding->onValueChangeCb = value->apply;
			panel.bindingGroup += std::move( binding );
		} else if ( auto* value = std::get_if<BoolSetting>( &setting.value ) ) {
			auto* check = createBoolControl( panel, setting, view );
			check->setChecked( value->get() );
			panel.connections +=
				check->connect( Event::OnValueChange, [check, value]( const Event* ) {
					value->set( check->isChecked() );
				} );
		} else if ( auto* value = std::get_if<ChoiceSetting>( &setting.value ) ) {
			auto* row = createRow( panel, setting, view, SETTINGS_CHOICE_ROW_LAYOUT.root() );
			auto* dropDown = row->find<UIDropDownModelList>( "setting_control_widget" );
			auto model = ItemListOwnerModel<String>::create( value->choices );
			dropDown->setModel( model );
			const size_t selected = value->get();
			if ( selected < value->choices.size() ) {
				dropDown->getListView()->getSelection().set( model->index( selected, 0 ) );
				dropDown->setText( value->choices[selected] );
			}
			if ( selected < value->descriptions.size() )
				dropDown->setTooltipText( value->descriptions[selected] );
			panel.connections +=
				dropDown->connect( Event::OnValueChange, [dropDown, value]( const Event* ) {
					if ( dropDown->getListView()->getSelection().isEmpty() )
						return;
					const size_t selected = dropDown->getListView()->getSelection().first().row();
					if ( selected < value->descriptions.size() )
						dropDown->setTooltipText( value->descriptions[selected] );
					value->set( selected );
				} );
		} else if ( auto* value = std::get_if<EditableChoiceSetting>( &setting.value ) ) {
			auto* row =
				createRow( panel, setting, view, SETTINGS_EDITABLE_CHOICE_ROW_LAYOUT.root() );
			auto* combo = row->find<UIComboBox>( "setting_control_widget" );
			for ( const auto& choice : value->choices )
				combo->getListBox()->addListBoxItem( choice );
			combo->setText( value->get() );
			panel.connections +=
				combo->connect( Event::OnValueChange, [combo, value]( const Event* ) {
					if ( !value->set( combo->getText() ) ) {
						combo->addClass( "error" );
						combo->getDropDownList()->addClass( "error" );
						return;
					}
					combo->removeClass( "error" );
					combo->getDropDownList()->removeClass( "error" );
				} );
		} else if ( auto* value = std::get_if<IntegerSetting>( &setting.value ) ) {
			auto* row = createRow( panel, setting, view, SETTINGS_INTEGER_ROW_LAYOUT.root() );
			auto* spin = row->find<UISpinBox>( "setting_control_widget" );
			spin->setMinValue( value->min )->setMaxValue( value->max );
			spin->unsetTabFocusable();
			spin->getButtonPushUp()->asType<UIWidget>()->unsetTabFocusable();
			spin->getButtonPushDown()->asType<UIWidget>()->unsetTabFocusable();
			spin->setValue( value->get() );
			panel.connections +=
				spin->connect( Event::OnValueChange, [spin, value]( const Event* ) {
					value->set( static_cast<int>( spin->getValue() ) );
				} );
		} else if ( auto* value = std::get_if<TextSetting>( &setting.value ) ) {
			auto* row = createRow( panel, setting, view, SETTINGS_TEXT_ROW_LAYOUT.root() );
			auto* input = row->find<UITextInput>( "setting_control_widget" );
			if ( value->password )
				input->setMode( UITextInput::TextInputMode::Password );
			input->setText( String::fromUtf8( value->get() ) );
			auto commit = [input, value] {
				if ( !value->set( input->getText().toUtf8() ) ) {
					input->addClass( "error" );
					return;
				}
				input->removeClass( "error" );
			};
			if ( value->commitOnFocusLoss ) {
				const auto debounceTag = reinterpret_cast<Action::UniqueID>( input );
				panel.connections += input->connect(
					Event::OnTextChanged, [input, commit, debounceTag]( const Event* ) {
						input->debounce( commit, Milliseconds( 500 ), debounceTag );
					} );
				auto flush = [input, commit, debounceTag]( const Event* ) {
					input->removeActionsByTag( debounceTag );
					commit();
				};
				panel.connections += input->connect( Event::OnPressEnter, flush );
				panel.connections += input->connect( Event::OnFocusLoss, std::move( flush ) );
			} else {
				panel.connections +=
					input->connect( Event::OnTextChanged,
									[commit = std::move( commit )]( const Event* ) { commit(); } );
			}
		} else if ( auto* value = std::get_if<FloatSetting>( &setting.value ) ) {
			auto* row = createRow( panel, setting, view, SETTINGS_INTEGER_ROW_LAYOUT.root() );
			auto* spin = row->find<UISpinBox>( "setting_control_widget" );
			spin->setMinValue( value->min )->setMaxValue( value->max )->setClickStep( value->step );
			spin->allowFloatingPoint( true )->setValue( value->get() );
			spin->unsetTabFocusable();
			spin->getButtonPushUp()->asType<UIWidget>()->unsetTabFocusable();
			spin->getButtonPushDown()->asType<UIWidget>()->unsetTabFocusable();
			panel.connections +=
				spin->connect( Event::OnValueChange,
							   [spin, value]( const Event* ) { value->set( spin->getValue() ); } );
		} else if ( auto* value = std::get_if<ActionSetting>( &setting.value ) ) {
			auto* row = createRow( panel, setting, view, SETTINGS_ACTION_ROW_LAYOUT.root() );
			auto* button = row->find<UIPushButton>( "setting_control_widget" );
			button->setText( value->buttonText );
			panel.connections += button->connect( Event::MouseClick, [value]( const Event* event ) {
				if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
					value->action();
			} );
		}
		if ( view.row && !setting.enabled )
			setNodeTreeEnabled( view.row, false );
	}
	container->second->endAttributesTransaction();
	panel.materializedCategories.insert( category );
}

void SettingsPanel::materializeVisibleSettings( PanelState& panel, const String& query ) {
	const std::string queryUtf8 = query.toUtf8();
	const bool aggregate = String::endsWith( panel.selectedCategory, ".*" );
	const std::string aggregatePrefix =
		aggregate ? panel.selectedCategory.substr( 0, panel.selectedCategory.size() - 1 )
				  : std::string{};
	for ( const auto& category : panel.model.categories() ) {
		bool materialize =
			query.empty() ? category.id == panel.selectedCategory ||
								( aggregate && String::startsWith( category.id, aggregatePrefix ) )
						  : String::icontains( category.parent, query ) ||
								String::icontains( category.name, query );
		if ( !materialize && !query.empty() ) {
			for ( const auto& setting : panel.model.settings() ) {
				const auto& descriptor = setting.descriptor;
				if ( descriptor.category == category.id &&
					 ( String::icontains( descriptor.name, query ) ||
					   String::icontains( descriptor.description, query ) ||
					   String::icontains( descriptor.group, query ) ||
					   String::icontains( descriptor.id, queryUtf8 ) ) ) {
					materialize = true;
					break;
				}
			}
		}
		if ( materialize )
			materializeCategory( panel, category.id );
	}
}

static void setNodeTreeEnabled( Node* node, bool enabled ) {
	node->setEnabled( enabled );
	for ( Node* child = node->getFirstChild(); child; child = child->getNextNode() )
		setNodeTreeEnabled( child, enabled );
}

void SettingsPanel::setCategoryEnabled( PanelState& panel, const std::string& category,
										bool enabled, const std::string& excludedSetting ) {
	auto& settings = panel.model.settings();
	for ( size_t i = 0; i < settings.size(); ++i ) {
		auto& setting = settings[i];
		const auto& descriptor = setting.descriptor;
		if ( descriptor.category != category || descriptor.id == excludedSetting )
			continue;
		setting.enabled = enabled;
		if ( i < panel.settingViews.size() && panel.settingViews[i].row )
			setNodeTreeEnabled( panel.settingViews[i].row, enabled );
	}
}

void SettingsPanel::addUserSettings( PanelState& panel ) {
	addCategory( panel, "general.behavior", mApp->i18n( "general", "General" ),
				 mApp->i18n( "behavior", "Behavior" ) );
	addBool( panel,
			 { "welcomeScreen", "general.behavior",
			   mApp->i18n( "welcome_screen_enable", "Enable Welcome Screen" ),
			   mApp->i18n( "welcome_screen_enable_desc",
						   "Show the welcome screen when no document or folder is open." ) },
			 &mApp->getConfig().ui.welcomeScreen );
	addBool(
		panel,
		{ "openFilesInNewWindow", "general.behavior",
		  mApp->i18n( "open_files_in_new_window_enable", "Open Files in New Window" ),
		  mApp->i18n(
			  "open_files_in_new_window_desc",
			  "Open files received from the file explorer or command line in a new window." ) },
		&mApp->getConfig().ui.openFilesInNewWindow );
	addBool(
		panel,
		{ "openProjectInNewWindow", "general.behavior",
		  mApp->i18n( "open_project_in_new_window", "Open Project in New Window" ),
		  mApp->i18n( "open_project_in_new_window_tooltip",
					  "Open folders in a new window when another project is already loaded." ) },
		&mApp->getConfig().ui.openProjectInNewWindow );
	addBool( panel,
			 { "nativeFileDialogs", "general.behavior",
			   mApp->i18n( "use_native_file_dialogs", "Enable Native File Dialogs" ),
			   mApp->i18n( "use_native_file_dialogs_tooltip",
						   "Use operating-system file dialogs when available." ) },
			 &mApp->getConfig().ui.nativeFileDialogs );
	addBool( panel,
			 { "imagesQuickPreview", "general.behavior",
			   mApp->i18n( "quick_preview_images", "Quick Preview Images" ),
			   mApp->i18n( "quick_preview_images_tooltip",
						   "Preview images without opening a permanent editor tab." ) },
			 &mApp->getConfig().ui.imagesQuickPreview );

	addCategory( panel, "editor.appearance", mApp->i18n( "editor", "Editor" ),
				 mApp->i18n( "appearance", "Appearance" ) );
	std::vector<String> editorSchemeNames;
	std::vector<std::string> editorSchemeIds;
	for ( const auto& [name, scheme] : mApp->getSplitter()->getColorSchemes() ) {
		editorSchemeNames.emplace_back( name );
		editorSchemeIds.emplace_back( name );
	}
	auto selectedEditorScheme =
		std::find( editorSchemeNames.begin(), editorSchemeNames.end(),
				   String( mApp->getSplitter()->getCurrentColorSchemeName() ) );
	const size_t selectedEditorSchemeIndex =
		selectedEditorScheme == editorSchemeNames.end()
			? 0
			: static_cast<size_t>( selectedEditorScheme - editorSchemeNames.begin() );
	if ( !editorSchemeNames.empty() )
		addChoice(
			panel,
			{ "editorColorScheme", "editor.appearance",
			  mApp->i18n( "syntax_color_scheme", "Syntax Color Scheme" ),
			  mApp->i18n( "syntax_color_scheme_desc",
						  "Choose the colors used for syntax highlighting and editor surfaces." ) },
			editorSchemeNames, [selectedEditorSchemeIndex] { return selectedEditorSchemeIndex; },
			[this, editorSchemeIds = std::move( editorSchemeIds )]( size_t selected ) {
				mApp->getSplitter()->setColorScheme(
					editorSchemeIds[std::min( selected, editorSchemeIds.size() - 1 )] );
			} );
	auto addEditorBool = [this, &panel]( std::string id, const char* nameKey, const char* name,
										 const char* descriptionKey, const char* description,
										 bool CodeEditorConfig::* member, auto apply ) {
		addBool( panel,
				 { std::move( id ), "editor.appearance", mApp->i18n( nameKey, name ),
				   mApp->i18n( descriptionKey, description ) },
				 &( mApp->getConfig().editor.*member ), [this, apply]( bool value ) {
					 mApp->getSplitter()->forEachEditor(
						 [value, apply]( UICodeEditor* editor ) { apply( editor, value ); } );
				 } );
	};
	addEditorBool( "showLineNumbers", "show_line_numbers", "Show Line Numbers",
				   "show_line_numbers_desc", "Display line numbers beside the editor.",
				   &CodeEditorConfig::showLineNumbers,
				   []( UICodeEditor* editor, bool value ) { editor->setShowLineNumber( value ); } );
	addEditorBool(
		"showWhiteSpaces", "show_white_spaces", "Show White Spaces", "show_white_spaces_desc",
		"Display visible markers for whitespace characters.", &CodeEditorConfig::showWhiteSpaces,
		[]( UICodeEditor* editor, bool value ) { editor->setShowWhitespaces( value ); } );
	addEditorBool(
		"highlightCurrentLine", "highlight_current_line", "Highlight Current Line",
		"highlight_current_line_desc", "Highlight the line containing the primary cursor.",
		&CodeEditorConfig::highlightCurrentLine,
		[]( UICodeEditor* editor, bool value ) { editor->setHighlightCurrentLine( value ); } );
	addBool( panel,
			 { "showDocumentInfo", "editor.appearance",
			   mApp->i18n( "show_doc_info", "Show Document Info" ),
			   mApp->i18n( "show_doc_info_desc",
						   "Display document encoding and line-ending information." ) },
			 &mApp->getConfig().editor.showDocInfo, [this]( bool value ) {
				 if ( mApp->getDocInfo() )
					 mApp->getDocInfo()->setVisible( value );
			 } );
	addEditorBool(
		"showLineEndings", "show_line_endings", "Show Line Endings", "show_line_endings_desc",
		"Display markers for line-ending characters.", &CodeEditorConfig::showLineEndings,
		[]( UICodeEditor* editor, bool value ) { editor->setShowLineEndings( value ); } );
	addEditorBool(
		"showIndentationGuides", "show_indentation_guides", "Show Indentation Guides",
		"show_indentation_guides_desc", "Display vertical guides for indentation levels.",
		&CodeEditorConfig::showIndentationGuides,
		[]( UICodeEditor* editor, bool value ) { editor->setShowIndentationGuides( value ); } );
	addEditorBool( "minimap", "show_minimap", "Show Minimap", "show_minimap_desc",
				   "Display a compact overview of the document.", &CodeEditorConfig::minimap,
				   []( UICodeEditor* editor, bool value ) { editor->showMinimap( value ); } );
	addEditorBool(
		"relativeLinePositions", "show_lines_relative_position", "Show Lines Relative Position",
		"show_lines_relative_position_desc", "Show line numbers relative to the primary cursor.",
		&CodeEditorConfig::linesRelativePosition,
		[]( UICodeEditor* editor, bool value ) { editor->showLinesRelativePosition( value ); } );
	addEditorBool(
		"highlightMatchingBracket", "highlight_matching_brackets", "Highlight Matching Bracket",
		"highlight_matching_brackets_desc",
		"Highlight the bracket paired with the one beside the cursor.",
		&CodeEditorConfig::highlightMatchingBracket,
		[]( UICodeEditor* editor, bool value ) { editor->setHighlightMatchingBracket( value ); } );
	addEditorBool(
		"highlightSelectionMatch", "highlight_selection_match", "Highlight Selection Match",
		"highlight_selection_match_desc", "Highlight text matching the current selection.",
		&CodeEditorConfig::highlightSelectionMatch,
		[]( UICodeEditor* editor, bool value ) { editor->setHighlightSelectionMatch( value ); } );
	addEditorBool( "verticalScrollbar", "enable_vertical_scrollbar", "Enable Vertical Scrollbar",
				   "enable_vertical_scrollbar_desc", "Display the editor's vertical scrollbar.",
				   &CodeEditorConfig::verticalScrollbar, []( UICodeEditor* editor, bool value ) {
					   editor->setVerticalScrollBarEnabled( value );
				   } );
	addEditorBool( "horizontalScrollbar", "enable_horizontal_scrollbar",
				   "Enable Horizontal Scrollbar", "enable_horizontal_scrollbar_desc",
				   "Display the editor's horizontal scrollbar.",
				   &CodeEditorConfig::horizontalScrollbar, []( UICodeEditor* editor, bool value ) {
					   editor->setHorizontalScrollBarEnabled( value );
				   } );
	addEditorBool( "inlineColorBoxes", "enable_inline_color_boxes", "Enable Color Boxes",
				   "enable_inline_color_boxes_tooltip",
				   "Display inline color boxes beside recognized color values.",
				   &CodeEditorConfig::inlineColorBoxes, []( UICodeEditor* editor, bool value ) {
					   editor->setEnableInlineColorBoxes( value );
				   } );
	addEditorBool( "colorPreview", "enable_color_preview", "Enable Color Preview",
				   "enable_color_preview_tooltip", "Preview selected color values in the editor.",
				   &CodeEditorConfig::colorPreview,
				   []( UICodeEditor* editor, bool value ) { editor->setColorPreview( value ); } );
	addEditorBool( "colorPickerSelection", "enable_color_picker", "Enable Color Picker",
				   "enable_color_picker_tooltip",
				   "Open the color picker for recognized color selections.",
				   &CodeEditorConfig::colorPickerSelection, []( UICodeEditor* editor, bool value ) {
					   editor->setEnableColorPickerOnSelection( value );
				   } );

	addCategory( panel, "editor.navigation", mApp->i18n( "editor", "Editor" ),
				 mApp->i18n( "navigation", "Navigation" ) );
	addBool( panel,
			 { "singleClickNavigation", "editor.navigation",
			   mApp->i18n( "treeview_single_click_nav", "Single Click Navigation" ),
			   mApp->i18n( "treeview_single_click_nav_tooltip",
						   "Open project-tree files with a single click." ) },
			 &mApp->getConfig().editor.singleClickNavigation, [this]( bool value ) {
				 if ( mApp->getProjectTreeView() )
					 mApp->getProjectTreeView()->setSingleClickNavigation( value );
			 } );
	addBool( panel,
			 { "syncProjectTree", "editor.navigation",
			   mApp->i18n( "sync_project_tree", "Synchronize Project Tree with Editor" ),
			   mApp->i18n( "sync_project_tree_tooltip",
						   "Select the active document in the project tree." ) },
			 &mApp->getConfig().editor.syncProjectTreeWithEditor );
	addBool(
		panel,
		{ "restoreSelectionOnFocus", "editor.navigation",
		  mApp->i18n( "restore_editor_selection_on_focus", "Restore Editor Selection on Focus" ),
		  mApp->i18n( "restore_editor_selection_on_focus_tooltip",
					  "Restore each editor split's last cursor and selection when focused." ) },
		&mApp->getConfig().editor.restoreEditorSelectionOnFocus,
		[this]( bool value ) { mApp->getSplitter()->setRestoreEditorSelectionOnFocus( value ); } );

	addCategory( panel, "editor.formatting", mApp->i18n( "editor", "Editor" ),
				 mApp->i18n( "formatting", "Formatting" ) );
	addChoice(
		panel,
		{ "lineWrapMode", "editor.formatting", mApp->i18n( "line_wrap", "Line Wrap" ),
		  mApp->i18n( "line_wrap_desc", "Controls how long lines wrap in the editor." ) },
		{ mApp->i18n( "no_wrap", "No Wrap" ), mApp->i18n( "wrap_word", "Word Wrap" ),
		  mApp->i18n( "wrap_letter", "Letter Wrap" ) },
		[this] {
			switch ( mApp->getConfig().editor.wrapMode ) {
				case LineWrapMode::NoWrap:
					return size_t{ 0 };
				case LineWrapMode::Letter:
					return size_t{ 2 };
				case LineWrapMode::Word:
				default:
					return size_t{ 1 };
			}
		},
		[this]( size_t selected ) {
			auto mode = selected == 0	? LineWrapMode::NoWrap
						: selected == 2 ? LineWrapMode::Letter
										: LineWrapMode::Word;
			mApp->getConfig().editor.wrapMode = mode;
			mApp->getSplitter()->forEachEditor(
				[mode]( UICodeEditor* editor ) { editor->setLineWrapMode( mode ); } );
		} );
	addInteger(
		panel,
		{ "lineBreakingColumn", "editor.formatting",
		  mApp->i18n( "line_breaking_column", "Line Breaking Column" ),
		  mApp->i18n( "line_breaking_column_desc",
					  "Column used for wrapping and the editor width guide. Set 0 to disable." ) },
		0, 1000, [this] { return mApp->getConfig().doc.lineBreakingColumn; },
		[this]( int value ) {
			mApp->getConfig().doc.lineBreakingColumn = value;
			mApp->getSplitter()->forEachEditor(
				[value]( UICodeEditor* editor ) { editor->setLineBreakingColumn( value ); } );
		} );
	addChoice(
		panel,
		{ "lineWrapType", "editor.formatting", mApp->i18n( "wrap_type", "Wrap Against" ),
		  mApp->i18n( "wrap_type_desc",
					  "Choose whether lines wrap at the viewport or line breaking column." ) },
		{ mApp->i18n( "viewport", "Viewport" ),
		  mApp->i18n( "line_breaking_column", "Line Breaking Column" ) },
		[this] { return mApp->getConfig().editor.wrapType == LineWrapType::Viewport ? 0 : 1; },
		[this]( size_t selected ) {
			auto value = selected == 0 ? LineWrapType::Viewport : LineWrapType::LineBreakingColumn;
			mApp->getConfig().editor.wrapType = value;
			mApp->getSplitter()->forEachEditor(
				[value]( UICodeEditor* editor ) { editor->setLineWrapType( value ); } );
		} );
	addBool( panel,
			 { "wrapKeepIndentation", "editor.formatting",
			   mApp->i18n( "keep_indentation", "Keep Indentation" ),
			   mApp->i18n( "keep_indentation_desc",
						   "Preserve indentation on wrapped continuation lines." ) },
			 &mApp->getConfig().editor.wrapKeepIndentation, [this]( bool value ) {
				 mApp->getSplitter()->forEachEditor( [value]( UICodeEditor* editor ) {
					 editor->setLineWrapKeepIndentation( value );
				 } );
			 } );

	addCategory( panel, "editor.folding", mApp->i18n( "editor", "Editor" ),
				 mApp->i18n( "code_folding", "Code Folding" ) );
	addBool( panel,
			 { "codeFoldingEnabled", "editor.folding", mApp->i18n( "enabled", "Enabled" ),
			   mApp->i18n( "code_folding_enabled_desc", "Enable syntax-aware code folding." ) },
			 &mApp->getConfig().editor.codeFoldingEnabled, [this]( bool value ) {
				 mApp->getSplitter()->forEachDoc( [value]( TextDocument& doc ) {
					 doc.getFoldRangeService().setEnabled( value );
				 } );
			 } );
	addBool( panel,
			 { "codeFoldingAlwaysVisible", "editor.folding",
			   mApp->i18n( "code_folding_always_display_folds", "Folds Always Visible" ),
			   mApp->i18n( "code_folding_always_visible_desc",
						   "Always display fold controls in the editor gutter." ) },
			 &mApp->getConfig().editor.codeFoldingAlwaysVisible, [this]( bool value ) {
				 mApp->getSplitter()->forEachEditor(
					 [value]( UICodeEditor* editor ) { editor->setFoldsAlwaysVisible( value ); } );
			 } );

	addCategory( panel, "editor.tabs", mApp->i18n( "editor", "Editor" ),
				 mApp->i18n( "tab_bar", "Tab Bar" ) );
	addBool( panel,
			 { "hideTabBar", "editor.tabs", mApp->i18n( "hide_tabbar", "Hide Tab Bar" ),
			   mApp->i18n( "hide_tabbar_tooltip", "Always hide the tab bar." ) },
			 &mApp->getConfig().editor.hideTabBar,
			 [this]( bool value ) { mApp->getSplitter()->setHideTabBar( value ); } );
	addBool( panel,
			 { "hideTabBarOnSingleTab", "editor.tabs",
			   mApp->i18n( "hide_tabbar_on_single_tab", "Hide Tab Bar on Single Tab" ),
			   mApp->i18n( "hide_tabbar_on_single_tab_tooltip",
						   "Hide the tab bar when a tab widget contains only one item." ) },
			 &mApp->getConfig().editor.hideTabBarOnSingleTab,
			 [this]( bool value ) { mApp->getSplitter()->setHideTabBarOnSingleTab( value ); } );
	addBool( panel,
			 { "tabSwitcher", "editor.tabs",
			   mApp->i18n( "display_tab_switcher", "Display Tab Switcher" ),
			   mApp->i18n( "display_tab_switcher_tooltip",
						   "Display a tab switcher in the center of each tab widget." ) },
			 &mApp->getConfig().editor.tabSwitcher );
	addBool( panel,
			 { "openDocumentsInMainSplit", "editor.tabs",
			   mApp->i18n( "open_documents_in_main_split", "Open Documents in Main Split" ),
			   mApp->i18n( "open_documents_in_main_split_desc",
						   "Open externally requested documents in the main editor split." ) },
			 &mApp->getConfig().editor.openDocumentsInMainSplit );
	addChoice(
		panel,
		{ "tabJumpMode", "editor.tabs", mApp->i18n( "tab_jump_mode", "Tab Jump Mode" ),
		  mApp->i18n( "tab_jump_mode_desc",
					  "Choose how keyboard navigation cycles through tabs." ) },
		{ mApp->i18n( "linear", "Linear" ), mApp->i18n( "chronological", "Chronological" ) },
		[this] {
			return mApp->getConfig().editor.tabJumpMode == UITabWidget::TabJumpMode::Linear ? 0 : 1;
		},
		[this]( size_t selected ) {
			mApp->getConfig().editor.tabJumpMode = selected == 0
													   ? UITabWidget::TabJumpMode::Linear
													   : UITabWidget::TabJumpMode::Chronological;
		},
		{ mApp->i18n( "jump_mode_linear_tooltip",
					  "Linear Jump Mode will switch tabs in the order they are displayed" ),
		  mApp->i18n(
			  "jump_mode_chronological_tooltip",
			  "Chronological Jump Mode will switch tabs in the last focused / visited order." ) } );
	addChoice(
		panel,
		{ "newTabPosition", "editor.tabs", mApp->i18n( "new_tab_position", "New Tab Position" ),
		  mApp->i18n( "new_tab_position_desc", "Choose where newly opened tabs are inserted." ) },
		{ mApp->i18n( "new_tab_position_after_active", "After Active Tab" ),
		  mApp->i18n( "new_tab_position_last", "Last" ),
		  mApp->i18n( "new_tab_position_first", "First" ),
		  mApp->i18n( "new_tab_position_left_of_active", "Left of Active Tab" ) },
		[this] {
			switch ( mApp->getConfig().editor.newTabPosition ) {
				case NewTabPosition::AfterActive:
					return size_t{ 0 };
				case NewTabPosition::First:
					return size_t{ 2 };
				case NewTabPosition::LeftOfActive:
					return size_t{ 3 };
				default:
					return size_t{ 1 };
			}
		},
		[this]( size_t selected ) {
			static constexpr NewTabPosition::Position positions[] = {
				NewTabPosition::AfterActive, NewTabPosition::Last, NewTabPosition::First,
				NewTabPosition::LeftOfActive };
			mApp->getConfig().editor.newTabPosition = positions[std::min( selected, size_t{ 3 } )];
		} );

	addCategory( panel, "editor.document", mApp->i18n( "editor", "Editor" ),
				 mApp->i18n( "document_defaults", "Document Defaults" ) );
	auto addDocumentBool = [this, &panel]( std::string id, const char* nameKey, const char* name,
										   const char* descriptionKey, const char* description,
										   bool DocumentConfig::* member,
										   std::function<void( bool )> apply = {} ) {
		addBool( panel,
				 { std::move( id ), "editor.document", mApp->i18n( nameKey, name ),
				   mApp->i18n( descriptionKey, description ) },
				 &( mApp->getConfig().doc.*member ), std::move( apply ) );
	};
	addDocumentBool( "autoDetectIndentType", "auto_detect_indent_type_and_width",
					 "Auto Detect Indent Type & Width", "auto_detect_indent_type_desc",
					 "Detect indentation settings when opening each document.",
					 &DocumentConfig::autoDetectIndentType );
	addChoice(
		panel,
		{ "autoIndent", "editor.document", mApp->i18n( "auto_indent", "Auto-Indent" ),
		  mApp->i18n( "auto_indent_tooltip", "Control indentation added after pressing Enter." ) },
		{ mApp->i18n( "auto_indent_none", "None" ),
		  mApp->i18n( "auto_indent_preserve", "Preserve" ),
		  mApp->i18n( "auto_indent_smart", "Smart" ) },
		[this] { return static_cast<size_t>( mApp->getConfig().doc.autoIndent ); },
		[this]( size_t selected ) {
			auto value =
				static_cast<TextDocument::AutoIndentConfig>( std::min( selected, size_t{ 2 } ) );
			mApp->getConfig().doc.autoIndent = value;
			mApp->getSplitter()->forEachEditor(
				[value]( UICodeEditor* editor ) { editor->getDocument().setAutoIndent( value ); } );
		},
		{ mApp->i18n( "auto_indent_none_desc", "No automatic indentation." ),
		  mApp->i18n( "auto_indent_preserve_desc",
					  "Preserve the indentation level of the previous line." ),
		  mApp->i18n( "auto_indent_smart_desc",
					  "Preserve indentation and indent between auto-closed brackets." ) } );
	addDocumentBool( "indentSpaces", "indent_spaces", "Indent Using Spaces", "indent_spaces_desc",
					 "Insert spaces instead of tab characters for indentation.",
					 &DocumentConfig::indentSpaces );
	addDocumentBool( "trimTrailingWhitespaces", "trim_trailing_whitespaces",
					 "Trim Trailing Whitespaces", "trim_trailing_whitespaces_desc",
					 "Remove trailing whitespace when saving files.",
					 &DocumentConfig::trimTrailingWhitespaces );
	addDocumentBool( "forceNewLineAtEndOfFile", "force_new_line_at_end_of_file",
					 "Force New Line at End of File", "force_new_line_at_end_of_file_desc",
					 "Ensure saved files end with a newline.",
					 &DocumentConfig::forceNewLineAtEndOfFile );
	addDocumentBool( "writeUnicodeBOM", "write_unicode_bom", "Write Unicode BOM",
					 "write_unicode_bom_desc", "Write a Unicode byte-order mark when saving.",
					 &DocumentConfig::writeUnicodeBOM );
	addDocumentBool( "tabStops", "tab_stops", "Tab Stops", "tab_stops_desc",
					 "Align tabs to consistent positions on the tab grid.",
					 &DocumentConfig::tabStops, [this]( bool value ) {
						 mApp->getSplitter()->forEachEditor(
							 [value]( UICodeEditor* editor ) { editor->setTabStops( value ); } );
					 } );
	addDocumentBool( "tabOut", "tab_out", "Tab Out", "tab_out_desc",
					 "Press Tab before a closing character to move past it.",
					 &DocumentConfig::tabOutEnabled, [this]( bool value ) {
						 mApp->getSplitter()->forEachEditor( [value]( UICodeEditor* editor ) {
							 editor->getDocument().setTabOutEnabled( value );
						 } );
					 } );
	addInteger(
		panel,
		{ "indentWidth", "editor.document", mApp->i18n( "indent_width", "Indent Width" ),
		  mApp->i18n( "indent_width_desc", "Columns in one indentation level." ) },
		1, 16, [this] { return mApp->getConfig().doc.indentWidth; },
		[this]( int value ) { mApp->getConfig().doc.indentWidth = value; } );
	addInteger(
		panel,
		{ "tabWidth", "editor.document", mApp->i18n( "tab_width", "Tab Width" ),
		  mApp->i18n( "tab_width_desc", "Columns used to display a tab character." ) },
		1, 16, [this] { return mApp->getConfig().doc.tabWidth; },
		[this]( int value ) { mApp->getConfig().doc.tabWidth = value; } );
	addChoice(
		panel,
		{ "lineEndings", "editor.document", mApp->i18n( "line_endings", "Line Endings" ),
		  mApp->i18n( "line_endings_desc", "Default line-ending sequence for new files." ) },
		{ "Windows (CR/LF)", "Unix (LF)", "Macintosh (CR)" },
		[this] {
			switch ( mApp->getConfig().doc.lineEndings ) {
				case TextFormat::LineEnding::CRLF:
					return size_t{ 0 };
				case TextFormat::LineEnding::CR:
					return size_t{ 2 };
				default:
					return size_t{ 1 };
			}
		},
		[this]( size_t selected ) {
			mApp->getConfig().doc.lineEndings =
				selected == 0
					? TextFormat::LineEnding::CRLF
					: ( selected == 2 ? TextFormat::LineEnding::CR : TextFormat::LineEnding::LF );
		} );
	const String autoCloseGroup =
		mApp->i18n( "auto_close_brackets_and_tags", "Auto-Close Brackets & Tags" );
	addSubcategoryHeading( panel, "editor.document", autoCloseGroup );
	addBool( panel,
			 { "autoCloseXMLTags", "editor.document",
			   mApp->i18n( "auto_close_xml_tags", "Auto Close XML Tags" ),
			   mApp->i18n( "auto_close_xml_tags_desc", "Automatically insert closing XML tags." ),
			   autoCloseGroup },
			 &mApp->getConfig().editor.autoCloseXMLTags, [this]( bool value ) {
				 mApp->getSplitter()->forEachEditor(
					 [value]( UICodeEditor* editor ) { editor->setAutoCloseXMLTags( value ); } );
			 } );
	auto addAutoClosePair = [this, &panel, &autoCloseGroup]( std::string id, const char* nameKey,
															 const char* name, std::string pair ) {
		auto containsPair = [this, pair] {
			auto pairs = String::split( mApp->getConfig().editor.autoCloseBrackets, ',' );
			return std::find( pairs.begin(), pairs.end(), pair ) != pairs.end();
		};
		addBool( panel,
				 { std::move( id ), "editor.document", mApp->i18n( nameKey, name ),
				   mApp->i18n( "auto_close_pair_desc",
							   "Automatically insert the matching closing character." ),
				   autoCloseGroup },
				 std::move( containsPair ), [this, pair = std::move( pair )]( bool enabled ) {
					 auto pairs = String::split( mApp->getConfig().editor.autoCloseBrackets, ',' );
					 auto found = std::find( pairs.begin(), pairs.end(), pair );
					 if ( enabled && found == pairs.end() )
						 pairs.emplace_back( pair );
					 else if ( !enabled && found != pairs.end() )
						 pairs.erase( found );
					 mApp->getConfig().editor.autoCloseBrackets = String::join( pairs, ',' );
					 auto closePairs =
						 mApp->makeAutoClosePairs( mApp->getConfig().editor.autoCloseBrackets );
					 mApp->getSplitter()->forEachEditor( [closePairs]( UICodeEditor* editor ) {
						 editor->getDocument().setAutoCloseBrackets( !closePairs.empty() );
						 editor->getDocument().setAutoCloseBracketsPairs( closePairs );
					 } );
				 } );
	};
	addAutoClosePair( "autoCloseBrackets", "brackets", "Brackets ()", "()" );
	addAutoClosePair( "autoCloseCurlyBrackets", "curly_brackets", "Curly Brackets {}", "{}" );
	addAutoClosePair( "autoCloseSquareBrackets", "square_brakcets", "Square Brackets []", "[]" );
	addAutoClosePair( "autoCloseSingleQuotes", "single_quotes", "Single Quotes ''", "''" );
	addAutoClosePair( "autoCloseDoubleQuotes", "double_quotes", "Double Quotes \"\"", "\"\"" );
	addAutoClosePair( "autoCloseBackQuotes", "back_quotes", "Back Quotes ``", "``" );
	addBool( panel,
			 { "autoReloadOnDiskChange", "editor.document",
			   mApp->i18n( "autoreload_on_disk_change", "Auto-Reload on Disk Change" ),
			   mApp->i18n( "autoreload_on_disk_change_desc",
						   "Reload unmodified documents when their files change on disk." ) },
			 &mApp->getConfig().editor.autoReloadOnDiskChange );

	addCategory( panel, "general.workspace", mApp->i18n( "general", "General" ),
				 mApp->i18n( "workspace", "Workspace" ) );
	addBool( panel,
			 { "sessionSnapshot", "general.workspace",
			   mApp->i18n( "session_snapshot", "Session Snapshot & Periodic Backup" ),
			   mApp->i18n( "session_snapshot_desc",
						   "Preserve unsaved document changes between sessions." ) },
			 &mApp->getConfig().workspace.sessionSnapshot );
	addBool( panel,
			 { "restoreLastSession", "general.workspace",
			   mApp->i18n( "restore_last_session", "Restore Last Session" ),
			   mApp->i18n( "restore_last_session_desc",
						   "Reopen the previous workspace and documents at startup." ) },
			 &mApp->getConfig().workspace.restoreLastSession );
	addBool( panel,
			 { "checkForUpdatesAtStartup", "general.workspace",
			   mApp->i18n( "check_updates_at_startup", "Check for Updates at Startup" ),
			   mApp->i18n( "check_updates_at_startup_desc",
						   "Check whether a newer ecode release is available after startup." ) },
			 &mApp->getConfig().workspace.checkForUpdatesAtStartup );

	addCategory( panel, "appearance.theme", mApp->i18n( "appearance", "Appearance" ),
				 mApp->i18n( "theme_and_language", "Theme & Language" ) );
	addChoice(
		panel,
		{ "uiColorScheme", "appearance.theme",
		  mApp->i18n( "ui_prefes_color_scheme", "UI Prefers Color Scheme" ),
		  mApp->i18n(
			  "ui_prefers_color_scheme_desc",
			  "Choose whether the interface follows the system, light, or dark appearance." ) },
		{ mApp->i18n( "system", "System" ), mApp->i18n( "light", "Light" ),
		  mApp->i18n( "dark", "Dark" ) },
		[this] {
			switch ( mApp->getUIColorScheme() ) {
				case ColorSchemeExtPreference::System:
					return size_t{ 0 };
				case ColorSchemeExtPreference::Light:
					return size_t{ 1 };
				default:
					return size_t{ 2 };
			}
		},
		[this]( size_t selected ) {
			static constexpr ColorSchemeExtPreference values[] = { ColorSchemeExtPreference::System,
																   ColorSchemeExtPreference::Light,
																   ColorSchemeExtPreference::Dark };
			mApp->setUIColorSchemeFromUserInteraction( values[std::min( selected, size_t{ 2 } )] );
		},
		{ mApp->i18n(
			  "prefers_color_scheme_system_tooltip",
			  "System options will try to pick the system-wide currently preferred color scheme." ),
		  mApp->i18n( "prefers_color_scheme_light_tooltip",
					  "Always use the light interface color scheme." ),
		  mApp->i18n( "prefers_color_scheme_dark_tooltip",
					  "Always use the dark interface color scheme." ) } );
	std::vector<String> themeNames{ mApp->i18n( "default_theme", "Default Theme" ),
									mApp->i18n( "syntax_color_scheme", "Syntax Color Scheme" ) };
	std::vector<std::string> themeIds{ "default_theme", "syntax_color_scheme" };
	for ( const auto& file :
		  FileSystem::filesInfoGetInPath( mApp->getThemesPath(), true, true, true ) ) {
		if ( file.getExtension() != "css" )
			continue;
		auto name = FileSystem::fileRemoveExtension( file.getFileName() );
		themeNames.emplace_back( name );
		themeIds.emplace_back( std::move( name ) );
	}
	auto currentTheme =
		mApp->getConfig().ui.theme.empty() ? "default_theme" : mApp->getConfig().ui.theme;
	auto selectedTheme = std::find( themeIds.begin(), themeIds.end(), currentTheme );
	const size_t selectedThemeIndex = selectedTheme == themeIds.end()
										  ? 0
										  : static_cast<size_t>( selectedTheme - themeIds.begin() );
	addChoice(
		panel,
		{ "uiTheme", "appearance.theme", mApp->i18n( "ui_thene", "UI Theme" ),
		  mApp->i18n( "ui_theme_desc",
					  "Choose the stylesheet used by the application interface." ) },
		themeNames, [selectedThemeIndex] { return selectedThemeIndex; },
		[this, themeIds = std::move( themeIds )]( size_t selected ) {
			const auto& id = themeIds[std::min( selected, themeIds.size() - 1 )];
			mApp->getConfig().ui.theme = id == "default_theme" ? "" : id;
			mApp->setTheme(
				id == "default_theme"
					? mApp->getDefaultThemePath()
					: ( id == "syntax_color_scheme" ? id : mApp->getThemesPath() + id + ".css" ) );
		} );
	std::map<std::string, std::string> languages;
	for ( const auto& file :
		  FileSystem::filesInfoGetInPath( mApp->geti18nPath(), false, true, false, true ) ) {
		if ( file.getExtension() != "xml" )
			continue;
		auto id = FileSystem::fileRemoveExtension( file.getFileName() );
		std::string data;
		FileSystem::fileGet( file.getFilepath(), data );
		LuaPattern pattern( "title=\"(.-)\"" );
		PatternMatcher::Range matches[2];
		if ( pattern.matches( data, matches ) )
			languages[data.substr( matches[1].start, matches[1].end - matches[1].start )] = id;
		else
			languages[id] = id;
	}
	std::vector<String> languageNames;
	std::vector<std::string> languageIds;
	for ( const auto& [name, id] : languages ) {
		languageNames.emplace_back( name );
		languageIds.emplace_back( id );
	}
	auto currentLanguage =
		mApp->getConfig().ui.language.empty() ? "en" : mApp->getConfig().ui.language;
	auto selectedLanguage = std::find( languageIds.begin(), languageIds.end(), currentLanguage );
	const size_t selectedLanguageIndex =
		selectedLanguage == languageIds.end()
			? 0
			: static_cast<size_t>( selectedLanguage - languageIds.begin() );
	if ( !languageIds.empty() )
		addChoice(
			panel,
			{ "uiLanguage", "appearance.theme", mApp->i18n( "ui_language", "UI Language" ),
			  mApp->i18n( "ui_language_desc",
						  "Choose the interface language. Restart required." ) },
			languageNames, [selectedLanguageIndex] { return selectedLanguageIndex; },
			[this, languageIds = std::move( languageIds )]( size_t selected ) {
				mApp->getConfig().ui.language =
					languageIds[std::min( selected, languageIds.size() - 1 )];
			} );

	addCategory( panel, "appearance.fonts", mApp->i18n( "appearance", "Appearance" ),
				 mApp->i18n( "fonts_and_scale", "Fonts & Scale" ) );
	addAction(
		panel,
		{ "uiFont", "appearance.fonts",
		  mApp->i18n( "ui_font_and_size_ellipsis", "UI Font & Size..." ),
		  mApp->i18n( "ui_font_desc", "Choose the proportional font used by the interface." ) },
		mApp->i18n( "choose_font", "Choose Font..." ), [this, &panel] {
			mApp->openFontDialog( mApp->getConfig().ui.sansSerifFont, false, false,
								  [this, &panel] { refreshTextSetting( panel, "uiFontSize" ); } );
		} );
	addAction(
		panel,
		{ "editorFont", "appearance.fonts",
		  mApp->i18n( "editor_font_and_size_ellipsis", "Editor Font & Size..." ),
		  mApp->i18n( "editor_font_desc", "Choose the monospace font used by code editors." ) },
		mApp->i18n( "choose_font", "Choose Font..." ), [this, &panel] {
			mApp->openFontDialog( mApp->getConfig().ui.monospaceFont, true, false, [this, &panel] {
				refreshTextSetting( panel, "editorFontSize" );
			} );
		} );
	addAction(
		panel,
		{ "terminalFont", "appearance.fonts",
		  mApp->i18n( "terminal_font_and_size_ellipsis", "Terminal Font & Size..." ),
		  mApp->i18n( "terminal_font_desc", "Choose the monospace font used by terminals." ) },
		mApp->i18n( "choose_font", "Choose Font..." ), [this, &panel] {
			mApp->openFontDialog( mApp->getConfig().ui.terminalFont, true, true, [this, &panel] {
				refreshTextSetting( panel, "terminalFontSize" );
			} );
		} );
	addAction( panel,
			   { "fallbackFont", "appearance.fonts",
				 mApp->i18n( "fallback_font_ellipsis", "Fallback Font..." ),
				 mApp->i18n( "fallback_font_desc", "Choose the font used for missing glyphs." ) },
			   mApp->i18n( "choose_font", "Choose Font..." ),
			   [this] { mApp->runCommand( "fallback-font" ); } );
	auto addFontSize = [this, &panel]( SettingDescriptor binding, std::function<std::string()> get,
									   std::function<void( const StyleSheetLength& )> set ) {
		addText(
			panel, std::move( binding ), std::move( get ),
			[set = std::move( set )]( const std::string& text ) {
				if ( !StyleSheetLength::isLength( text ) )
					return false;
				set( StyleSheetLength::fromString( text ) );
				return true;
			},
			true );
	};
	addFontSize(
		{ "uiFontSize", "appearance.fonts", mApp->i18n( "ui_font_size", "UI Font Size" ),
		  mApp->i18n( "ui_font_size_desc", "Set the font size used by the application UI." ) },
		[this] { return mApp->getConfig().ui.fontSize.toString(); },
		[this]( const StyleSheetLength& size ) {
			mApp->getSettingsActions()->setUIFontSize( size );
		} );
	addFontSize(
		{ "panelFontSize", "appearance.fonts",
		  mApp->i18n( "ui_panel_font_size", "Panel Font Size" ),
		  mApp->i18n( "ui_panel_font_size_desc", "Set the font size used by side panels." ) },
		[this] { return mApp->getConfig().ui.panelFontSize.toString(); },
		[this]( const StyleSheetLength& size ) {
			mApp->getSettingsActions()->setUIPanelFontSize( size );
		} );
	addFontSize(
		{ "editorFontSize", "appearance.fonts",
		  mApp->i18n( "editor_font_size", "Editor Font Size" ),
		  mApp->i18n( "editor_font_size_desc", "Set the default code editor font size." ) },
		[this] { return mApp->getConfig().editor.fontSize.toString(); },
		[this]( const StyleSheetLength& size ) {
			mApp->getSettingsActions()->setEditorFontSize( size );
		} );
	addFontSize(
		{ "terminalFontSize", "appearance.fonts",
		  mApp->i18n( "terminal_font_size", "Terminal Font Size" ),
		  mApp->i18n( "terminal_font_size_desc",
					  "Set the default integrated terminal font size." ) },
		[this] { return mApp->getConfig().term.fontSize.toString(); },
		[this]( const StyleSheetLength& size ) {
			mApp->getSettingsActions()->setTerminalFontSize( size );
		} );
	addFloat(
		panel,
		{ "uiScaleFactor", "appearance.fonts", mApp->i18n( "ui_scale_factor", "UI Scale Factor" ),
		  mApp->i18n( "ui_scale_factor_desc",
					  "Scale the complete user interface from 1 to 6. Restart required." ) },
		1, 6, 0.1,
		[this] { return std::max<Float>( 1, mApp->getConfig().windowState.pixelDensity ); },
		[this]( double value ) { mApp->getConfig().windowState.pixelDensity = value; } );
	addBool( panel,
			 { "editorFontInInputFields", "appearance.fonts",
			   mApp->i18n( "editor_font_in_input_fields", "Editor Font in Input Fields" ),
			   mApp->i18n( "editor_font_in_input_fields_desc",
						   "Use the editor font for text input controls." ) },
			 &mApp->getConfig().ui.editorFontInInputFields );
	addChoice(
		panel,
		{ "fontHinting", "appearance.fonts", mApp->i18n( "ui_font_hint", "Font Hinting" ),
		  mApp->i18n( "ui_font_hint_desc", "Control glyph alignment to the pixel grid." ) },
		{ mApp->i18n( "none", "None" ), mApp->i18n( "slight", "Slight" ),
		  mApp->i18n( "full", "Full" ) },
		[this] { return static_cast<size_t>( mApp->getConfig().ui.fontHinting ); },
		[this]( size_t selected ) {
			static constexpr FontHinting values[] = { FontHinting::None, FontHinting::Slight,
													  FontHinting::Full };
			auto value = values[std::min( selected, size_t{ 2 } )];
			mApp->getConfig().ui.fontHinting = value;
			defaultResourceScope().getFontService().setHinting( value );
			mApp->getSplitter()->forEachWidgetType( UI_TYPE_TERMINAL, []( UIWidget* widget ) {
				widget->asType<UITerminal>()->syncFontRenderingConfig();
			} );
		} );
	addChoice(
		panel,
		{ "fontAntialiasing", "appearance.fonts",
		  mApp->i18n( "ui_font_antialiasing", "Font Anti-Aliasing" ),
		  mApp->i18n( "ui_font_antialiasing_desc", "Choose how glyph edges are smoothed." ) },
		{ mApp->i18n( "none", "None" ), mApp->i18n( "grayscale", "Grayscale" ),
		  mApp->i18n( "subpixel", "Subpixel" ) },
		[this] { return static_cast<size_t>( mApp->getConfig().ui.fontAntialiasing ); },
		[this]( size_t selected ) {
			static constexpr FontAntialiasing values[] = {
				FontAntialiasing::None, FontAntialiasing::Grayscale, FontAntialiasing::Subpixel };
			auto value = values[std::min( selected, size_t{ 2 } )];
			mApp->getConfig().ui.fontAntialiasing = value;
			defaultResourceScope().getFontService().setAntialiasing( value );
			mApp->getSplitter()->forEachWidgetType( UI_TYPE_TERMINAL, []( UIWidget* widget ) {
				widget->asType<UITerminal>()->syncFontRenderingConfig();
			} );
		} );
	auto addFontFeature = [this, &panel]( std::string id, const char* nameKey, const char* name,
										  const char* descriptionKey, const char* description,
										  Uint32 feature, bool editorFeature,
										  const String& group ) {
		addBool(
			panel,
			{ std::move( id ), "appearance.fonts", mApp->i18n( nameKey, name ),
			  mApp->i18n( descriptionKey, description ), group },
			[this, feature, editorFeature] {
				const Uint32 features = editorFeature ? mApp->getConfig().editor.fontFeatures
													  : mApp->getConfig().ui.fontFeatures;
				return ( features & feature ) != 0;
			},
			[this, feature, editorFeature]( bool enabled ) {
				Uint32& features = editorFeature ? mApp->getConfig().editor.fontFeatures
												 : mApp->getConfig().ui.fontFeatures;
				if ( enabled )
					features |= feature;
				else
					features &= ~feature;
				if ( editorFeature ) {
					mApp->getSplitter()->forEachEditor( [features]( UICodeEditor* editor ) {
						editor->setLigatureFeatures( features );
					} );
				} else {
					mApp->getUISceneNode()->setDefaultTextHints( features );
				}
			} );
	};
	for ( bool editorFeature : { false, true } ) {
		const std::string prefix = editorFeature ? "editor" : "ui";
		const String group = editorFeature
								 ? mApp->i18n( "editor_font_features", "Editor Font Features" )
								 : mApp->i18n( "ui_font_features", "UI Font Features" );
		addSubcategoryHeading( panel, "appearance.fonts", group );
		addFontFeature( prefix + "StandardLigatures", "standard_ligatures",
						"Standard Ligatures (liga)", "standard_ligatures_desc",
						"Typographic combinations such as fi, fl, and ffi, depending on the font.",
						TextHints::StandardLigatures, editorFeature, group );
		addFontFeature( prefix + "ContextualAlternates", "contextual_alternates",
						"Contextual Alternates (calt)", "contextual_alternates_desc",
						"Context-dependent alternatives, including many programming ligatures.",
						TextHints::ContextualAlternates, editorFeature, group );
		addFontFeature( prefix + "ContextualLigatures", "contextual_ligatures",
						"Contextual Ligatures (clig)", "contextual_ligatures_desc",
						"Ligatures applied in specific contexts to improve readability.",
						TextHints::ContextualLigatures, editorFeature, group );
		addFontFeature( prefix + "DiscretionaryLigatures", "discretionary_ligatures",
						"Discretionary Ligatures (dlig)", "discretionary_ligatures_desc",
						"Optional decorative or stylistic ligatures provided by the font.",
						TextHints::DiscretionaryLigatures, editorFeature, group );
	}

	addCategory( panel, "editor.advanced", mApp->i18n( "editor", "Editor" ),
				 mApp->i18n( "advanced", "Advanced" ) );
	addText(
		panel,
		{ "lineSpacing", "editor.advanced", mApp->i18n( "line_spacing", "Line Spacing" ),
		  mApp->i18n( "line_spacing_desc",
					  "Set additional vertical spacing between editor lines. Set 0 to disable." ) },
		[this] { return mApp->getConfig().editor.lineSpacing.toString(); },
		[this]( const std::string& text ) {
			if ( !StyleSheetLength::isLength( text ) )
				return false;
			mApp->getConfig().editor.lineSpacing = StyleSheetLength::fromString( text );
			mApp->getSplitter()->forEachEditor( [this]( UICodeEditor* editor ) {
				editor->setLineSpacing( mApp->getConfig().editor.lineSpacing );
			} );
			return true;
		} );
	addText(
		panel,
		{ "cursorBlinkingTime", "editor.advanced",
		  mApp->i18n( "cursor_blinking_time", "Cursor Blinking Time" ),
		  mApp->i18n( "cursor_blinking_time_desc",
					  "Set the text cursor blink interval. Set 0 to disable." ) },
		[this] { return mApp->getConfig().editor.cursorBlinkingTime.toString(); },
		[this]( const std::string& text ) {
			Time value;
			if ( !SettingsPage::parseNonNegativeSettingsTime( text, value ) )
				return false;
			mApp->getConfig().editor.cursorBlinkingTime = value;
			mApp->getSplitter()->forEachEditor(
				[value]( UICodeEditor* editor ) { editor->setCursorBlinkTime( value ); } );
			return true;
		} );
	addEditableChoice(
		panel,
		{ "indentTabCharacter", "editor.advanced",
		  mApp->i18n( "indent_tab_character", "Indent Tab Character" ),
		  mApp->i18n( "indent_tab_character_desc",
					  "Choose the character inserted when indenting with Tab." ) },
		{ String( u8"»" ), String( u8"→" ), String( u8"⇒" ), String( u8"↪" ), String( u8"⇢" ),
		  String( u8"↣" ) },
		[this] {
			return mApp->getConfig().editor.tabIndentCharacter.empty()
					   ? String( u8"»" )
					   : String::fromUtf8( mApp->getConfig().editor.tabIndentCharacter );
		},
		[this]( const String& value ) {
			if ( value.size() != 1 )
				return false;
			mApp->getConfig().editor.tabIndentCharacter = value.toUtf8();
			mApp->getSplitter()->forEachEditor( [character = value[0]]( UICodeEditor* editor ) {
				editor->setTabIndentCharacter( character );
			} );
			return true;
		} );
	addChoice(
		panel,
		{ "indentTabAlignment", "editor.advanced",
		  mApp->i18n( "indent_tab_alignment", "Indent Tab Alignment" ),
		  mApp->i18n( "indent_tab_alignment_desc",
					  "Align text within the visual width of indentation tabs." ) },
		{ mApp->i18n( "left", "Left" ), mApp->i18n( "center", "Center" ),
		  mApp->i18n( "right", "Right" ) },
		[this] {
			return mApp->getConfig().editor.tabIndentAlignment == CharacterAlignment::Left
					   ? size_t{ 0 }
					   : ( mApp->getConfig().editor.tabIndentAlignment == CharacterAlignment::Center
							   ? size_t{ 1 }
							   : size_t{ 2 } );
		},
		[this]( size_t selected ) {
			static constexpr CharacterAlignment values[] = {
				CharacterAlignment::Left, CharacterAlignment::Center, CharacterAlignment::Right };
			auto value = values[std::min( selected, size_t{ 2 } )];
			mApp->getConfig().editor.tabIndentAlignment = value;
			mApp->getSplitter()->forEachEditor(
				[value]( UICodeEditor* editor ) { editor->setTabIndentAlignment( value ); } );
		} );
	addText(
		panel,
		{ "foldRefreshFrequency", "editor.advanced",
		  mApp->i18n( "folds_refresh_freq", "Folds Refresh Frequency" ),
		  mApp->i18n(
			  "folds_refresh_freq_desc",
			  "Set how frequently code folding ranges are recalculated (minimum 1 second)." ) },
		[this] { return mApp->getConfig().editor.codeFoldingRefreshFreq.toString(); },
		[this]( const std::string& text ) {
			Time value;
			if ( !SettingsPage::parseNonNegativeSettingsTime( text, value ) ||
				 value < Seconds( 1 ) )
				return false;
			mApp->getConfig().editor.codeFoldingRefreshFreq = value;
			mApp->getSplitter()->forEachEditor(
				[value]( UICodeEditor* editor ) { editor->setFoldsRefreshTime( value ); } );
			return true;
		} );
	addText(
		panel,
		{ "tabOutCharacters", "editor.advanced",
		  mApp->i18n( "set_tab_out_characters", "Set Tab Out Characters" ),
		  mApp->i18n( "set_tab_out_characters_message",
					  "Set the characters the cursor can move past when pressing Tab." ) },
		[this] { return mApp->getConfig().doc.tabOutChars; },
		[this]( const std::string& text ) {
			mApp->getConfig().doc.tabOutChars = text;
			String characters = String::fromUtf8( text );
			mApp->getSplitter()->forEachEditor( [&characters]( UICodeEditor* editor ) {
				editor->getDocument().setTabOutChars( characters );
			} );
			return true;
		} );

	addCategory( panel, "window.screenshots", mApp->i18n( "window", "Window" ),
				 mApp->i18n( "screenshots", "Screenshots" ) );
	addAction( panel,
			   { "screenshotSavePath", "window.screenshots",
				 mApp->i18n( "set_screenshot_save_path", "Screenshot Save Path" ),
				 mApp->i18n( "screenshot_save_path_desc", "Choose where screenshots are saved." ) },
			   mApp->i18n( "configure", "Configure..." ),
			   [this] { mApp->getSettingsActions()->setScreenshotSavePath(); } );
	addText(
		panel,
		{ "screenshotFilenamePattern", "window.screenshots",
		  mApp->i18n( "set_screenshot_filename_pattern", "Screenshot Filename Pattern" ),
		  mApp->i18n( "screenshot_filename_pattern_desc",
					  "Set the timestamp-based screenshot filename pattern." ) },
		[this] { return mApp->getConfig().screenshot.filenamePattern; },
		[this]( const std::string& pattern ) {
			std::string filename = DateTimeController::formatCurrentDate( pattern );
			if ( !DateTimeController::isValidDateFormat( pattern ) || filename.empty() ||
				 filename.find_first_of( "<>:\"/\\|?*" ) != std::string::npos )
				return false;
			mApp->getConfig().screenshot.filenamePattern = pattern;
			return true;
		} );
	static const std::vector<String> screenshotFormats{ "PNG", "JPG", "WEBP", "QOI",
														"BMP", "TGA", "DDS" };
	auto screenshotFormat = String( mApp->getConfig().screenshot.saveFormat ).toUpper().toUtf8();
	auto selectedScreenshotFormat = std::find( screenshotFormats.begin(), screenshotFormats.end(),
											   String::fromUtf8( screenshotFormat ) );
	addChoice(
		panel,
		{ "screenshotSaveFormat", "window.screenshots",
		  mApp->i18n( "set_screenshot_save_format", "Screenshot Save Format" ),
		  mApp->i18n( "screenshot_save_format_desc", "Choose the image format for screenshots." ) },
		screenshotFormats,
		[selected = static_cast<size_t>(
			 selectedScreenshotFormat == screenshotFormats.end()
				 ? 0
				 : selectedScreenshotFormat - screenshotFormats.begin() )] { return selected; },
		[this]( size_t selected ) {
			String format = screenshotFormats[std::min( selected, screenshotFormats.size() - 1 )];
			mApp->getConfig().screenshot.saveFormat = format.toLower().toUtf8();
		} );

	addCategory( panel, "window.renderer", mApp->i18n( "window", "Window" ),
				 mApp->i18n( "renderer", "Renderer" ) );
	addBool(
		panel,
		{ "vsync", "window.renderer", mApp->i18n( "vsync", "VSync" ),
		  mApp->i18n( "vsync_desc",
					  "Synchronize rendering with the display refresh rate. Restart required." ) },
		&mApp->getConfig().context.VSync, [this]( bool ) {
			mApp->saveConfig();
			mApp->getNotificationCenter()->addNotification(
				mApp->i18n( "vsync_changed",
							"Vsync configuration changed.\nRestart ecode to see the changes." )
					.unescape() );
		} );
	addInteger(
		panel,
		{ "frameRateLimit", "window.renderer", mApp->i18n( "frame_rate_limit", "Frame Rate Limit" ),
		  mApp->i18n( "frame_rate_limit_desc",
					  "Limit rendered frames per second. Set 0 to disable." ) },
		0, 1000, [this] { return mApp->getConfig().context.FrameRateLimit; },
		[this]( int value ) {
			mApp->getConfig().context.FrameRateLimit = value;
			mApp->saveConfig();
			mApp->getWindow()->setFrameRateLimit( value );
			mApp->getNotificationCenter()->addNotification(
				mApp->i18n( "frame_rate_limit_applied", "Frame Rate Limit Applied" ) );
		} );
	std::vector<GraphicsLibraryVersion> rendererVersions =
		Renderer::getAvailableGraphicsLibraryVersions();
	std::vector<String> rendererVersionNames;
	rendererVersionNames.reserve( rendererVersions.size() );
	for ( const auto version : rendererVersions )
		rendererVersionNames.emplace_back( Renderer::graphicsLibraryVersionToString( version ) );
	auto selectedRendererVersion = std::find( rendererVersions.begin(), rendererVersions.end(),
											  mApp->getConfig().context.Version );
	const size_t selectedRendererVersionIndex =
		selectedRendererVersion == rendererVersions.end()
			? 0
			: static_cast<size_t>( selectedRendererVersion - rendererVersions.begin() );
	if ( !rendererVersions.empty() )
		addChoice(
			panel,
			{ "rendererVersion", "window.renderer",
			  mApp->i18n( "ui_renderer_version", "Renderer Version" ),
			  mApp->i18n( "ui_renderer_version_desc",
						  "Select the graphics API version used by ecode. Restart required." ) },
			rendererVersionNames,
			[selectedRendererVersionIndex] { return selectedRendererVersionIndex; },
			[this, rendererVersions = std::move( rendererVersions )]( size_t selected ) {
				mApp->getConfig().context.Version =
					rendererVersions[std::min( selected, rendererVersions.size() - 1 )];
				mApp->saveConfig();
				mApp->getNotificationCenter()->addNotification(
					mApp->i18n( "glversion_changed",
								"Renderer version changed.\nRestart ecode to see the changes." )
						.unescape() );
			} );
	addChoice(
		panel,
		{ "multisamples", "window.renderer",
		  mApp->i18n( "ui_multisamples_level", "Multisample Anti-Aliasing Level" ),
		  mApp->i18n( "ui_multisamples_level_desc",
					  "Set renderer multisampling. Restart required." ) },
		{ "0", "2", "4", "8", "16" },
		[this] {
			static constexpr Uint32 values[] = { 0, 2, 4, 8, 16 };
			auto found = std::find( std::begin( values ), std::end( values ),
									mApp->getConfig().context.Multisamples );
			return found == std::end( values ) ? size_t{ 0 }
											   : size_t( found - std::begin( values ) );
		},
		[this]( size_t selected ) {
			static constexpr Uint32 values[] = { 0, 2, 4, 8, 16 };
			mApp->getConfig().context.Multisamples = values[std::min( selected, size_t{ 4 } )];
			mApp->saveConfig();
			mApp->getNotificationCenter()->addNotification(
				mApp->i18n( "multisamples_changed", "Multisample Anti-Aliasing Level "
													"applied.\nRestart ecode to see the changes." )
					.unescape() );
		},
		{ mApp->i18n( "multisamples_disabled_desc", "Disable multisample anti-aliasing." ),
		  mApp->i18n( "multisamples_2x_desc", "Use 2x multisample anti-aliasing." ),
		  mApp->i18n( "multisamples_4x_desc", "Use 4x multisample anti-aliasing." ),
		  mApp->i18n( "multisamples_8x_desc", "Use 8x multisample anti-aliasing." ),
		  mApp->i18n( "multisamples_16x_desc", "Use 16x multisample anti-aliasing." ) } );

	addCategory( panel, "terminal.behavior", mApp->i18n( "terminal", "Terminal" ),
				 mApp->i18n( "behavior", "Behavior" ) );
	addChoice(
		panel,
		{ "newTerminalOrientation", "terminal.behavior",
		  mApp->i18n( "new_terminal_behavior", "New Terminal Behavior" ),
		  mApp->i18n( "new_terminal_behavior_desc",
					  "Choose where newly created terminal sessions are opened." ) },
		{ mApp->i18n( "open_in_same_tabbar", "Open in Current Tab Bar" ),
		  mApp->i18n( "open_in_vertical_split", "Open in New Vertical Split" ),
		  mApp->i18n( "open_in_horizontal_split", "Open in New Horizontal Split" ),
		  mApp->i18n( "open_in_statusbar_panel", "Open in Status Bar Panel" ) },
		[this] {
			switch ( mApp->getConfig().term.newTerminalOrientation ) {
				case NewTerminalOrientation::Vertical:
					return size_t{ 1 };
				case NewTerminalOrientation::Horizontal:
					return size_t{ 2 };
				case NewTerminalOrientation::StatusBarPanel:
					return size_t{ 3 };
				default:
					return size_t{ 0 };
			}
		},
		[this]( size_t selected ) {
			static constexpr NewTerminalOrientation::Orientation orientations[] = {
				NewTerminalOrientation::Same, NewTerminalOrientation::Vertical,
				NewTerminalOrientation::Horizontal, NewTerminalOrientation::StatusBarPanel };
			mApp->getConfig().term.newTerminalOrientation =
				orientations[std::min( selected, size_t{ 3 } )];
		} );
	addBool( panel,
			 { "terminalExclusiveMode", "terminal.behavior",
			   mApp->i18n( "enable_exclusive_mode_by_default", "Enable Exclusive Mode by Default" ),
			   mApp->i18n( "enable_exclusive_mode_by_default_tooltip",
						   "Disable global keybindings in newly created terminals." ) },
			 &mApp->getConfig().term.exclusiveMode );
	addBool( panel,
			 { "closeTerminalTabOnExit", "terminal.behavior",
			   mApp->i18n( "close_terminal_tab_on_exit", "Close Terminal Tab on Exit" ),
			   mApp->i18n( "close_terminal_tab_on_exit_tooltip",
						   "Close a terminal tab when its main process exits." ) },
			 &mApp->getConfig().term.closeTerminalTabOnExit, [this]( bool value ) {
				 mApp->getSplitter()->forEachWidgetType(
					 UI_TYPE_TERMINAL, [value]( UIWidget* widget ) {
						 widget->asType<UITerminal>()->getTerm()->setKeepAlive( !value );
					 } );
			 } );
	addBool( panel,
			 { "warnBeforeClosingTerminal", "terminal.behavior",
			   mApp->i18n( "warn_before_closing_tab", "Warn Before Closing Tab" ),
			   mApp->i18n( "warn_before_closing_tab_tooltip",
						   "Ask before closing a terminal while a program is running." ) },
			 &mApp->getConfig().term.warnBeforeClosingTab );
	addAction( panel,
			   { "terminalShell", "terminal.behavior",
				 mApp->i18n( "configure_terminal_shell", "Configure Terminal Shell" ),
				 mApp->i18n( "configure_terminal_shell_desc",
							 "Set the shell executable and command-line arguments." ) },
			   mApp->i18n( "configure", "Configure..." ),
			   [this] { mApp->runCommand( "configure-terminal-shell" ); } );
	addInteger(
		panel,
		{ "terminalScrollback", "terminal.behavior",
		  mApp->i18n( "terminal_scrollback", "Terminal Scrollback" ),
		  mApp->i18n( "configure_terminal_scrollback_desc",
					  "Set the number of terminal history lines retained." ) },
		0, std::numeric_limits<int>::max(),
		[this] {
			return static_cast<int>( std::min<Uint64>( mApp->getConfig().term.scrollback,
													   std::numeric_limits<int>::max() ) );
		},
		[this]( int value ) { mApp->getConfig().term.scrollback = static_cast<Uint64>( value ); } );
	addAction( panel,
			   { "terminalWorkingDirectory", "terminal.behavior",
				 mApp->i18n( "configure_terminal_working_dir",
							 "Configure Terminal Default Working Directory" ),
				 mApp->i18n( "configure_terminal_working_dir_desc",
							 "Choose the working directory used by new terminals." ) },
			   mApp->i18n( "configure", "Configure..." ),
			   [this] { mApp->runCommand( "configure-terminal-working-dir" ); } );

	addCategory( panel, "terminal.appearance", mApp->i18n( "terminal", "Terminal" ),
				 mApp->i18n( "appearance", "Appearance" ) );
	std::vector<String> terminalSchemeNames;
	std::vector<std::string> terminalSchemeIds;
	for ( const auto& [name, scheme] : mApp->getTerminalManager()->getTerminalColorSchemes() ) {
		terminalSchemeNames.emplace_back( name );
		terminalSchemeIds.emplace_back( name );
	}
	auto selectedTerminalScheme =
		std::find( terminalSchemeNames.begin(), terminalSchemeNames.end(),
				   String( mApp->getTerminalManager()->getTerminalCurrentColorScheme() ) );
	const size_t selectedTerminalSchemeIndex =
		selectedTerminalScheme == terminalSchemeNames.end()
			? 0
			: static_cast<size_t>( selectedTerminalScheme - terminalSchemeNames.begin() );
	if ( !terminalSchemeNames.empty() )
		addChoice(
			panel,
			{ "terminalColorScheme", "terminal.appearance",
			  mApp->i18n( "terminal_color_scheme", "Terminal Color Scheme" ),
			  mApp->i18n( "terminal_color_scheme_desc",
						  "Choose the colors used by integrated terminals." ) },
			terminalSchemeNames,
			[selectedTerminalSchemeIndex] { return selectedTerminalSchemeIndex; },
			[this, terminalSchemeIds = std::move( terminalSchemeIds )]( size_t selected ) {
				mApp->getTerminalManager()->setTerminalColorScheme(
					terminalSchemeIds[std::min( selected, terminalSchemeIds.size() - 1 )] );
			} );
	addChoice(
		panel,
		{ "terminalScrollbarType", "terminal.appearance",
		  mApp->i18n( "scrollbar_type", "Scrollbar Type" ),
		  mApp->i18n( "scrollbar_type_desc",
					  "Place the terminal scrollbar over or outside its content." ) },
		{ mApp->i18n( "overlay", "Overlay" ), mApp->i18n( "outside", "Outside" ) },
		[this] { return mApp->getConfig().term.scrollBarType == ScrollViewType::Overlay ? 0 : 1; },
		[this]( size_t selected ) {
			auto value = selected == 0 ? ScrollViewType::Overlay : ScrollViewType::Outside;
			mApp->getConfig().term.scrollBarType = value;
			mApp->getSplitter()->forEachWidgetType( UI_TYPE_TERMINAL, [value]( UIWidget* widget ) {
				widget->asType<UITerminal>()->setScrollViewType( value );
			} );
		},
		{ mApp->i18n( "scroll_overlay_tooltip", "Scrollbar appears over content." ),
		  mApp->i18n( "scroll_outside_tooltip",
					  "Scrollbar has its own space, never covers content." ) } );
	addChoice(
		panel,
		{ "terminalCursorStyle", "terminal.appearance",
		  mApp->i18n( "cursor_style", "Cursor Style" ),
		  mApp->i18n( "cursor_style_desc", "Choose the terminal cursor shape and animation." ) },
		{ mApp->i18n( "blinking_block", "Blinking Block" ),
		  mApp->i18n( "steady_block", "Steady Block" ),
		  mApp->i18n( "blink_underline", "Blink Underline" ),
		  mApp->i18n( "steady_underline", "Steady Underline" ),
		  mApp->i18n( "blink_bar", "Blink Bar" ), mApp->i18n( "steady_bar", "Steady Bar" ) },
		[this] {
			static constexpr TerminalCursorMode modes[] = {
				TerminalCursorMode::BlinkingBlock,	TerminalCursorMode::SteadyBlock,
				TerminalCursorMode::BlinkUnderline, TerminalCursorMode::SteadyUnderline,
				TerminalCursorMode::BlinkBar,		TerminalCursorMode::SteadyBar };
			auto found = std::find( std::begin( modes ), std::end( modes ),
									mApp->getConfig().term.cursorStyle );
			return found == std::end( modes ) ? size_t{ 0 } : size_t( found - std::begin( modes ) );
		},
		[this]( size_t selected ) {
			static constexpr TerminalCursorMode modes[] = {
				TerminalCursorMode::BlinkingBlock,	TerminalCursorMode::SteadyBlock,
				TerminalCursorMode::BlinkUnderline, TerminalCursorMode::SteadyUnderline,
				TerminalCursorMode::BlinkBar,		TerminalCursorMode::SteadyBar };
			auto value = modes[std::min( selected, size_t{ 5 } )];
			mApp->getConfig().term.cursorStyle = value;
			mApp->getSplitter()->forEachWidgetType( UI_TYPE_TERMINAL, [value]( UIWidget* widget ) {
				widget->asType<UITerminal>()->getTerm()->setCursorMode( value );
			} );
		} );
	addChoice(
		panel,
		{ "terminalScrollbarMode", "terminal.appearance",
		  mApp->i18n( "scrollbar_mode", "Scrollbar Mode" ),
		  mApp->i18n( "scrollbar_mode_desc", "Control when the terminal scrollbar is visible." ) },
		{ mApp->i18n( "auto", "Auto" ), mApp->i18n( "always_visible", "Always Visible" ),
		  mApp->i18n( "always_hidden", "Always Hidden" ) },
		[this] {
			return mApp->getConfig().term.scrollBarMode == ScrollBarMode::Auto
					   ? 0
					   : ( mApp->getConfig().term.scrollBarMode == ScrollBarMode::AlwaysOn ? 1
																						   : 2 );
		},
		[this]( size_t selected ) {
			auto value = selected == 0 ? ScrollBarMode::Auto
									   : ( selected == 1 ? ScrollBarMode::AlwaysOn
														 : ScrollBarMode::AlwaysOff );
			mApp->getConfig().term.scrollBarMode = value;
			mApp->getSplitter()->forEachWidgetType( UI_TYPE_TERMINAL, [value]( UIWidget* widget ) {
				widget->asType<UITerminal>()->setVerticalScrollMode( value );
			} );
		},
		{ mApp->i18n( "scrollbar_mode_auto_tooltip",
					  "With an overlay scrollbar, show it when the mouse moves over the terminal. "
					  "With an outside scrollbar, show it when there is a scrollable area." ),
		  mApp->i18n( "scrollbar_mode_always_visible_tooltip",
					  "Keep the terminal scrollbar visible." ),
		  mApp->i18n( "scrollbar_mode_always_hidden_tooltip",
					  "Keep the terminal scrollbar hidden." ) } );
}

void SettingsPanel::addPluginSettings( PanelState& panel ) {
	auto* manager = mApp->getPluginManager();
	if ( !manager )
		return;
	manager->forEachPlugin( [this, &panel]( Plugin* plugin ) {
		if ( !plugin || !plugin->isReady() || !plugin->hasSettingsPage() ||
			 !plugin->hasFileConfig() )
			return;
		std::string error;
		auto document = SettingsDocument::load( plugin->getFileConfigPath(), error );
		if ( !document ) {
			Log::warning( "Could not load settings for plugin %s: %s", plugin->getId(), error );
			return;
		}
		const std::string category = "plugins." + plugin->getId();
		addCategory( panel, category, mApp->i18n( "plugins", "Plugins" ), plugin->getTitle() );
		SettingsPage page( panel.model, document, category, plugin->getId() );
		plugin->registerSettings( page );
		const std::string path = document->path();
		addAction( panel,
				   { plugin->getId() + ".edit-json", category,
					 mApp->i18n( "edit_settings_json", "Edit Settings JSON" ),
					 mApp->i18n( "edit_settings_json_desc",
								 "Open the complete plugin configuration file." ) },
				   mApp->i18n( "open_file_ellipsis", "Open File..." ), [this, &panel, path] {
					   auto* window = panel.window;
					   window->runOnMainThread( [this, window, path] {
						   window->closeWindow();
						   mApp->getUISceneNode()->runOnMainThread(
							   [this, path] { mApp->focusOrLoadFile( path ); } );
					   } );
				   } );
		panel.documents.emplace_back( std::move( document ) );
	} );
}

void SettingsPanel::addProjectSettings( PanelState& panel ) {
	addCategory( panel, "editor.document", mApp->i18n( "editor", "Editor" ),
				 mApp->i18n( "document", "Document" ) );
	addBool( panel,
			 { "useGlobalSettings", "editor.document",
			   mApp->i18n( "use_global_settings", "Use Global Settings" ),
			   mApp->i18n( "use_global_settings_desc",
						   "Inherit document defaults from the user settings." ) },
			 &mApp->getProjectConfig().useGlobalSettings, [this, &panel]( bool useGlobalSettings ) {
				 setCategoryEnabled( panel, "editor.document", !useGlobalSettings,
									 "useGlobalSettings" );
			 } );
	auto addProjectBool = [this, &panel]( std::string id, const char* nameKey, const char* name,
										  const char* descriptionKey, const char* description,
										  bool DocumentConfig::* member ) {
		addBool( panel,
				 { std::move( id ), "editor.document", mApp->i18n( nameKey, name ),
				   mApp->i18n( descriptionKey, description ) },
				 &( mApp->getProjectConfig().doc.*member ) );
	};
	addProjectBool( "trimTrailingWhitespaces", "trim_trailing_whitespaces",
					"Trim Trailing Whitespaces", "trim_trailing_whitespaces_desc",
					"Remove trailing whitespace when saving files.",
					&DocumentConfig::trimTrailingWhitespaces );
	addProjectBool( "forceNewLineAtEndOfFile", "force_new_line_at_end_of_file",
					"Force New Line at End of File", "force_new_line_at_end_of_file_desc",
					"Ensure saved files end with a newline.",
					&DocumentConfig::forceNewLineAtEndOfFile );
	addProjectBool( "autoDetectIndentType", "auto_detect_indent_type_and_width",
					"Auto Detect Indent Type & Width", "auto_detect_indent_type_desc",
					"Detect indentation settings when opening each document.",
					&DocumentConfig::autoDetectIndentType );
	addChoice(
		panel,
		{ "autoIndent", "editor.document", mApp->i18n( "auto_indent", "Auto-Indent" ),
		  mApp->i18n( "auto_indent_tooltip", "Control indentation added after pressing Enter." ) },
		{ mApp->i18n( "auto_indent_none", "None" ),
		  mApp->i18n( "auto_indent_preserve", "Preserve" ),
		  mApp->i18n( "auto_indent_smart", "Smart" ) },
		[this] { return static_cast<size_t>( mApp->getProjectConfig().doc.autoIndent ); },
		[this]( size_t selected ) {
			mApp->getProjectConfig().doc.autoIndent =
				static_cast<TextDocument::AutoIndentConfig>( std::min( selected, size_t{ 2 } ) );
		},
		{ mApp->i18n( "auto_indent_none_desc", "No automatic indentation." ),
		  mApp->i18n( "auto_indent_preserve_desc",
					  "Preserve the indentation level of the previous line." ),
		  mApp->i18n( "auto_indent_smart_desc",
					  "Preserve indentation and indent between auto-closed brackets." ) } );
	addProjectBool( "indentSpaces", "indent_spaces", "Indent Using Spaces", "indent_spaces_desc",
					"Insert spaces instead of tab characters for indentation.",
					&DocumentConfig::indentSpaces );
	addProjectBool( "writeUnicodeBOM", "write_unicode_bom", "Write Unicode BOM",
					"write_unicode_bom_desc", "Write a Unicode byte-order mark when saving.",
					&DocumentConfig::writeUnicodeBOM );
	addInteger(
		panel,
		{ "indentWidth", "editor.document", mApp->i18n( "indent_width", "Indent Width" ),
		  mApp->i18n( "indent_width_desc",
					  "Number of columns inserted for one indentation level." ) },
		1, 16, [this] { return mApp->getProjectConfig().doc.indentWidth; },
		[this]( int value ) { mApp->getProjectConfig().doc.indentWidth = value; } );
	addInteger(
		panel,
		{ "tabWidth", "editor.document", mApp->i18n( "tab_width", "Tab Width" ),
		  mApp->i18n( "tab_width_desc", "Number of columns used to display a tab character." ) },
		1, 16, [this] { return mApp->getProjectConfig().doc.tabWidth; },
		[this]( int value ) { mApp->getProjectConfig().doc.tabWidth = value; } );
	addInteger(
		panel,
		{ "lineBreakingColumn", "editor.document",
		  mApp->i18n( "line_breaking_column", "Line Breaking Column" ),
		  mApp->i18n( "line_breaking_column_desc",
					  "Column used for wrapping and the editor width guide." ) },
		0, 1000, [this] { return mApp->getProjectConfig().doc.lineBreakingColumn; },
		[this]( int value ) { mApp->getProjectConfig().doc.lineBreakingColumn = value; } );
	addChoice(
		panel,
		{ "lineEndings", "editor.document", mApp->i18n( "line_endings", "Line Endings" ),
		  mApp->i18n( "line_endings_desc", "Default line-ending sequence for new files." ) },
		{ "Windows (CR/LF)", "Unix (LF)", "Macintosh (CR)" },
		[this] {
			switch ( mApp->getProjectConfig().doc.lineEndings ) {
				case TextFormat::LineEnding::CRLF:
					return size_t{ 0 };
				case TextFormat::LineEnding::CR:
					return size_t{ 2 };
				default:
					return size_t{ 1 };
			}
		},
		[this]( size_t selected ) {
			mApp->getProjectConfig().doc.lineEndings =
				selected == 0
					? TextFormat::LineEnding::CRLF
					: ( selected == 2 ? TextFormat::LineEnding::CR : TextFormat::LineEnding::LF );
		} );
	setCategoryEnabled( panel, "editor.document", !mApp->getProjectConfig().useGlobalSettings,
						"useGlobalSettings" );

	addCategory( panel, "languages.file_associations", mApp->i18n( "languages", "Languages" ),
				 mApp->i18n( "file_associations", "File Associations" ) );
	addChoice(
		panel,
		{ "hExtLanguageType", "languages.file_associations",
		  mApp->i18n( "treat_h_files_as", "Treat .h Files As" ),
		  mApp->i18n( "treat_h_files_as_desc",
					  "Choose the default language for ambiguous .h header files." ) },
		{ mApp->i18n( "auto-detect", "Auto-Detect" ), "C", "C++", "Objective-C", "Objective-C++" },
		[this] {
			switch ( mApp->getProjectConfig().hExtLanguageType ) {
				case HExtLanguageType::C:
					return size_t{ 1 };
				case HExtLanguageType::CPP:
					return size_t{ 2 };
				case HExtLanguageType::ObjectiveC:
					return size_t{ 3 };
				case HExtLanguageType::ObjectiveCPP:
					return size_t{ 4 };
				default:
					return size_t{ 0 };
			}
		},
		[this]( size_t selected ) {
			static constexpr HExtLanguageType values[] = {
				HExtLanguageType::AutoDetect, HExtLanguageType::C, HExtLanguageType::CPP,
				HExtLanguageType::ObjectiveC, HExtLanguageType::ObjectiveCPP };
			auto value = values[std::min( selected, size_t{ 4 } )];
			mApp->getProjectConfig().hExtLanguageType = value;
			mApp->getSplitter()->forEachEditor( [value]( UICodeEditor* editor ) {
				auto& document = editor->getDocument();
				document.setHExtLanguageType( value );
				if ( document.getFileInfo().getExtension() == "h" ) {
					editor->resetSyntaxDefinition();
				}
			} );
		} );
}

void SettingsPanel::filter( PanelState& panel ) {
	String query = panel.search ? panel.search->getText() : String{};
	query.trim().toLower();
	if ( query.size() < 2 )
		query.clear();
	materializeVisibleSettings( panel, query );
	if ( !query.empty() ) {
		panel.pageTitle->setText( mApp->i18n( "search_results", "Search Results" ) );
	} else if ( auto title = panel.categoryTitles.find( panel.selectedCategory );
				title != panel.categoryTitles.end() ) {
		panel.pageTitle->setText( title->second );
	}
	UnorderedSet<std::string> matchingCategories;
	const std::string queryUtf8 = query.toUtf8();
	const bool aggregate = String::endsWith( panel.selectedCategory, ".*" );
	const std::string aggregatePrefix =
		aggregate ? panel.selectedCategory.substr( 0, panel.selectedCategory.size() - 1 )
				  : std::string{};
	if ( !query.empty() ) {
		for ( const auto& setting : panel.model.settings() ) {
			const auto& binding = setting.descriptor;
			if ( String::icontains( binding.name, query ) ||
				 String::icontains( binding.description, query ) ||
				 String::icontains( binding.group, query ) ||
				 String::icontains( binding.id, queryUtf8 ) )
				matchingCategories.insert( binding.category );
		}
	}
	if ( queryUtf8 != panel.categoryFilter ) {
		panel.categoryFilter = queryUtf8;
		if ( auto model = std::static_pointer_cast<SettingsCategoryModel>( panel.categoryModel ) )
			model->filter( queryUtf8, matchingCategories );
	}
	panel.categories->expandAll();
	for ( auto& [category, heading] : panel.categoryHeadings )
		heading->setVisible( query.empty() && aggregate &&
							 String::startsWith( category, aggregatePrefix ) );
	const auto& settings = panel.model.settings();
	for ( size_t i = 0; i < settings.size(); ++i ) {
		const auto& binding = settings[i].descriptor;
		const auto categoryName = panel.categorySearchText.find( binding.category );
		const bool categoryMatches = categoryName != panel.categorySearchText.end() &&
									 String::icontains( categoryName->second, query );
		const bool matches =
			query.empty()
				? binding.category == panel.selectedCategory ||
					  ( aggregate && String::startsWith( binding.category, aggregatePrefix ) )
				: categoryMatches || String::icontains( binding.name, query ) ||
					  String::icontains( binding.description, query ) ||
					  String::icontains( binding.group, query ) ||
					  String::icontains( binding.id, queryUtf8 );
		if ( i < panel.settingViews.size() && panel.settingViews[i].row )
			panel.settingViews[i].row->setVisible( matches );
	}
	for ( const auto& category : panel.model.categories() ) {
		auto section = panel.categorySections.find( category.id );
		if ( section == panel.categorySections.end() )
			continue;
		bool visible = false;
		for ( size_t i = 0; i < settings.size(); ++i ) {
			if ( settings[i].descriptor.category == category.id && i < panel.settingViews.size() &&
				 panel.settingViews[i].row && panel.settingViews[i].row->isVisible() ) {
				visible = true;
				break;
			}
		}
		section->second->setVisible( visible );
	}
	for ( auto& subcategory : panel.subcategoryHeadings ) {
		if ( !subcategory.heading )
			continue;
		const bool hasVisibleSetting = std::any_of(
			settings.begin(), settings.end(), [&panel, &subcategory]( const auto& setting ) {
				const auto& binding = setting.descriptor;
				const size_t index = &setting - panel.model.settings().data();
				return binding.category == subcategory.category &&
					   binding.group == subcategory.name && index < panel.settingViews.size() &&
					   panel.settingViews[index].row && panel.settingViews[index].row->isVisible();
			} );
		subcategory.heading->setVisible( hasVisibleSetting );
	}
}

} // namespace ecode
