/*
print("--MESSAGE PORT-------------------------------------------------");
try {
    var ret = tizen.messageport.requestLocalMessagePort('MessagePortA');
    print(ret);
} catch (e) {
    print(e);
    print("failed to get message port");
}

print("--FILESYSTEM---------------------------------------------------");
try {
    var ret = tizen.newfilesystem.maxPathLength;
    print(ret);
    ret = tizen.newfilesystem.resolve('/opt/usr/media/ejjeong/test1.js', onSuccessCallback, onErrorCallback);
    print(ret);
} catch (e) {
    print(e);
    print("failure in newfs");
}
*/

var lightSensor;
var canUseSensor = false;
print("--SENSOR-------------------------------------------------------");
try {
    print(tizen.sensorservice);
    var ret;
    var sensors = tizen.sensorservice.getAvailableSensors();
    print("available sensors : " + sensors);
    for (var i in sensors)
        print(i + " : " + sensors[i]);
    lightSensor = tizen.sensorservice.getDefaultSensor('LIGHT');
    print("light sensor : " + lightSensor);
    ret = lightSensor.start(function(succ) {
        print("light sensor started");
        canUseSensor = true;
        print(succ);
        ret = lightSensor.getLightSensorData(onSuccessCallback('sensor'), onErrorCallback('sensor'));
        print(ret);
    }, function(err) {
        print(err.toString());
        print("failed to start light sensor");
    });
    print(ret);
} catch (e) {
    print(e.toString());
    print("failed to use light sensor");
}

/*
print("--APPLICATION--------------------------------------------------");
try {
    var ret = tizen.application.getCurrentApplication();
    print(ret);
} catch (e) {
    print(e);
    print("failed to get current application");
}

print("--SYSTEM INFORMATION-------------------------------------------");
try {
    print(tizen.systeminfo.getPropertyValue);
    var ret = tizen.systeminfo.getPropertyValue("CPU", onSuccessCallback('sysinfo'), onErrorCallback('sysinfo'));
    print("getpropvalue state : " + ret);
} catch (e) {
    print(e);
    print("failed to get sysinfo");
}
*/

function onSuccessCallback(id) {
    return function (succ) {
        print("CALLBACK: " + id + ": succ --------");
        console.log(succ);
        if (typeof succ == "object") {
            for (var i in succ) {
                print(i + " : " + succ[i]);
            }
            document.getElementById("time").firstChild.data = succ.lightLevel;
        }
    }
}
function onErrorCallback(id) {
    return function (error) {
        print("CALLBACK: " + id + ": error -------");
        console.log(error);
        if (typeof error == "object") {
            for (var i in error) {
                print(i + " : " + error[i]);
            }
        }
    }
}
