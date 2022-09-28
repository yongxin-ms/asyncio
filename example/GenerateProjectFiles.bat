@set trunk_dir=%cd%
pushd %trunk_dir%

md %trunk_dir%\BuildDebug
cd %trunk_dir%\BuildDebug
cmake %trunk_dir%

popd
pause