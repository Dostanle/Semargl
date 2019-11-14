    $(document).ready(function() {
        /*    $("#ventBut1").click(function() {
          $("#collapse1").collapse('show');
          $("#collapse2").collapse('hide');
          $("#collapse3").collapse('hide');
          $("#collapse4").collapse('hide');
        */
        $(".perem").each(function(index1, elem1) {
            $(elem1).find(".per").each(function(index2, elem2) {
                $(elem2).click(function() {
                    $(elem2).parent().parent().parent().find(".pokaz").each(function(index3, elem3) {
                        if (index2 == index3) {
                            $(elem3).collapse('show');
                        } else {
                            $(elem3).collapse('hide');
                        }
                    });
                });
            });
        });
        $(".alio").each(function(index1, elem1) {
            $(elem1).children(".pokaz").each(function(index2, elem2) {
                $(elem2).on('show.bs.collapse', function() {
                    $(elem2).parent().parent().find(".dis").addClass("disabled");
                    $(elem2).parent().parent().find(".per").each(function(index3, elem3) {
                        if (index2 == index3) {
                            $(elem3).addClass("active");
                        }
                    });
                });
                $(elem2).on('shown.bs.collapse', function() {
                    $(elem2).parent().parent().find(".dis").removeClass("disabled");
                    $(elem2).parent().parent().find(".per").each(function(index3, elem3) {
                        if (index2 == index3) {
                            $(elem3).addClass("active");
                        }
                    });
                });
                $(elem2).on('hide.bs.collapse', function() {
                    $(elem2).parent().parent().find(".dis").addClass("disabled");
                    $(elem2).parent().parent().find(".per").each(function(index3, elem3) {
                        if (index2 == index3) {
                            $(elem3).removeClass("active");
                        }
                    });
                });
                $(elem2).on('hidden.bs.collapse', function() {
                    $(elem2).parent().parent().find(".dis").removeClass("disabled");
                    $(elem2).parent().parent().find(".per").each(function(index3, elem3) {
                        if (index2 == index3) {
                            $(elem3).removeClass("active");
                        }
                    });
                });
            });
        });

        /*     $(".parPokaz").each(
            function(index, elem) {
              $(elem).on('show.bs.collapse', function() {
                $(".dis").addClass("disabled");
                $("#parBut" + (index + 1)).addClass("active");
              });
              $(elem).on('shown.bs.collapse', function() {
                $(".dis").removeClass("disabled");
                $("#parBut" + (index + 1)).addClass("active");
              });
              $(elem).on('hide.bs.collapse', function() {
                $(".dis").addClass("disabled");
                $("#parBut" + (index + 1)).removeClass("active");
              });
              $(elem).on('hidden.bs.collapse', function() {
                $(".dis").removeClass("disabled");
                $("#parBut" + (index + 1)).removeClass("active");
              });
            }
          ); */


        $("#ventIntDiap").bootstrapSlider({
            id: "sliderVentInt",
            min: 5,
            max: 60,
            value: 15,
            focus: true,
        });
        $("#ventChasDiap").bootstrapSlider({
            id: "sliderVentChas",
            min: 1,
            max: 10,
            value: 5,
            focus: true,
        });
        $("#ventVolDiap").bootstrapSlider({
            id: "sliderVentVol",
            min: 10,
            max: 90,
            value: 25,
            selection: 'after',
            focus: true,
        });
        $("#ventTempDiap").bootstrapSlider({
            id: "sliderVentTemp",
            min: 10,
            max: 45,
            value: 20,
            selection: 'after',
            focus: true,
        });
        $("#ventNichIntDiap").bootstrapSlider({
            id: "sliderNichVentInt",
            min: 5,
            max: 60,
            value: 15,
            focus: true,
        });
        $("#ventNichChasDiap").bootstrapSlider({
            id: "sliderNichVentChas",
            min: 1,
            max: 10,
            value: 5,
            focus: true,
        });
        $("#ventNichVolDiap").bootstrapSlider({
            id: "sliderNichVentVol",
            min: 10,
            max: 90,
            value: 25,
            selection: 'after',
            focus: true,
        });
        $("#ventNichTempDiap").bootstrapSlider({
            id: "sliderNichVentTemp",
            min: 10,
            max: 45,
            value: 20,
            selection: 'after',
            focus: true,
        });
        $("#svitDiap").bootstrapSlider({
            id: "sliderSvit",
            min: 0,
            max: 23,
            value: [0, 10],
            focus: true,
        });
        $("#parIntDiap").bootstrapSlider({
            id: "sliderParInt",
            min: 10,
            max: 60,
            value: 15,
            focus: true,
        });
        $("#parChasDiap").bootstrapSlider({
            id: "sliderParChas",
            min: 1,
            max: 10,
            value: 5,
            focus: true,
        });
        $("#parVolDiap").bootstrapSlider({
            id: "sliderParVol",
            min: 5,
            max: 95,
            value: [10, 50],
            focus: true,
        });
        $("#vodaIntDiap").bootstrapSlider({
            id: "sliderVodaInt",
            min: 1,
            max: 20,
            value: 15,
            focus: true,
        });
        $("#vodaChasDiap").bootstrapSlider({
            id: "sliderVodaChas",
            min: 1,
            max: 10,
            value: 5,
            focus: true,
        });
        $("#vodaZemDiap").bootstrapSlider({
            id: "sliderZemVol",
            min: 10,
            max: 4096,
            value: 25,
            focus: true,
        });
        $("#ventIntDiap").bootstrapSlider().on("change", function(event) {
            var a = event.value.newValue;
            if ($("#ventChasDiap").bootstrapSlider('getValue') > a) {
                $("#ventChasDiap").bootstrapSlider('setValue', a, true);
            }
            $("#ventChasDiap").bootstrapSlider('setAttribute', 'max', (a - 1));
            $("#ventChasDiap").bootstrapSlider('setValue', $("#ventChasDiap").bootstrapSlider('getValue'), true);
        });

        $("#ventNichIntDiap").bootstrapSlider().on("change", function(event) {
            var a = event.value.newValue;
            if ($("#ventNichChasDiap").bootstrapSlider('getValue') > a) {
                $("#ventNichChasDiap").bootstrapSlider('setValue', a, true);
            }
            $("#ventNichChasDiap").bootstrapSlider('setAttribute', 'max', (a - 1));
            $("#ventNichChasDiap").bootstrapSlider('setValue', $("#ventNichChasDiap").bootstrapSlider('getValue'), true);
        });

        $("#parIntDiap").bootstrapSlider().on("change", function(event) {
            var a = event.value.newValue;
            if ($("#parChasDiap").bootstrapSlider('getValue') > a) {
                $("#parChasDiap").bootstrapSlider('setValue', a, true);
            }
            $("#parChasDiap").bootstrapSlider('setAttribute', 'max', (a - 1));
            $("#parChasDiap").bootstrapSlider('setValue', $("#parChasDiap").bootstrapSlider('getValue'), true);
        });
        /////////////////////////////////////////////////////////////////////////////////   
        $("#svitDiap").bootstrapSlider().on("slideStop", function() {
            jsonSvit();
        });

        $("#parIntDiap").bootstrapSlider().on("slideStop", function() {
            jsonPar();
        });
        $("#parChasDiap").bootstrapSlider().on("slideStop", function() {
            jsonPar();
        });

        $("#vodaIntDiap").bootstrapSlider().on("slideStop", function() {
            jsonVoda();
        });
        $("#vodaChasDiap").bootstrapSlider().on("slideStop", function() {
            jsonVoda();
        });
        $("#vodaZemDiap").bootstrapSlider().on("slideStop", function() {
            jsonVoda();
        });


        $("#parVolDiap").bootstrapSlider().on("slideStop", function() {
            if ($("#ventNichBut2").hasClass("active")) {
                setVent();
            }
            if ($("#ventBut2").hasClass("active")) {
                setVent();
            }
            jsonPar();
        });
        $("#parVolDiap").bootstrapSlider().on("change", function() {
            if ($("#ventNichBut2").hasClass("active")) {
                setVent();
            }
            if ($("#ventBut2").hasClass("active")) {
                setVent();
            }
        });

        function setVent() {
            var a = $("#parVolDiap").bootstrapSlider('getValue');
            if ($("#ventNichVolDiap").bootstrapSlider('getValue') < a[1]) {
                $("#ventNichVolDiap").bootstrapSlider('setValue', a[1] + 5, true);
            }
            if ($("#ventVolDiap").bootstrapSlider('getValue') < a[1]) {
                $("#ventVolDiap").bootstrapSlider('setValue', a[1] + 5, true);
            }
        }
        /////////////////////////////////////////////////////////////////////////////////    
        $("#ventNichVolCheck").change(function() {
            if ($("#parBut2").hasClass("active")) {
                setNightHum();
            }
            jsonVentNich();
        });
        $("#ventNichTempCheck").change(function() {
            if ($("#parBut2").hasClass("active")) {
                setNightHum();
            }
            jsonVentNich();
        });
        $("#ventNichVolDiap").bootstrapSlider().on("slideStop", function(event) {
            if ($("#parBut2").hasClass("active")) {
                setNightHum();
            }
            jsonVentNich();
        });
        $("#ventNichTempDiap").bootstrapSlider().on("slideStop", function(event) {
            if ($("#parBut2").hasClass("active")) {
                setNightHum();
            }
            jsonVentNich();
        });
        $("#ventNichIntDiap").bootstrapSlider().on("slideStop", function() {
            jsonVentNich();
        });
        $("#ventNichChasDiap").bootstrapSlider().on("slideStop", function() {
            jsonVentNich();
        });

        function setNightHum() {
            if ($("#ventNichVolCheck").prop("checked")) {
                var a = $("#ventNichVolDiap").bootstrapSlider('getValue');
                var b = $("#parVolDiap").bootstrapSlider('getValue');
                if (b[1] > a) {
                    if (b[0] > a) {
                        if ((a - 15) <= 0) {
                            $("#parVolDiap").bootstrapSlider('setValue', [5, 10], true);
                        } else {
                            $("#parVolDiap").bootstrapSlider('setValue', [a - 10, a - 5], true);
                        }
                    } else {
                        $("#parVolDiap").bootstrapSlider('setValue', [b[0], a - 5], true);
                    }
                }
            }
        }
        /////////////////////////////////////////////////////////////////////////////////  
        $("#ventVolCheck").change(function() {
            if ($("#parBut2").hasClass("active")) {
                setHum();
            }
            jsonVent();
        });
        $("#ventTempCheck").change(function() {
            if ($("#parBut2").hasClass("active")) {
                setHum();
            }
            jsonVent();
        });
        $("#ventVolDiap").bootstrapSlider().on("slideStop", function(event) {
            if ($("#parBut2").hasClass("active")) {
                setHum();
            }
            jsonVent();
        });
        $("#ventTempDiap").bootstrapSlider().on("slideStop", function(event) {
            if ($("#parBut2").hasClass("active")) {
                setHum();
            }
            jsonVent();
        });
        $("#ventIntDiap").bootstrapSlider().on("slideStop", function() {
            jsonVent();
        });
        $("#ventChasDiap").bootstrapSlider().on("slideStop", function() {
            jsonVent();
        });

        function setHum() {
            if ($("#ventVolCheck").prop("checked")) {
                var a = $("#ventVolDiap").bootstrapSlider('getValue');
                var b = $("#parVolDiap").bootstrapSlider('getValue');
                if (b[1] > a) {
                    if (b[0] > a) {
                        if ((a - 15) <= 0) {
                            $("#parVolDiap").bootstrapSlider('setValue', [5, 10], true);
                        } else {
                            $("#parVolDiap").bootstrapSlider('setValue', [a - 10, a - 5], true);
                        }
                    } else {
                        $("#parVolDiap").bootstrapSlider('setValue', [b[0], a - 5], true);
                    }
                }
            }
        }
        /////////////////////////////////////////////////////////////////////////////////    
        //$("#vodaIntDiap").bootstrapSlider().on("change", function(event) {
        //  var a = event.value.newValue;
        //  if ($("#vodaChasDiap").bootstrapSlider('getValue') > a) {
        //    $("#vodaChasDiap").bootstrapSlider('setValue', a, true);
        //  }
        //  $("#vodaChasDiap").bootstrapSlider('setAttribute', 'max', (a - 1));
        //  $("#vodaChasDiap").bootstrapSlider('setValue', $("#vodaChasDiap").bootstrapSlider('getValue'), true);
        //});

        $("#ventVolCheck").change(function() {
            if (this.checked) {
                $("#ventVolDiap").bootstrapSlider("enable");
                $("#sliderVentVol .slider-selection").css('background', 'green');
            } else {
                $("#ventVolDiap").bootstrapSlider("disable");
                $("#sliderVentVol .slider-selection").css('background', '#ccc');
            }
        });
        $("#ventTempCheck").change(function() {
            if (this.checked) {
                $("#ventTempDiap").bootstrapSlider("enable");
                $("#sliderVentTemp .slider-selection").css('background', 'green');
            } else {
                $("#ventTempDiap").bootstrapSlider("disable");
                $("#sliderVentTemp .slider-selection").css('background', '#ccc');
            }
        });
        $("#ventNichVolCheck").change(function() {
            if (this.checked) {
                $("#ventNichVolDiap").bootstrapSlider("enable");
                $("#sliderNichVentVol .slider-selection").css('background', 'green');
            } else {
                $("#ventNichVolDiap").bootstrapSlider("disable");
                $("#sliderNichVentVol .slider-selection").css('background', '#ccc');
            }
        });
        $("#ventNichTempCheck").change(function() {
            if (this.checked) {
                $("#ventNichTempDiap").bootstrapSlider("enable");
                $("#sliderNichVentTemp .slider-selection").css('background', 'green');
            } else {
                $("#ventNichTempDiap").bootstrapSlider("disable");
                $("#sliderNichVentTemp .slider-selection").css('background', '#ccc');
            }
        });

        $("#ventVolDiap").bootstrapSlider("disable");
        $("#sliderVentVol .slider-selection").css('background', '#ccc');

        $("#ventTempDiap").bootstrapSlider("disable");
        $("#sliderVentTemp .slider-selection").css('background', '#ccc');

        $("#ventNichVolDiap").bootstrapSlider("disable");
        $("#sliderNichVentVol .slider-selection").css('background', '#ccc');

        $("#ventNichTempDiap").bootstrapSlider("disable");
        $("#sliderNichVentTemp .slider-selection").css('background', '#ccc');

        /* 
            $("#ventPerNichButtons").click(function() {
              var json = {};
              json.form = 1;
              json.type = 6;
              json.ventNight = getVentNich();

              console.log('json: ' + JSON.stringify(json));
              connection.send(JSON.stringify(json));
            }); */

        function getVentNich() {
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

        function jsonVentNich() {
            var json = {};
            json.form = 1;
            json.type = 6;
            json.ventNight = getVentNich();
            json.hum = getPar();

            console.log('json: ' + JSON.stringify(json));
            connection.send(JSON.stringify(json));
        }

        $("#ventPerNichButtons").children(".per").each(function(index, elem) {
            if (index != 1) {
                $(elem).click(function() {
                    jsonVentNich();
                });
            } else {
                $(elem).click(function() {
                    if ($("#parBut2").hasClass("active")) {
                        setNightHum();
                    }
                    jsonVentNich();
                });
            }
        });

        function getVent() {
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

        function jsonVent() {
            var json = {};
            json.form = 2;
            json.type = 6;
            json.vent = getVent();
            json.hum = getPar();

            console.log('json: ' + JSON.stringify(json));
            connection.send(JSON.stringify(json));
        }
        $("#ventPerButtons").children(".per").each(function(index, elem) {
            if (index != 1) {
                $(elem).click(function() {
                    jsonVent();
                });
            } else {
                $(elem).click(function() {
                    if ($("#parBut2").hasClass("active")) {
                        setHum();
                    }
                    jsonVent();
                });
            }
        });

        function getSvit() {
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
            if (min < 0 || min > 1440) {
                min = 1440 + min;
            }
            if (max < 0 || max > 1440) {
                max = 1440 + max;
            }
            
            

            var light = {
                toggle: toggle + 1,
                interval: [min, max],
            };
            return light;
        }

        function jsonSvit() {
            var json = {};
            json.form = 3;
            json.type = 6;
            json.light = getSvit();

            console.log('json: ' + JSON.stringify(json));
            connection.send(JSON.stringify(json));
        }
        $("#svitPerButtons").click(function() {
            jsonSvit();
        });

        function getPar() {
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

        function jsonPar() {
            var json = {};
            json.form = 4;
            json.type = 6;
            json.hum = getPar();
            json.vent = getVent();
            json.ventNight = getVentNich();

            console.log('json: ' + JSON.stringify(json));
            connection.send(JSON.stringify(json));
        }
        $("#parPerButtons").children(".per").each(function(index, elem) {
            if (index != 1) {
                $(elem).click(function() {
                    jsonPar();
                });
            } else {
                $(elem).click(function() {
                    if ($("#ventNichBut2").hasClass("active") || $("#ventBut2").hasClass("active")) {
                        setVent();
                    }
                    jsonPar();
                });
            }
        });

        function getVoda() {
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

        function jsonVoda() {
            var json = {};
            json.form = 5;
            json.type = 6;
            json.water = getVoda();

            console.log('json: ' + JSON.stringify(json));
            connection.send(JSON.stringify(json));
        }

        $("#vodaPerButtons").click(function() {
            jsonVoda();
        });

    });