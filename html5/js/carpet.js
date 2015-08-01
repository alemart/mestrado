// height = camera.positionCartographic.height; long = camera.positionCartographic.longitude; lat = camera.positionCartographic.latitude; heading = camera.heading; roll = camera.roll; pitch = camera.pitch;
//camera.flyTo({destination: viewer.scene.globe.ellipsoid.cartographicToCartesian(new Cesium.Cartographic(long, lat, height)), duration: 5, orientation: {heading: heading, roll: roll, pitch: pitch}})


String.prototype.capitalize = function(){ return this.replace( /(^|\s)([a-z])/g , function(m,a,b){ return a+b.toUpperCase(); } ); };

function sign(n) { return n >= 0.0 ? 1.0 : -1.0; }

function toFixed(num, fixed) {
    fixed = fixed || 2;
    fixed = Math.pow(10, fixed);
    return Math.floor(num * fixed) / fixed;
}

var viewer = new Cesium.Viewer('cesiumContainer', {
    animation: false,
    baseLayerPicker: false,
    fullscreenButton: false,
    geocoder: true,
    homeButton: false,
    infoBox: false,
    sceneModePicker: false,
    selectionIndicator: false,
    timeline: false,
    navigationHelpButton: false,
    navigationInstructionsInitiallyVisible: false,
    creditContainer: $('<div></div>').appendTo('body').hide().get(0),
    imageryProvider: new Cesium.BingMapsImageryProvider({
        url: '//dev.virtualearth.net',
        mapStyle: Cesium.BingMapsStyle.AERIAL
    })
});

var showLabelsMap = false;
var labelsMap = viewer.scene.imageryLayers.addImageryProvider(new Cesium.BingMapsImageryProvider({
    url: '//dev.virtualearth.net',
    mapStyle: Cesium.BingMapsStyle.AERIAL_WITH_LABELS
}));

setInterval(function() {
    var heightThreshold = 5500;
    if(viewer.camera.positionCartographic.height < heightThreshold)
        labelsMap.alpha = 1.0;
    else
        labelsMap.alpha = showLabelsMap ? 1.0 : 0.0;
    $(viewer.geocoder.container).hide();
}, 1000);

viewer.camera.flyTo({
    destination: Cesium.Cartesian3.fromDegrees(-46.655232, -23.562660, 1000), // av paulista: long, lat, height
    //destination: new Cesium.Cartesian3.fromDegrees(-0.8163576419426691, -0.5076627811799496, 370894.094844516),
    //destination: new Cesium.Cartesian3(4020730.722030813, -4216144.109132254, -2780846.105934037),
    //destination: new Cesium.Cartesian3(4365659.26034321, -4603659.864141245, -2743336.9414761104),
//    destination: new Cesium.Cartesian3(6921046.830099078, -10513930.565034667, -5719348.610832904),
//destination: new Cesium.Cartesian3.fromDegrees(-0.8156750524896583, -0.41135729550890415, 572.8214813320328),
//destination: Cesium.Cartesian3.fromDegrees(    -0.8156750524896583,    -0.41135729550890415,    10572.8214813320328),
destination: new Cesium.Cartesian3(4009347.904457057, -4259390.8342380915, -2534149.157888208),
    orientation: {
        heading: 0,
        pitch: 0.25*-Cesium.Math.PI/2,
        roll: 0
    },
    duration: 0
});

//
// status
//
var setStatus = (function() {
    var obj = $('<span></span>').appendTo('body').css({
        position: 'absolute',
        right: 10, bottom: 0,
        font: '50pt sans-serif',
        color: '#ff7',
        'font-weight': 'bold',
        'text-shadow': '#000 3px 3px 5px',
        'text-align': 'right',
        'z-index': 9999999,
        padding: 10
    }), tmr = null;

    function setStatus(txt)
    {
        obj.html(txt || '').show();
        if(tmr) clearTimeout(tmr);
        tmr = setTimeout(function() { obj.fadeOut(); }, 5000);
    }

    return setStatus;
})();


//
// mousemove simulator
//
var mouseSimulator = (function() {

    return {
        x: 0,
        y: 0,
        ready: true,
        wx: 0, // walking dx
        wy: 0, // walking dy
        wb: 0, // walking button

        setup: function(duration) {
            var that = this;
            var el = $(document.elementFromPoint(10, 10)); // a canvas
            el.on('mousemove', function(e) {
                that.x = e.pageX;
                that.y = e.pageY;
                //console.log('recebi mousemove: ' + e.pageX + ', ' + e.pageY);
            }).on('mouseup', function(e) {
                that.x = e.pageX;
                that.y = e.pageY;
                //console.log('recebi mouseup: ' + e.pageX + ', ' + e.pageY);
            }).on('mousedown', function(e) {
                that.x = e.pageX;
                that.y = e.pageY;
                //console.log('recebi mousedown: ' + e.pageX + ', ' + e.pageY);
            });

            // walking
            var dur = duration !== undefined ? duration : 0.5/2;
            (function f() {
                var that = this;
                function g() { setTimeout(function() { f.call(that); }, 0.1/2); }
                this.move(this.wx, this.wy, this.wb, dur - 0.065/2, g);
            }).call(this);
        },

        trigger: function(eventName, button) {
            var el = document.getElementsByTagName('canvas')[0];//document.elementFromPoint(this.x, this.y);
            var ev = new MouseEvent(eventName, {
                bubbles: true,
                cancelable: true,
                currentTarget: el,
                clientX: this.x,
                clientY: this.y,
                button: button || 0,
                buttons: (button == 0) ? 1 : ((button == 1) ? 4 : (button == 2) ? 2 : 0)
            });
            el.dispatchEvent(ev);
        },

        click: function() {
            this.trigger('click');
        },

        start: function(button) { // 0 = left, 1 = middle, 2 = right
            if(this.ready) {
                var canvas = document.getElementsByTagName('canvas')[0];
                this.x = canvas.width/2;
                this.y = canvas.height/2;
                this.ready = false;
                this.trigger('mousedown', button);
            }
        },

        finish: function(button) {
            if(!this.ready) {
                var canvas = document.getElementsByTagName('canvas')[0];
                this.x = canvas.width/2;
                this.y = canvas.height/2;
                this.trigger('mouseup', button);
                this.ready = true;
            }
        },

        move: function(dx, dy, button, duration, callback) { // (dx, dy) in pixels, duration in seconds
            var that = this, last = Date.now(), t = 0, tmr = null;
            button = (button !== undefined) ? button : 0;
            duration = (duration !== undefined) ? duration * 1000.0 : 1000;

            if(dx * dx + dy * dy < 1.0) {
                setTimeout(function() {
                    if(callback)
                        callback.call(that);
                }, duration);
                return;
            }

            this.start(button);
            var sx = this.x, sy = this.y;

            (function f() {
                var now = Date.now();
                t += now - last;
                if(t < duration) {
                    last = now;
                    that.x = Math.ceil(dx * (t / duration)) + sx;
                    that.y = Math.ceil(dy * (t / duration)) + sy;
                    that.trigger('mousemove', button);
                    tmr = setTimeout(f, 2);
                }
                else {
                    that.finish(button);
                    if(callback)
                        callback.call(that);
                }
            })();
        },

        keepWalking: function(dx, dy, button) {
            this.wx = Math.floor(dx);
            this.wy = Math.floor(dy);
            this.wb = button !== undefined ? button : this.wb;
        },

        stopWalking: function() {
            this.wx = this.wy = 0.01;
        },

        isWalking: function() {
            return (this.wx * this.wx + this.wy * this.wy > 1.0);
        }

        
    };
})();
mouseSimulator.setup();

//
// camera speed
//
var setSpeed = (function() {
    var moveSpd = 0.0;

    var moveIntv = setInterval(function() { 
        var ellipsoid = viewer.scene.globe.ellipsoid, h;
        var MAGIC = 0.01 * (h = ellipsoid.cartesianToCartographic(viewer.camera.position).height);
        viewer.camera.moveForward(moveSpd * MAGIC);
        var pos = ellipsoid.cartesianToCartographic(viewer.camera.position);
        pos.height = h;
        viewer.camera.setView({positionCartographic: pos});
    }, 33);

    function setSpeed(spd) {
        if(spd !== undefined)
            moveSpd = spd;
        return moveSpd;
    }
/*
    function setSpeed(spd)
    {
        if(spd !== undefined) {
            moveSpd = spd;
            var MAGIC = 24.0;
            mouseSimulator.keepWalking(0, MAGIC * spd);
        }
        return moveSpd;
    }
*/
    return setSpeed;
})();

function getSpeed()
{
    return setSpeed();
}

//
// camera direction
//
var DEG90 = 240.0 / 90.0; // magic

var yawPitch = (function() {

    var oldYaw = 0.0, oldPitch = 0.0;

    function set(yaw, pitch, callback) {
        if(yaw !== undefined && pitch !== undefined) {
            mouseSimulator.keepWalking(0, 0);
            setTimeout(function() {
                mouseSimulator.move(DEG90 * (oldYaw - yaw), DEG90 * (oldPitch - pitch), 1, 1.0, callback);
                oldYaw = yaw;
                oldPitch = pitch;
            }, 500);
        }
    }

    function get() {
        return { yaw: oldYaw, pitch: oldPitch };
    }

    function _set(yaw, pitch) {
        oldYaw = yaw;
        oldPitch = pitch;
    }

    return { get: get, set: set, _set: _set };

})();

function _setYawPitch(yaw, pitch) { return yawPitch._set(yaw, pitch); }
function setYawPitch(yaw, pitch, callback) { return yawPitch.set(yaw, pitch, callback); }
function getYaw() { return yawPitch.get().yaw; }
function getPitch() { return yawPitch.get().pitch; }
function setYaw(yaw, callback) { return setYawPitch(yaw, getPitch(), callback); }
function setPitch(pitch, callback) { return setYawPitch(getYaw(), pitch, callback); }

var setRoll = (function() {
    var old = 0.0

    function setRoll(angle)
    {
        if(angle !== undefined) {
            viewer.camera.twistRight(angle - old);
            old = angle;
        }
        return old;
    }

    return setRoll;
})();

function getRoll()
{
    return setRoll();
}

//
// camera ground projector
//
var groundProjector = (function() {
    var last = null;

    return {
        project: function(screenPoint) {
            var camera = viewer.camera;
            var ellipsoid = viewer.scene.globe.ellipsoid;
            var ray = camera.getPickRay(screenPoint);
            var intersection = Cesium.IntersectionTests.rayEllipsoid(ray, ellipsoid);
            var point = intersection ? Cesium.Ray.getPoint(ray, intersection.start) : last;
            return (last = point);
        }
    };

})();

//
// checkpoints
//
var chkents = {};

function createCheckpoint(name)
{
    var pos = _computeCheckpointPosition(wand, viewer.camera.position, viewer.camera.heading, viewer.camera.pitch, viewer.camera.roll);

    localStorage.setItem('cp_*', JSON.stringify(
        $.unique($.merge(JSON.parse(localStorage.getItem('cp_*') || '[]'), [name]))
    ));

    localStorage.setItem('cp_' + name, JSON.stringify({
        position: { x: viewer.camera.position.x, y: viewer.camera.position.y, z: viewer.camera.position.z },
        orientation: { heading: viewer.camera.heading, pitch: viewer.camera.pitch, roll: viewer.camera.roll },
        checkpointPosition: { x: pos.x, y: pos.y, z: pos.z },
        myOrientation: { yaw: getYaw(), pitch: getPitch(), roll: getRoll() }
    }));

    _createCheckpointEntity(name, pos);
    setStatus('Checkpoint criado.');
    return true;
}

function gotoCheckpoint(name)
{
    var time = 3.0;
    var obj = JSON.parse(localStorage.getItem('cp_' + name));
    var wasFlying = isFlying();
    stopFlying();
    stateMachine.changeState(stateMachine.state.DISCRETE);
    setStatus('Destino: ' + name.capitalize());

    if(obj) {
        if(isFlying()) {
            viewer.camera.flyTo({
                destination: new Cesium.Cartesian3(obj.position.x, obj.position.y, obj.position.z),
                orientation: {
                    heading: obj.orientation.heading,
                    pitch: obj.orientation.pitch,
                    roll: obj.orientation.roll
                },
                //endTransform: Cesium.Matrix4.IDENTITY, // no, thx
                duration: time,
            });
            _setYawPitch(obj.myOrientation.yaw, obj.myOrientation.pitch);
            setRoll(obj.myOrientation.roll);
        }
        else {
            var ellipsoid = viewer.scene.globe.ellipsoid;
            var chkpos = ellipsoid.cartesianToCartographic(obj.checkpointPosition);
            var campos = ellipsoid.cartesianToCartographic(obj.position);
            var pos = chkpos.clone();
            pos.height = campos.height;
            viewer.camera.flyTo({
                destination: ellipsoid.cartographicToCartesian(pos),
                endTransform: Cesium.Matrix4.IDENTITY,
                duration: time,
            });
        }
        return true;
    }
    else {
        viewer.geocoder.viewModel.searchText = name + ' são paulo brasil';
        viewer.geocoder.viewModel.search();
        return false;
    }
}

function removeCheckpoint(name)
{
    if(chkents[name]) {
        viewer.entities.remove(chkents[name]);
        delete chkents[name];
        var l = JSON.parse(localStorage.getItem('cp_*'));
        var idx = l.indexOf(name);
        if(idx >= 0)
            l.splice(idx, 1);
        localStorage.setItem('cp_*', JSON.stringify(l));
        setStatus('Checkpoint removido.');
        return true;
    }
    else
        return false;
}

function listCheckpoints()
{
    return JSON.parse(localStorage.getItem('cp_*') || '[]');
}

function clearCheckpoints()
{
    var l = listCheckpoints();
    for(var k in l)
        removeCheckpoint(l[k]);
    localStorage.setItem('cp_*', '[]');
    return true;
}

// ===============================================

function loadCheckpoints()
{
    var chks = JSON.parse(localStorage.getItem('cp_*') || '[]');
    for(var k in chks) {
        var name = chks[k];
        var chk = JSON.parse(localStorage.getItem('cp_' + name));
        _createCheckpointEntity(name, chk.checkpointPosition);
    }
}

function checkpoints()
{
    var l = {};
    var chks = JSON.parse(localStorage.getItem('cp_*') || '[]');
    for(var k in chks) {
        var name = chks[k];
        var chk = JSON.parse(localStorage.getItem('cp_' + name));
        l[name] = chk;
    }
    return l;
}

// atScreen ({x: ?, y: ?}) is in [0,1]^2
function _computeCheckpointPosition(atScreen, cameraPosition, heading, pitch, roll)
{
    var restoreCamera = (function(camera) {
        var position = camera.position.clone();
        var heading = camera.heading;
        var pitch = camera.pitch;
        var roll  = camera.roll;
        function restoreCamera() {
            camera.flyTo({
                destination: position,
                orientation: { heading: heading, pitch: pitch, roll: roll },
                duration: 0
            });
        }
        return restoreCamera;
    })(viewer.camera);

    var newpos = (function(camera, ellipsoid) {
        camera.flyTo({
            destination: cameraPosition,
            orientation: {
                heading: heading,
                pitch: pitch,
                roll: roll
            },
            duration: 0
        });
        var ray = camera.getPickRay(new Cesium.Cartesian2(viewer.canvas.clientWidth * atScreen.x, viewer.canvas.clientHeight * atScreen.y));
        var intersection = Cesium.IntersectionTests.rayEllipsoid(ray, ellipsoid);
        var point = Cesium.Ray.getPoint(ray, intersection.start);
        return point;
    })(viewer.camera, viewer.scene.globe.ellipsoid);

    restoreCamera();
    return newpos;
}

function _createCheckpointEntity(name, position)
{
    if(chkents[name]) {
        viewer.entities.remove(chkents[name]);
        delete chkents[name];
    }

    chkents[name] = viewer.entities.add({
        name: name,
        position: position,
        point: {
            pixelSize: 5,
            color: Cesium.Color.RED,
            outlineColor: Cesium.Color.BLACK,
            outlineWidth: 1
        },
        label: {
            text: name.capitalize(),
            font: '24pt sans-serif',
            style: Cesium.LabelStyle.FILL_AND_OUTLINE,
            outlineWidth: 2,
            verticalOrigin: Cesium.VerticalOrigin.BOTTOM,
            pixelOffset: new Cesium.Cartesian2(0, -9)
        }
    });
}

setTimeout(loadCheckpoints, 1000);

//
// discrete navigation
//
function gotoPointedPlace(zoomFactor)
{
    zoomFactor = (typeof(zoomFactor) == 'number') ? zoomFactor : 1.0;
    var ellipsoid = viewer.scene.globe.ellipsoid;
    var currentPosition = viewer.camera.positionCartographic;
    var pointedPosition = ellipsoid.cartesianToCartographic(new Cesium.Cartesian3(wand.pointedX, wand.pointedY, wand.pointedZ));
    var heading = viewer.camera.heading;
    var pitch = viewer.camera.pitch;
    var roll = viewer.camera.roll;
    var nextPosition = pointedPosition;
    nextPosition.height = zoomFactor * currentPosition.height;
    viewer.camera.flyTo({
        destination: ellipsoid.cartographicToCartesian(nextPosition),
        /*orientation: {
            heading: heading,
            pitch: pitch,
            roll: roll
        },*/
        endTransform: Cesium.Matrix4.IDENTITY,
        duration: 2.0
    });
    setStatus('Centralizando...');
}

function zoomIn()
{
    gotoPointedPlace(0.25);
    setStatus('Aproximando...');
}

function zoomOut(duration)
{
    gotoPointedPlace(1.0 / 0.25);
    setStatus('Afastando...');
}

//
// continuous navigation
//
function startFlying()
{
    setStatus('Iniciando vôo!');
    stateMachine.startFlying();
}

function stopFlying()
{
    setStatus('Fim do vôo.');
    stateMachine.stopFlying();
}

function isFlying()
{
    return stateMachine.isFlying();
}

//
// speech recognition
//
var speech = new webkitSpeechRecognition();
var speechCallback = (function() {
    var f = function(g, state) { return function(param) { if(!state || stateMachine.currentState() == stateMachine.state[state]) g(param); }; };

    return {
        'goto': f(gotoCheckpoint),
        'create': f(createCheckpoint),
        'remove': f(removeCheckpoint),
        'jump': f(gotoPointedPlace, 'DISCRETE'),
        'zoomin': f(zoomIn, 'DISCRETE'),
        'zoomout': f(zoomOut, 'DISCRETE'),
        'fly': f(startFlying, 'CONTINUOUS'),
        'stop': f(stopFlying, 'CONTINUOUS')
    };
})();
speech.continuous = true;
speech.interimResults = false;
speech.lang = 'pt-BR';
speech.onresult = function(e) {
    var str = '';
    for(var i = e.resultIndex; i < e.results.length; i++) {
        if(e.results[i].isFinal)
            str += e.results[i][0].transcript;
    }
    console.log('[speech] listened: ' + str);

    var match, re = {
        'goto': [ /^\s*v[áa]i? p[a]?r[ao]\s*[ao]?\s+(.*)\s*$/, /^\s*v[áa]i? n[ao]\s+(.*)\s*$/ ], // vá para (...)
        'create': [ /^\s*aqui [ée]?m?\s*[ao]?\s+(.*)\s*$/ ], // aqui é (...)
        'remove': [ /^\s*remov[ae]\s*[ao]?\s+(.*)\s*$/ ], // remova (...)
        'jump': [ /^\s*aqui\s*$/, /^\s*q\s*$/, /^\s*vai\s*$/, /^\s*centraliz[ae]r?\s*$/ ], // aqui, vai, centraliza
        'zoomin': [ /^\s*zoo[mn]\s*$/, /^\s*[sz]u[mn]\s*$/, /^\s*aproxim[eaoui]\s*(se)?\s*$/, /^\s*g1\s*$/ ], // zoom, aproxime, aproxime-se
        'zoomout': [ /^\s*afast[eaou]\s*(se)?\s*$/, /^\s*afac[ei]\s*$/, /^\s*avast\s*$/, /^\s*apart[ae]\s*$/ ], // afaste, afaste-se
        'fly': [ /^\s*sobrev[oô][ea]r?\s*$/, /^\s*v[ôo][eoiau]r?\s*$/, /^\s*inici(ar|e) v[ôo][ou]\s*$/ ], // sobrevoe, voe
        'stop': [ /^\s*par[eai]r?\s*$/, /^\s*(termin|finaliz|par)(a|ar|e) v[ôo][ou]\s*$/ ] // pare
    };
    for(var k in re) {
        for(var i in re[k]) {
            var match = re[k][i].exec(str);
            if(match) {
                console.log('[speech] ' + k + ': ' + match[1]);
                if(speechCallback[k])
                    speechCallback[k].call(window, match[1], str);
                return;
            }
        }
    }
    console.log('[speech] no match.');
};
speech.onend = function() { speech.start(); console.log('[speech] restarted'); };
speech.start();

//
// magic wand TUIO interface
//
var wand = (function() {
    var $1 = new DollarRecognizer();
    var visible = false;
    var active = false;
    var gesture = [], gesture3 = [];
    var entity = $('<span style="background-color: #ff0; width: 50px; height: 50px; position: fixed; left: 0px; top: 0px; border-radius: 50px"></span>').appendTo('body').hide();
    var cesiumEntity = null;
    var trailEntities = [];

    var that = {
        // yellow part of the wand
        x: 0,
        y: 0,
        z: 0.5,
        prevX: 0,
        prevY: 0,
        prevZ: 0,

        // blue part of the wand
        bottomX: 0,
        bottomY: 0,
        bottomZ: 0.5,
        prevBottomX: 0,
        prevBottomY: 0,
        prevBottomZ: 0,

        // direction
        directionX: 0,
        directionY: 0,
        directionZ: 0,

        // where is it pointing (in world coordinates?)
        pointedX: 0,
        pointedY: 0,
        pointedZ: 0,

        // utilities
        isVisible: function() {
            return visible;
        },
        isActive: function() {
            return active;
        },

        // gesture recognition
        callback: {
            circle: null,
            v: null,
            hat: null,
            arrow: null
        }
    };

    var intv = setInterval(function() { // UPDATE function
        // grab tuio stuff
        function grabTuioStuff(color) {
            var objs = wall.objects();
            for(k in objs) {
                var wo = objs[k];
                if(wo.type().match((color == "yellow") ? (/^wand_yellow/) : (/^wand_blue/)))
                    return wo;
            }
            return null;
        };
        var tuioWand = grabTuioStuff("yellow");
        var tuioBottomWand = grabTuioStuff("blue");

        // update position
        var a = 0.5; // peso do antigo
        if(tuioWand) {
            that.prevX = that.x;
            that.prevY = that.y;
            that.prevZ = that.z;
            that.x = tuioWand.x();
            that.y = tuioWand.y();
            that.z = tuioWand.z();
            that.x = a * that.prevX + (1.0 - a) * that.x;
            that.y = a * that.prevY + (1.0 - a) * that.y;
            that.z = a * that.prevZ + (1.0 - a) * that.z;
            active = !!(tuioWand.type().match(/:active$/));
            visible = true;
        }
        else
            visible = false;

        // update direction
        if(tuioBottomWand) {
            that.prevBottomX = that.bottomX;
            that.prevBottomY = that.bottomY;
            that.prevBottomZ = that.bottomZ;
            that.bottomX = tuioBottomWand.x();
            that.bottomY = tuioBottomWand.y();
            that.bottomZ = tuioBottomWand.z();
            that.bottomX = a * that.prevBottomX + (1.0 - a) * that.bottomX;
            that.bottomY = a * that.prevBottomY + (1.0 - a) * that.bottomY;
            that.bottomZ = a * that.prevBottomZ + (1.0 - a) * that.bottomZ;
            that.directionX = that.x - that.bottomX;
            that.directionY = that.y - that.bottomY;
            that.directionZ = that.z - that.bottomZ;
            var norm = Math.sqrt(that.directionX * that.directionX + that.directionY * that.directionY + that.directionZ * that.directionZ);
            that.directionX /= norm;
            that.directionY /= norm;
            that.directionZ /= norm;
        }

        // project wand on plane z = zk
        if(Math.abs(that.bottomZ - that.z) > 0.00001) {
            var z = 0.1; // plane equation
            var lambda = (z - that.z) / (that.bottomZ - that.z);
            var px = that.x + lambda * (that.bottomX - that.x);
            var py = that.y + lambda * (that.bottomY - that.y);
            var pz = that.z + lambda * (that.bottomZ - that.z);
            var projected = groundProjector.project(new Cesium.Cartesian2(viewer.canvas.clientWidth * px, viewer.canvas.clientHeight * py));
            var projected = groundProjector.project(new Cesium.Cartesian2(viewer.canvas.clientWidth * that.x, viewer.canvas.clientHeight * that.y)); // FIXME
            if(projected) {
                that.pointedX = projected.x;
                that.pointedY = projected.y;
                that.pointedZ = projected.z;
            }
        }

        // collecting gestures
        if(visible && active) {
            gesture.push(new Point(that.x, that.y)); // 2D projection
            gesture3.push({x: that.x, y: that.y, z: that.z});
        }

        // recognize a gesture
        if(visible && !active && gesture.length >= 20) {
            console.log('[$1] recognizing...');
            var g = $1.Recognize(gesture);
            console.log('[$1] recognized "' + g.Name + '" (' + g.Score + ').');
            var cb = that.callback[g.Name];
            if(cb)
                cb(gesture3.slice(), g.Score);
            gesture.length = 0;
            gesture3.length = 0;
        }
        if(visible && !active) gesture.length = 0;

        // cursor entity
        entity.css({'left': viewer.canvas.clientWidth * that.x + 'px', 'top': viewer.canvas.clientHeight * that.y + 'px', 'background-color': active ? '#ff0' : '#faa'});
        //entity.show();
        if(cesiumEntity)
            viewer.entities.remove(cesiumEntity);
        cesiumEntity = viewer.entities.add({
            name: 'wand',
            position: projected,
            point: {
                pixelSize: 15,
                color: active ? Cesium.Color.YELLOW : Cesium.Color.DEEPPINK,
                outlineColor: active ? Cesium.Color.WHITE : Cesium.Color.BLACK,
                outlineWidth: 2
            },
            /*label: {
                text: toFixed(wand.x) + ', ' + toFixed(wand.y) + ', ' + toFixed(wand.z),
                font: '14pt sans-serif',
                style: Cesium.LabelStyle.FILL_AND_OUTLINE,
                outlineWidth: 2,
                verticalOrigin: Cesium.VerticalOrigin.BOTTOM,
                pixelOffset: new Cesium.Cartesian2(0, -9)
            }*/
        });

        // gesture trail
        if(visible && active) {
            var e = viewer.entities.add({
                position: projected,
                point: {
                    pixelSize: 10,
                    color: active ? Cesium.Color.YELLOW : Cesium.Color.DEEPPINK,
                    outlineColor: Cesium.Color.WHITE,
                    outlineWidth: 1
                },
            });
            trailEntities.push(e);
        }
        if(!active) {
            for(var k in trailEntities) {
                var e = trailEntities[k];
                viewer.entities.remove(e);
            }
            trailEntities.length = 0;
        }
    }, 1000 / 30);

    return that;
})();

//
// state machine
//
var stateMachine = (function() {
    var state = 0;
    var intv = null;
    var gesture = null;
    var isFlying = false;
    var a = 0, desiredHeading = viewer.camera.heading, initialHeading = viewer.camera.heading, desiredPitch = viewer.camera.pitch, initialPitch = viewer.camera.pitch;
    var oldang = 0, newang = 0;
    var destheight = viewer.camera.positionCartographic.height;
    var angoffset = 0, dangoffset = 0;
    var wasActive = false;

    function update()
    {
        var dt = 1.0 / 33.0;
        mouseSimulator.stopWalking();

        // sweet camera effect
if(0)
        if(a < 1.0) {
            viewer.camera.setView({
                heading: (1.0 - a) * initialHeading + a * desiredHeading,
                pitch: (1.0 - a) * initialPitch + a * desiredPitch,
                roll: viewer.camera.roll
            });
            a += 0.75 * dt;
            if(a >= 1.0) {
                viewer.camera.setView({
                    heading: desiredHeading,
                    pitch: desiredPitch,
                    roll: viewer.camera.roll
                });
            }
            return;
        }

        // spells
        if(!wasActive && wand.isActive()) {
            destheight = viewer.camera.positionCartographic.height;
        }

        // flying around
        if(this.currentState() == this.state.CONTINUOUS && isFlying && !wand.isActive() && wand.isVisible()) {
            var pi = Math.PI, twopi = 2 * Math.PI;
            var theta = Math.asin(wand.directionY);
            var wandInclination = theta * 180 / pi;
            var pitch = -0.3926990816989673;
            //setStatus(Math.floor(wandInclination) + "º");

            function validAngle(wandAngle, thetaAcc) // in degrees
            {
                wangAngle = ((wandAngle % 360) + 360) % 360;
                thetaAcc = (((thetaAcc || 30) % 360) + 360) % 360;
                /*return (
                    (wandAngle >= 0 && wandAngle < thetaAcc) ||
                    (wandAngle >= 180 - thetaAcc && wandAngle < 180) ||
                    (wandAngle >= 180 && wandAngle < 180 + thetaAcc) ||
                    (wandAngle >= 360 - thetaAcc && wandAngle < 360)
                );*/
                var tg = Math.tan(wandAngle * Math.PI / 180);
                var threshold = Math.tan(thetaAcc * Math.PI / 180);
                return (tg >= -threshold && tg < threshold);
            }

            // heading
            dangoffset += (dangoffset < angoffset) ? 180*dt : 0;
            if(validAngle(wandInclination, 75)) {
            //if(Math.abs(wand.directionY) <= 0.96) {
                var a = 0.05, angstep = 1;
                var k = Math.floor(((Math.floor(-Math.atan2(wand.directionX, wand.directionZ) * 180/pi) + 360) % 360) / angstep) * angstep;
                oldang = newang; //viewer.camera.heading; //newang;
                if(Math.abs((k%360) - oldang) >= 300)
                    newang = k % 360;
                else
                newang = (((a * (k % 360) + (1-a) * oldang) % 360) + 360) % 360;
                viewer.camera.setView({
                    heading: (newang + dangoffset) * pi/180 + 1.23*pi,
                    pitch: pitch,//viewer.camera.pitch,
                    roll: viewer.camera.roll
                });
            }
            else {
                viewer.camera.setView({
                    heading: (oldang + dangoffset) * pi/180 + 1.23*pi,
                    pitch: pitch,//viewer.camera.pitch,
                    roll: viewer.camera.roll
                });
            }

            // up-down
            var spd = 0.5 * (viewer.camera.positionCartographic.height);
            var pos = viewer.camera.positionCartographic.clone(), threshold = 100;
            if(Math.abs(pos.height - destheight) > threshold) {
                if(pos.height < destheight)
                    pos.height = pos.height + spd * dt;
                else if(pos.height > destheight)
                    pos.height = pos.height - spd * dt;
                viewer.camera.setView({ positionCartographic: pos });
            }

            // move forwards
            var fspeed = 0.0;
/*
            if(Math.abs(wand.directionY) > 0.875)
                fspeed = 0.0; // em pé
            else if(Math.abs(wand.directionY) > 0.6)
                fspeed = 0.0; //0.5;
            else
                fspeed = 1.0;
*/
//setStatus(wand.directionY);
            fspeed = validAngle(wandInclination, 45) ? 1.0 : 0.0;
            setSpeed(3*fspeed);
/*
            var threshold = { x: 0.23, y: 0.2, z: 0.25 };

            // direction
            if(Math.abs(wand.x - 0.5) >= threshold.x) {
                var speed = Cesium.Math.toRadians(5 * Math.abs(wand.x - 0.5)/0.1);
                viewer.camera.setView({
                    heading: viewer.camera.heading + sign(wand.x - 0.5) * speed * dt,
                    pitch: viewer.camera.pitch,
                    roll: viewer.camera.roll
                });
            }

            // up-down
            else if(Math.abs(wand.y - 0.5) >= threshold.y) {
                var MINHEIGHT = 350;
                var speed = 1.0 * (Math.abs(wand.y - 0.5) * viewer.camera.positionCartographic.height);
                var pos = viewer.camera.positionCartographic.clone();
                pos.height = Math.max(MINHEIGHT, pos.height - sign(wand.y - 0.5) * speed * dt);
                viewer.camera.setView({
                    positionCartographic: pos
                });
            }

            // move forwards
            else if(Math.abs(wand.z - 0.5) >= threshold.z) {
                var f = Math.min(100, 200 * Math.abs(wand.z - 0.5));
                mouseSimulator.keepWalking(0, -sign(wand.z - 0.5) * f, 0);
            }
*/
        }
        else
            setSpeed(0);


        wasActive = wand.isActive();
    }

    var that = {
        state: {
            DISCRETE: 0,
            CONTINUOUS: 1,
        },

        currentState: function() {
            return state;
        },

        state2str: function(st) {
            for(var k in this.state) {
                if(this.state[k] == st)
                    return k;
            }
            return 'UNKNOWN';
        },

        changeState: function(st) {
            var oldState = state;
            console.log('[state] changing from ' + this.state2str(state) + ' to ' + this.state2str(st));

            // old state
            switch(state) {
            case this.state.DISCRETE:
                break;

            case this.state.CONTINUOUS:
                break;
            }
            initialHeading = desiredHeading = viewer.camera.heading;
            initialPitch = desiredPitch = viewer.camera.pitch;

            // new state
            switch(state = st) {
            case this.state.DISCRETE:
                if(oldState != state) {
                    initialHeading = viewer.camera.heading;
                    initialPitch = viewer.camera.pitch;
                    desiredHeading = Cesium.Math.toRadians(90); // north (0 is east)
                    desiredPitch = Cesium.Math.toRadians(-90); // looking down
                    a = 0.0;
                }
                //setStatus('Navegação discreta');
                break;

            case this.state.CONTINUOUS:
/*
                var i = gesture[0], f = gesture[gesture.length - 1];
                //var angle = Math.atan2(f.z - i.z, f.x - i.x);
                var angle = Math.atan2(f.y - i.y, f.x - i.x);
                angle += Math.PI / 2.0;
                if(angle > Math.PI) angle -= 2.0 * Math.PI;
                console.log('Got angle: ' + Cesium.Math.toDegrees(angle));
*/
                initialHeading = viewer.camera.heading;
                desiredHeading = initialHeading;// + angle;
                initialPitch = viewer.camera.pitch;
                desiredPitch = Cesium.Math.toRadians(-30);
                viewer.camera.setView({
                    heading: viewer.camera.heading, //angle,
                    pitch: viewer.camera.pitch, //Cesium.Math.toRadians(-30),
                    roll: Cesium.Math.toRadians(0)
                });
                a = 0;
                this.startFlying();
                //setStatus('Navegação contínua');
                break;
            }
        },

        startFlying: function() {
            isFlying = true;
        },

        stopFlying: function() {
            isFlying = false;
        },

        isFlying: function() {
            return isFlying;
        }
    };

    //wand.callback.circle = function(g) { gesture = g; that.changeState(that.state.DISCRETE); };
    //wand.callback.arrow = function(g) { gesture = g; that.changeState(that.state.CONTINUOUS); };
    //wand.callback.hat = wand.callback.v = wand.callback.arrow;

    function changeHeight(sgn) { var factor = 15; destheight = viewer.camera.positionCartographic.height * (sign(sgn) > 0 ? factor : (1.0/factor)); }
    wand.callback.hat = function(g) { gesture = g; changeHeight(1); setStatus('&uarr;'); }
    wand.callback.v = function(g) { gesture = g; changeHeight(-1); setStatus('&darr;'); }
    wand.callback.arrow = function(g) { gesture = g; setStatus('arrow'); }
    wand.callback.circle = function(g) { gesture = g; setStatus('&#8635;'); angoffset += 180; }

    intv = setInterval(function() { update.call(that); }, 1000/33);

    return that;
})();

stateMachine.changeState(stateMachine.state.CONTINUOUS);


var step = 0;
setInterval(function() {
    if(getSpeed() > 0) {
        if((step++) % 2)
            $('#cat1').addClass('hidden');
        else
            $('#cat1').removeClass('hidden');
    }

}, 300);

var tt = 0;
setInterval(function() {
    $('#crossfade').css('left', ($(window).innerWidth() - 140)/2);
    $('#crossfade').css('bottom', 250 + 10 * Math.cos(Math.PI * 0.05 * tt++));
}, 1000/33);
