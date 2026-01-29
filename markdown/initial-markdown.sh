pushd markdown/third_party/lynx-textra
./tools/hab sync .
source tools/envsetup.sh
python3 ./tools/build_cmake_environment.py --gn-args "is_debug=false use_flutter_cxx=false"
popd
