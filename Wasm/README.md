# WebAssembly FMUs (FMI 3 co-simulation)

Build Wasm components as standard FMUs with the binary under `binaries/wasm32-wasip2/<Model>.wasm`.

**Prerequisites:** WASI SDK (`WASI_SDK_PATH`), latest `wit-bindgen` and `wasm-tools` (e.g. `cargo install wit-bindgen-cli wasm-tools`).

```powershell
$env:WASI_SDK_PATH = "E:\wasi-sdk-34.0-x86_64-windows"
cmake -S . -B build-wasm -DFMI_TARGET=wasm -DFMI_VERSION=3
cmake --build build-wasm --config Release
```

Native builds (default):

```powershell
cmake -S . -B build-native -DFMI_TARGET=native -DFMI_VERSION=3
cmake --build build-native --config Release
```

**Wasm models:** BouncingBall, Dahlquist, Feedthrough, Resource, Stair, VanDerPol, Roberts, StateSpace (not Clocks).
