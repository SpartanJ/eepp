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

	void setFilter( const std::string& filter );

	void setProcesses( std::vector<std::pair<Uint64, std::string>>&& processes );

	std::pair<Uint64, std::string> getProcess( const ModelIndex& index ) const;

  protected:
	std::vector<std::pair<Uint64, std::string>> mProcesses;
	std::vector<std::pair<Uint64, std::string_view>> mProcessesFiltered;
	std::string mFilter;
	UISceneNode* mSceneNode{ nullptr };
};

} // namespace ecode
