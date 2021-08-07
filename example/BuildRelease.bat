@set trunk_dir=%cd%
pushd %trunk_dir%

md %trunk_dir%\release
cd %trunk_dir%\release
cmake -DCMAKE_BUILD_TYPE=Release %trunk_dir%
cmake --build ./

popd
pause