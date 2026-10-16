#include <atomic>
#include <eepp/core/debug.hpp>
#include <eepp/core/task.hpp>

namespace EE {

static void defaultTaskDiagnosticHandler( TaskDiagnostic diagnostic ) {
#ifdef EE_DEBUG
	switch ( diagnostic ) {
		case TaskDiagnostic::Abandoned:
			eePRINTL( "TASK CONTRACT VIOLATION: the last TaskResolver of a task was destroyed "
					  "without being called. The task chain was abandoned." );
			break;
		case TaskDiagnostic::ResolvedTwice:
			eePRINTL( "TASK CONTRACT VIOLATION: a TaskResolver was called for a completed task. "
					  "The value was discarded." );
			break;
	}
#else
	(void)diagnostic;
#endif
}

static std::atomic<TaskDiagnostics::Handler> sTaskDiagnosticHandler{
	&defaultTaskDiagnosticHandler };

TaskDiagnostics::Handler TaskDiagnostics::setHandler( Handler handler ) {
	return sTaskDiagnosticHandler.exchange( handler ? handler : &defaultTaskDiagnosticHandler );
}

void TaskDiagnostics::report( TaskDiagnostic diagnostic ) {
	sTaskDiagnosticHandler.load( std::memory_order_acquire )( diagnostic );
}

} // namespace EE
