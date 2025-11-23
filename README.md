# bvh-exp

Quick build & run (MinGW / MSYS2)

 - Configure and generate MinGW Makefiles (from `build/`):

```bash
mkdir -p build
cd build
cmake -G "MinGW Makefiles" \
	-DCMAKE_MAKE_PROGRAM="/c/msys64/mingw64/bin/mingw32-make.exe" \
	-DCMAKE_C_COMPILER="/c/msys64/mingw64/bin/gcc.exe" \
	-DCMAKE_CXX_COMPILER="/c/msys64/mingw64/bin/g++.exe" ..
```

 - Build:

```bash
mingw32-make -j$(nproc)
```

 - Run (uses `dataset/rays` by default). If you built in `build/`:

```bash
cd build
./bvh_exp.exe ../dataset/scenes/bunny.obj
# or just run rays without a model:
./bvh_exp.exe -- ../dataset/rays
```

Notes
- The program accepts a model path as the first argument and an optional rays file as the second. The rays file format is one ray per line:
	`ox oy oz dx dy dz tmin tmax` (comments starting with `#` and blank lines are ignored).
 - If you run the binary from `build/` the code tries several candidate locations for the `rays` file (cwd, ../dataset/rays, and exe-relative locations).

If you prefer Visual Studio / NMake, configure CMake with the appropriate generator and tools for that environment.