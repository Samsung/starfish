#!/bin/bash
echo "=========== [ Install NodeWebkit Env ] ============"
sudo apt-get install nodejs-legacy
sudo npm install nw -g
cd tool/pixel_test/nw_capture
npm install fs
npm install system
npm install mkdirp