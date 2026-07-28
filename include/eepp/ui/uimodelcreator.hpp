#ifndef EE_UI_UIMODELCREATOR_HPP
#define EE_UI_UIMODELCREATOR_HPP

#include <eepp/core.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace EE { namespace UI {

class UIWidget;

namespace Models {
class Model;
}

class EE_API UIModelCreator {
  public:
	typedef std::function<std::shared_ptr<Models::Model>( const std::string&, UIWidget* )>
		ModelCreatorCb;

	static std::shared_ptr<Models::Model> createFromName( const std::string& name,
														  UIWidget* widget = nullptr );

	static void addModelCreator( const std::string& name, const ModelCreatorCb& cb );

	static void removeModelCreator( const std::string& name );

	static bool existsModelCreator( const std::string& name );

	static std::vector<std::string> getModelNames();

  protected:
	static UnorderedMap<std::string, ModelCreatorCb> modelCreators;

	static void createBaseModelList();

	static bool baseModelsCreated;
};

}} // namespace EE::UI

#endif
