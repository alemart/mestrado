window.TuioMaps = (function() {
    var client = null,
    screenW = null,
    screenH = null,
    time = null,
    map = null,
    oldFinger = null,
    oldScaleData = null,
    overlay = null,

    init = function(host) {
        screenW = $(window).innerWidth();
        screenH = $(window).innerHeight();
        time = new Date().getTime();
        map = new google.maps.Map(document.getElementById("map_canvas"), {
            center: new google.maps.LatLng(-23.557955484359667, -46.72789598381518),
            zoom: 15,
            scaleControl: false,
            mapTypeId: google.maps.MapTypeId.ROADMAP
        });
        overlay = new google.maps.OverlayView();
        overlay.draw = function() { };
        overlay.setMap(map);

        initClient(host);
    },

    initClient = function(host) {
        client = new Tuio.Client({
            host: host
        });
        client.on("connect", onConnect);
        client.connect();
    },

    onConnect = function() {
        mainloop();
    },

    mainloop = function() {
        // utilities
        var isFinger = function(tuioObject) { return tuioObject.getSymbolId() == 1; };
        var isEraser = function(tuioObject) { return tuioObject.getSymbolId() == 6; };
        var prevKey = null, currKey = null, eraserKey = null, cnt = 0;
        var objects = client.getTuioObjects();
        for(var key in objects) {
            var object = objects[key];

            prevKey = currKey;
            currKey = key;
            cnt++;
            if(isEraser(object))
                eraserKey = key;

            console.log("[TUIO] object #" + object.sessionId + " at (" + object.getScreenX(screenW) + ", " + object.getScreenY(screenH) + ") of type " + object.symbolId);
        }

        // event treatment
        if(cnt == 0) {
            // not touching anything
            oldFinger = null;
            oldScaleData = null;
        }
        else if(eraserKey) {
            // got an eraser
            handleEraser(objects[eraserKey]);
        }
        else if(cnt == 1 && isFinger(objects[currKey])) {
            // got a finger
            handleTranslation(objects[currKey]);
        }
        else if(cnt == 2 && isFinger(objects[currKey]) && isFinger(objects[prevKey])) {
            // got two fingers
            handleScale(objects[currKey], objects[prevKey]);
        }
        else {
            // got markers only
            handleMarkers(objects);
        }

        // loop
        requestAnimationFrame(mainloop);
    },

    pixel2latlng = function(x, y) {
        return overlay.getProjection().fromContainerPixelToLatLng(new google.maps.Point(x, y));
    },

    pixels2meters = function(value) { // pixels to meters conversion; this function takes the zoom of the map into account
        var diagonalPx = Math.sqrt(screenW * screenW + screenH * screenH);
        var ratioPx = value / diagonalPx;
        /*var pointA = new google.maps.Point(0, 0);
        var pointB = new google.maps.Point(screenW * ratioPx, screenH * ratioPx);
        var latlngA = overlay.getProjection().fromContainerPixelToLatLng(pointA);
        var latlngB = overlay.getProjection().fromContainerPixelToLatLng(pointB);*/
        var latlngA = pixel2latlng(0, 0);
        var latlngB = pixel2latlng(screenW * ratioPx, screenH * ratioPx);
        return google.maps.geometry.spherical.computeDistanceBetween(latlngA, latlngB);
    },

    handleTranslation = function(object) {
        var x = object.getScreenX(screenW);
        var y = object.getScreenY(screenH);
        var d = oldFinger ? {x: x - oldFinger.x, y: y - oldFinger.y} : {x: 0, y: 0};
        map.panBy(-d.x, -d.y);
        oldFinger = {x: x, y: y};
    },

    handleScale = function(objectA, objectB) {
        var dist = function(xa, ya, xb, yb) { return Math.sqrt((xa - xb) * (xa - xb) + (ya - yb) * (ya - yb)); };
        var xa = objectA.getScreenX(screenW);
        var ya = objectA.getScreenY(screenH);
        var xb = objectB.getScreenX(screenW);
        var yb = objectB.getScreenY(screenH);
        var initialDistance = oldScaleData ? oldScaleData.initialDistance : null;

        if(oldScaleData) {
            var d = dist(xa, ya, xb, yb);
            var ratio = d / initialDistance;
            var z = Math.round( Math.log(ratio) / Math.LN2 ); // lg(ratio)
            if(z != 0)
                initialDistance = d;
            map.setZoom(map.getZoom() + z); // zoom in [0, 19], see http://stackoverflow.com/questions/9356724/google-map-api-zoom-range
        }
        else
            initialDistance = dist(xa, ya, xb, yb);

        oldScaleData = { xa: xa, ya: ya, xb: xb, yb: yb, initialDistance: initialDistance };
    },

    handleEraser = function(object) {
        // circle.setMap(null)
        // circle = null
    },

    handleMarkers = function(objects) {
        for(key in objects) {
            var object = objects[key];
            if(object.symbolId >= 2 && object.symbolId <= 5) {
                var x = object.getScreenX(screenW);
                var y = object.getScreenY(screenH);
                var color = (function(type) {
                    switch(type) {
                        case 1: return "#fff"; // finger
                        case 2: return "#ff0";
                        case 3: return "#0af";
                        case 4: return "#0f0";
                        case 5: return "#f0f";
                        case 6: return "#777"; // eraser
                        default: return "#744"; // none
                    }
                })(object.symbolId);

                var circle = new google.maps.Circle({
                    center: pixel2latlng(x, y),
                    radius: pixels2meters(20),
                    fillColor: color,
                    strokeWeight: 0,
                    clickable: false,
                    map: map
                });
            }
        }
    };



    // public functions
    return {
        init: function(host) { return init(host); },
        map: function() { return map; }
    };
}());
