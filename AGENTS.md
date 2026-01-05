# Repository Guidelines

## Project Structure & Module Organization
- `CvGameCoreDLL_Expansion2/` — primary C++ gameplay sources, headers, and the `VoxPopuli.vcxproj` solution used to build the DLL.  
- `CvGameCoreDLLUtil/`, `ThirdPartyLibs/`, `docs/`, and `Mod/` — supporting utilities, external dependencies, documentation, and packaged mod assets.  
- `build_vp_clang.py` plus `.pyproj` files — automation scripts for clang-based builds; keep them in sync with MSVC settings before use.

## Build, Test, and Development Commands
- `msbuild CvGameCoreDLL_Expansion2/VoxPopuli.vcxproj /p:Configuration=Release /m` — canonical Windows build; mirrors in-game shipping DLL expectations.  
- `msbuild ... /p:Configuration=Debug` — enables assertions and logging when iterating locally.  
- `python build_vp_clang.py --config release` — optional clang pipeline; run only after MSVC builds to cross-check warnings.  
- No automated tests live in-tree; manual validation is performed in-game using hotseat saves from `BuildOutput/`.

## Coding Style & Naming Conventions
- Follow existing Firaxis style: tabs for indentation in legacy files, spaces in newer modules; match the surrounding file before editing.  
- Prefer CamelCase for classes (`GameStatePipe`), PascalCase for functions, and `m_` prefixes for member variables.  
- Keep includes alphabetized, guard platform-specific sections with `#if defined(...)`, and avoid introducing STL features unavailable to VS2013.  
- Run `clang-format` only when a file already follows that layout; otherwise, format manually to avoid massive diffs.

## VS2013 C++11 Compatibility Warnings
**CRITICAL**: This codebase targets Visual Studio 2013, which has **partial and unreliable C++11 support**. Avoid these patterns:

- **`std::function`**: Not reliably supported. Use function pointers, macros, or inline code instead.
- **Complex lambdas with captures**: May fail to compile or cause cascading parse errors. Simple lambdas may work, but test carefully.
- **Other advanced C++11 features**: Test thoroughly before relying on them.

**When you encounter `std::function` or lambda errors:**
1. Replace with macros (e.g., `#define CALC_RANKING(...)` for repetitive logic)
2. Use function pointers for callbacks
3. Inline the logic directly
4. Use templates (but verify VS2013 compatibility)

**Example**: The `get_demographics` handler in `CvGame.cpp` originally used `std::function<long(const PlayerDemoStats&)>` with lambdas, which caused cascading compilation errors. The fix was to replace it with a `CALC_RANKING` macro that inlines the ranking calculation logic.

**Key Takeaway**: Prefer C++98/03-compatible patterns, or thoroughly test any C++11 features before committing.

## Testing Guidelines
- Smoke-test every DLL change in-game: load a standard map, advance at least five turns, and interact with the systems you touched.  
- When touching AI or networking code, capture autosaves from `/Documents/My Games/Sid Meier's Civilization 5/Saves/` and attach repro steps in the PR.  
- If you add Lua hooks, exercise them via FireTuner scripts before shipping.

## Commit & Pull Request Guidelines
- Use imperative commit subjects (e.g., `Add GameStatePipe logging`) with concise bodies describing risk/validation.  
- Each PR should include: summary of changes, testing notes (saves, turn counts, build configs), and any relevant screenshot/log attachments.  
- Link to tracker issues where applicable; squash fixup commits before requesting review.  
- CI is not available, so reviewers expect you to state the MSVC configuration used and whether gameplay saves were validated.
