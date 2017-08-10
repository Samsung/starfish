#!/bin/bash
sudo apt-get install libzmq-dev npm
wget https://dl.nwjs.io/v0.17.0/nwjs-v0.17.0-linux-x64.tar.gz
tar xvzf nwjs-v0.17.0-linux-x64.tar.gz
npm install node-gyp
npm install zmq
npm install nw-gyp
cd node_modules/zmq
../nw-gyp/bin/nw-gyp.js rebuild --target=0.17.0
cd -
