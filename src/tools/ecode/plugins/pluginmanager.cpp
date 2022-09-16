#include "pluginmanager.hpp"
#include <eepp/ui/uitableview.hpp>

namespace ecode {

PluginManager::PluginManager( const std::string& resourcesPath, const std::string& pluginsPath,
							  std::shared_ptr<ThreadPool> pool ) :
	mResourcesPath( resourcesPath ), mPluginsPath( pluginsPath ), mThreadPool( pool ) {}

PluginManager::~PluginManager() {
	for ( auto& plugin : mPlugins )
		eeDelete( plugin.second );
}

void PluginManager::registerPlugin( const PluginDefinition& def ) {
	mDefinitions[def.id] = def;
}

UICodeEditorPlugin* ecode::PluginManager::get( const std::string& id ) {
	auto findIt = mPlugins.find( id );
	if ( findIt != mPlugins.end() )
		return findIt->second;
	return nullptr;
}

bool PluginManager::setEnabled( const std::string& id, bool enable ) {
	mPluginsEnabled[id] = enable;
	UICodeEditorPlugin* plugin = get( id );
	if ( enable && plugin == nullptr && hasDefinition( id ) ) {
		UICodeEditorPlugin* newPlugin = mDefinitions[id].creatorFn( this );
		mPlugins.insert( std::pair<std::string, UICodeEditorPlugin*>( id, newPlugin ) );
		if ( onPluginEnabled )
			onPluginEnabled( newPlugin );
		return true;
	}
	if ( !enable && plugin != nullptr ) {
		eeSAFE_DELETE( plugin );
		mPlugins.erase( id );
	}
	return false;
}

bool PluginManager::isEnabled( const std::string& id ) const {
	return mPluginsEnabled.find( id ) != mPluginsEnabled.end() ? mPluginsEnabled.at( id ) : false;
}

const std::string& PluginManager::getResourcesPath() const {
	return mResourcesPath;
}

const std::string& PluginManager::getPluginsPath() const {
	return mPluginsPath;
}

const std::map<std::string, bool>& PluginManager::getPluginsEnabled() const {
	return mPluginsEnabled;
}

void PluginManager::onNewEditor( UICodeEditor* editor ) {
	for ( auto& plugin : mPlugins )
		editor->registerPlugin( plugin.second );
}

void PluginManager::setPluginsEnabled( const std::map<std::string, bool>& pluginsEnabled ) {
	mPluginsEnabled = pluginsEnabled;
	for ( const auto& plugin : pluginsEnabled ) {
		if ( plugin.second && get( plugin.first ) == nullptr )
			setEnabled( plugin.first, true );
	}
}

const std::shared_ptr<ThreadPool>& PluginManager::getThreadPool() const {
	return mThreadPool;
}

const std::map<std::string, PluginDefinition>& PluginManager::getDefinitions() const {
	return mDefinitions;
}

const PluginDefinition* PluginManager::getDefinitionIndex( const Int64& index ) const {
	const PluginDefinition* def = nullptr;
	Int64 i = 0;
	for ( const auto& curDef : mDefinitions ) {
		if ( index == i )
			def = &curDef.second;
		++i;
	}
	return def;
}

bool PluginManager::hasDefinition( const std::string& id ) {
	return mDefinitions.find( id ) != mDefinitions.end();
}

std::shared_ptr<PluginsModel> PluginsModel::create( PluginManager* manager ) {
	return std::make_shared<PluginsModel>( manager );
}

size_t PluginsModel::rowCount( const ModelIndex& ) const {
	return mManager->getDefinitions().size();
}

std::string PluginsModel::columnName( const size_t& col ) const {
	eeASSERT( col < mColumnNames.size() );
	return mColumnNames[col];
}

Variant PluginsModel::data( const ModelIndex& index, ModelRole role ) const {
	if ( role == ModelRole::Display ) {
		const PluginDefinition* def = mManager->getDefinitionIndex( index.row() );
		if ( def == nullptr )
			return {};
		switch ( index.column() ) {
			case Columns::Version:
				return Variant( def->versionString.c_str() );
			case Columns::Description:
				return Variant( def->description.c_str() );
			case Columns::Title:
				return Variant( def->name.c_str() );
			case Columns::Id:
				return Variant( def->id.c_str() );
		}
	}
	return {};
}

UIWindow* UIPluginManager::New( UISceneNode* sceneNode, PluginManager* manager ) {
	UIWindow* win = sceneNode
						->loadLayoutFromString( R"xml(
	<window
		id="plugin-manager-window"
		lw="800dp" lh="400dp"
		padding="8dp"
		window-title="Plugin Manager"
		window-flags="default|maximize|shadow"
		window-min-size="300dp 300dp">
		<vbox lw="mp" lh="mp">
			<tableview id="plugin-manager-table" lw="mp" lh="fixed" layout_weight="1">
			</tableview>
			<vbox lw="mp" lh="wc">
				<hbox margin-top="4dp" layout-gravity="right">
					<pushbutton id="plugin-manager-enabled" enabled="false" />
					<!-- <pushbutton id="plugin-manager-preferences" enabled="false" text="Enabled" /> -->
					<pushbutton id="plugin-manager-close" text="Close" icon="close" margin-left="4dp" />
				</hbox>
			</vbox>
		</vbox>
	</window>
	)xml" )
						->asType<UIWindow>();
	UIWidget* cont = win->getContainer();
	UIPushButton* enable = cont->find<UIPushButton>( "plugin-manager-enabled" );
	UIPushButton* close = cont->find<UIPushButton>( "plugin-manager-close" );
	UITableView* tv = win->getContainer()->find<UITableView>( "plugin-manager-table" );
	close->addEventListener( Event::MouseClick, [win]( const Event* event ) {
		const MouseEvent* mevent = static_cast<const MouseEvent*>( event );
		if ( mevent->getFlags() & EE_BUTTON_LMASK )
			win->closeWindow();
	} );

	win->setTitle( sceneNode->i18n( "plugin_manager", "Plugin Manager" ) );
	enable->setText( sceneNode->i18n( "enable", "Select a Plugin" ) );

	auto updateButtonsState = [sceneNode, enable, manager]( const ModelIndex& index ) {
		const PluginDefinition* def = manager->getDefinitionIndex( index.row() );
		if ( def == nullptr )
			return;
		enable->setEnabled( true );
		enable->setText( manager->isEnabled( def->id ) ? sceneNode->i18n( "disable", "Disable" )
													   : sceneNode->i18n( "enable", "Enable" ) );
	};
	enable->addEventListener(
		Event::MouseClick, [updateButtonsState, tv, manager]( const Event* event ) {
			const MouseEvent* mevent = static_cast<const MouseEvent*>( event );
			if ( mevent->getFlags() & EE_BUTTON_LMASK && !tv->getSelection().isEmpty() ) {
				const PluginDefinition* def =
					manager->getDefinitionIndex( tv->getSelection().first().row() );
				if ( def == nullptr )
					return;
				manager->setEnabled( def->id, !manager->isEnabled( def->id ) );
				updateButtonsState( tv->getSelection().first() );
			}
		} );
	tv->setOnSelection( updateButtonsState );
	tv->setModel( PluginsModel::create( manager ) );
	tv->setColumnsVisible(
		{ PluginsModel::Title, PluginsModel::Description, PluginsModel::Version } );
	tv->setAutoColumnsWidth( true );
	tv->setRowHeight( PixelDensity::dpToPx( 64 ) );
	win->center();
	return win;
}

} // namespace ecode
