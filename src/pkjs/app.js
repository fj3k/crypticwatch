var defaultLoc = 'NSW_PT131';
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
  if (pos && !(pos.coords.latitude === 0 && pos.coords.longitude === 0)) {
    url += '?lat=' + pos.coords.latitude + '&lng=' + pos.coords.longitude;
  } else if (defaultLoc.length > 0) {
    url += '?acc=' + defaultLoc;
  }

  xhrRequest(url, 'GET',
    function(responseText) {
      var json = JSON.parse(responseText);

      var keys = require('message_keys');
      for (var i = 0; i < json.weather.forecast.length; i++) {
        var tempMax = json.weather.forecast[i].max;
//        var tempMin = parseInt(json.weather.forecast[i].min, 10);
        var icon = parseInt(json.weather.forecast[i].icon, 10);
  
        var dictionary = {};
        var icokey = keys.Forecast + i;
        var tempkey = keys.Temperature + i;
        dictionary[icokey] = icon;
        dictionary[tempkey] = (tempMax.length > 0) ? parseInt(tempMax, 10) : -40;
  
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
