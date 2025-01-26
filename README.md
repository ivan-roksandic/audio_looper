# Audio Looper


## Compiling with emscripten
- Get the enscripten SDK: `git clone https://github.com/emscripten-core/emsdk.git`
- Get install latest package: `emsdk install latest`
- Activate the latest package: `emsdk activate latest`
- make the build directory: `mkdir embuild && cd embuild`
- Build with cmake `emcmake cmake .. -DSDL_THREADS=1` make sure the working directory is './embuild'
- Either run `emmake make -j4` or `ninja` based on whats the default build system.
- Start a webserver inside embuild (should be a app.js, app.wasm and app.html file generated)


Source of the compile instructions: [https://wiki.libsdl.org/SDL3/README/emscripten](https://wiki.libsdl.org/SDL3/README/emscripten)

