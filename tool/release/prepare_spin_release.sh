make clean
./build_third_party.sh
make tizen_wearable_arm.lib.release -j
make tizen_wearable_emulator.lib.release -j

git branch -D tizen_2.3
git checkout -b tizen_2.3
mkdir -p prebuilt/tizen-wearable-2.3-armv7l/
mkdir -p prebuilt/tizen-wearable-2.3-i586/
cp out/tizen_2.3.1/arm/lib/release/libWebWidgetEngine.so prebuilt/tizen-wearable-2.3-armv7l/
cp out/tizen_2.3.1/x86/lib/release/libWebWidgetEngine.so prebuilt/tizen-wearable-2.3-i586/
git add -f prebuilt/
git commit -m "Add prebuilt libWebWidgetEngine.so"
git log --graph --color --pretty=format:"%x1b[31m%h%x09%x1b[32m%d%x1b[0m%x20%s"
