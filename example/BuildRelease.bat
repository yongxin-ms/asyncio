@set trunk_dir=%cd%
pushd %trunk_dir%

md %trunk_dir%\BuildRelease
cd %trunk_dir%\BuildRelease
cmake -DCMAKE_BUILD_TYPE=Release %trunk_dir%
cmake --build ./

popd
pause