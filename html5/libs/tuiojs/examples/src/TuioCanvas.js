var TuioCanvas = {
    init: function() {
        this.Main.init();
    }
};

TuioCanvas.Main = (function() {
    var client = null,
    screenW = null,
    screenH = null,
    time = null,
    canvas = null,
    context = null,
    objSize = 50,

    init = function() {
        screenW = $(window).innerWidth();
        screenH = $(window).innerHeight();
        time = new Date().getTime();
        canvas = $("#tuioCanvas").get(0);
        canvas.width = screenW;
        canvas.height = screenH;
        context = canvas.getContext("2d");

        initClient();
    },

    initClient = function() {
        client = new Tuio.Client({
            host: "http://localhost:5000"
        });
        client.on("connect", onConnect);
        client.connect();
    },

    onConnect = function() {
        draw();
    },

    starting = true,
    draw = function() {
        requestAnimationFrame(draw);

        if(starting) {
            starting = false;
            context.fillStyle = "#000000";
            context.fillRect(0, 0, canvas.width, canvas.height);
        }

        var cursors = client.getTuioCursors(),
        objects = client.getTuioObjects();

        for (var i in cursors) {
            drawCursor(cursors[i]);
        }

        for (var i in objects) {
            drawToxObject(objects[i]);
        }
    },

    drawCursor = function(cursor) {
        context.fillStyle = "#009fe3";
        context.beginPath();
        context.arc(
            cursor.getScreenX(screenW),
            cursor.getScreenY(screenH),
            objSize * 0.5,
            0,
            Math.PI * 2
        );
        context.closePath();
        context.fill();
    },

    drawObject = function(object) {
        context.save();

        context.translate(
            object.getScreenX(screenW) + objSize * 0.5, 
            object.getScreenY(screenH) + objSize * 0.5
        );
        context.rotate(object.getAngle());

        context.fillStyle = "#ffffff";
        context.fillRect(-objSize * 0.5, -objSize * 0.5, objSize, objSize);

        context.restore();
    },

    drawToxObject = function(object) {
        context.fillStyle = (function(type) {
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

        context.beginPath();
        context.arc(
            object.getScreenX(screenW),
            object.getScreenY(screenH),
            objSize * 0.5,
            0,
            Math.PI * 2
        );
        context.closePath();
        context.fill();
    };

    return {
        init: init
    };
}());
