#ifndef EE_CORE_EXECUTOR_HPP
#define EE_CORE_EXECUTOR_HPP

#include <eepp/config.hpp>
#include <eepp/core/small_unique_function.hpp>

namespace EE {

/** @brief Abstract destination for work items.
 *
 * Core code schedules continuations through this interface without knowing whether they run on a
 * thread pool, the UI thread, an I/O thread, or inline. Implementations must run every accepted
 * function exactly once, unless the executor is being destroyed, in which case pending functions
 * may be destroyed without running (their owned state, such as task sources, is then released
 * normally).
 */
class EE_API Executor {
  public:
	/** Inline capacity of an executor work item. Continuations that capture a callable, a task
	 * source and a task state fit without a heap allocation. */
	static constexpr std::size_t FunctionInlineSize = 64;

	using Function = SmallUniqueFunction<void(), FunctionInlineSize>;

	virtual ~Executor() = default;

	virtual void execute( Function function ) = 0;
};

/** @brief Executor that runs every function immediately on the calling thread. */
class EE_API InlineExecutor : public Executor {
  public:
	static InlineExecutor& instance();

	void execute( Function function ) override { function(); }
};

} // namespace EE

#endif
