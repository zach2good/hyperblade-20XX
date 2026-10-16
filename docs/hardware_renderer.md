# Hardware renderer plan

The game keeps its original software renderer and gains a hardware one on the SDL GPU API. The
player can swap between them at any time, and turn the hardware renderer's features on and off.
The software renderer has no features: it is the original look, and the fallback where there is
no GPU device.

## What the original did

The engine transforms, clips, lights and sorts every face on the CPU. Lighting is flat, one
directional light per face. Hidden faces are removed by painter's order: there is no z-buffer.
Its Direct3D path took the finished screen-space triangles (`D3DTLVERTEX`) in execute buffers and
blended textures with `MODULATE`. That path was never reachable in this build, and none of it
carries over.

Model textures are 333 24-bit BMPs in `Data/Textures`, square powers of two from 16 to 256. The
textured models are in `Data/Models/Mdltxt` and the shaded ones in `Data/Models/Mdlshd`. The
software renderer never textured: it needed `mdmtex.dll`, which never shipped.

## Shape

- One `SDL_GPUDevice` and one window. The 2D presenter is made on that device with
  `SDL_CreateGPURenderer`, so the menus, the HUD layer and the software frame draw through it
  unchanged.
- Where no GPU device can be made (the web, or a GPU without Vulkan, D3D12 or Metal), the
  presenter falls back to a plain `SDL_CreateRenderer` and only the software renderer is offered.
- The hardware renderer draws the 3D scene into its own colour and depth targets. The result is
  wrapped as an `SDL_Texture` and drawn under the HUD layer, so nothing above it knows which
  renderer ran.
- Meshes are uploaded once: positions, normals and UVs. Each frame the engine builds a draw list
  of mesh, material and world matrix, and the vertex shader projects with the engine's cameras.
- GPU resources are built the first time the hardware renderer runs and kept, so swapping is free
  after that.

## Model sets

`Mdltxt` is not `Mdlshd` with UVs added. Only 15 of 171 models share their geometry: the skaters
were retopologised and 77 of their rigs moved, the arena shell is taller, and textured props have
far fewer faces. Most textured materials have colour 0, so the textured set drawn untextured comes
out black. Two `Mdlshd` files are corrupt, CHISOM and NEMPTM.

So both sets are loaded, and the texture switch picks which one the hardware renderer draws:
`Mdltxt` with textures on, `Mdlshd` with them off, which is the original shaded look. The software
renderer always draws `Mdlshd`. Bones rebuild their world matrices each frame from the drawn
set's own rig, so swapping a set means pointing each object at the other set's model. Physics
reads the arena's own surface, not the visual mesh, so play is the same in either.

The software path rescales UVs in place into texel units when it loads its textures
(`RescaleSoftTexCoords`), so the hardware renderer keeps its own copy of the UVs.

## Switches

One settings struct, saved under `[Graphics]` in `booie.ini`. Keys toggle it for now, and the
ImGui debug menu will later. The software renderer ignores everything but the renderer choice.

| Switch | Values |
| --- | --- |
| Renderer | software, hardware (F2) |
| Textures | off, on (F3) |
| Texture filtering | nearest, linear (F7) |
| Textured drome | off, on (F8) |
| Shading | flat, smooth |
| Lights | original, several |
| Shadows | original, shadow-mapped |

With every switch on its original setting, the hardware renderer should match the software look.

## Phases

0. Groundwork. Compare the two model sets; map where the engine keeps meshes, matrices,
   materials and its shadow and transparent passes; build HLSL shaders to DXIL and SPIR-V at
   build time through CPM.
1. Shared device and swap. The presenter on the GPU device, with the fallback; the settings
   struct and the swap key; the hardware renderer draws a cleared frame under the HUD.
2. Geometry in the original look. Upload meshes, build the draw list, shade flat with the one
   light, depth test, keep the back-to-front pass for transparent and shadow faces, and the head
   cam as a second viewport.
3. Textures, through stb_image into GPU textures owned by `TexCacheEntry`.
4. Smooth shading and several lights.
5. Shadow maps from the main light.
6. Delete the DirectDraw and Direct3D code and the DirectDraw HUD, and unlink `ddraw` and `dxguid`.

Later passes, such as ambient occlusion, reflections and compute-traced shadows, build on the same
depth buffer and mesh buffers.
