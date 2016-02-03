var fs = require('fs');
var system = require('system');
if (typeof String.prototype.endsWith !== 'function') {
    String.prototype.endsWith = function(suffix) {
        return this.indexOf(suffix, this.length - suffix.length) !== -1;
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
var basePath = "../../test/";
var filelist = [];
var acceptCSSList = [];
var acceptTagList = [];

function initialize() {
    console.log("#!/bin/sh");
    var stream = fs.open('supportCSSList.csv', 'r');
    while (!stream.atEnd()) {
        var line = stream.readLine();
        var token = line.split(",").map( function(s) {return s.trim();} );
        acceptCSSList = acceptCSSList.concat(token);
    }
    console.log( " ## Supported CSS Properties : " + acceptCSSList);
    stream.close();

    var stream2 = fs.open('supportHTMLList.csv', 'r');
    while (!stream2.atEnd()) {
        var line = stream2.readLine();
        var token = line.split(",").map( function(s) {return s.trim();} );
        acceptTagList = acceptTagList.concat(token);
    }
    console.log( " ## Supported HTML Tags : " + acceptTagList);
    stream2.close();

    if (args.length == 1) {
        testPath = "/unittest/css";
        basePath = "../../test/";
    }
    else if (args.length == 2) {
        if (args[1] != "-f") {
            testPath = "/unittest/" + args[1];
            return true;
        }
    }
    else if (args.length == 3) {
        if (args[1] == "-f" && fs.isFile(args[2]) && args[2].endsWith(".html")) {
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
    console.log("Usage: phantomjs testsetChecker.js [directory in test/unittest]       check all html pages in directory");
    console.log("   or: phantomjs testsetChecker.js -f [filepath=*.html]               check only one page\n");

    phantom.exit();
}

var pathToTest = basePath + "/" + testPath + "/";

var page = require('webpage').create();

page.viewportSize = {
    width: 360,
    height: 360
};
if (filelist.length == 0) {
    var list = fs.list(pathToTest);
    for (var i = 0; i < list.length; i++) {
        if (fs.isFile(pathToTest + list[i]) && list[i].endsWith(".html")) {
            filelist.push(list[i]);
        }
    }
}

var idx = 0;
var processing = false;
var cnt = 0;

page.onLoadStarted = function() {
    processing = true;
//    console.log('# [' + filelist[idx] + '] Loading Started...');
};

page.onLoadFinished = function() {
    var result = page.evaluate(function() {
        var usedCSSList = {};
        var usedTagList = {};
        var styleTags = document.getElementsByTagName("style");
        for (var i = 0; i < styleTags.length; i++) {
            var rules = styleTags[i].sheet.cssRules;
            for (var j = 0; j < rules.length; j++) {
                for (var k = 0; k < rules[j].style.length; k++) {
                    usedCSSList[rules[j].style[k].trim()] = 1;
                }
            }
        }
        var allTags = document.getElementsByTagName("*");
        for (var i = 0; i < allTags.length; i++) {
            if (allTags[i].hasAttribute("style")) {
                var inlineStyleList = allTags[i].getAttribute("style").split(';').map(function(a) { return a.split(':')[0]; });
                for (var j = 0; j < inlineStyleList.length; j++) {
                    if (inlineStyleList[j].trim() != "")
                        usedCSSList[inlineStyleList[j].trim()] = 1;
                }
            }
            usedTagList[allTags[i].localName] = 1;
        }
        return [usedCSSList, usedTagList];
    });
    //console.log(JSON.stringify(result));
    var check = function(res) {
        if (res == false) {
            console.log("result is false");
            return false;
        }
        for (var css in res[0]) {
            if (!acceptCSSList.includes(css)) {
                console.log("## [CSS] " + css + " is not Supported");
                return false;
            }
        }
        for (var tag in res[1]) {
            if (!(acceptTagList.includes(tag))) {
                console.log("## [HTML] " + tag + " is not Supported");
                return false;
            }
        }
        return true;
    };
    if (check(result)) {
        console.log("# " + filelist[idx] + " Passed..");
    } else {
        cnt++;
        console.log("rm "+pathToTest + filelist[idx]);
    }
    idx++;
    processing = false;
};

setInterval(function() {
  if (!processing && idx < filelist.length) {
    var file = pathToTest + filelist[idx];
    page.open(file);
  } else if (idx == filelist.length) {
    console.log(" ## " + cnt + " tests are NOT passed");
    phantom.exit();
  }
}, 250);

