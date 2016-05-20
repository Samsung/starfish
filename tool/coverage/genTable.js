var fs = require("fs");


function main() {
    if (process.argv.length != 4) {
        console.log("Usage: "+process.argv[0]+ " " +process.argv[1] + " spec.txt file.raw.csv");
        return;
    }

    var spec = process.argv[2];
    var file = process.argv[3];
    run(spec, file);
}

function run(spec, file) {

    // read attrs and methods
    var features = {};
    var domList = fs.readFileSync(spec, "utf8").split("\n").slice(1); // removing title
    for (var i=0; i < domList.length; i++) {
        var name = domList[i]; // methods and attr column
        if (name != undefined) {
            name = name.trim();
            if (name != "" && name.indexOf("#") != 0) {
                features[name] = "";
            }
        }
    }

    // create a table containing all features
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
            if (features[token] == "") { // key exists
                table[filename][token] = "O";
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


    // print title
    process.stdout.write("TC");
    for (var i=0; i < keys.length; i++) {
        process.stdout.write('\t'+keys[i]);
    }
    console.log();

    for (var row in table) {
        process.stdout.write(row); // print filename
        var values = table[row];

        for (var j=0; j < keys.length; j++) {
            var v = keys[j];
            process.stdout.write('\t');
            if (values[v] == "O") {
                process.stdout.write(values[v]);
            }
        }
        console.log();
    }
}

main();
