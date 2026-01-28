pushd markdown/third_party/lynx-textra
source tools/envsetup.sh
./tools/hab sync .
python3 ./tools/build_cmake_environment.py --gn-args "is_debug=false use_flutter_cxx=false"
popd
