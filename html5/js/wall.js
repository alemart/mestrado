//
// Magic Wall TUIO Interface
// C++ <-> HTML5 
//

var lifetime = {};

function WallObject(sessionId, symbolId, x, y, z)
{
    this._session = sessionId;
    this._symbol = symbolId;
    this._x = x;
    this._y = y;
    this._z = z ? z : 0.0;
    this._lifetime = (sessionId in lifetime ? ++lifetime[sessionId] : lifetime[sessionId] = 1);
}

WallObject.prototype = {

    // object ID
    id: function() {
        return this._session;
    },

    // type can be one of the following:
    // 'none', 'eraser', 'finger',
    // 'marker_blue', 'marker_yellow', 'marker_green', 'marker_magenta',
    // 'wand_blue', 'wand_yellow', 'wand_green', 'wand_magenta'
    type: function() {
        switch(this._symbol) {
            case 1:  return 'finger';
            case 2:  return 'marker_yellow'; //'none'; //'marker_yellow';
            case 3:  return 'marker_blue'; //'finger'; //'marker_blue';
            case 4:  return 'marker_green';
            case 5:  return 'marker_magenta'; //'finger'; //'marker_magenta'; // FIXME
            case 6:  return 'eraser'; //'none'; //'eraser'; // FIXME
            case 7:  return 'wand_yellow';
            case 8:  return 'wand_yellow:active';
            case 9:  return 'wand_blue';
            default: return 'none';
        }
    },

    // get the position (in [0,1]^3 space)
    x: function() {
        return Math.round(this._x * 100.0) / 100.0;
    },
    y: function() {
        return Math.round(this._y * 100.0) / 100.0;
    },
    z: function() {
        return Math.round(this._z * 100.0) / 100.0;
    },

    // helper function to get the color
    // associated with this object
    color: function(alpha) {
        var a = alpha === undefined ? 1.0 : parseFloat(alpha);
        switch(this._symbol) {
            case 1:  return 'rgba(255, 255, 255, ' + a + ')'; // finger
            case 2:  return 'rgba(255, 255, 0, ' + a + ')';
            case 3:  return 'rgba(0, 170, 255, ' + a + ')';
            case 4:  return 'rgba(0, 255, 0, ' + a + ')';
            case 5:  return 'rgba(255, 0, 255, ' + a + ')';
            case 6:  return 'rgba(119, 119, 119, ' + a + ')'; // eraser
            case 7:  return 'rgba(255, 255, 0, ' + a + ')';
            case 8:  return 'rgba(170, 170, 0, ' + a + ')';
            case 9:  return 'rgba(0, 170, 255, ' + a + ')';
            default: return 'rgba(170, 68, 68, ' + a + ')'; // none
        }
    },

    // how old am i?
    lifetime: function() {
        return this._lifetime;
    }

};

$(function() { window.wall = (function(host, fps) {

    // private stuff
    var client = null;
    var sweetSpot = new SweetSpot();
    var wallObjects = [ ];

    // mainloop
    var mainloop = function() {
        // classify wall objects
        var markers = [], wands = [], fingers = [], erasers = [];
        var tuioObjects = client.getTuioObjects();
        for(var k in tuioObjects) {
            var tuioObject = tuioObjects[k];
            var wallObject = new WallObject(
                tuioObject.getSessionId(),
                tuioObject.getSymbolId(),
                tuioObject.getX(),
                tuioObject.getY(),
                tuioObject.getAngle()
            );

            if(wallObject.type().match(/^marker_/))
                markers.push(wallObject);
            else if(wallObject.type().match(/^wand_/))
                wands.push(wallObject);
            else if(wallObject.type() == 'eraser')
                erasers.push(wallObject);
            else if(wallObject.type() == 'finger')
                fingers.push(wallObject);

            console.log('WallObject #' + wallObject.id() + ' of type "' + wallObject.type() + '" at (' + wallObject.x() + ', ' + wallObject.y() + ', ' + wallObject.z() + ')');
        }

        function allGreen(m) {
            for(var k in m) {
                if(!m[k].type().match(/_green$/))
                    return false;
            }
            return true;
        }

        function allOld(m, x) {
            for(var k in m) {
                if(m[k].lifetime() < x)
                    return false;
            }
            return m.length > 0;
        }

        // filter stuff
        while(wallObjects.length > 0)
            wallObjects.pop();
        if(fingers.length == 0 && (markers.length == 0 || (markers.length > 0 && allGreen(markers))) && allOld(erasers, 3)) {
            wallObjects = wallObjects.concat(erasers);
            markers.length = 0;
        }
        if(wallObjects.length == 0) {
            wallObjects = wallObjects.concat(markers);
            wallObjects = wallObjects.concat(wands);
            wallObjects = wallObjects.concat(fingers);
        }
    };

    // constructor
    (function(host) {
        client = new Tuio.Client({ host: host });
        client.on('connect', function() {
            console.log('Connected to the TUIOjs server!');
            setInterval(mainloop, 1000.0 / fps);
        });
        console.log('Connecting to TUIOjs server at ' + host + '...');
        client.connect();
    })(host);

    // public stuff
    return {
        objects: function() { return wallObjects; }, // returns an array of all WallObjects
        sweetSpot: function() { return sweetSpot; }, // gets the sweetSpot
        i2w: sweetSpot.i2w // a neat shortcut
    };

})('https://localhost:5000', 30); });
