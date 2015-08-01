var camera, scene, renderer, skybox, t = 0;
var mesh = null;
var finger = { };

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

    var loader = new THREE.JSONLoader();
    loader.load('res/car.js', function (geometry, materials) {
      var material = new THREE.MeshLambertMaterial({
        map: THREE.ImageUtils.loadTexture('res/gtare.jpg'),   
        colorAmbient: [0.480000026226044, 0.480000026226044, 0.480000026226044],
        colorDiffuse: [0.480000026226044, 0.480000026226044, 0.480000026226044],
        colorSpecular: [0.8999999761581421, 0.8999999761581421, 0.8999999761581421]
      });
      
      mesh = new THREE.Mesh(
        geometry,
        material
      );

      mesh.receiveShadow = true;
      mesh.castShadow = true;
      mesh.rotation.y = -Math.PI/5;

        var s = 1.0; //1.25;
        mesh.scale.x = s;
        mesh.scale.y = s;
        mesh.scale.z = s;

        mesh.position.y -= 0;

      scene.add(mesh);
      render(); 
    });

    document.body.appendChild(renderer.domElement);
    setTimeout(animate, 500);
}

function animate() {
    requestAnimationFrame( animate );
    render(t += 0.005);
}

function render(t) {
/*
    if(wand.visible) {
        var mesh = wand.active ? wand.mesh.active : wand.mesh.inactive;
        mesh.traverse(function(obj) { obj.visible = true; });
        mesh.position.x = -150 + 350 * wand.x;
        mesh.position.y = 300 - 300 * wand.y;
        mesh.position.z = -150 + 300 * wand.z;
        mesh.rotation.x = -3.14/4;
        mesh.scale.x = 0.4;
        mesh.scale.y = 0.4;
        mesh.scale.z = 0.4;
        (!wand.active ? wand.mesh.active : wand.mesh.inactive).traverse(function(obj) { obj.visible = false; });
        if(wand.active) {
            collectGesture();
            createTrailElement();
        }
    }
    else {
        $.each([wand.mesh.active, wand.mesh.inactive], function(idx, mesh) {
            mesh.traverse(function(obj) {
                obj.visible = false;
            });
        });
    }

    if((wand.visible && !wand.active) || !wand.visible) {
        if(gesture.length > 40) {
            recognizeGesture();
            while(gesture.length > 0)
                gesture.pop();
        }
        for(k in trail)
            scene.remove(trail[k]);
        trail.length = 0;
    }
*/

    grabTuioStuff();
    renderer.render(scene, camera);
}

function grabTuioStuff()
{
    var f = (function() {
        var objs = wall.objects();
        for(k in objs) {
            var wo = objs[k];
            if(wo.type().match(/finger$/))
                return wo;
        }
        return null;
    })();

/*
    if(tuioWand) {
        wand.visible = true;
        wand.x = tuioWand.x();
        wand.y = tuioWand.y();
        wand.z = tuioWand.z();
        wand.active = !!(tuioWand.type().match(/:active$/));
    }
    else
        wand.visible = true; //false;
*/
    if(f) {
        console.log(f.x());
        if(f.id() in finger) {
            var current = (finger[f.id()].current = f);
            var first = finger[f.id()].first;
            var rot = finger[f.id()].rot;
            var k = 250;
            mesh.rotation.y = rot + (current.x() - first.x()) * (3.14159 / 180.0) * k;
        }
        else
            finger[f.id()] = { first: f, current: f, rot: mesh.rotation.y };
    }
}

