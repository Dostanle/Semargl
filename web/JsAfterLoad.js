$(document).ready(function() {
  
  function requestIndicators() {
    var json = {};
    json.type = 7;

    console.log('json: ' + JSON.stringify(json));
    connection.send(JSON.stringify(json));
}
  
function checkTime(i) {
  if (i < 10) {
      i = "0" + i;
  }
  return i;
}

function startTime() {
  var dateClient = Date.now();
  var today = new Date(dateClient - new Date(dateDifference));
  var y = today.getFullYear();
  var mo = today.getMonth();
  var d = today.getDay();
  var h = today.getHours();
  var m = today.getMinutes();
  var s = today.getSeconds();
  mo = checkTime(mo);
  d = checkTime(d);
  m = checkTime(m);
  s = checkTime(s);
  document.getElementById('clock').innerHTML = h + ":" + m + ":" + s;
  t = setTimeout(function() {
      startTime();
  }, 1000);
}
function getIndicators(){
  requestIndicators();
  t = setTimeout(function() {
      getIndicators();
  }, 10000);
}
startTime();
getIndicators();


});