#pragma once

#include <eepp/ui/models/model.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Models;

namespace EE::UI {
class UISceneNode;
}

namespace ecode {

class ProcessesModel : public Model {
  public:
	enum Columns { ID, Name };

	ProcessesModel( std::vector<std::pair<Uint64, std::string>>&& processes,
					UISceneNode* sceneNode );

	virtual size_t rowCount( const ModelIndex& ) const;

	virtual size_t columnCount( const ModelIndex& ) const;

	virtual std::string columnName( const size_t& colIdx ) const;

	virtual Variant data( const ModelIndex& modelIndex, ModelRole role ) const;

  protected:
	std::vector<std::pair<Uint64, std::string>> mProcesses;
	UISceneNode* mSceneNode{ nullptr };
};

} // namespace ecode
