# Working with Ghidra and GhidraMCP

There is no shared Ghidra server. Each contributor imports their own copy of `HYPERX.EXE` into a
local Ghidra project (`.gpr`).

The repository, not Ghidra, is the source of truth:

- The C++ source in `src/`, its headers and its markers (`// FUNCTION: HYPERX 0xADDR`) define
  function identities, names, signatures and types.
- `symbols/functions.csv`, `symbols/vtable_gaps.csv` and `symbols/data_gaps.csv` hold the
  function inventory and the known gaps.

Two tools connect a local Ghidra database to the repository:

1. The GhidraMCP HTTP bridge on port 8089 answers queries while Ghidra is open. `GHIDRA_BRIDGE`
   overrides its URL.
2. `tools/publish_ghidra.py` pushes the names, types, vtables and globals from the build into
   a closed Ghidra database.

## Setup

### Create a local Ghidra project

1. Start Ghidra.
2. Select **File, New Project, Non-Shared Project**.
3. Give it a name (for example `hyperx.gpr`) and a local directory.
4. Import the retail `HYPERX.EXE` (v1.0.0.1, MD5 `420c5c49ee2ae5ae9adf329bb2ee5702`).
5. Open the binary in CodeBrowser and run Auto Analysis with the default settings.

### Install GhidraMCP

1. In the project manager, select **File, Install Extensions**, then **+**. Select the
   GhidraMCP zip or directory and restart Ghidra.
2. Open `HYPERX.EXE` in CodeBrowser. Select **File, Configure, Miscellaneous** and enable
   **GhidraMCP**. Older plugin builds list it under Experimental as GhidraMCPPlugin.
3. Check the bridge. The tools reach it at `http://127.0.0.1:8089` unless `GHIDRA_BRIDGE`
   names another URL:

   ```bash
   curl -s http://127.0.0.1:8089/check_connection
   python -c "import sys; sys.path.append('tools'); import ghidra_api; print(ghidra_api.check())"
   ```

   The reply names the loaded program. The name must contain `HYPERX`.

## Pulling from Ghidra

The endpoints below belong to GhidraMCP. Our tools use only `check_connection`,
`list_functions`, `get_function_by_address` and `disassemble_function`. Check the others against
your plugin build.

Decompile a function:

```bash
curl -s "http://127.0.0.1:8089/decompile_function?address=0x00401000"
```

```python
import sys
sys.path.append('tools')
import ghidra_api

print(ghidra_api.get("decompile_function", address="0x00401000"))
```

Get one function's prototype and calling convention:

```bash
curl -s "http://127.0.0.1:8089/get_function_by_address?address=0x00401000"
```

Write every function's address, name, size, convention and signature into
`symbols/functions.csv`:

```bash
python tools/pull_functions.py
```

Cross references:

```bash
curl -s "http://127.0.0.1:8089/get_function_callers?address=0x00401000"
curl -s "http://127.0.0.1:8089/get_function_callees?address=0x00401000"
curl -s "http://127.0.0.1:8089/get_xrefs_to?address=0x00401000"
```

Repository queries that need no Ghidra:

```bash
python tools/name_at.py 0x00401000
python tools/unresolved_calls.py
```

Types and struct layouts:

```bash
curl -s "http://127.0.0.1:8089/get_struct_layout?name=Player"
curl -s "http://127.0.0.1:8089/search_data_types?query=HUD"
```

## Pushing to Ghidra

### Batch publish

`tools/publish_ghidra.py` runs `reccmp` headless. It reads the build PDB and imports names,
demangled signatures, globals, vtables and struct types into your `.gpr`.

1. Build:

   ```bash
   cmake --build build
   ```

   The build writes `build/out/HYPERX_sym.exe` and `build/out/HYPERX_sym.pdb`.
2. Close the Ghidra GUI. CodeBrowser holds an exclusive lock on the project.
3. Publish:

   ```bash
   python tools/publish_ghidra.py --project "C:/path/to/your/hyperx.gpr" --file /HYPERX.EXE
   ```

   - `--project <path.gpr>`: the local `.gpr` file.
   - `--file /HYPERX.EXE`: the program path inside the project. It starts with `/`.
   - `--ghidra <dir>`: the Ghidra root, if `GHIDRA_INSTALL_DIR` is not set.
   - `--dry-run`: check the arguments and paths, and import nothing.

### Live push

With Ghidra open, the GhidraMCP endpoints change one item at a time:

```bash
curl "http://127.0.0.1:8089/rename_function_by_address?address=0x00401000&new_name=HUD__CreateSurfaces"
curl "http://127.0.0.1:8089/set_function_prototype?address=0x00401000&prototype=void+__thiscall+HUD__CreateSurfaces(HUD*+this)"
curl "http://127.0.0.1:8089/rename_global_variable?address=0x004c2000&new_name=g_gameState"
curl "http://127.0.0.1:8089/set_decompiler_comment?address=0x00401000&comment=Some+note"
curl -s "http://127.0.0.1:8089/save_program"
```

An MCP client with GhidraMCP configured calls the same operations as tools:
`decompile_function`, `get_function_by_address`, `rename_function_by_address`,
`set_function_prototype`, `set_decompiler_comment` and `save_program`.

## Rules

1. `tools/publish_ghidra.py` overwrites names and types in the project you pass it. Publish only
   to a project you made for this work, never to a database you keep for reading.
2. Names and comments pushed to Ghidra are plain ASCII.
3. A Ghidra name is a guess at the original author's intent. When a Ghidra signature conflicts
   with the VC6 code or the vtable layout, `reccmp` and the binary's bytes decide.
   [re_corrections.md](re_corrections.md) lists the names known to be wrong.
