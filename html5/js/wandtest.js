function sign(x) { return x >= 0 ? 1 : -1; }
var camera, scene, renderer, skybox, t = 0;
/*
var wand = {
    x: 0,
    y: 0,
    z: 0.5,
    visible: true,
    active: true,
    mesh: {
        active: null,
        inactive: null
    }
};
var trail = [];
var gesture = [], gesture2 = [];
var rcg = new DollarRecognizer();
*/
var trail = [];
var gesture = [], gesture2 = [];
var rcg = new DollarRecognizer();





//
// magic wand TUIO interface
//
var wand = (function() {
    var $1 = new DollarRecognizer();
    var visible = false;
    var active = false;
    var gesture = [], gesture3 = [];

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
        },

        // this demo
        mesh: {
            active: null,
            inactive: null
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

        // collecting gestures
        if(visible && active) {
            gesture.push(new Point(that.x, that.y)); // 2D projection
            gesture3.push({x: that.x, y: that.y, z: that.z});
        }

        // recognize a gesture
        if(visible && !active && gesture.length >= 85) {
            console.log('[$1] recognizing...');
            var g = $1.Recognize(gesture);
            console.log('[$1] recognized "' + g.Name + '" (' + g.Score + ').');
            var cb = that.callback[g.Name];
            if(cb)
                cb(gesture3.slice(), g.Score);
            gesture.length = 0;
            gesture3.length = 0;
        }
    }, 1000 / 30);

    return that;
})();


















window.onload = init;
function init() {
    scene = new THREE.Scene();

    camera = new THREE.PerspectiveCamera( 75, window.innerWidth / window.innerHeight, 1, 10000 );
    camera.position.x = 0;
    camera.position.y = 150;
    camera.position.z = 300;

    renderer = new THREE.WebGLRenderer();//CanvasRenderer();
    renderer.setSize( window.innerWidth, window.innerHeight );

    var loader = new THREE.ObjectLoader;
    loader.load('res/scene.json', function(obj) {
        skybox = obj;
        skybox.scale.x = 2;
        scene.add(skybox);
    });

    loader.load('res/wand.json', function(obj) {
        wand.mesh.active = obj;
        scene.add(wand.mesh.active);
    });

    loader.load('res/wand2.json', function(obj) {
        wand.mesh.inactive = obj;
        scene.add(wand.mesh.inactive);
    });

/*
var geometry = new THREE.TextGeometry("Olá mundo!", {font: "droid serif", weight: "bold", size: 1, height: 0.4});
var material = new THREE.MeshBasicMaterial({color: 0xffffff});
var text = new THREE.Mesh(geometry, material);
text.position.x = 10;
text.position.y = 10;
text.position.z = 10;
scene.add(text);
*/
    document.body.appendChild(renderer.domElement);
    setTimeout(animate, 500);
//    animate();
return;
    for(var k in wand.callback) {
        wand.callback[k] = function(seq, score) {
            var str = k + ' (' + score + ') !';
            $('#status').html('Gesture: ' + str);
        };
    }
}

function animate() {
    requestAnimationFrame( animate );
    render(t += 0.005);
}

function render(t) {
    if(wand.isVisible()) {
        var mesh = wand.isActive() ? wand.mesh.active : wand.mesh.inactive;
        mesh.traverse(function(obj) { obj.visible = true; console.log(obj); });
        mesh.position.x = -150 + 350 * wand.x;
        mesh.position.y = 300 - 300 * wand.y;
        mesh.position.z = -150 + 300 * wand.z;
        //mesh.rotation.x = -3.14/4;
        mesh.rotation.z = sign(wand.directionY) * Math.atan2(wand.directionY, wand.directionX) - 3.14/2;
        mesh.rotation.x = -Math.atan2(wand.directionZ, wand.directionY) - 3.14;
        mesh.scale.x = 0.4;
        mesh.scale.y = 0.4;
        mesh.scale.z = 0.4;
        (!wand.isActive() ? wand.mesh.active : wand.mesh.inactive).traverse(function(obj) { obj.visible = false; });
        if(wand.isActive()) {
            collectGesture();
            createTrailElement();
        }
    }
    else if(0) {
        $.each([wand.mesh.active, wand.mesh.inactive], function(idx, mesh) {
            mesh.traverse(function(obj) {
                obj.visible = false;
            });
        });
    }

    if((wand.isVisible() && !wand.isActive()) || !wand.isVisible()) {
        if(gesture.length > 40) {
            recognizeGesture();
            while(gesture.length > 0)
                gesture.pop();
        }
        for(k in trail)
            scene.remove(trail[k]);
        trail.length = 0;
    }

    //grabTuioStuff();
    renderer.render(scene, camera);
}

function collectGesture()
{
    gesture.push(new Point(200 * wand.x, 200 * wand.y));
    gesture2.push(new Point(200 * wand.x, 200 * wand.z));
}

function createTrailElement()
{
    var mesh = wand.isActive() ? wand.mesh.active : wand.mesh.inactive;
    var geometry = new THREE.SphereGeometry( 5, 32, 32 );
    var material = new THREE.MeshBasicMaterial( {color: 0xffff66} );
    var sphere = new THREE.Mesh( geometry, material );
    sphere.position.x = mesh.position.x;
    sphere.position.y = mesh.position.y;
    sphere.position.z = mesh.position.z;
    scene.add( sphere );
    trail.push(sphere);
    if(trail.length > 1000) {
        scene.remove( trail.shift() );
    }
}

function recognizeGesture()
{
    var g = rcg.Recognize(gesture);
    var g2 = rcg.Recognize(gesture2);
    //alert("xy: " + g.Name + " (" + g.Score + ")\nxz: " + g2.Name + " (" + g2.Score + ")");
    var str = g.Name + " (" + g.Score + ")";
    //str += ' | ' + g2.Name + " (" + g2.Score + ")";
    $('#status').html('Gesture: ' + str);
}

/*
function grabTuioStuff()
{
    var tuioWand = (function() {
        var objs = wall.objects();
        for(k in objs) {
            var wo = objs[k];
            if(wo.type().match(/^wand_yellow/))
                return wo;
        }
        return null;
    })();

    if(tuioWand) {
        wand.visible = true;
        wand.x = tuioWand.x();
        wand.y = tuioWand.y();
        wand.z = tuioWand.z();
        wand.active = !!(tuioWand.type().match(/:active$/));
    }
    else
        wand.visible = true; //false;
}
*/
