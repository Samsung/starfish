var fs = require('fs');
var system = require('system');
if (typeof String.prototype.endsWith !== 'function') {
    String.prototype.endsWith = function(suffix) {
        return this.indexOf(suffix, this.length - suffix.length) !== -1;
    };
}
if (!String.prototype.startsWith) {
    String.prototype.startsWith = function(searchString, position){
      position = position || 0;
      return this.substr(position, searchString.length) === searchString;
  };
}
if (!Array.prototype.includes) {
  Array.prototype.includes = function(searchElement /*, fromIndex*/ ) {
    'use strict';
    var O = Object(this);
    var len = parseInt(O.length) || 0;
    if (len === 0) {
      return false;
    }
    var n = parseInt(arguments[1]) || 0;
    var k;
    if (n >= 0) {
      k = n;
    } else {
      k = len + n;
      if (k < 0) {k = 0;}
    }
    var currentElement;
    while (k < len) {
      currentElement = O[k];
      if (searchElement === currentElement ||
         (searchElement !== searchElement && currentElement !== currentElement)) { // NaN !== NaN
        return true;
      }
      k++;
    }
    return false;
  };
}

var args = system.args;
var platformName = "linux";
var testPath = "";
var filelist = [];
var acceptCSSList = [];
var acceptTagList = [];
var acceptValues = {};
var curpath = "tool/pixel_test/";

function initialize() {
    var stream = fs.open(curpath + 'supportCSSList.csv', 'r');
    var preprop;
    stream.readLine();      // Title
    while (!stream.atEnd()) {
        var line = stream.readLine();
        if (line.startsWith("#")) continue;
        var tokens = line.split(",");
        var prop = tokens[2].trim();
        if (prop != "") {           // property column exists
            acceptCSSList = acceptCSSList.concat(prop);
            preprop = prop;
        }
        if (tokens[3] && tokens[3].trim() != "") {  // value column exists
            if (!acceptValues[preprop])
                acceptValues[preprop] = [];
            acceptValues[preprop].push(tokens[3].trim());
        }
    }
    console.log( " ## Supported CSS Properties : " + acceptCSSList);
    console.log( " ## Supported CSS Prop-Value Pairs : " + JSON.stringify(acceptValues));
    stream.close();

    var stream2 = fs.open(curpath + 'supportHTMLList.csv', 'r');
    var line = "";
    do {
        line = stream2.readLine();
    } while (!line.startsWith("Element"));
    while (true) {
        if (!line.startsWith("#")) {
            var token = line.split(",")[2].trim();
            if (token != "")
                acceptTagList = acceptTagList.concat(token);
        }
        if (stream2.atEnd()) break;
        line = stream2.readLine();
    }
    console.log( " ## Supported HTML Tags : " + acceptTagList);
    stream2.close();

    if (args.length == 1) {
        testPath = "test/reftest/csswg-test/";
    }
    else if (args.length == 2) {
        if (args[1] != "-f") {
            testPath = "test/reftest/csswg-test/" + args[1];
            return true;
        }
    }
    else if (args.length == 3) {
        if (args[1] == "-f" && fs.isFile(args[2]) &&
            (args[2].endsWith(".html") || args[2].endsWith(".htm")) &&
            !(args[2].endsWith("-expected.html"))) {
            var last = args[2].lastIndexOf('/');
            filelist[0] = args[2].substring(last);

            var index = args[2].indexOf("test/");
            if (index >= 0)
                testPath = args[2].substring(index+5, last);
            else
                testPath = filelist[0];
            return true;
        }
    }
    return false;
}

if (!initialize()) {
    console.log("Usage: phantomjs testsetChecker.js [directory in test/reftest/csswg-test]       check all html pages in directory");
    console.log("   or: phantomjs testsetChecker.js -f [filepath=*.html]               check only one page\n");

    phantom.exit();
}

var pathToTest = testPath + "/";

var page = require('webpage').create();

page.viewportSize = {
    width: 360,
    height: 360
};

var makeFileList = function(path) {
//    console.log(path);
    var list = fs.list(path);
//    console.log(list);
    for (var i = 0; i < list.length; i++) {
        if (list[i] == "." || list[i] == "..")
            continue;
        if (fs.isDirectory(path + list[i])) {
            makeFileList(path + list[i] + "/");
        }
        if (fs.isFile(path + list[i]) &&
            (list[i].endsWith(".html") || list[i].endsWith(".htm")) &&
            !(list[i].endsWith("-expected.html") || list[i].endsWith("-expected.htm")) &&
            !(list[i].endsWith("-ref.html") || list[i].endsWith("-ref.htm"))
           ) {
            filelist.push(path + list[i]);
        }
    }
};

if (filelist.length == 0) {
    makeFileList(pathToTest);
}

var idx = 0;
var processing = false;
var passcnt = 0;
var failcnt = 0;

page.onLoadStarted = function() {
    processing = true;
//    console.log('# [' + filelist[idx] + '] Loading Started...');
};

page.onLoadFinished = function() {
    var result = page.evaluate(function() {
        var usedCSSList = {};
        var usedTagList = {};
        var styleTags = document.getElementsByTagName("style");
        var changePropName = function(prop, nextprop) {
            if (!nextprop) return prop;
            if (prop == "overflow-x" && nextprop.trim() == "overflow-y")
                prop = "overflow";
            else if (prop == "background-repeat-x" && nextprop == "background-repeat-y")
                prop = "background-repeat";
            return prop;
        }
        for (var i = 0; i < styleTags.length; i++) {
            if (!styleTags[i] || !styleTags[i].sheet) continue;
            var rules = styleTags[i].sheet.cssRules;
            for (var j = 0; j < rules.length; j++) {
                if (rules[j].constructor != CSSStyleRule) return false;
                for (var k = 0; k < rules[j].style.length; k++) {
                    var prop = rules[j].style[k].trim();
                    if ((prop == "background-attachment") || (prop == "background-clip")
                            || (prop == "background-origin") || (prop == "background-position")
                       ) {
                        if (rules[j].style[prop].trim() == "initial")
                            continue;
                    }
                    var newprop = changePropName(prop, rules[j].style[k+1]);
                    if (prop != newprop) k++;
                    usedCSSList[newprop] = 1;
                }
            }
        }
        var allTags = document.getElementsByTagName("*");
        for (var i = 0; i < allTags.length; i++) {
            if (allTags[i].hasAttribute("style")) {
                var inlineStyleList = allTags[i].getAttribute("style").split(';').map(function(a) { return a.split(':'); });
                for (var j = 0; j < inlineStyleList.length; j++) {
                    if (inlineStyleList[j][0].trim() != "") {
                        var prop = inlineStyleList[j][0].trim();
                        if ((prop == "background-attachment") || (prop == "background-clip")
                                || (prop == "background-origin") || (prop == "background-position")
                           ) {
                            if (inlineStyleList[j][1].trim() == "initial"
                                    || inlineStyleList[j][1].trim() == "initial initial")
                                continue;
                        }
                        var newprop = changePropName(prop, inlineStyleList[j+1]? inlineStyleList[j+1][0] : undefined );
                        if (prop != newprop) j++;
                        usedCSSList[newprop] = 1;
                    }
                }
            }
            usedTagList[allTags[i].localName] = 1;
        }
        return [usedCSSList, usedTagList];
    });
    //console.log(JSON.stringify(result));
    var check = function(res) {
        if (res == false) {
            console.log("## [ETC] rule is NOT Supported");
            return false;
        }
        for (var css in res[0]) {   // usedCSSList
            if (!acceptCSSList.includes(css)) {
                console.log("## [CSS] " + css + " is NOT Supported");
                return false;
            }
        }
        for (var tag in res[1]) {   // usedHTMLList
            if (!(acceptTagList.includes(tag))) {
                console.log("## [HTML] " + tag + " is not Supported");
                return false;
            }
        }
        return true;
    };
    if (check(result)) {
        passcnt++;
        console.log(filelist[idx]);
    } else {
        failcnt++;
        console.log("# " + filelist[idx] + " Not Passed..");
    }
    idx++;
    processing = false;
};

setInterval(function() {
  if (!processing && idx < filelist.length) {
    var file = filelist[idx];
//    console.log(file);
    page.open(file);
  } else if (idx == filelist.length) {
    console.log(" ### Fail: " + failcnt + " tests");
    console.log(" ### Pass: " + passcnt + " tests");
    console.log(" ### TOTAL: " + (passcnt + failcnt) + " tests");
    phantom.exit();
  }
}, 250);

