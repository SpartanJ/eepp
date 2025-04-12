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

class StackModel : public Model {
  public:
	enum Columns { ID, Name, SourceName, SourcePath, Line, Column };

	StackModel( StackTraceInfo&& stack, UISceneNode* sceneNode );

	virtual size_t rowCount( const ModelIndex& ) const;

	virtual size_t columnCount( const ModelIndex& ) const;

	virtual std::string columnName( const size_t& colIdx ) const;

	virtual Variant data( const ModelIndex& modelIndex, ModelRole role ) const;

	void setStack( StackTraceInfo&& stack );

	void resetStack();

	const StackFrame& getStack( size_t index ) const;

	void setCurrentScopeId( int scope );

  protected:
	StackTraceInfo mStack;
	UISceneNode* mSceneNode{ nullptr };
	int mCurrentScopeId{ 0 };
};

} // namespace ecode
