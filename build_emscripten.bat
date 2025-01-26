call .\emsdk\emsdk_env.bat

if not exist .\embuild (
  mkdir embuild
)

pushd .\embuild

emcmake cmake ..

popd