## GameStatePipe Command/Protocol Plan

---

## Protocol Reference

All messages are newline-delimited JSON objects.

### Outgoing Messages (DLL → Client)

| Type | Description | Key Fields |
|------|-------------|------------|
| `turn_start` | Sent at beginning of player's turn | `player_id`, `turn`, `player_name?`, `is_human?`, `state` |
| `turn_complete` | Sent when turn processing finishes | `turn`, `player_id`, `player_name`, `is_human`, `state` |
| `notification` | Game notification triggered | `player_id`, `notification_type`, `lookup_index`, `turn`, `message?`, `summary?`, `x?`, `y?`, `game_data_index?`, `extra_game_data?` |
| `popup` | UI popup triggered | `popup_type`, `turn`, `data1?`, `data2?`, `data3?`, `flags?`, `option1?`, `option2?`, `text?` |
| `diplomatic_message` | AI diplomacy message | `player`, `diplo_ui_state`, `animation`, `turn`, `message?`, `data1?`, `deal?` |
| `tech_researched` | Technology completed | `player`, `tech`, `partial`, `turn`, `tech_name?` |
| `action_result` | Response to action command | `request_id`, `success`, `result?` or `error?`, `state_delta?` |
| `state_refresh` | Response to get_state | `request_id`, `state` |
| `notifications_result` | Response to get_notifications | `request_id`, `player_id`, `count`, `notifications[]` |
| `demographics_result` | Response to get_demographics | `request_id`, `active_player_id`, `players[]`, `rankings` |
| `economic_overview_result` | Response to get_economic_overview | `request_id`, `active_player_id`, `players[]` |
| `turn_end_ack` | Turn successfully ended | `turn`, `player_id`, `forced?` |
| `turn_end_initiated` | Turn end in progress | `turn`, `player_id`, `forced?`, `note` |
| `error` | Error response | `code`, `message`, varies by error |

### Incoming Messages (Client → DLL)

| Type | Description | Required Fields |
|------|-------------|-----------------|
| `action` | Execute game action | `request_id`, `action.kind`, action-specific fields |
| `get_state` | Request current game state | `request_id` |
| `get_notifications` | Get notification history for player | `request_id`, `player_id?` (defaults to active) |
| `get_demographics` | Get stats for all major civs | `request_id` |
| `get_economic_overview` | Get economic data for all major civs | `request_id` |
| `end_turn` | End the current turn | (none) |
| `forced_end_turn` | Force end turn (bypasses some blocks) | (none) |

### Action Kinds

| Kind | Description | Fields |
|------|-------------|--------|
| `move_unit` | Move a unit | `unit_id`, `to` (array: [x, y]) |

### Error Codes

| Code | Context |
|------|---------|
| `CANNOT_END_TURN` | Blocking condition prevents turn end |
| `AI_NOT_READY` | AI still processing |
| `TURN_ALREADY_ENDED` | Turn already ended |
| `TURN_END_FAILED` | doControl failed to end turn |
| `CANNOT_FORCE_END_TURN` | Blocking type prevents forced end |
| `UNKNOWN_ACTION` | Action kind not implemented |
| `UNIT_NOT_FOUND` | Unit ID not found |
| `INVALID_PLOT` | Target coordinates invalid |
| `INVALID_COORDINATES` | Missing/malformed coordinates |
| `UNKNOWN_MESSAGE_TYPE` | Unrecognized message type |
| `INVALID_JSON` | Failed to parse JSON |

### Message Schemas

#### `get_notifications` / `notifications_result`

Request:
```json
{
  "type": "get_notifications",
  "request_id": "abc123",
  "player_id": 0
}
```

Response:
```json
{
  "type": "notifications_result",
  "request_id": "abc123",
  "player_id": 0,
  "count": 25,
  "notifications": [
    {
      "index": 0,
      "id": 12345,
      "turn": 15,
      "message": "Your Warrior has been promoted!",
      "summary": "Unit Promoted",
      "dismissed": false
    }
  ]
}
```

Notes:
- Returns ALL notifications (up to 150 max per player), including dismissed ones
- `player_id` defaults to active player if not specified
- Notifications are in-memory only, not persisted to database

#### `get_demographics` / `demographics_result`

Request:
```json
{
  "type": "get_demographics",
  "request_id": "abc123"
}
```

Response:
```json
{
  "type": "demographics_result",
  "request_id": "abc123",
  "active_player_id": 0,
  "players": [
    {
      "player_id": 0,
      "name": "Alexander",
      "civ": "Greek Empire",
      "is_human": true,
      "stats": {
        "population": 1500000,
        "food": 42,
        "production": 35,
        "gold": 250,
        "land": 28,
        "military": 450,
        "approval": 12,
        "literacy": 8
      }
    }
  ],
  "rankings": {
    "population": { "best": 2000000, "best_player": 1, "worst": 500000, "worst_player": 3, "average": 1200000 },
    "food": { "best": 42, "best_player": 0, "worst": 15, "worst_player": 2, "average": 28 },
    "production": { ... },
    "gold": { ... },
    "land": { ... },
    "military": { ... },
    "approval": { ... },
    "literacy": { ... }
  }
}
```

Notes:
- Returns stats for ALL alive major civs (enables full ranking calculations)
- Stats match the Demographics UI screen calculations
- `literacy` = number of techs known by the player's team

#### `get_economic_overview` / `economic_overview_result`

Request:
```json
{
  "type": "get_economic_overview",
  "request_id": "abc123"
}
```

Response:
```json
{
  "type": "economic_overview_result",
  "request_id": "abc123",
  "active_player_id": 0,
  "players": [
    {
      "player_id": 0,
      "name": "Alexander",
      "civ": "Greek Empire",
      "is_human": true,
      "treasury": {
        "current_gold": 125000,
        "gross_income": 15000,
        "net_income": 4500,
        "lifetime_gross": 2500000
      },
      "income": {
        "cities": 8500,
        "city_connections": 3500,
        "international_trade": 1200,
        "diplomacy": 1500,
        "religion": 800,
        "traits": 0,
        "minor_civs": 500,
        "vassal_taxes": 0
      },
      "expenses": {
        "units": 6000,
        "buildings": 4000,
        "improvements": 800,
        "vassal_maintenance": 0,
        "vassal_tax_owed": 0,
        "diplomacy": 0
      },
      "units": {
        "total": 25,
        "maintenance_free": 5
      },
      "trade_routes": {
        "internal": 8
      }
    }
  ]
}
```

Notes:
- All gold values in "times 100" format (divide by 100 for display)
- Returns ALL alive major civs (consistent with demographics pattern)
- Matches the data shown in Economic Overview UI screen
- `income.diplomacy` = positive diplomatic income, `expenses.diplomacy` = negative diplomatic costs

---

## Roadmap

1. **Stabilize GameStatePipe Integration**
   - Update both clang build drivers (`build_vp_clang.py`, `build_vp_clang_sdk.py`) to compile `CvGameCoreDLL_Expansion2/GameStatePipe.cpp`; current scripts hard-code their TU lists and will miss the new source, breaking Clang-Debug/Release.
   - Rebuild all configurations (MSVC + clang), copy the DLL into `(1) Community Patch`, and rerun the smoke tests for `list_units`, `move_unit`, and `unit_action` to ensure telemetry still flows end-to-end.

2. **Harden the Command Channel**
   - Extend the parser so commands can include quoted values (unit names with spaces) and produce clearer error codes when validation fails.
   - Add optional request IDs/sequence numbers to every command and echo them back in responses so clients can correlate async replies.
   - Document the protocol (verbs, required/optional args, JSON response schema, delimiter rules) so external clients have a reference.

3. **Expand Gameplay Controls & Telemetry**
   - Enrich `SendTurnData` (or add a `query_state` verb) with city summaries, unit highlights, and turn-phase info, ensuring payload size stays sane.
   - Implement additional verbs: `list_cities`, `set_city_production`, `execute_mission <mission_id>`, `select_unit`, `lua <hook>`—building out the dispatcher incrementally.
   - Define when automatic broadcasts fire (start/end turn, after each command) so controllers always receive timely updates without flooding the pipe.
