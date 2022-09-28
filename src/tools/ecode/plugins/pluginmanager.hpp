#ifndef ECODE_PLUGINMANAGER_HPP
#define ECODE_PLUGINMANAGER_HPP

#include <eepp/ui/models/model.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <memory>
#include <string>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Models;

namespace ecode {

class PluginManager;

typedef std::function<UICodeEditorPlugin*( const PluginManager* pluginManager )> PluginCreatorFn;

struct PluginDefinition {
	std::string id;
	std::string name;
	std::string description;
	PluginCreatorFn creatorFn;
	int versionNumber{ 0 };
	std::string versionString{ "0" };
};

class PluginManager {
  public:
	PluginManager( const std::string& resourcesPath, const std::string& pluginsPath,
				   std::shared_ptr<ThreadPool> pool );

	~PluginManager();

	void registerPlugin( const PluginDefinition& def );

	UICodeEditorPlugin* get( const std::string& id );

	bool setEnabled( const std::string& id, bool enable );

	bool isEnabled( const std::string& id ) const;

	const std::string& getResourcesPath() const;

	const std::string& getPluginsPath() const;

	const std::map<std::string, bool>& getPluginsEnabled() const;

	void onNewEditor( UICodeEditor* editor );

	void setPluginsEnabled( const std::map<std::string, bool>& pluginsEnabled );

	const std::shared_ptr<ThreadPool>& getThreadPool() const;

	std::function<void( UICodeEditorPlugin* )> onPluginEnabled;

	const std::map<std::string, PluginDefinition>& getDefinitions() const;

	const PluginDefinition* getDefinitionIndex( const Int64& index ) const;

  protected:
	std::string mResourcesPath;
	std::string mPluginsPath;
	std::map<std::string, UICodeEditorPlugin*> mPlugins;
	std::map<std::string, bool> mPluginsEnabled;
	std::map<std::string, PluginDefinition> mDefinitions;
	std::shared_ptr<ThreadPool> mThreadPool;

	bool hasDefinition( const std::string& id );
};

class PluginsModel : public Model {
  public:
	enum Columns { Id, Title, Enabled, Description, Version };

	static std::shared_ptr<PluginsModel> New( PluginManager* manager );

	PluginsModel( PluginManager* manager ) : mManager( manager ) {}

	virtual ~PluginsModel() {}

	virtual size_t rowCount( const ModelIndex& ) const;

	virtual size_t columnCount( const ModelIndex& ) const { return mColumnNames.size(); }

	virtual std::string columnName( const size_t& col ) const;

	virtual void setColumnName( const size_t& index, const std::string& name ) {
		eeASSERT( index <= Columns::Version );
		mColumnNames[index] = name;
	}

	virtual Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const;

	virtual void update() { onModelUpdate(); }

	PluginManager* getManager() const;

  protected:
	PluginManager* mManager;
	std::vector<std::string> mColumnNames{ "Id", "Title", "Enabled", "Description", "Version" };
};

class UIPluginManager {
  public:
	static UIWindow* New( UISceneNode* sceneNode, PluginManager* manager,
						  std::function<void( const std::string& )> loadFileCb );
};

} // namespace ecode

#endif // ECODE_PLUGINMANAGER_HPP
