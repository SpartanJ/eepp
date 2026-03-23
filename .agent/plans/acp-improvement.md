# Plan: ACP Support Improvement for ecode

This plan outlines the steps to complete and improve the Agent Client Protocol (ACP) implementation in `ecode`.

## Current Status & Gaps
The current implementation has the basic structure but misses several critical features for a full ACP experience:
- **Terminal output** is not captured (returns empty).
- **Tool call updates** are not handled in the UI.
- **Plan schema** is non-standard.
- **Slash commands** are not exposed to the user.
- **Terminal limits** are ignored.

## Proposed Steps

### 1. Research & Refinement (Done)
- [x] Analyze ACP documentation.
- [x] Review current `ecode` implementation.
- [x] Identify missing pieces.

### 2. Phase 1: Terminal Output Capture (Done)
We need a way to capture the output from the terminal process to satisfy `terminal/output` requests.
- [x] Modify `src/modules/eterm/include/eterm/terminal/terminalemulator.hpp`:
    - Add `using DataCb = std::function<void(const char*, size_t)>;`
    - Add `void setDataCb(DataCb cb);`
- [x] Modify `src/modules/eterm/src/eterm/terminal/terminalemulator.cpp`:
    - Call `mDataCb` in `ttyread()` when new data is received.
- [x] Update `src/tools/ecode/plugins/aiassistant/acp/agentsession.hpp`:
    - Add an output buffer to `TermData`.
- [x] Update `src/tools/ecode/plugins/aiassistant/acp/agentsession.cpp`:
    - Set the `DataCb` in `onTerminalCreated`.
    - Implement `onTerminalOutput` to return and clear the buffer.
    - Respect `outputByteLimit`.

### 3. Phase 2: UI & Protocol Updates
- [x] Update `src/tools/ecode/plugins/aiassistant/chatui.cpp`:
    - Fix `plan` update handling to use the standard `entries` schema.
    - Implement `tool_call_update` handling.
    - Update tool call UI to reflect status (pending, in_progress, completed, failed, cancelled).
- [x] Implement embedded terminals in tool calls:
    - When a `tool_call_update` contains a `terminalId`, link the terminal UI to that tool call bubble if possible.

### 4. Phase 3: UX Improvements
- [ ] Expose slash commands:
    - Show `mAvailableCommands` in the UI.
    - Add simple autocompletion for `/` commands in `mChatInput`.
- [ ] Enhance terminal management:
    - Ensure `terminal/wait_for_exit` works correctly (async).

### 5. Phase 4: Validation
- [ ] Test with a compatible ACP agent.
- [ ] Verify file system tools.
- [ ] Verify terminal creation and output reading.
- [ ] Verify plan updates.
- [ ] Verify tool call permissions.
