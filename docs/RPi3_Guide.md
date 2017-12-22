# Raspberry Pi 3 Setup Guide

This guide shows you how to run Minimized Web Engine (MWE) on
Raspberry Pi 3 (RPi3) running Tizen 4.0. This guide is tested on Ubuntu 16.04.

## Setup Tizen 4.0 on RPi3

### Install required packages

```sh
sudo apt-get install pv minicom
```

### Download required tools

```sh
wget https://git.tizen.org/cgit/platform/kernel/u-boot/plain/scripts/tizen/sd_fusing_rpi3.sh?h=tizen --output-document=sd_fusing_rpi3.sh
chmod 755 sd_fusing_rpi3.sh
```

### Download Tizen 4.0 for RPi3 images

```sh
wget -r -np -nc -nd -l1 -A 'tizen*.tar.gz' "http://download.tizen.org/snapshots/tizen/4.0-unified/latest/images/standard/iot-boot-arm64-rpi3"
wget -r -np -nc -nd -l1 -A 'tizen*.tar.gz' "http://download.tizen.org/snapshots/tizen/4.0-unified/latest/images/standard/tv-wayland-armv7l-rpi3"
```

If you are a Samsung developer, use the following for faster downloads. Replace ``userID`` with your ID.

```sh
wget -r -np -nc -nd -l1 -A 'tizen*.tar.gz' "http://165.213.149.200/download/public_mirror/tizen/4.0-unified/latest/images/standard/iot-boot-arm64-rpi3" --user=userID --ask-password
wget -r -np -nc -nd -l1 -A 'tizen*.tar.gz' "http://165.213.149.200/download/public_mirror/tizen/4.0-unified/latest/images/standard/tv-wayland-armv7l-rpi3" --user=userID --ask-password
```

### Install Tizen 4.0 for RPi3
Insert an SD card to an Ubuntu PC, and check the device name. In this guide, we
assume that the device name is ``/dev/sdb``. Replace ``XXX`` with the version number, which is usually the timestamp.

```sh
sudo fdisk -l
```

Format the SD card, and install Tizen 4.0

```
sudo ./sd_fusing_rpi3.sh -d /dev/sdb --format
sudo ./sd_fusing_rpi3.sh -d /dev/sdb -b tizen-4.0-unified_XXX_tv-wayland-armv7l-rpi3.tar.gz
sudo ./sd_fusing_rpi3.sh -d /dev/sdb -b tizen-4.0-unified_XXX_iot-boot-arm64-rpi3.tar.gz
```

### Connect to RPi3
1. Insert the SD Card to RPi3.
2. Power up RPi3, and connect it to a host PC.
3. Run a terminal such as minicom.
4. Use userid=root, and password=tizen

#### How to use minicom
```sh
sudo  minicom -b 115200 -o -D /dev/ttyUSB0
```

Turn off hardware flow control by
```
CTRL-A Z -> Shift O -> select 'Serial port setup' -> Shift F -> select 'Exit'
```

## Install MWE on RPi3

### Download required packages
Download the following packages from a Tizen VD repo.

* vconf-internal-keys-tv-XXX.armv7l.rpm
* factory-api-XXX.armv7l.rpm
* libwayland-extension-tv-client-XXX.armv7l.rpm
* vd-win-util-XXX.armv7l.rpm

If you are a Samsung developer, use the following for faster downloads

```sh
wget -r -np -nc -nd -l1 -A 'vconf-internal-keys-tv-[0-9]*.armv7l.rpm' "http://168.219.244.109/products/tv/archive/2018/MAIN2018/KantM/latest/repos/product/armv7l/packages/armv7l/"
wget -r -np -nc -nd -l1 -A 'factory-api-[0-9]*.armv7l.rpm' "http://168.219.244.109/products/tv/archive/2018/MAIN2018/KantM/latest/repos/product/armv7l/packages/armv7l/"
wget -r -np -nc -nd -l1 -A 'libwayland-extension-tv-client-[0-9]*.armv7l.rpm' "http://168.219.244.109/products/tv/archive/2018/MAIN2018/KantM/latest/repos/product/armv7l/packages/armv7l/"
wget -r -np -nc -nd -l1 -A 'vd-win-util-[0-9]*.armv7l.rpm' "http://168.219.244.109/products/tv/archive/2018/MAIN2018/KantM/latest/repos/product/armv7l/packages/armv7l/"
```

### Install required packages
After connecting to RPi3, install all required packages.

```sh
mount -o rw,remount /
rpm -Uvh *.rpm
```

### Install MWE
```sh
rpm -Uvh lightweight-web-engine-*.rpm
```

## Optional RPi3 Setup

### Ethernet setup
Connect a lan cable, and run the following.

```sh
connmanctl
connmanctl> services
* AR Wired ethernet_XX_cable

connmanctl> config ethernet_XX_cable --ipv4 manual <ipaddress> <netmask> [gateway]
connmanctl> exit

echo 'nameserver xx.xx.xx.xx' > /etc/resolv.conf
```

### Setup ssh and openssh-server
```sh
rpm -Uvh openssh-client-XXX.armv7l.rpm
rpm -Uvh openssh-server-XXX.armv7l.rpm
systemctl enable sshd
systemctl start sshd
```
