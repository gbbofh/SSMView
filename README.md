# SSMView

A C and [raylib](https://www.raylib.com/) viewer for SunStorm's SSM vertex-animated models (including *Primal Prey*). It shows named animations, interpolates poses using durations stored in the file, and highlights separate mesh groups and a suspected stationary origin pair. Supply your own game assets.

## Build and run

Requires a C11 compiler and CMake 3.18 or newer. CMake uses an installed raylib 5.x if found, otherwise fetches raylib 5.5 from its official repository. Building raylib on Linux also requires the usual desktop OpenGL/X11 development packages. On Windows, use MSYS2 MinGW64 or a CMake supported Visual Studio environment.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/ssmview path/to/model.ssm
```

With a multiconfiguration generator, the executable is typically `build/Release/ssmview.exe`. You can also start the viewer without a path and drop an `.ssm` file onto the window. Drop another file to switch models. If a file is invalid, the viewer reports the error and retains the currently loaded model.

| Action | Control |
| --- | --- |
| Orbit / pan / zoom | Left drag / right drag / wheel over the 3D view |
| Choose animation | Click its name, or Up/Down |
| Pause or play | Space or Play button |
| Step through poses | Left/Right (pauses playback) |
| Loop, speed | Buttons in the side panel |
| Wireframe | W or Wireframe button |
| Highlight suspected origin pair | M or Origin pair button |
| Recenter camera | R or Reset button |

The file uses Z-up coordinates. The viewer maps them to raylib's Y-up space as `(x, z, -y)`; the red, green, and blue lines indicate file X, Z, and Y axes respectively. Mesh groups get distinct colors. Geometry is displayed without textures: SSM references external STX files and this project does not yet decode them or map skin texture IDs to render materials.

## Format notes and changes from the original viewer

The loader now uses bounds-checked little-endian reads and binary file mode instead of casting bytes to C structs. The five counts at offset 8 are 16-bit unsigned values. Each triangle table record is 36 bytes: byte 0 is the mesh ID (except in the first record), bytes 4..9 are three 16-bit vertex indices, and bytes 12..35 are six float32 UV values. Bytes 1..3 and 10..11 are retained but unidentified. The first record's byte 0 stores the **number of meshes**, but its remaining bytes still describe a triangle. The viewer displays it as part of mesh 0 (the following records use mesh ID 0). Other triangles are grouped by their actual mesh IDs without assuming matching IDs are adjacent.

Each frame contains a name, one unidentified byte per mesh, `vertex_count` float32 triples, and an eight-byte tail. An animation has a name, 16-bit frame indices, **one float32 duration per frame** (seconds, inferred from values such as 0.100 and 0.025), and an eight-byte tail. Playback uses these durations instead of an assumed fixed frame rate or automatically changing animations. The bytes following the animations appear to encode mesh and skin metadata, but have not been fully decoded.

The Origin pair overlay looks for two triangles sharing an edge, with vertices unchanged across all frames and lying near the file's Z=0 plane. In the supplied T. rex and triceratops files these are records 0–1 and 632–633, respectively. This geometric heuristic does not find a matching pair in the dragonfly file. The pair's purpose in the original engine is still unknown, and record 0 is **not** always part of it: triceratops record 0 moves during animation.

Build the `ssmparse` target to check the parser without opening a graphics window: `build/ssmparse path/to/model.ssm`.
