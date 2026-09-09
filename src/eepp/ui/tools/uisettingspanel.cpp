#include <algorithm>
#include <deque>
#include <eepp/system/log.hpp>
#include <eepp/ui/databinding/uibindinggroup.hpp>
#include <eepp/ui/databinding/uidatabind.hpp>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uidropdownmodellist.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uitreeview.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

using namespace EE::UI::Models;

namespace EE::UI::Tools {

class UISettingsCategoryModel final : public Model {
  public:
	struct Node {
		std::string id;
		std::string text;
		Node* parent{ nullptr };
		std::vector<Node*> children;
		std::vector<Node*> visibleChildren;
	};

	static std::shared_ptr<UISettingsCategoryModel>
	create( const std::vector<std::pair<std::string, std::vector<std::string>>>& categories,
			const UnorderedMap<std::string, std::string>& ids ) {
		return std::make_shared<UISettingsCategoryModel>( categories, ids );
	}

	UISettingsCategoryModel(
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

struct SettingView {
	UIWidget* row{ nullptr };
};

struct SubcategoryHeading {
	std::string category;
	String name;
	UITextView* heading{ nullptr };
};

struct UISettingsPanel::Impl {
	EventConnectionList connections;
	UITextInput* search{ nullptr };
	UITreeView* categories{ nullptr };
	UIScrollView* scroll{ nullptr };
	UILinearLayout* settings{ nullptr };
	UITextView* pageTitle{ nullptr };
	std::shared_ptr<Model> categoryModel;
	UIBindingGroup bindingGroup;
	SettingsModel model;
	std::vector<std::pair<std::string, std::vector<std::string>>> categoryItems;
	UnorderedMap<std::string, std::string> categoryIds;
	UnorderedMap<std::string, String> categorySearchText;
	UnorderedMap<std::string, String> categoryTitles;
	UnorderedMap<std::string, UITextView*> categoryHeadings;
	UnorderedMap<std::string, UIWidget*> categorySections;
	UnorderedMap<std::string, UILinearLayout*> categoryContainers;
	UnorderedSet<std::string> materializedCategories;
	std::vector<SubcategoryHeading> subcategoryHeadings;
	std::vector<SettingView> settingViews;
	std::string selectedCategory;
	std::string categoryFilter;
	String searchResultsText{ "Search Results" };
	bool built{ false };
};

void SettingsModel::clear() {
	mCategories.clear();
	mGroups.clear();
	mSettings.clear();
}

bool SettingsModel::addCategory( SettingsCategory category ) {
	if ( category.id.empty() ||
		 std::any_of( mCategories.begin(), mCategories.end(),
					  [&category]( const auto& item ) { return item.id == category.id; } ) )
		return false;
	mCategories.emplace_back( std::move( category ) );
	return true;
}

bool SettingsModel::addGroup( SettingsGroup group ) {
	if ( !hasCategory( group.category ) )
		return false;
	mGroups.emplace_back( std::move( group ) );
	return true;
}

bool SettingsModel::addSetting( SettingDefinition setting ) {
	if ( setting.descriptor.id.empty() || !hasCategory( setting.descriptor.category ) ||
		 std::any_of( mSettings.begin(), mSettings.end(), [&setting]( const auto& item ) {
			 return item.descriptor.id == setting.descriptor.id;
		 } ) )
		return false;
	mSettings.emplace_back( std::move( setting ) );
	return true;
}

bool SettingsModel::hasCategory( const std::string& id ) const {
	return std::any_of( mCategories.begin(), mCategories.end(),
						[&id]( const auto& item ) { return item.id == id; } );
}

UISettingsPanel* UISettingsPanel::New( UIWidget* parent ) {
	return eeNew( UISettingsPanel, ( parent ) );
}

UISettingsPanel::UISettingsPanel( UIWidget* parent ) :
	UILinearLayout( "settingspanel", UIOrientation::Vertical ), mImpl( std::make_unique<Impl>() ) {
	setParent( parent );
	setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	auto* layout = getUISceneNode()->loadLayoutFromString( SETTINGS_PANEL_LAYOUT, this );
	mImpl->search = layout->find<UITextInput>( "settings_filter" );
	mImpl->categories = layout->find<UITreeView>( "settings_categories" );
	mImpl->settings = layout->find<UILinearLayout>( "settings_rows" );
	mImpl->pageTitle = layout->find<UITextView>( "settings_page_title" );
	mImpl->scroll = layout->find<UIScrollView>( "settings_scroll" );
	mImpl->scroll->setVerticalScrollMode( ScrollBarMode::Auto );
	mImpl->scroll->setHorizontalScrollMode( ScrollBarMode::AlwaysOff );
	disableTabFocusTree( mImpl->categories->getVerticalScrollBar() );
	disableTabFocusTree( mImpl->categories->getHorizontalScrollBar() );
	disableTabFocusTree( mImpl->scroll->getVerticalScrollBar() );
	disableTabFocusTree( mImpl->scroll->getHorizontalScrollBar() );
}

UISettingsPanel::~UISettingsPanel() = default;

SettingsModel& UISettingsPanel::getModel() {
	return mImpl->model;
}

const SettingsModel& UISettingsPanel::getModel() const {
	return mImpl->model;
}

bool UISettingsPanel::addCategory( std::string id, String parent, String name ) {
	if ( mImpl->built || !mImpl->model.addCategory( { id, parent, name } ) )
		return false;
	addCategory( *mImpl, id, parent, name );
	return true;
}

bool UISettingsPanel::addGroup( std::string category, String name ) {
	if ( mImpl->built )
		return false;
	return mImpl->model.addGroup(
		{ std::move( category ), std::move( name ), mImpl->model.settings().size() } );
}

bool UISettingsPanel::addBool( SettingDescriptor descriptor, bool* value,
							   std::function<void( bool )> apply ) {
	return !mImpl->built &&
		   mImpl->model.addSetting(
			   { std::move( descriptor ), BoolPointerSetting{ value, std::move( apply ) } } );
}

bool UISettingsPanel::addBool( SettingDescriptor descriptor, std::function<bool()> get,
							   std::function<void( bool )> set ) {
	return !mImpl->built &&
		   mImpl->model.addSetting(
			   { std::move( descriptor ), BoolSetting{ std::move( get ), std::move( set ) } } );
}

bool UISettingsPanel::addChoice( SettingDescriptor descriptor, std::vector<String> choices,
								 std::function<size_t()> get, std::function<void( size_t )> set,
								 std::vector<String> choiceDescriptions ) {
	return !mImpl->built &&
		   mImpl->model.addSetting(
			   { std::move( descriptor ),
				 ChoiceSetting{ std::move( choices ), std::move( choiceDescriptions ),
								std::move( get ), std::move( set ) } } );
}

bool UISettingsPanel::addEditableChoice( SettingDescriptor descriptor, std::vector<String> choices,
										 std::function<String()> get,
										 std::function<bool( const String& )> set ) {
	return !mImpl->built &&
		   mImpl->model.addSetting( { std::move( descriptor ),
									  EditableChoiceSetting{ std::move( choices ), std::move( get ),
															 std::move( set ) } } );
}

bool UISettingsPanel::addInteger( SettingDescriptor descriptor, int min, int max,
								  std::function<int()> get, std::function<void( int )> set ) {
	return !mImpl->built && min <= max &&
		   mImpl->model.addSetting(
			   { std::move( descriptor ),
				 IntegerSetting{ min, max, std::move( get ), std::move( set ) } } );
}

bool UISettingsPanel::addText( SettingDescriptor descriptor, std::function<std::string()> get,
							   std::function<bool( const std::string& )> set,
							   bool commitOnFocusLoss ) {
	return !mImpl->built &&
		   mImpl->model.addSetting(
			   { std::move( descriptor ),
				 TextSetting{ std::move( get ), std::move( set ), commitOnFocusLoss } } );
}

bool UISettingsPanel::addFloat( SettingDescriptor descriptor, double min, double max, double step,
								std::function<double()> get, std::function<void( double )> set ) {
	return !mImpl->built && min <= max && step > 0 &&
		   mImpl->model.addSetting(
			   { std::move( descriptor ),
				 FloatSetting{ min, max, step, std::move( get ), std::move( set ) } } );
}

bool UISettingsPanel::addAction( SettingDescriptor descriptor, String buttonText,
								 std::function<void()> action ) {
	return !mImpl->built && action &&
		   mImpl->model.addSetting(
			   { std::move( descriptor ),
				 ActionSetting{ std::move( buttonText ), std::move( action ) } } );
}

void UISettingsPanel::build() {
	if ( mImpl->built )
		return;
	mImpl->built = true;
	mImpl->settings->beginAttributesTransaction();
	materializeCategory( *mImpl, mImpl->selectedCategory );
	setupCategories( *mImpl );
	mImpl->settings->endAttributesTransaction();
	selectCategory( *mImpl, mImpl->selectedCategory );
	mImpl->connections += mImpl->search->connect( Event::OnTextChanged, [this]( const Event* ) {
		String query = mImpl->search->getText();
		query.trim();
		const UintPtr debounceTag = reinterpret_cast<UintPtr>( this );
		if ( query.size() < 2 ) {
			mImpl->search->removeActionsByTag( debounceTag );
			filter( *mImpl );
			return;
		}
		mImpl->search->debounce( [this] { filter( *mImpl ); }, Milliseconds( 150 ), debounceTag );
	} );
}

void UISettingsPanel::selectCategory( const std::string& category ) {
	selectCategory( *mImpl, category );
}

void UISettingsPanel::setCategoryEnabled( const std::string& category, bool enabled,
										  const std::string& excludedSetting ) {
	setCategoryEnabled( *mImpl, category, enabled, excludedSetting );
}

void UISettingsPanel::refreshTextSetting( const std::string& id ) {
	refreshTextSetting( *mImpl, id );
}

void UISettingsPanel::setSearchResultsText( String text ) {
	mImpl->searchResultsText = std::move( text );
}

void UISettingsPanel::setFilter( String filterText ) {
	const UintPtr debounceTag = reinterpret_cast<UintPtr>( this );
	mImpl->search->setText( std::move( filterText ) );
	mImpl->search->removeActionsByTag( debounceTag );
	filter( *mImpl );
}

void UISettingsPanel::focusSearch() {
	mImpl->search->setFocus();
	mImpl->search->getDocument().selectAll();
}

void UISettingsPanel::focusCategories() {
	auto selected = mImpl->categories->getSelection().first();
	if ( selected.isValid() )
		mImpl->categories->setSelection( selected );
	mImpl->categories->setFocus();
}

bool UISettingsPanel::isBuilt() const {
	return mImpl->built;
}

void UISettingsPanel::addCategory( Impl& panel, const std::string& id, const String& parent,
								   const String& name ) {
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
	auto* section =
		getUISceneNode()->loadLayoutFromString( SETTINGS_CATEGORY_HEADING_LAYOUT, panel.settings );
	auto* heading = section->find<UITextView>( "settings_category_heading" );
	heading->setText( name );
	heading->setId( "settings_category_" + id );
	panel.categoryHeadings[id] = heading;
	panel.categorySections[id] = section;
	panel.categoryContainers[id] = section->find<UILinearLayout>( "settings_category_rows" );
	if ( panel.selectedCategory.empty() )
		panel.selectedCategory = id;
}

void UISettingsPanel::selectCategory( Impl& panel, const std::string& category ) {
	if ( category.empty() || !panel.categories )
		return;
	String title;
	if ( String::endsWith( category, ".*" ) ) {
		const std::string prefix = category.substr( 0, category.size() - 1 );
		for ( const auto& [parent, children] : panel.categoryItems ) {
			if ( children.empty() )
				continue;
			auto id = panel.categoryIds.find( parent + '/' + children.front() );
			if ( id != panel.categoryIds.end() && String::startsWith( id->second, prefix ) ) {
				title = String::fromUtf8( parent );
				break;
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

void UISettingsPanel::addSubcategoryHeading( Impl& panel, const std::string& category,
											 const String& name ) {
	panel.model.addGroup( { category, name, panel.model.settings().size() } );
}

void UISettingsPanel::setupCategories( Impl& panel ) {
	auto model = UISettingsCategoryModel::create( panel.categoryItems, panel.categoryIds );
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
			auto* node = static_cast<UISettingsCategoryModel::Node*>( index.internalData() );
			if ( !node || node->id.empty() )
				return;
			panel.selectedCategory = node->id;
			panel.pageTitle->setText( node->text );
			filter( panel );
			panel.scroll->getVerticalScrollBar()->setValue( 0, false );
		} );
}

UIWidget* UISettingsPanel::createRow( Impl& panel, SettingDefinition& setting, SettingView& view,
									  pugi::xml_node layout ) {
	auto& binding = setting.descriptor;
	auto container = panel.categoryContainers.find( binding.category );
	eeASSERT( container != panel.categoryContainers.end() );
	auto* row = getUISceneNode()->loadLayoutNodes( layout, container->second, 0 );
	row->setId( "setting_" + binding.id );
	row->find<UITextView>( "setting_name" )->setText( binding.name );
	auto* description = row->find<UITextView>( "setting_description" );
	description->setText( binding.description );
	view.row = row;
	return row;
}

UICheckBox* UISettingsPanel::createBoolControl( Impl& panel, SettingDefinition& setting,
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

void UISettingsPanel::refreshTextSetting( Impl& panel, const std::string& id ) {
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

void UISettingsPanel::materializeCategory( Impl& panel, const std::string& category ) {
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
				getUISceneNode()
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

void UISettingsPanel::materializeVisibleSettings( Impl& panel, const String& query ) {
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

void UISettingsPanel::setCategoryEnabled( Impl& panel, const std::string& category, bool enabled,
										  const std::string& excludedSetting ) {
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

void UISettingsPanel::filter( Impl& panel ) {
	String query = panel.search ? panel.search->getText() : String{};
	query.trim().toLower();
	if ( query.size() < 2 )
		query.clear();
	materializeVisibleSettings( panel, query );
	if ( !query.empty() ) {
		panel.pageTitle->setText( panel.searchResultsText );
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
		if ( auto model = std::static_pointer_cast<UISettingsCategoryModel>( panel.categoryModel ) )
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

} // namespace EE::UI::Tools
