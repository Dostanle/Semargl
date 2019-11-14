/* jshint -W100 */
var connection = new WebSocket('ws://' + location.hostname + '/ws', ['arduino']);
var dateDifference;
var dateServer;
var settings;

connection.onopen = function(e) { //при конекті отримує дані показників wVal та yVal тощо
    setTimeout(requestIndicators(), 1000);
    setTimeout(getSettings(), 9000);

    console.log(e);
};
connection.onerror = function(error) {
    console.log('WebSocket Error ', error);
};
connection.onmessage = function(e) {
    console.log('Server: ', e.data);
    var serverJson = JSON.parse(e.data);
    switch (serverJson.type) {
        case 1:
            break;
        case 2:
            console.log('New User & Password: ', serverJson.user + " " + serverJson.pass);
            break;
        case 3:
            console.log('New WiFi & Password: ', serverJson.ssid + " " + serverJson.ssidpass);
            break;
        case 4:
            console.log('New Date & Time: ', serverJson.setDate);
            break;
        case 5:
            dateServerUnix = parseInt(serverJson.serverClock);
            dateServer = new Date(parseInt(serverJson.serverClock));
            dateDifference = new Date() - dateServer;
            console.log('serverJson.serverClock: ', serverJson.serverClock);
            console.log('dateServer: ', dateServer);
            break;
        case 6:
            var ventNich;
            var vent;
            var light;
            var hum;
            var water;
            switch (serverJson.form) {
                case 1:
                    console.log('Message from server: ', JSON.stringify(serverJson));
                    break;
                case 2:
                    console.log('Message from server: ', JSON.stringify(serverJson));
                    break;
                case 3:
                    console.log('Message from server: ', JSON.stringify(serverJson));
                    break;
                case 4:
                    console.log('Message from server: ', JSON.stringify(serverJson));
                    break;
                case 5:
                    console.log('Message from server: ', JSON.stringify(serverJson));
                    break;
                case 6:
                    settings = serverJson;
                    setSettings(serverJson);

                    console.log('Message from server: ', JSON.stringify(serverJson));
                    break;
                case 7:
                    setSettings(serverJson);

                    console.log('Message from server: ', JSON.stringify(serverJson));
                    break;
                default:
                    console.log('Message from server: ', JSON.stringify(serverJson));
            }
            break;
        case 7:
            dateServer = new Date(parseInt(serverJson.serverClock));
            dateDifference = new Date() - dateServer;
            document.getElementById('soil').innerHTML = serverJson.soil;
            document.getElementById('hum').innerHTML = serverJson.hum;
            document.getElementById('temp').innerHTML = serverJson.temp;
            break;
        case 8:
            document.getElementById('deblog1').innerHTML = serverJson.waterLevel;

            console.log('Message from server: ', JSON.stringify(serverJson));
            break;
        case 9:
            document.getElementById('deblog2').innerHTML = serverJson.countLines;

            console.log('Message from server: ', JSON.stringify(serverJson));
            break;
        default:
            console.log('Message from server: ', JSON.stringify(serverJson));
    }
};

function sendJSON() {
    var type = 1;
    var w = parseInt(document.getElementById('w').value);
    var json = {
        "type": type,
        "w": w
    };
    console.log('json: ' + JSON.stringify(json));
    connection.send(JSON.stringify(json));
}

function changeUserPass() {
    var type = 2;
    var user = document.getElementById("user").value;
    var pass = document.getElementById("pass").value;
    var pattern = new RegExp(/^[a-zA-Z0-9]+$/);
    var userCheck = pattern.test(user);
    var passCheck = pattern.test(pass);
    if (userCheck == true && passCheck == true && user.length <= 20 && pass.length <= 20) {
        if (confirm("Для внесення змін треба перезавантажити Семаргл, перезавантажити?")) {
            alert("Пристрій буде перезавантажено");
            var json = {
                "type": type,
                "user": user,
                "pass": pass
            };
            connection.send(JSON.stringify(json));
            console.log('json: ' + JSON.stringify(json));
        } else {
            alert("Внесіть зміни пізніше і перезавантажте Семаргл");
            return;
        }
    } else if (userCheck !== true || user.length > 20) {
        alert("Логін має складатись тільки з цифр та латинських літер не більше 20 символів");
    } else if (passCheck !== true || pass.length > 20) {
        alert("Пароль має складатись тільки з цифр та латинських літер не більше 20 символів");
    }
}

function addWIFINetwork() {
    var type = 3;
    var ssid = document.getElementById("ssid").value;
    var ssidpass = document.getElementById("ssidpass").value;
    var pattern = new RegExp(/^[a-zA-Z0-9]+$/);
    var ssidCheck = pattern.test(ssid);
    var passCheck = pattern.test(ssidpass);
    if (ssidCheck == true && passCheck == true && ssid.length <= 32 && ssidpass.length <= 32) {
        alert("Пристрій буде перезавантажено");
        var json = {
            "type": type,
            "ssid": ssid,
            "ssidpass": ssidpass
        };
        connection.send(JSON.stringify(json));
        console.log('json: ' + JSON.stringify(json));
    } else if (ssidCheck !== true || ssid.length > 32) {
        alert("SSID мережі має складатись тільки з цифр та латинських літер не більше 32 символів");
    } else if (passCheck !== true || ssidpass.length > 32) {
        alert("Пароль має складатись тільки з цифр та латинських літер не більше 32 символів");
    }
}

function changeDate() {
    var date = document.getElementById("date").value;
    var time = document.getElementById("time").value;
    if (confirm("Змінити час на " + date + " " + time + "?")) {
        var type = 4;
        var json = {
            "type": type,
            "date": date,
            "time": time
        };
        console.log('json: ' + JSON.stringify(json));
        connection.send(JSON.stringify(json));
        location.reload();
    } else {
        alert("Зміни не буде збережено");
        return;
    }
}

function sendSettings() {
    var json = {};
    json.form = 6;
    json.type = 6;
    json.ventNight = sendVentNich();
    json.vent = sendVent();
    json.light = sendSvit();
    json.hum = sendPar();
    json.water = sendVoda();

    console.log('json: ' + JSON.stringify(json));
    connection.send(JSON.stringify(json));
}

function getSettings() {
    var json = {};
    json.form = 7;
    json.type = 6;

    console.log('json: ' + JSON.stringify(json));
    connection.send(JSON.stringify(json));
}

function requestIndicators() {
    var json = {};
    json.type = 7;

    console.log('json: ' + JSON.stringify(json));
    connection.send(JSON.stringify(json));
}

function setSettings(serverJson) {
    console.log('serverJson: ' + JSON.stringify(serverJson));
    ventNich = serverJson.ventNight;
    $("#ventNichIntDiap").bootstrapSlider('setValue', ventNich.interval[0], true);
    $("#ventNichChasDiap").bootstrapSlider('setValue', ventNich.interval[1], true);
    if (ventNich.detector[0] == 1) {
        $("#ventNichVolCheck").prop("checked", true);
        $("#ventNichVolDiap").bootstrapSlider("enable");
        $("#sliderNichVentVol .slider-selection").css('background', 'green');
    } else {
        $("#ventNichVolCheck").prop("checked", false);
        $("#ventNichVolDiap").bootstrapSlider("disable");
        $("#sliderNichVentVol .slider-selection").css('background', '#ccc');
    }
    if (ventNich.detector[2] == 1) {
        $("#ventNichTempCheck").prop("checked", true);
        $("#ventNichTempDiap").bootstrapSlider("enable");
        $("#sliderNichVentTemp .slider-selection").css('background', 'green');
    } else {
        $("#ventNichTempCheck").prop("checked", false);
        $("#ventNichTempDiap").bootstrapSlider("disable");
        $("#sliderNichVentTemp .slider-selection").css('background', '#ccc');
    }
    $("#ventNichVolDiap").bootstrapSlider('setValue', ventNich.detector[1], true);
    $("#ventNichTempDiap").bootstrapSlider('setValue', ventNich.detector[3], true);

    vent = serverJson.vent;
    $("#ventIntDiap").bootstrapSlider('setValue', vent.interval[0], true);
    $("#ventChasDiap").bootstrapSlider('setValue', vent.interval[1], true);
    if (vent.detector[0] == 1) {
        $("#ventVolCheck").prop("checked", true);
        $("#ventVolDiap").bootstrapSlider("enable");
        $("#sliderVentVol .slider-selection").css('background', 'green');
    } else {
        $("#ventVolCheck").prop("checked", false);
        $("#ventVolDiap").bootstrapSlider("disable");
        $("#sliderVentVol .slider-selection").css('background', '#ccc');
    }
    if (vent.detector[2] == 1) {
        $("#ventTempCheck").prop("checked", true);
        $("#ventTempDiap").bootstrapSlider("enable");
        $("#sliderVentTemp .slider-selection").css('background', 'green');
    } else {
        $("#ventTempCheck").prop("checked", false);
        $("#ventTempDiap").bootstrapSlider("disable");
        $("#sliderVentTemp .slider-selection").css('background', '#ccc');
    }
    $("#ventVolDiap").bootstrapSlider('setValue', vent.detector[1], true);
    $("#ventTempDiap").bootstrapSlider('setValue', vent.detector[3], true);

    light = serverJson.light;
    var timezone = new Date().getTimezoneOffset();
    var minLight = parseInt((light.interval[0] / 60) + (timezone / 60 * -1));
    var maxLight = parseInt((light.interval[1] / 60) + (timezone / 60 * -1));
    if (minLight >= 24){
        minLight = minLight - 24;
    }
    if (maxLight >= 24){
        maxLight = maxLight - 24;
    }
    $("#svitDiap").bootstrapSlider('setValue', [minLight, maxLight]);

    hum = serverJson.hum;
    $("#parIntDiap").bootstrapSlider('setValue', hum.interval[0], true);
    $("#parChasDiap").bootstrapSlider('setValue', hum.interval[1], true);
    $("#parVolDiap").bootstrapSlider('setValue', [hum.detector[0], hum.detector[1]], true);

    water = serverJson.water;
    $("#vodaIntDiap").bootstrapSlider('setValue', water.interval[0], true);
    $("#vodaChasDiap").bootstrapSlider('setValue', water.interval[1], true);
    $("#vodaZemDiap").bootstrapSlider('setValue', water.detector, true);

    $("#svitPerButtons").find(".per").each(function(index, elem) {
        if (light.toggle == index + 1) {
            $(elem).addClass('active');
            $(elem).parent().parent().parent().find(".pokaz").each(function(index2, elem2) {
                if (index == index2) {
                    $(elem2).collapse('show');
                }
            });
        }
    });
    $("#ventPerButtons").find(".per").each(function(index, elem) {
        if (vent.toggle == index + 1) {
            $(elem).addClass('active');
            $(elem).parent().parent().parent().find(".pokaz").each(function(index2, elem2) {
                if (index == index2) {
                    $(elem2).collapse('show');
                }
            });
        }
    });
    $("#ventPerNichButtons").find(".per").each(function(index, elem) {
        if (ventNich.toggle == index + 1) {
            $(elem).addClass('active');
            $(elem).parent().parent().parent().find(".pokaz").each(function(index2, elem2) {
                if (index == index2) {
                    $(elem2).collapse('show');
                }
            });
        }
    });
    $("#parPerButtons").find(".per").each(function(index, elem) {
        if (hum.toggle == index + 1) {
            $(elem).addClass('active');
            $(elem).parent().parent().parent().find(".pokaz").each(function(index2, elem2) {
                if (index == index2) {
                    $(elem2).collapse('show');
                }
            });
        }
    });
    $("#vodaPerButtons").find(".per").each(function(index, elem) {
        if (water.toggle == index + 1) {
            $(elem).click();
            $(elem).parent().parent().parent().find(".pokaz").each(function(index2, elem2) {
                if (index == index2) {
                    $(elem2).collapse('show');
                }
            });
        }
    });
}

function sendVentNich() {
    var toggle;
    var interval = $("#ventNichIntDiap").bootstrapSlider('getValue');
    var chas = $("#ventNichChasDiap").bootstrapSlider('getValue');
    var volCheck = $("#ventNichVolCheck").is(":checked");
    var tempCheck = $("#ventNichTempCheck").is(":checked");
    var vol = $("#ventNichVolDiap").bootstrapSlider('getValue');
    var temp = $("#ventNichTempDiap").bootstrapSlider('getValue');
    $("#ventPerNichButtons").children(".per").each(function(index, elem) {
        if ($(elem).hasClass("active")) {
            toggle = index;
        }
    });
    if (volCheck == true) {
        volCheck = 1;
    } else {
        volCheck = 0;
    }
    if (tempCheck == true) {
        tempCheck = 1;
    } else {
        tempCheck = 0;
    }
    var ventNight = {
        toggle: toggle + 1,
        interval: [interval, chas],
        detector: [volCheck, vol, tempCheck, temp]
    };
    return ventNight;
}

function sendVent() {
    var toggle;
    var interval = $("#ventIntDiap").bootstrapSlider('getValue');
    var chas = $("#ventChasDiap").bootstrapSlider('getValue');
    var volCheck = $("#ventVolCheck").is(":checked");
    var tempCheck = $("#ventTempCheck").is(":checked");
    var vol = $("#ventVolDiap").bootstrapSlider('getValue');
    var temp = $("#ventTempDiap").bootstrapSlider('getValue');
    $("#ventPerButtons").children(".per").each(function(index, elem) {
        if ($(elem).hasClass("active")) {
            toggle = index;
        }
    });
    if (volCheck == true) {
        volCheck = 1;
    } else {
        volCheck = 0;
    }
    if (tempCheck == true) {
        tempCheck = 1;
    } else {
        tempCheck = 0;
    }
    var vent = {
        toggle: toggle + 1,
        interval: [interval, chas],
        detector: [volCheck, vol, tempCheck, temp]
    };
    return vent;
}

function sendSvit() {
    var toggle;
    var interval = $("#svitDiap").bootstrapSlider('getValue');
    $("#svitPerButtons").children(".per").each(function(index, elem) {
        if ($(elem).hasClass("active")) {
            toggle = index;
        }
    });
    var timezone = new Date().getTimezoneOffset();
    var min = interval[0] * 60 + timezone;
    var max = interval[1] * 60 + timezone;
    console.log('svit: ' + (interval[0] * 60 + timezone));
    console.log('svit: ' + (interval[1] * 60 + timezone));
    if (min < 0 || min > 1440) {
        min = 1440 + min;
    } else if (min == 1440) {
        min = 0;
    }
    if (max < 0 || max > 1440) {
        max = 1440 + max;
    } else if (max == 1440) {
        max = 0;
    }

    var light = {
        toggle: toggle + 1,
        interval: [min, max],
    };
    return light;
}

function sendPar() {
    var toggle;
    var interval = $("#parIntDiap").bootstrapSlider('getValue');
    var chas = $("#parChasDiap").bootstrapSlider('getValue');
    var humDiap = $("#parVolDiap").bootstrapSlider('getValue');
    $("#parPerButtons").children(".per").each(function(index, elem) {
        if ($(elem).hasClass("active")) {
            toggle = index;
        }
    });
    var hum = {
        toggle: toggle + 1,
        interval: [interval, chas],
        detector: [humDiap[0], humDiap[1]]
    };
    return hum;
}

function sendVoda() {
    var toggle;
    var interval = $("#vodaIntDiap").bootstrapSlider('getValue');
    var chas = $("#vodaChasDiap").bootstrapSlider('getValue');
    var waterDiap = $("#vodaZemDiap").bootstrapSlider('getValue');
    $("#vodaPerButtons").children(".per").each(function(index, elem) {
        if ($(elem).hasClass("active")) {
            toggle = index;
        }
    });
    var water = {
        toggle: toggle + 1,
        interval: [interval, chas],
        detector: waterDiap
    };
    return water;
}