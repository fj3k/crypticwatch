var useLoc = true;
var cachedLoc = null;

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function weatherService(pos) {
  var url = 'http://fj3k.com/pebble/';
  if (pos) url += '?lat=' + pos.coords.latitude + '&lng=' + pos.coords.longitude;

  xhrRequest(url, 'GET',
    function(responseText) {
      var json = JSON.parse(responseText);

      var keys = require('message_keys');
      for (var i = 0; i < json.weather.forecast.length; i++) {
        var tempMax = Math.round(json.weather.forecast[i].max);
//        var tempMin = Math.round(json.weather.forecast[i].min);
        var icon = parseInt(json.weather.forecast[i].icon, 10);
  
        var dictionary = {};
        var icokey = keys.Forecast + i;
        var tempkey = keys.Temperature + i;
        dictionary[icokey] = icon;
        dictionary[tempkey] = tempMax;
  
        // Send to Pebble
        Pebble.sendAppMessage(dictionary, sendAppSuccess, sendAppError);
      }
    }
  );
}

function sendAppSuccess(e) {
  console.log('Success sending weather info to Pebble!');
}

function sendAppError(e) {
  console.log('Error sending weather info to Pebble!');
}

function locationSuccess(pos) {
  cachedLoc = pos;
  weatherService(pos);
}

function locationError(err) {
  weatherService(cachedLoc);
}

function getWeather() {
  if (useLoc) {
    navigator.geolocation.getCurrentPosition(
      locationSuccess,
      locationError,
      {timeout: 15000, maximumAge: 60000}
    );
  } else {
    weatherService();
  }
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
  function(e) {
    console.log('PebbleKit JS ready!');

    // Get the initial weather
    getWeather();
  }
);
// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
    getWeather();
  }
);
