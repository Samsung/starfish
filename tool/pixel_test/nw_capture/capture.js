// Load in dependencies
var fs = require('fs');
var system = require('system');
var mkdirp = require('mkdirp');
var numCPUs = 4;//require('os').cpus().length;
process.chdir('../../../');
var gui = nw;
var win = gui.Window.get();
var args = gui.App.argv;
console.log = process.stdout.write;
var console = (function(){
    var util = require('util');
    return {
        log:function(x){
            process.stdout.write(util.format.apply(util, arguments) + "\n");
        }
    }
})()
Array.prototype.chunk = function(pieces) {
    pieces = pieces || 2;
    var len = this.length;
    var mid = (len/pieces);
    var chunks = [];
    var start = 0;
    for(var i=0;i<pieces;i++) {
        var last = start+mid;
        if (!len%pieces >= i) {
            last = last-1
        }
        chunks.push(this.slice(start,last+1) || []);
        start = last+1;
    }
    return chunks;
}

var W = 360, H = 360;
var RES_STR = "out/x64/exe/debug/reftest/pixel_test/";
var pathToSave = "";
var path = "";
var finishCnt = 0;

var getRenderFilePath = function (filepath) {
    var last = filepath.lastIndexOf('/');
    var point = filepath.lastIndexOf('.');
    var filename = filepath.substring(last, point);
    var start = filepath.indexOf("reftest/");
    filepath = filepath.substring(start+8, last);
    filename += "_expected.png";

    if (pathToSave == "") {
        path = RES_STR + filepath;
    } else {
    }
    return ( path + filename );
    //return "out/" + filepath + filename;
}

function takeSnapshot(filelist, key, callback) {
    var filepath = filelist[0];
    var index = 0;

    gui.Window.open(filepath,
            { width: W, height:H, position:'center', focus:false, inject_js_end:"inject.js", frame:false },
            function(wing) {
                //console.log("# tc: " + filepath);
                console.log("  - Group " + key + " Window Open : " + filelist.length + " tests");
                wing.on('loaded',function() {
                    //console.log('loaded called');
                    setTimeout(function() {
                        wing.capturePage(function(img) {
                            var renderFileName = getRenderFilePath(wing.window.location.href);
                            //console.log(renderFileName);
                            mkdirp(path, function(err) {
                                fs.writeFile(renderFileName, img, function (err) {
                                    if (err) throw err;
                                    console.log(renderFileName);
                                    //console.log(wing.window.location.href);
                                    if (index >= filelist.length - 1) {
                                        wing.close();
                                        finishCnt++;
                                        console.log("--- window close");
                                        if (finishCnt == numCPUs) {
                                            console.log("--- nw capture COMPLETE!");
                                            gui.App.quit();
                                        }
                                    } else {
                                        wing.window.location.href = filelist[++index];
                                    }
                                });
                                //                                wing.close();
                                //callback("finish "+renderFileName);
                            });
                        }, {format: 'png', datatype: 'buffer'});
                    }, 1000);
                });
            }
    );
};
nw.process.on('exit',function(){
    var fs = nw.require('fs');
});

function execute() {
    if (args.length >= 2) {
        if (args.length >= 3 && args[2] != "pc")
            pathToSave = args[2];
        console.log("--- nw capture START!: " + args[1] + (args[2]? " " + args[2] : ""));
        if (args[0] == "-f" && fs.existsSync(args[1]) && (args[1].endsWith(".html") || args[1].endsWith(".htm"))) {
            setTimeout(function() { takeSnapshot([args[1]]) },1000);
            return true;
        } else if (args[0] == "-l" && fs.existsSync(args[1])) {
            var files = fs.readFileSync(args[1]).toString().split("\n").filter( function(v) {
                return v.startsWith("test/");
            });
            console.log("  # of Test cases : " + files.length);
            //console.log(numCPUs);
            if (files.length <= numCPUs) {
                setTimeout(function() { takeSnapshot(files) },1000);
                return true;
            }
            var list = files.chunk(numCPUs).filter ( function(v) { return v.length > 0; } );

            list.forEach( function(filelist, key) {
                setTimeout(function() {
                    takeSnapshot(filelist, key);
                },1000);
            });
            //console.log(filelist);
            return true;
        }
    }
    return false;
}

window.onload = function(){
    if (args[args.length-1] == "pc") {
        W = 800;
        H = 600;
    }
    if (!execute()) {
        console.log("Usage: nw . -f [filepath=*.html]       capture only one page\n");
        console.log("Usage: nw . -l [filepath=*]            capture file list\n");

        gui.App.quit();
    }
    //setTimeout(function() { gui.App.quit(); }, 6000);
};
