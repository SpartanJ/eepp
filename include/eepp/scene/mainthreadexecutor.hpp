#ifndef EE_SCENE_MAINTHREADEXECUTOR_HPP
#define EE_SCENE_MAINTHREADEXECUTOR_HPP

#include <eepp/core/executor.hpp>
#include <memory>

namespace EE { namespace Scene {

class Node;

/** @brief Executor that runs functions on the main (UI) thread.
 *
 * Functions are queued and drained in submission order by a single Node::runOnMainThread()
 * runnable per batch, so bursts of continuations cost one scheduled action. Execution is always
 * deferred to the next scene update, even when execute() is called from the main thread, which is
 * what Task::then() requires.
 *
 * The dispatcher node must outlive the executor, or shutdown() must be called before the node is
 * destroyed. Destroying the executor drops queued functions that have not run yet (releasing what
 * they own, for example abandoning the tasks of the resolvers they hold).
 */
class EE_API MainThreadExecutor : public Executor {
  public:
	explicit MainThreadExecutor( Node* dispatcher );

	/** Invokes wake after scheduling each batch, on the submitting thread and outside executor
	 * locks. The callback must be nonblocking and safe to call from concurrent submitters. Its
	 * wake destination must remain valid until any concurrent execute() calls have returned. */
	MainThreadExecutor( Node* dispatcher, Function wake );

	~MainThreadExecutor() override;

	MainThreadExecutor( const MainThreadExecutor& ) = delete;

	MainThreadExecutor& operator=( const MainThreadExecutor& ) = delete;

	void execute( Function function ) override;

	/** Detaches the executor from its dispatcher node and drops functions still in its queue.
	 * Waits for in-flight scheduling on the node. A batch already taken by a scene update, and
	 * any pending wake callbacks, can still run. The executor itself must outlive execute(). */
	void shutdown();

	/** @return Number of functions waiting for the main thread. */
	size_t pending() const;

  private:
	struct State;
	std::shared_ptr<State> mState;
};

}} // namespace EE::Scene

#endif
