var module = {};
var requireHash = {};
var print = console.log;
console.logd = print;

function require(name) {
    if (!requireHash[name]) {
        try {
            load(prefix + name + ".js");
            print("Loaded " + name + ".js");
        } catch (e) {
            print(e);
            print("Failed to load " + prefix + name + ".js");
        }
        requireHash[name] = module.exports;
    }
    return requireHash[name];
}

function loadPlugin(tizen, name) {
    try {
        load(prefix + name);
        for (var it in module.exports) {
            tizen[it] = module.exports[it]['value'];
        }
        print("Loaded plugin " + name);
        gc();
    } catch (e) {
        print(e);
        print("Failed to load plugin " + prefix + name);
    }
    return tizen;
}

function WebAPIException(type, msg) {
    return Error(msg);
}

print("Initialize plugins ============================================");
var start = new Date();
var tizen = {};
loadPlugin(tizen, "tizen.messageport.plugin");
//loadPlugin(tizen, "tizen.filesystem.plugin");
loadPlugin(tizen, "tizen.newfilesystem.plugin");
tizen.filesystem = tizen.newfilesystem;
loadPlugin(tizen, "tizen.sensorservice.plugin");
loadPlugin(tizen, "tizen.application.plugin");
loadPlugin(tizen, "tizen.systeminfo.plugin");
var end = new Date();
print("Plugin initialization took " + (end-start) + "ms");
print("===============================================================");

