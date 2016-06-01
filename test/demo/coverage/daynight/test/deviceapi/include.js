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
    Object.defineProperty(tizen, name, {
        get: function() {
            delete tizen[name];
            try {
                load(prefix + "tizen." + name + ".plugin");
                var hasName = false;
                for (var it in module.exports) {
                    Object.defineProperty(tizen, it, module.exports[it]);
                    if (it == name) hasName = true;
                }
                if (!hasName) {
                    throw "Plugin code do not have name " + name;
                }
                gc();
                print("Loaded plugin " + name);
            } catch (e) {
                print(e);
                print("Failed to load plugin " + prefix + name);
            }
            return tizen[name];
        },
        enumerable: true,
        configurable: true
    });
}

function WebAPIException(type, msg) {
    return Error(msg);
}

print("Initialize plugins ============================================");
var start = new Date();
var tizen = {};
loadPlugin(tizen, "messageport");
//loadPlugin(tizen, "tizen.filesystem.plugin");
loadPlugin(tizen, "newfilesystem");
//tizen.filesystem = tizen.newfilesystem;
loadPlugin(tizen, "sensorservice");
loadPlugin(tizen, "application");
loadPlugin(tizen, "systeminfo");
var end = new Date();
print("Plugin initialization took " + (end-start) + "ms");
print("===============================================================");

