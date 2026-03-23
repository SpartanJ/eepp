import sys
import json
import time

def send_notification(method, params):
    msg = {"jsonrpc": "2.0", "method": method, "params": params}
    sys.stdout.write(json.dumps(msg) + "\n")
    sys.stdout.flush()

def send_request(msg_id, method, params):
    msg = {"jsonrpc": "2.0", "id": msg_id, "method": method, "params": params}
    sys.stdout.write(json.dumps(msg) + "\n")
    sys.stdout.flush()

def main():
    last_req_id = 1000
    
    for line in sys.stdin:
        try:
            req = json.loads(line)
            method = req.get("method")
            msg_id = req.get("id")

            if not method and "result" in req:
                # This is a response to one of our requests (like terminal/create)
                result = req.get("result")
                if "terminalId" in result:
                    term_id = result["terminalId"]
                    # Now link this terminal to our tool call
                    send_notification("session/update", {
                        "sessionId": "test-session",
                        "update": {
                            "sessionUpdate": "tool_call_update",
                            "toolCallId": "build-1",
                            "status": "running",
                            "terminalId": term_id
                        }
                    })
                    # Wait 2 seconds then complete it
                    time.sleep(2)
                    send_notification("session/update", {
                        "sessionId": "test-session",
                        "update": {
                            "sessionUpdate": "tool_call_update",
                            "toolCallId": "build-1",
                            "status": "completed",
                            "rawOutput": "Build successful: 0 errors, 0 warnings"
                        }
                    })
                    # Final message
                    send_notification("session/update", {
                        "sessionId": "test-session",
                        "update": {
                            "sessionUpdate": "agent_message_chunk",
                            "content": {"text": "\n\nAll tasks finished successfully!"}
                        }
                    })
                continue

            if method == "initialize":
                sys.stdout.write(json.dumps({"jsonrpc": "2.0", "id": msg_id, "result": {
                    "protocolVersion": 1,
                    "agentCapabilities": {"loadSession": True}
                }}) + "\n")
                sys.stdout.flush()
            
            elif method == "session/new":
                sys.stdout.write(json.dumps({"jsonrpc": "2.0", "id": msg_id, "result": {
                    "sessionId": "test-session", "configOptions": []
                }}) + "\n")
                sys.stdout.flush()

                # Send available commands
                send_notification("session/update", {
                    "sessionId": "test-session",
                    "update": {
                        "sessionUpdate": "available_commands_update",
                        "availableCommands": [
                            {"name": "search", "description": "Search in codebase"},
                            {"name": "build", "description": "Build the project"},
                            {"name": "test", "description": "Run tests"}
                        ]
                    }
                })

            elif method == "session/prompt":
                # 1. Send Plan
                send_notification("session/update", {
                    "sessionId": "test-session",
                    "update": {
                        "sessionUpdate": "plan",
                        "plan": {
                            "entries": [
                                {"title": "Analyzing project structure", "status": "completed"},
                                {"title": "Running build tests", "status": "running"},
                                {"title": "Deploying changes", "status": "pending"}
                            ]
                        }
                    }
                })

                # 2. Tool Call 1 (Synchronous style)
                send_notification("session/update", {
                    "sessionId": "test-session",
                    "update": {
                        "sessionUpdate": "tool_call",
                        "toolCallId": "search-1",
                        "title": "Searching for UI components",
                        "status": "completed",
                        "rawInput": {"pattern": "UIButton", "dir": "src/"},
                        "rawOutput": ["src/ui/button.cpp", "src/ui/button.hpp"]
                    }
                })

                # 3. Thought
                send_notification("session/update", {
                    "sessionId": "test-session",
                    "update": {
                        "sessionUpdate": "agent_thought_chunk",
                        "content": {"text": "I found the buttons. Now I will try to build the project."}
                    }
                })

                # 4. Tool Call 2 (Terminal style)
                send_notification("session/update", {
                    "sessionId": "test-session",
                    "update": {
                        "sessionUpdate": "tool_call",
                        "toolCallId": "build-1",
                        "title": "Compiling project",
                        "status": "running"
                    }
                })

                # 5. Request Terminal for Tool Call 2
                last_req_id += 1
                send_request(last_req_id, "terminal/create", {
                    "sessionId": "test-session",
                    "command": "make",
                    "args": ["-j4"]
                })

                # Confirm prompt request
                sys.stdout.write(json.dumps({"jsonrpc": "2.0", "id": msg_id, "result": {"stopReason": "complete"}}) + "\n")
                sys.stdout.flush()

        except Exception as e:
            sys.stderr.write(f"Error: {str(e)}\n")

if __name__ == "__main__":
    main()
