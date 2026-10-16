#include <eepp/core/executor.hpp>

namespace EE {

InlineExecutor& InlineExecutor::instance() {
	static InlineExecutor executor;
	return executor;
}

} // namespace EE
