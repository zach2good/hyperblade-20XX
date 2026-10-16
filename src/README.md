# Source

One class per `.h` and `.cpp` pair. The `// FUNCTION: HYPERX 0x...` markers come from the decomp
and name each function's address in the original.

| Folder | What's in it |
|---|---|
| `anim/` | Skeletal animation: clips, bindings and players |
| `app/` | The application shell, configuration, and the bring-up code (`bringup.cpp`) |
| `common/` | Behaviour objects and agenda owners shared by the game objects |
| `engine/` | The game engine, its objects, teams and arenas |
| `graphics/` | DirectDraw, Direct3D and the software renderer |
| `input/` | Keyboard, joystick and cheat input |
| `jpeg/` | Markers for the IJG JPEG routines the original links |
| `math/` | Vectors, matrices and the float helpers |
| `net/` | DirectPlay sessions and network records |
| `physics/` | Collision shapes and responses |
| `planvm/` | The plan VM that runs the AI plans |
| `sound/` | DirectSound and the audio manager |
| `ui/` | Menu screens, widgets and the HUD |

`globals.cpp` and `globals.h` hold the globals no single class owns. The JPEG library itself is
under `third_party/`.
