## GameStatePipe Command/Protocol Plan

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
