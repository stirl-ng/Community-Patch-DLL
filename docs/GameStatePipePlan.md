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
| `move_unit_result` | Response to move_unit | `request_id`, `success`, `result?` or `error?`, `state_delta?` |
| `select_unit_result` | Response to select_unit | `request_id`, `success`, `result?` or `error?`, `state_delta?` |
| `state_refresh` | Response to get_state | `request_id`, `state` |
| `notifications_result` | Response to get_notifications | `request_id`, `player_id`, `count`, `notifications[]` |
| `demographics_result` | Response to get_demographics | `request_id`, `active_player_id`, `players[]`, `rankings` |
| `economic_overview_result` | Response to get_economic_overview | `request_id`, `active_player_id`, `players[]` |
| `player_status_result` | Response to get_player_status | `request_id`, `player_id`, `turn`, `turn_string`, `science`, `gold`, `trade_routes`, `happiness`, `golden_age`, `culture`, `tourism`, `faith` |
| `units_result` | Response to get_units | `request_id`, `player_id`, `turn`, `units[]` |
| `tile_info_result` | Response to inspect_tile | `request_id`, `x`, `y`, `is_visible`, `is_revealed`, terrain/feature/resource/improvement/route/owner/units/city/yields info |
| `cities_result` | Response to get_cities | `request_id`, `player_id`, `turn`, `cities[]` |
| `control_executed` | Response to do_control | `request_id`, `control_type` |
| `can_do_control_result` | Response to can_do_control | `request_id`, `control_type`, `can_do` |
| `turn_end_ack` | Turn successfully ended | `turn`, `player_id`, `forced?` |
| `turn_end_initiated` | Turn end in progress | `turn`, `player_id`, `forced?`, `note` |
| `error` | Error response | `code`, `message`, `request_id?`, varies by error |

### Incoming Messages (Client → DLL)

| Type | Description | Required Fields |
|------|-------------|-----------------|
| `move_unit` | Move a unit to target coordinates | `request_id`, `unit_id`, `to` (array: [x, y]) |
| `select_unit` | Select a unit | `request_id`, `unit_id`, `clear?`, `toggle?`, `sound?` |
| `get_state` | Request current game state | `request_id` |
| `get_notifications` | Get notification history for player | `request_id`, `player_id?` (defaults to active) |
| `get_demographics` | Get stats for all major civs | `request_id` |
| `get_economic_overview` | Get economic data for all major civs | `request_id` |
| `get_player_status` | Get detailed status for a player | `request_id`, `player_id?` (defaults to active) |
| `get_units` | Get all units for a player | `request_id`, `player_id?` (defaults to active) |
| `inspect_tile` | Get detailed information about a map tile | `request_id`, `x`, `y` |
| `get_cities` | Get all cities for a player | `request_id`, `player_id?` (defaults to active) |
| `do_control` | Execute a control command | `request_id`, `control_type` |
| `can_do_control` | Check if a control can be executed | `request_id`, `control_type` |
| `end_turn` | End the current turn | (none) |

### Action Messages

Actions are explicit message types (not wrapped in a generic `action` message) for consistency with query and control messages.

| Message Type | Description | Required Fields | Optional Fields |
|--------------|-------------|----------------|----------------|
| `move_unit` | Move a unit to target coordinates | `request_id`, `unit_id`, `to` (array: [x, y]) | (none) |
| `select_unit` | Select a unit | `request_id`, `unit_id` | `clear` (default: true), `toggle` (default: false), `sound` (default: true) |

### Error Codes

| Code | Context |
|------|---------|
| `CANNOT_END_TURN` | Blocking condition prevents turn end |
| `AI_NOT_READY` | AI still processing |
| `TURN_ALREADY_ENDED` | Turn already ended |
| `TURN_END_FAILED` | doControl failed to end turn |
| `CANNOT_FORCE_END_TURN` | Blocking type prevents forced end |
| `UNKNOWN_MESSAGE_TYPE` | Message type not recognized (covers both unknown actions and queries) |
| `UNIT_NOT_FOUND` | Unit ID not found |
| `UNIT_NOT_OWNED` | Unit does not belong to active player |
| `INVALID_PLOT` | Target coordinates invalid |
| `INVALID_COORDINATES` | Missing/malformed coordinates |
| `INVALID_PLAYER` | Player not alive or invalid |
| `INVALID_CONTROL_TYPE` | Invalid control type specified |
| `CANNOT_DO_CONTROL` | Control cannot be executed at this time |
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

#### `get_player_status` / `player_status_result`

Request:
```json
{
  "type": "get_player_status",
  "request_id": "abc123",
  "player_id": 0
}
```

Response:
```json
{
  "type": "player_status_result",
  "request_id": "abc123",
  "player_id": 0,
  "turn": 15,
  "turn_string": "400 BC",
  "science": {
    "per_turn_times100": 4500
  },
  "gold": {
    "current_times100": 125000,
    "per_turn_times100": 4500
  },
  "trade_routes": {
    "international_used": 3,
    "international_available": 5
  },
  "happiness": {
    "excess": 12,
    "is_empire_unhappy": false
  },
  "golden_age": {
    "turns_remaining": 0,
    "progress_times100": 7500,
    "progress_threshold": 10000,
    "is_active": false
  },
  "culture": {
    "current_times100": 2500,
    "per_turn_times100": 1200,
    "next_policy_cost": 500
  },
  "tourism": {
    "per_turn": 0
  },
  "faith": {
    "current_times100": 5000,
    "per_turn_times100": 800
  }
}
```

Notes:
- `player_id` defaults to active player if not specified
- All values marked `times100` should be divided by 100 for display
- `trade_routes.international_used` actually returns trade units (not routes) for historical reasons

#### `get_units` / `units_result`

Request:
```json
{
  "type": "get_units",
  "request_id": "abc123",
  "player_id": 0
}
```

Response:
```json
{
  "type": "units_result",
  "request_id": "abc123",
  "player_id": 0,
  "turn": 15,
  "units": [
    {
      "id": 12345,
      "x": 42,
      "y": 37,
      "unit_type": 5,
      "unit_type_name": "Warrior",
      "moves_remaining": 2,
      "max_moves": 2,
      "damage": 0,
      "max_hit_points": 20,
      "experience": 5,
      "level": 1,
      "can_move": true,
      "is_combat_unit": true,
      "plot": {
        "terrain_type": 1,
        "feature_type": -1
      }
    }
  ]
}
```

Notes:
- `player_id` defaults to active player if not specified
- Returns all units owned by the player
- `plot` information is included when available

#### `inspect_tile` / `tile_info_result`

Request:
```json
{
  "type": "inspect_tile",
  "request_id": "abc123",
  "x": 42,
  "y": 37
}
```

Response:
```json
{
  "type": "tile_info_result",
  "request_id": "abc123",
  "x": 42,
  "y": 37,
  "is_visible": true,
  "is_revealed": true,
  "terrain_type": 1,
  "feature_type": -1,
  "is_water": false,
  "is_hills": false,
  "is_mountain": false,
  "is_river": true,
  "is_fresh_water": true,
  "is_coastal": false,
  "resource_type": 3,
  "resource_name": "Iron",
  "resource_quantity": 1,
  "improvement_type": 2,
  "improvement_name": "Mine",
  "improvement_pillaged": false,
  "route_type": 1,
  "route_name": "Road",
  "route_pillaged": false,
  "owner_id": 0,
  "owner_name": "Alexander",
  "is_owned_by_active_player": true,
  "units": [
    {
      "id": 12345,
      "owner_id": 0,
      "unit_type": 5,
      "unit_type_name": "Warrior",
      "is_combat_unit": true,
      "is_visible": true
    }
  ],
  "city": {
    "id": 1,
    "name": "Athens",
    "owner_id": 0,
    "population": 5
  },
  "yields": {
    "food": 2,
    "production": 1,
    "gold": 0,
    "science": 0,
    "culture": 0,
    "faith": 0
  }
}
```

Notes:
- Information visibility depends on fog of war (`is_visible`, `is_revealed`)
- `units` and `city` only included if tile is visible
- `yields` only included if tile is owned by active player or has a visible improvement
- Uses revealed versions of owner/improvement/route to respect fog of war

#### `get_cities` / `cities_result`

Request:
```json
{
  "type": "get_cities",
  "request_id": "abc123",
  "player_id": 0
}
```

Response:
```json
{
  "type": "cities_result",
  "request_id": "abc123",
  "player_id": 0,
  "turn": 15,
  "cities": [
    {
      "id": 1,
      "name": "Athens",
      "x": 42,
      "y": 37,
      "population": 5,
      "is_capital": true,
      "is_puppet": false,
      "production": {
        "type": "unit",
        "unit_type": 5,
        "name": "Warrior",
        "turns_left": 3,
        "progress": 50
      },
      "yields_per_turn": {
        "food_times100": 800,
        "production_times100": 500,
        "gold_times100": 200,
        "science_times100": 150,
        "culture_times100": 100,
        "faith_times100": 50
      },
      "food": {
        "stored": 15,
        "turns_to_growth": 5,
        "threshold": 20
      },
      "buildings": [
        {
          "building_type": 10,
          "name": "Monument",
          "count": 1
        }
      ],
      "strength": 10,
      "defense_modifier": 0,
      "founded_turn": 1
    }
  ]
}
```

Notes:
- `player_id` defaults to active player if not specified
- `production` can be `null` if city is not producing anything
- `production.type` is either `"unit"` or `"building"`
- All yield values marked `times100` should be divided by 100 for display
- `buildings` array lists all buildings in the city with their counts

#### `move_unit` / `move_unit_result`

Request:
```json
{
  "type": "move_unit",
  "request_id": "abc123",
  "unit_id": 12345,
  "to": [42, 37]
}
```

Response:
```json
{
  "type": "move_unit_result",
  "request_id": "abc123",
  "success": true,
  "result": {
    "message": "Unit moved"
  },
  "state_delta": {
    "units": [
      {
        "id": 12345,
        "x": 42,
        "y": 37,
        "moves_remaining": 1
      }
    ]
  }
}
```

Notes:
- `to` must be an array with exactly 2 elements: [x, y]
- Unit must exist and be owned by a player
- Target plot must be valid
- Mission is queued immediately; unit will move on next update

#### `select_unit` / `select_unit_result`

Request:
```json
{
  "type": "select_unit",
  "request_id": "abc123",
  "unit_id": 12345,
  "clear": true,
  "toggle": false,
  "sound": true
}
```

Response:
```json
{
  "type": "select_unit_result",
  "request_id": "abc123",
  "success": true,
  "result": {
    "message": "Unit selected"
  },
  "state_delta": {
    "selected_unit": {
      "id": 12345,
      "x": 42,
      "y": 37
    }
  }
}
```

Notes:
- `clear` (default: true) - clear previous selection
- `toggle` (default: false) - toggle selection instead of replacing
- `sound` (default: true) - play selection sound
- Unit must belong to active player

---

## Implementation Status

### ✅ Completed Features

1. **Core Protocol Infrastructure**
   - JSON-based newline-delimited message protocol
   - Named pipe communication (Windows only)
   - Request/response correlation via `request_id`
   - Comprehensive error handling with error codes

2. **Turn Management**
   - `turn_start` and `turn_complete` automatic broadcasts
   - `end_turn` command
   - Turn end blocking detection and reporting

3. **Game State Queries**
   - `get_state` - basic game state (placeholder implementation)
   - `get_notifications` - notification history
   - `get_demographics` - player statistics and rankings
   - `get_economic_overview` - economic data for all major civs
   - `get_player_status` - detailed player status (science, gold, culture, etc.)
   - `get_units` - all units for a player
   - `get_cities` - all cities for a player
   - `inspect_tile` - detailed tile information with fog of war support

4. **Game Actions**
   - `move_unit` - move units to target coordinates (explicit message type)
   - `select_unit` - select units in the game (explicit message type)

5. **Control Commands**
   - `do_control` - execute control commands
   - `can_do_control` - check if control can be executed

6. **Event Broadcasting**
   - `notification` - game notifications
   - `popup` - UI popups
   - `diplomatic_message` - AI diplomacy messages
   - `tech_researched` - technology completion

### 🔄 Future Enhancements

1. **Stabilize GameStatePipe Integration**
   - Update both clang build drivers (`build_vp_clang.py`, `build_vp_clang_sdk.py`) to compile `CvGameCoreDLL_Expansion2/GameStatePipe.cpp`; current scripts hard-code their TU lists and will miss the new source, breaking Clang-Debug/Release.
   - Rebuild all configurations (MSVC + clang), copy the DLL into `(1) Community Patch`, and rerun the smoke tests to ensure telemetry still flows end-to-end.

2. **Enhance Game State**
   - Enrich `get_state` response with full game state (currently placeholder)
   - Add city summaries, unit highlights, and turn-phase info to `turn_start`/`turn_complete` messages
   - Ensure payload size stays reasonable

3. **Additional Actions** (as explicit message types)
   - `set_city_production` - change city production
   - `execute_mission` - execute unit missions
   - `attack` - attack with a unit
   - `fortify` - fortify a unit
   - Additional unit actions as needed
   - Lua hook execution support

4. **Protocol Improvements**
   - Support for quoted values in commands (unit names with spaces)
   - Enhanced error messages with more context
   - Protocol versioning support
