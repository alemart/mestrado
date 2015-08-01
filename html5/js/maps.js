//
// Maps app
//

// initializing...
$(function() {

window.map = new google.maps.Map(document.getElementById("map_canvas"), {
    center: new google.maps.LatLng(-23.557955484359667, -46.72789598381518),
    zoom: 15,
    scaleControl: false,
    disableDefaultUI: true,
    mapTypeId: google.maps.MapTypeId.ROADMAP
});

window.overlay = new google.maps.OverlayView();
window.overlay.draw = function() { };
window.overlay.setMap(map);

setInterval(mainloop, 1000.0 / 30.0);

});




//
// main loop
//
function mainloop()
{
    // filter markers, erasers and fingers
    var markers = [], erasers = [], fingers = [];
    var wlist = wall.objects();
    for(var k in wlist) {
        var wo = wlist[k];

        if(wo.type().match(/^marker_/))
            markers.push(wo);
        else if(wo.type() == 'eraser')
            erasers.push(wo);
        else if(wo.type() == 'finger')
            fingers.push(wo);
    }

    // process everything
    $.each(markers, function(idx, obj) { handleMarker(obj); });
    $.each(erasers, function(idx, obj) { handleEraser(obj); });
    if(fingers.length == 1)
        handleFingers(fingers[0], null);
    else if(fingers.length >= 2)
        handleFingers(fingers[0], fingers[1]);
    else
        handleFingers(null, null);
}









//
// Handlers
//

var circles = [];

// handle marker
function handleMarker(marker)
{
    var pos = wall.i2w(marker.x(), marker.y());
    circles.push(new google.maps.Circle({
        center: pixel2latlng(pos.x, pos.y),
        radius: pixels2meters(15),
        fillColor: marker.color(1.0),
        strokeWeight: 0,
        clickable: false,
        map: window.map
    }));

    cursors.addAt(pos.x, pos.y);
}

// handle eraser
function handleEraser(eraser)
{
    var radius = 100; // FIXME eraser.z (eraser.radius)
    var pos = wall.i2w(eraser.x(), eraser.y());

    // copy the circles[] array
    var c = [];
    for(k in circles)
        c.push(circles[k]);

    // compute cool stuff
    var radiusInMeters = pixels2meters(radius);
    var eraserCenter = pixel2latlng(pos.x, pos.y);

    // create a new circles[] array with relevant objects only
    circles = [];
    for(k in c) {
        var circle = c[k];
        var dist = google.maps.geometry.spherical.computeDistanceBetween(circle.getCenter(), eraserCenter);
        if(dist <= radiusInMeters)
            circle.setMap(null);
        else
            circles.push(circle);
    }

    // a cursor
    cursors.addAt(pos.x, pos.y, '#fff', radius);
}

// handle translation & scale
var lastFinger1 = null, lastFinger2 = null, origDistance = 1;
function handleFingers(finger1, finger2)
{
    var pos1 = finger1 ? wall.i2w(finger1.x(), finger1.y()) : null;
    var pos2 = finger2 ? wall.i2w(finger2.x(), finger2.y()) : null;

    if(finger1 && !finger2) {
        // TRANSLATION
        if(lastFinger1 && lastFinger1.id() == finger1.id()) {
            var lastPos1 = wall.i2w(lastFinger1.x(), lastFinger1.y());
            var delta = {x: pos1.x - lastPos1.x, y: pos1.y - lastPos1.y};
            window.map.panBy(-delta.x, -delta.y);
        }

        cursors.addAt(pos1.x, pos1.y);
    }
    else if(finger1 && finger2) {
        // SCALE
        var distance = Math.sqrt((pos1.x - pos2.x) * (pos1.x - pos2.x) + (pos1.y - pos2.y) * (pos1.y - pos2.y));
        if(!lastFinger1 || !lastFinger2)
            origDistance = distance;
        if(lastFinger1 && lastFinger2 && lastFinger1.id() == finger1.id() && lastFinger2.id() == finger2.id()) {
            var ratio = distance / origDistance;
            var z = Math.round( Math.log(ratio) / Math.LN2 ); // lg(ratio)
            if(z != 0)
                origDistance = distance;
            map.setZoom(map.getZoom() + z); // zoom in [0, 19], see http://stackoverflow.com/questions/9356724/google-map-api-zoom-range
        }

        cursors.addAt(pos1.x, pos1.y);
        cursors.addAt(pos2.x, pos2.y);
    }

    lastFinger1 = finger1;
    lastFinger2 = finger2;
}



//
// Utils
//

// convertion function
function pixel2latlng(x, y)
{
    return window.overlay.getProjection().fromContainerPixelToLatLng(new google.maps.Point(x, y));
}

// pixels to meters conversion; this function takes the zoom of the map into account
function pixels2meters(value)
{
    var screenW = $(window).innerWidth();
    var screenH = $(window).innerHeight();
    var diagonalPx = Math.sqrt(screenW * screenW + screenH * screenH);
    var ratioPx = value / diagonalPx;
    var latlngA = pixel2latlng(0, 0);
    var latlngB = pixel2latlng(screenW * ratioPx, screenH * ratioPx);
    return google.maps.geometry.spherical.computeDistanceBetween(latlngA, latlngB);
}




