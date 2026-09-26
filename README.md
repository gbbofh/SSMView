# SSMView

A C and [raylib](https://www.raylib.com/) viewer for SunStorm's SSM vertex-animated models (including *Primal Prey*). It shows named animations, interpolates poses using durations stored in the file, and highlights the separate mesh groups and the first triangle record. Supply your own game assets.

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
| Highlight first record | M or Marker button |
| Recenter camera | R or Reset button |

The file uses Z-up coordinates. The viewer maps them to raylib's Y-up space as `(x, z, -y)`; the red, green, and blue lines indicate file X, Z, and Y axes respectively. Mesh groups get distinct colors. Geometry is displayed without textures: SSM references external STX files and this project does not yet decode them or map skin texture IDs to render materials.

## Format notes and changes from the original viewer

The loader now uses bounds-checked little-endian reads and binary file mode instead of casting bytes to C structs. The five counts at offset 8 are 16-bit unsigned values. Each triangle table record is 36 bytes: byte 0 is the mesh ID (except in the first record), bytes 4..9 are three 16-bit vertex indices, and bytes 12..35 are six float32 UV values. Bytes 1..3 and 10..11 are retained but unidentified. The first record's byte 0 stores the **number of meshes**; its vertices form the small fixed triangle near the origin seen in the supplied models. The viewer draws it only when Marker is enabled. Other triangles are grouped by their actual mesh IDs, without assuming that matching IDs are adjacent.

Each frame contains a name, one unidentified byte per mesh, `vertex_count` float32 triples, and an eight-byte tail. An animation has a name, 16-bit frame indices, **one float32 duration per frame** (seconds, inferred from values such as 0.100 and 0.025), and an eight-byte tail. Playback uses these durations instead of an assumed fixed frame rate or automatically changing animations. The bytes following the animations appear to encode mesh and skin metadata, but have not been fully decoded. The purpose of the first triangle remains a hypothesis; drawing it as an origin marker does not imply that the engine used it that way.

Build the `ssmparse` target to check the parser without opening a graphics window: `build/ssmparse path/to/model.ssm`.
