var fs = require("fs");


var dup = {};

function main() {
    if (process.argv.length != 5) {
        console.log("Usage: "+process.argv[0]+ " " +process.argv[1] + " TCName spec.txt file.raw.csv");
        return;
    }

    var TCName = process.argv[2];
    var spec = process.argv[3];
    var file = process.argv[4];
    run(TCName, spec, file);
}

function run(TCName, spec, file) {

    // 1. read attrs and methods from spec
    var features = {};
    var domList = fs.readFileSync(spec, "utf8").split("\n").slice(1); // removing title
    var interfaces = [];
    var interfaceName = "";
    for (var i=0; i < domList.length; i++) {
        var line = domList[i]; // methods and attr column
        if (line != undefined) {
            line = line.trim();
            if (line != "" && line.indexOf("#") != 0) {
                var tokens = line.split(',');
                interfaces.push(tokens[0]);
                if (tokens[0].trim() != "") {
                    interfaceName = tokens[0].trim();
                }
                var methodName = tokens[1].trim();
                features[methodName] = interfaceName;
            }
        }
    }

    // 2. create a table containing all features
    var table = {};
    var keys = Object.keys(features);
    var methodList = fs.readFileSync(file, "utf8").split("\n");
    for (var i=0; i < methodList.length; i++) {
        var tokens = methodList[i].split("\t");
        var filename = tokens[0];

        if (filename == "") {
            continue;
        }

        table[filename] = {};
        for (var j=0; j < keys.length; j++) {
            table[filename][keys[j]] = "";
        }

        for (var j=1; j < tokens.length; j++) {
            var token = tokens[j];
            // 1. matches a key in Interface&&&method format
            if (features[token] != undefined) {
                table[filename][token] = "O";
            } else {
                token = token.split("&&&")[1];

                // 2. matches a method/attr
                if (features[token] != undefined) {
                    table[filename][token] = "O";
                } else if (features["*&&&"+token] != undefined) { // mataches a key in *&&&method format
                    table[filename]["*&&&"+token] = "O";
                }
            }
        }
    }

    // Add 'coverage row'
    table["coverage"] = {};
    for (var i=0; i < keys.length; i++) {
        var key = keys[i];
        var found = false;
        for (var row in table) {
            var values = table[row];
            if (values[key] == "O") {
                found = true;
                break;
            }
        }

        table["coverage"][key] = "";
        if (found) {
            table["coverage"][key] = "O"
        }
    }


    // print interface
    process.stdout.write("\t\t");
    for (var i=0; i < interfaces.length; i++) {
        if (features[keys[i]] != "Other") {
            process.stdout.write('\t'+interfaces[i]);
        }
    }
    console.log();


    // print methods
    process.stdout.write("\tTC\tCoverage");
    for (var i=0; i < keys.length; i++) {
        if (features[keys[i]] != "Other") {
            var method = keys[i]; // remove Interface&&& prefix if exists
            process.stdout.write('\t'+method.substring(method.lastIndexOf("&")+1));
         }
    }
    console.log();

    process.stdout.write(TCName);
    for (var row in table) {
        process.stdout.write("\t"+row); // print filename
        var values = table[row];

        // print coverage percentage
        var count=0;
        for (var j=0; j < keys.length; j++) {
            var v = keys[j];
            if (features[keys[j]] != "Other") {
                if (values[v] == "O") {
                    count++;
                }
            }
        }
        process.stdout.write("\t"+(count/keys.length).toFixed(2));

        // print O if method exists
        for (var j=0; j < keys.length; j++) {
            var v = keys[j];
            if (features[keys[j]] != "Other") {
                process.stdout.write('\t');
                if (values[v] == "O") {
                    process.stdout.write(values[v]);
                }
            }
        }
        console.log();
    }
}

main();
