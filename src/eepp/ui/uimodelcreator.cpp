#include <eepp/system/filesystem.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/ui/models/csspropertiesmodel.hpp>
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/models/widgettreemodel.hpp>
#include <eepp/ui/uimodelcreator.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

UnorderedMap<std::string, UIModelCreator::ModelCreatorCb> UIModelCreator::modelCreators;
bool UIModelCreator::baseModelsCreated = false;

void UIModelCreator::createBaseModelList() {
	if ( baseModelsCreated )
		return;
	baseModelsCreated = true;

	modelCreators["widgettree"] = []( const std::string&, UIWidget* widget ) {
		if ( !widget )
			return std::shared_ptr<Models::Model>{};
		return std::static_pointer_cast<Models::Model>(
			Models::WidgetTreeModel::New( widget->getUISceneNode() ) );
	};

	modelCreators["filesystem"] = []( const std::string& params, UIWidget* ) {
		if ( !params.empty() )
			return std::static_pointer_cast<Models::Model>(
				Models::FileSystemModel::New( params ) );
		return std::static_pointer_cast<Models::Model>(
			Models::FileSystemModel::New( FileSystem::getCurrentWorkingDirectory() ) );
	};

	modelCreators["diskdrives"] = []( const std::string&, UIWidget* ) {
		return std::static_pointer_cast<Models::Model>( Models::DiskDrivesModel::create() );
	};

	modelCreators["cssproperties"] = []( const std::string&, UIWidget* widget ) {
		return std::static_pointer_cast<Models::Model>(
			Models::CSSPropertiesModel::create( widget ) );
	};
}

std::shared_ptr<Models::Model> UIModelCreator::createFromName( const std::string& name,
															   UIWidget* widget ) {
	createBaseModelList();

	auto fn = FunctionString::parse( name );
	const std::string* modelName;
	std::string param;

	if ( !fn.isEmpty() ) {
		modelName = &fn.getName();
		const auto& params = fn.getParameters();
		if ( !params.empty() )
			param = params[0];
	} else {
		modelName = &name;
	}

	auto it = modelCreators.find( *modelName );
	if ( it == modelCreators.end() )
		return {};

	return it->second( param, widget );
}

void UIModelCreator::addModelCreator( const std::string& name, const ModelCreatorCb& cb ) {
	createBaseModelList();
	modelCreators[name] = cb;
}

void UIModelCreator::removeModelCreator( const std::string& name ) {
	createBaseModelList();
	modelCreators.erase( name );
}

bool UIModelCreator::existsModelCreator( const std::string& name ) {
	createBaseModelList();
	return modelCreators.find( name ) != modelCreators.end();
}

std::vector<std::string> UIModelCreator::getModelNames() {
	createBaseModelList();
	std::vector<std::string> names;
	for ( const auto& pair : modelCreators )
		names.push_back( pair.first );
	return names;
}

}} // namespace EE::UI
