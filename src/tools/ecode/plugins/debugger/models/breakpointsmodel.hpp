#pragma once

#include "../dap/protocol.hpp"
#include <eepp/core/containers.hpp>
#include <eepp/ui/models/model.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Models;
using namespace ecode::dap;

namespace EE::UI {
class UISceneNode;
}

namespace ecode {

class BreakpointsModel : public Model {
  public:
	enum Columns { Enabled, SourcePath, Line, Remove, Count };

	BreakpointsModel(
		const UnorderedMap<std::string, UnorderedSet<SourceBreakpointStateful>>& breakpoints,
		UISceneNode* sceneNode );

	virtual size_t rowCount( const ModelIndex& ) const;

	virtual size_t columnCount( const ModelIndex& ) const;

	virtual std::string columnName( const size_t& index ) const;

	virtual Variant data( const ModelIndex& modelIndex, ModelRole role ) const;

	void insert( const std::string& filePath, const SourceBreakpointStateful& breakpoint );

	void erase( const std::string& filePath, const SourceBreakpointStateful& breakpoint );

	void enable( const std::string& filePath, const SourceBreakpointStateful& breakpoint,
				 bool enable );

	void move( const std::string& doc, Int64 fromLine, Int64 toLine, Int64 numLines );

	const std::pair<std::string, SourceBreakpointStateful>& get( ModelIndex index );
  protected:
	std::vector<std::pair<std::string, SourceBreakpointStateful>> mBreakpoints;
	UISceneNode* mSceneNode{ nullptr };
};

} // namespace ecode
