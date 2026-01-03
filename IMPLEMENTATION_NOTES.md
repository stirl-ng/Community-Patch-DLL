# DLL ↔ Orchestrator Bridge Implementation Notes

## Current Implementation Status

### Completed
- ✅ Pipe name updated to `\\.\pipe\civv_llm` (matches orchestrator)
- ✅ Minimal JSON parser added to CvGame.cpp
- ✅ JSON parsing supports: objects, arrays, strings, numbers, booleans, null

### In Progress
- 🔄 Updating HandlePipeCommand to support JSON protocol messages
- 🔄 Implementing protocol message types: action, get_state, end_turn

### Protocol Message Types

#### Incoming (Orchestrator → DLL)
1. **action** - Execute a game action
   ```json
   {"type": "action", "request_id": "uuid", "action": {"kind": "move_unit", "unit_id": 5, "to": [10, 12]}}
   ```

2. **get_state** - Request full game state
   ```json
   {"type": "get_state", "request_id": "uuid"}
   ```

3. **end_turn** - Signal end of turn
   ```json
   {"type": "end_turn"}
   ```

#### Outgoing (DLL → Orchestrator)
1. **turn_start** - Turn begins, send state
   ```json
   {"type": "turn_start", "player_id": 0, "turn": 42, "state": {...}}
   ```

2. **action_result** - Response to action
   ```json
   {"type": "action_result", "request_id": "uuid", "success": true, "result": {...}, "state_delta": {...}}
   ```

3. **turn_end_ack** - Acknowledge turn end
   ```json
   {"type": "turn_end_ack", "turn": 42}
   ```

### Implementation Strategy

Given the size of the existing HandlePipeCommand function (~280 lines), we're taking an incremental approach:

1. Keep existing text-based commands working (list_units, move_unit, unit_action)
2. Add JSON protocol support alongside
3. Test both formats independently
4. Gradually migrate to full JSON protocol

### Next Steps

1. Add JSON message type detection to HandlePipeCommand
2. Implement action message handler
3. Implement get_state handler
4. Implement end_turn handler
5. Update SendTurnData to send turn_start format
6. Test with orchestrator

### Backward Compatibility

The text-based commands will remain functional during transition:
- `list_units player=0`
- `move_unit unit_id=5 x=10 y=12`
- `unit_action unit_id=5 action=sleep`

Once JSON protocol is stable, these can be deprecated.
