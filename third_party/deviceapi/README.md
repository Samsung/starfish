Tizen Device API loader for escargot
====================================

# Prerequisites

## gbs
https://source.tizen.org/documentation/developer-guide/getting-started-guide/installing-development-tools

## webapi plugin
```bash
# start from here if you want to build webapi plugins rpm
$ git clone http://10.113.64.203/StarFish/webapi-plugins.git
$ cd webapi_plugins
$ git checkout tizen_2.3.1
$ vi .gbs.conf # edit 'buildroot' as wherever you want;
$ gbs build -A [armv7l|i586] --incremental --include-all -P profile.spin_2.3.1_wearable.[arm|emulator]

# start from here if you already have rpm files
$ sdb push $buildroot/local/repos/spin_2.3.1_wearable.[arm|emulator]/[armv7l|i586]/RPMS/webapi-plugins-0.0.1-0.[armv7l|i586].rpm /opt/usr/
$ sdb shell rpm -i --force --nodeps /opt/usr/webapi-plugins-0.0.1-0.[armv7l|i586].rpm
```

# How to run

## Source code
```
$ git clone http://10.113.64.203/StarFish/starfish.git
$ git submodule init
$ git submodule update
```

## Build
```
$ ./build_third_party.sh
$ make tizen_wearable_arm.lib.[debug|release] TIZEN_DEVICE_API=true -j
```

## Run
Once you installed webapi plugin rpm into device, there's nothing more to do
