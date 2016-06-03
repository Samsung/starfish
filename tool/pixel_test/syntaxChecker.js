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
// NOTE: font property is exceptionally considered for other property check.
var acceptCSSList = ["font"];
var acceptTagList = [];
var acceptValues = {};
var allowedPropList = ["background-attachment", "background-clip", "background-origin",
                        "background-position", "background-position-x", "background-position-y",
                        "border-image-outset", "border-image-repeat"];
var curpath = "tool/pixel_test/";

function initialize() {
    var stream = fs.open(curpath + 'supportCSSList.csv', 'r');
    var preprop;
    stream.readLine();      // Title
    while (!stream.atEnd()) {
        var line = stream.readLine();
        //console.log(line);
        if (line.startsWith("#")) continue;
        var tokens = line.split(",");
        if (tokens.length <= 2) continue;
        var prop = tokens[2].trim();
        if (prop != "" && !acceptCSSList.includes(prop)) {           // property column exists
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
page.onConsoleMessage = function(msg, lineNum, sourceId) {
	console.log('#CONSOLE: ' + msg);
};

page.onLoadStarted = function() {
    processing = true;
//    console.log('# [' + filelist[idx] + '] Loading Started...');
};

var resEvaluate;
page.onLoadFinished = function() {
    resEvaluate = page.evaluate(function(acceptCSSList, acceptTagList, acceptValues, allowedPropList, includes, startsWith) {
        Array.prototype.includes = includes;
        String.prototype.startsWith = startsWith;
        var changePropName = function(prop, nextprop) {
            if (prop == "-webkit-transform")
                prop = "transform";
            if (prop == "-webkit-transform-origin-x" || prop == "-webkit-transform-origin-y")
                prop = "transform-origin";
            if (!nextprop) return prop;
            if (prop == "overflow-x" && nextprop.trim() == "overflow-y")
                prop = "overflow";
            else if (prop == "background-repeat-x" && nextprop == "background-repeat-y")
                prop = "background-repeat";
            return prop;
        }

        var allTags = document.getElementsByTagName("*");
        for (var i = 0; i < allTags.length; i++) {
            if (!acceptTagList.includes(allTags[i].localName))
                return ["HTML", allTags[i].localName];
            if (allTags[i].hasAttribute("style")) {
                var inlineStyleList = allTags[i].getAttribute("style").split(';').map(function(a) { return a.split(':'); });
                for (var j = 0; j < inlineStyleList.length; j++) {
                    if (inlineStyleList[j][0].trim() != "") {
                        var prop = inlineStyleList[j][0].trim();
                        if (allowedPropList.includes(prop)) {
                            var val = inlineStyleList[j][1].trim();
                            if (val == "initial"
                                    || val == "initial initial")
                                continue;
                        }
                        var newprop = changePropName(prop, inlineStyleList[j+1]? inlineStyleList[j+1][0] : undefined );
                        if (prop != newprop) j++;
                        if (!acceptCSSList.includes(newprop))
                            return ["CSS", newprop];
                        if (newprop in acceptValues) {
                            var val = inlineStyleList[j][1].trim();
                            if (newprop == "transform")
                                val = val.substring(0, val.indexOf("("));
                            if (!acceptValues[newprop].includes(val))
                                return ["CSS-Value", val];
                        }
                    }
                }
            }
        }
        var styleFiltering = function() {
            var styleTags = document.getElementsByTagName("style");
            for (var i = 0; i < styleTags.length; i++) {
                if (!styleTags[i] || !styleTags[i].sheet) continue;
                var rules = styleTags[i].sheet.cssRules;
                for (var j = 0; j < rules.length; j++) {
                    if (!rules[j] || rules[j].constructor != CSSStyleRule) return false;
                    if (rules[j].selectorText.indexOf(':') != -1 && rules[j].selectorText.indexOf(':active') == -1)
                        return false;
                    if (rules[j].style.cssText.indexOf('!important') != -1 || rules[j].style.cssText.indexOf('! important') != -1)
                        return false;
                    for (var k = 0; k < rules[j].style.length; k++) {
                        var prop = rules[j].style[k].trim();
                        if (allowedPropList.includes(prop)) {
                            var val = rules[j].style[prop].trim();
                            if (val == "initial")
                                continue;
                        }
                        var newprop = changePropName(prop, rules[j].style[k+1]);
                        if (prop != newprop) k++;
                        if (!acceptCSSList.includes(newprop))
                            return ["CSS", newprop];
                        if (newprop in acceptValues) {
                            var val = rules[j].style[prop];
                            if (newprop == "transform")
                                val = val.substring(0, val.indexOf("("));
                            if (!acceptValues[newprop].includes(val))
                                return ["CSS-Value", val];
                        }
                    }
                }
            }
            return true;
        };
        // NOTE: COPY from html2xml/index.html
        var d = document.createElement("script");
        var res;
        d.onload = function() {
            var resCnt = 0;
            var links = document.querySelectorAll("link")

            for (var i = 0; i < links.length; i ++) {
                // <link rel="stylesheet" href="/lib/w3.css">
                if (links[i].getAttribute("rel") == "stylesheet") {
                    resCnt++;
                    function result(rr) {
                        // console.log("ajax result")
                        console.log("----------" + rr)
                        links[arguments.callee.i].outerHTML = "<style>"
                            + rr +
                            "</style>";
                        resCnt--;
                    }
                    result.i = i;
                    var url = links[i].getAttribute("href");

                    var point;
                    if (window.point) {
                        point = window.point;
                    } else {
                        point = location.pathname.substring(0, location.pathname.lastIndexOf('/')) + "/"
                    }
                    url = "file://" + point + url;
                    $.ajax({async: false, type:"GET", url: url, success: result, error: function (error) {
                         console.log("ajax error (" + url + ") : " + error)
                        // console.log(arguments[0])
                        // console.log(arguments[1])
                        // console.log(arguments[2])
                        resCnt--;
                    }});
                }
            }
            // console.log( document.getElementsByTagName("style").length );
            document.result = styleFiltering();
            console.log("(result)"+document.result);
        }
        d.src = "https://ajax.googleapis.com/ajax/libs/jquery/1.12.0/jquery.min.js";
        document.body.appendChild(d);
        document.result = undefined;

    }, acceptCSSList, acceptTagList, acceptValues, allowedPropList, Array.prototype.includes, String.prototype.startsWith);
    //console.log(JSON.stringify(result));
};

var check = function(res) {
    if (res == false) {
        console.log("## [ETC] rule or pseudo selector is NOT Supported");
        return false;
    }
    if (res != true) {
        console.log("## [" + res[0] + "] " + res[1] + " is NOT Supported");
        return false;
    }
    return true;
};
var started = true;
setInterval(function() {
    if (resEvaluate == undefined) {
        var $ = page.evaluate(function(){return window.$})
        if ($) {
            resEvaluate = page.evaluate(function(){ return document.result });
        }
    }

    if (resEvaluate != undefined) {
        if (check(resEvaluate)) {
            passcnt++;
            console.log(filelist[idx]);
        } else {
            failcnt++;
            console.log("# " + filelist[idx] + " Not Passed..");
        }
        processing = false;
        idx++;
    }
    
  if (!processing && idx < filelist.length) {
    var file = filelist[idx];
    if (started || resEvaluate != undefined) {
//        console.log(file);
        page.open(file);
        started = false;
    }
  } else if (idx == filelist.length) {
//      console.log(resEvaluate);
    if (resEvaluate || $) {
        console.log(" ### Fail: " + failcnt + " tests");
        console.log(" ### Pass: " + passcnt + " tests");
        console.log(" ### TOTAL: " + (passcnt + failcnt) + " tests");
        phantom.exit();
    }
  }
  resEvaluate = undefined;
}, 500);

