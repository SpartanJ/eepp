#pragma once

#include <string>

using namespace std::literals;

#define DAP_CONTENT_LENGTH "Content-Length"
#define DAP_SEP "\r\n"

namespace ecode::dap {

constexpr int DAP_SEP_SIZE = 2;
static const auto DAP_TPL_HEADER_FIELD = "%1: %2" DAP_SEP;

static const auto DAP_SEQ = "seq"sv;
static const auto DAP_TYPE = "type"sv;
static const auto DAP_COMMAND = "command"sv;
static const auto DAP_ARGUMENTS = "arguments"sv;
static const auto DAP_BODY = "body"sv;

// capabilities
static const auto DAP_CLIENT_ID = "clientID"sv;
static const auto DAP_CLIENT_NAME = "clientName"sv;
static const auto DAP_ADAPTER_ID = "adapterID"sv;
static const auto DAP_LOCALE = "locale"sv;
static const auto DAP_LINES_START_AT1 = "linesStartAt1"sv;
static const auto DAP_COLUMNS_START_AT2 = "columnsStartAt1"sv;
static const auto DAP_PATH_FORMAT = "pathFormat"sv;
static const auto DAP_SUPPORTS_VARIABLE_TYPE = "supportsVariableType"sv;
static const auto DAP_SUPPORTS_VARIABLE_PAGING = "supportsVariablePaging"sv;
static const auto DAP_SUPPORTS_RUN_IN_TERMINAL_REQUEST = "supportsRunInTerminalRequest"sv;
static const auto DAP_SUPPORTS_MEMORY_REFERENCES = "supportsMemoryReferences"sv;
static const auto DAP_SUPPORTS_PROGRESS_REPORTING = "supportsProgressReporting"sv;
static const auto DAP_SUPPORTS_INVALIDATED_EVENT = "supportsInvalidatedEvent"sv;
static const auto DAP_SUPPORTS_MEMORY_EVENT = "supportsMemoryEvent"sv;

// pathFormat values
static const auto DAP_URI = "uri"sv;
static const auto DAP_PATH = "path"sv;

// type values
static const auto DAP_REQUEST = "request"sv;
static const auto DAP_EVENT = "event"sv;
static const auto DAP_RESPONSE = "response"sv;

// command values
static const auto DAP_INITIALIZE = "initialize"sv;
static const auto DAP_LAUNCH = "launch"sv;
static const auto DAP_ATTACH = "attach"sv;
static const auto DAP_MODULES = "modules"sv;
static const auto DAP_VARIABLES = "variables"sv;
static const auto DAP_SCOPES = "scopes"sv;
static const auto DAP_THREADS = "threads"sv;

// request commands
static const auto DAP_RUN_IN_TERMINAL = "runInTerminal";
static const auto DAP_START_DEBUGGING = "startDebugging";

// event values
static const auto DAP_OUTPUT = "output"sv;

// fields
static const auto DAP_NAME = "name"sv;
static const auto DAP_VALUE = "value"sv;
static const auto DAP_SYSTEM_PROCESS_ID = "systemProcessId"sv;
static const auto DAP_IS_LOCAL_PROCESS = "isLocalProcess"sv;
static const auto DAP_POINTER_SIZE = "pointerSize"sv;
static const auto DAP_START_METHOD = "startMethod"sv;
static const auto DAP_DATA = "data"sv;
static const auto DAP_VARIABLES_REFERENCE = "variablesReference"sv;
static const auto DAP_SOURCE = "source"sv;
static const auto DAP_GROUP = "group"sv;
static const auto DAP_LINE = "line"sv;
static const auto DAP_COLUMN = "column"sv;
static const auto DAP_PRESENTATION_HINT = "presentationHint"sv;
static const auto DAP_SOURCES = "sources"sv;
static const auto DAP_CHECKSUMS = "checksums"sv;
static const auto DAP_CATEGORY = "category"sv;
static const auto DAP_THREAD_ID = "threadId"sv;
static const auto DAP_ID = "id"sv;
static const auto DAP_MODULE_ID = "moduleId"sv;
static const auto DAP_REASON = "reason"sv;
static const auto DAP_FRAME_ID = "frameId"sv;
static const auto DAP_FILTER = "filter"sv;
static const auto DAP_START = "start"sv;
static const auto DAP_COUNT = "count"sv;
static const auto DAP_SINGLE_THREAD = "singleThread"sv;
static const auto DAP_ALL_THREADS_CONTINUED = "allThreadsContinued"sv;
static const auto DAP_SOURCE_REFERENCE = "sourceReference"sv;
static const auto DAP_BREAKPOINTS = "breakpoints"sv;
static const auto DAP_ADAPTER_DATA = "adapterData"sv;
static const auto DAP_CONDITION = "condition"sv;
static const auto DAP_HIT_CONDITION = "hitCondition"sv;
static const auto DAP_LOG_MESSAGE = "logMessage"sv;
static const auto DAP_LINES = "lines"sv;
static const auto DAP_ORIGIN = "origin"sv;
static const auto DAP_CHECKSUM = "checksum"sv;
static const auto DAP_ALGORITHM = "algorithm"sv;
static const auto DAP_BREAKPOINT = "breakpoint"sv;
static const auto DAP_EXPRESSION = "expression"sv;
static const auto DAP_CONTEXT = "context"sv;
static const auto DAP_RESULT = "result"sv;
static const auto DAP_TARGET_ID = "targetId"sv;
static const auto DAP_END_LINE = "endLine"sv;
static const auto DAP_END_COLUMN = "endColumn"sv;

} // namespace dap
