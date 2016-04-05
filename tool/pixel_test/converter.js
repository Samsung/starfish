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
var curpath = "tool/pixel_test/";

function initialize() {
    var stream = fs.open(curpath + args[1] + ".res", 'r'); // *.res
    var preprop;
    while (!stream.atEnd()) {
        var line = stream.readLine();
        console.log(line);
        filelist.push(line);
        if (line.startsWith("#")) continue;
    }
    stream.close();
    return true;
}

if (!initialize()) {
    console.log("Usage: phantomjs converter.js [directory in test/reftest/csswg-test]       check all html pages in directory");

    phantom.exit();
}

var pathToTest = "test/reftest/csswg-test/" + args[1] + "/";
//var pathToSave = "test/regression/reftest/csswg-test/" + args[1] + "_converted/";
var pathToSave = "test/reftest/csswg-test/Converted/"; //" + args[1] + "_converted/";
fs.makeDirectory(pathToSave);

var page = require('webpage').create();

page.viewportSize = {
    width: 360,
    height: 360
};

var idx = 0;
var processing = false;
var passcnt = 0;
var failcnt = 0;
page.onConsoleMessage = function(msg, lineNum, sourceId) {
	console.log('#CONSOLE: ' + msg);
};

page.onLoadStarted = function() {
    processing = true;
    console.log('# [' + filelist[idx] + '] Loading Started...');
};

var resEvaluate;
page.onLoadFinished = function() {
    console.log("INJECT:"+page.injectJs('tool/html2xml/cssParser.js'));
    resEvaluate = page.evaluate(function(acceptCSSList, acceptTagList, acceptValues, includes) {
        Array.prototype.includes = includes;

        var styleFiltering = function() {
            var parser = new CSSParser();
            var styleNum = 1;
            var styleTags = document.getElementsByTagName("style");
            for (var i = 0; i < styleTags.length; i++) {
                if (!styleTags[i] || !styleTags[i].sheet) continue;
                var parseResult = parser.parse(styleTags[i].textContent);
                //console.log(styleTags[i].textContent);
                var rules = parseResult.cssRules;
                var styleTxt = "";
                //var rules = styleTags[i].sheet.cssRules;
                for (var j = 0; j < rules.length; j++) {
                    var sel = rules[j].mSelectorText;
                    //console.log(sel);
                    //console.log(rules[j].cssText());
                    var className = "convertedClass"+styleNum;
                    styleNum++;
                    rules[j].mSelectorText = "." + className;
                    var decl = rules[j].declarations;
                    if (decl) {
                        var len = decl.length;
                        for (var k = 0; k < len; k++) {
                            //console.log(decl[k].valueText);
                            if (decl[k].property == "transform") {
                                var d = new jscsspDeclaration();
                                d.property = "-webkit-transform";
                                d.valueText = decl[k].valueText;
                                d.parsedCssText = d.property + ": " + d.valueText + ";";
                                d.values = decl[k].values;
                                rules[j].declarations.push(d);
                            } else if (decl[k].property == "font") {
                                d.valueText = "";
                                d.parsedCssText = "font:;";
                                d.values = null;
                            }
                        }
                    }
                    styleTxt += rules[j].cssText() + "\n";
                    var elems = document.querySelectorAll(sel);
                    for (var k = 0; k < elems.length; k++) {
                        var elem = elems[k];
                        var cl = elem.getAttribute("class");
                        if (cl) {
                            elem.setAttribute("class", cl +" " + className);
                        } else {
                            elem.setAttribute("class", className);
                        }
                    }
                }
                var newStyle = document.createElement("style");
                newStyle.innerHTML = styleTxt;
                styleTags[i].parentNode.appendChild(newStyle);
                styleTags[i].parentNode.removeChild(styleTags[i]);
            }
            //console.log(document.documentElement.innerHTML);
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
                        // console.log("----------" + rr)
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
            document.body.removeChild(d);
            document.result = styleFiltering();
            document.result = "<!DOCTYPE html>\n" + document.documentElement.outerHTML;
//            console.log("(result)"+document.result);
        }
        d.src = "https://ajax.googleapis.com/ajax/libs/jquery/1.12.0/jquery.min.js";
        document.body.appendChild(d);
        document.result = undefined;

    }, acceptCSSList, acceptTagList, acceptValues, Array.prototype.includes);
    //console.log(JSON.stringify(result));
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
        var newfilename = pathToSave + filelist[idx].substring(filelist[idx].indexOf("csswg-test") + String("csswg-test").length + 1);
        console.log(newfilename)
        //console.log(resEvaluate)
        fs.write(newfilename, resEvaluate);
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
        phantom.exit();
    }
  }
  resEvaluate = undefined;
}, 500);

