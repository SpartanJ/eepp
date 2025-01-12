#pragma once

#include "../dap/protocol.hpp"
#include <eepp/ui/models/model.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Models;

namespace EE::UI {
class UISceneNode;
}

namespace ecode {

using namespace dap;

class ThreadsModel : public Model {
  public:
	enum Columns { ID };

	ThreadsModel( const std::vector<DapThread>& threads, UISceneNode* sceneNode );

	virtual size_t rowCount( const ModelIndex& ) const;

	virtual size_t columnCount( const ModelIndex& ) const;

	virtual std::string columnName( const size_t& colIdx ) const;

	virtual Variant data( const ModelIndex& modelIndex, ModelRole role ) const;

	void setThreads( std::vector<DapThread>&& threads );

	void resetThreads();

	const DapThread& getThread( size_t index ) const;

	ModelIndex fromThreadId( int id );

	void setCurrentThreadId( int id );

  protected:
	std::vector<DapThread> mThreads;
	UISceneNode* mSceneNode{ nullptr };
	int mCurrentThreadId{ 1 };
};

} // namespace ecode
