% REM We use ninja for generator because VS does not work with conda build
cmake --preset windows-msvc-release -G "Ninja" -DBUILD_JARS=OFF -DTESTING=OFF
cmake --build --preset build-windows-msvc-release --target MODL --target MODL_Coclustering
