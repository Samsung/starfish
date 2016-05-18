var fs = require("fs");

function main() {
    if (process.argv.length != 3) {
        console.log("Usage: "+process.argv[0]+ " " +process.argv[1] + " file.html");
        return;
    }

    var file = process.argv[2];
    var c = fs.readFileSync(file, "utf8");
    //console.log(c);

    c = cleanContent(c);
    var funcs = getFunctions(c);
    var attrs = getAttrs(c);

    process.stdout.write(file);
    for (var i=0; i < funcs.length; i++) {
        process.stdout.write("\t"+funcs[i]);
    }
    for (var i=0; i < attrs.length; i++) {
        process.stdout.write("\t"+attrs[i]);
    }
    process.stdout.write("\n");
}

function cleanContent(content) {
    var c = content;
    c = removeComment(c, "<!--", "-->");
    c = removeComment(c, "//", "\n");
    c = removeComment(c, "/*", "*/");
    return c;
}

function removeComment(content, commentStart, commentEnd) {
    var s = content.indexOf(commentStart);
    var c = content;
    while (s > -1) {
        var e = c.indexOf(commentEnd, s);
        c = c.substring(0, s) + c.substring(e+commentEnd.length, c.length);
        s = c.indexOf(commentStart);
    }
    return c;
}


function getFunctions(content) {
    var results = getFunctionsSub(content);
    var keys = [];
    for (var key in results) {
        keys.push(key);
    }
    return keys;
}

function merge(dic1, dic2) {
    for (var key in dic2) {
        dic1[key] = dic2[key];
    }
}

function getFunctionsSub(content) {
    var reg = /_*\w+\(.*\)/g;
    var results = {};
    var found;
    while (found = reg.exec(content)) {
        var func = found[0];

        var e = func.indexOf("(");
        if (e >= 0) {
            var f = func.substring(0, e);
            results[f]="";
            var substr = func.substring(e+1, func.lastIndexOf(")")-1);
            merge(results, getFunctionsSub(substr));
        }
    }

    return results;
}

function getAttrs(content) {
    var reg = /\.\w+/g;
    var results = {};
    var found;
    while (found = reg.exec(content)) {
        var attr = found[0];
        attr = attr.substring(1);
        results[attr]="";
    }

    var keys = [];
    for (var key in results) {
        keys.push(key);
    }
    return keys;
}

main();

