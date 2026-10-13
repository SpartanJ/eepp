#include <atomic>
#include <eepp/core/debug.hpp>
#include <eepp/core/task.hpp>

namespace EE {

static void defaultTaskDiagnosticHandler( TaskDiagnostic diagnostic ) {
#ifdef EE_DEBUG
	switch ( diagnostic ) {
		case TaskDiagnostic::Abandoned:
			eePRINTL( "TASK CONTRACT VIOLATION: a TaskSource was destroyed without resolving its "
					  "task. The task chain was abandoned." );
			break;
		case TaskDiagnostic::ResolvedTwice:
			eePRINTL( "TASK CONTRACT VIOLATION: TaskSource::resolve() called on a completed task. "
					  "The value was discarded." );
			break;
		case TaskDiagnostic::TaskRetrievedTwice:
			eePRINTL( "TASK CONTRACT VIOLATION: TaskSource::task() called more than once." );
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
