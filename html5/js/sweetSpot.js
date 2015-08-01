//
// Sweet-spot
//

function SweetSpot()
{
    // Let:
    // - I be the [0,1]x[0,1] Input space (data is received from TUIO)
    // - S be the [0,1]x[0,1] SweetSpot space (corners: topleft(x1,y1), topright(x2,y2), bottomright(x3,y3), bottomleft(x4,y4) such that 0 <= xj, yj <= 1)
    // - N be the [0,1]x[0,1] NormalizedWall space (corners: topleft(0,0), topright(1,0), bottomright(1,1), bottomleft(0,1))
    // - W be the Wall space (corners: topleft(0,0), topright(w,0), bottomright(w,h), bottomleft(0,h) where (w,h) is the screen resolution)
    //
    // Give me f: I -> W such that f = g o h, where:
    // - h: I -> S is a homography
    // - g: S -> W is a scale transform
    var homography = null;

    // transforms
    var inputToSweetSpot = function(p) {
        var w = $V([p.x, p.y, 1]);
        var v = homography.x(w);
        return { x: v.e(1) / v.e(3), y: v.e(2) / v.e(3) };
    };

    var sweetSpotToWall = function(p) {
        var w = $(window).innerWidth(), h = $(window).innerHeight();
        return { x: w * p.x, y: h * p.y };
    };

    var inputToWall = function(p) {
        return sweetSpotToWall(inputToSweetSpot(p));
    };

    // homography matrix
    var computeHomography = function(q) {
        // p is in Input space
        // q is in SweetSpot space
        // homography maps Input space to SweetSpot space
        var p = {
            x1: 0.0, y1: 0.0,
            x2: 1.0, y2: 0.0,
            x3: 1.0, y3: 1.0,
            x4: 0.0, y4: 1.0
        };

        // solve Mw = v for w
        var v = $V([q.x1, q.y1, q.x2, q.y2, q.x3, q.y3, q.x4, q.y4]);
        var M = $M([
            [ p.x1, p.y1,    1,    0,    0,    0, -p.x1 * q.x1, -p.y1 * q.x1 ],
            [    0,    0,    0, p.x1, p.y1,    1, -p.x1 * q.y1, -p.y1 * q.y1 ],
            [ p.x2, p.y2,    1,    0,    0,    0, -p.x2 * q.x2, -p.y2 * q.x2 ],
            [    0,    0,    0, p.x2, p.y2,    1, -p.x2 * q.y2, -p.y2 * q.y2 ],
            [ p.x3, p.y3,    1,    0,    0,    0, -p.x3 * q.x3, -p.y3 * q.x3 ],
            [    0,    0,    0, p.x3, p.y3,    1, -p.x3 * q.y3, -p.y3 * q.y3 ],
            [ p.x4, p.y4,    1,    0,    0,    0, -p.x4 * q.x4, -p.y4 * q.x4 ],
            [    0,    0,    0, p.x4, p.y4,    1, -p.x4 * q.y4, -p.y4 * q.y4 ]
        ]);
        var MtMinv = (M.transpose().x(M)).inverse();
        if(MtMinv) {
            var pseudoInverse = (MtMinv).x(M.transpose());
            var w = pseudoInverse.x(v);
            return $M([
                [w.e(1), w.e(2), w.e(3)],
                [w.e(4), w.e(5), w.e(6)],
                [w.e(7), w.e(8),   1   ]
            ]);
        }
        else {
            alert("SweetSpot error: can't estimate the homography (MtM is singular)");
            return Matrix.I(3);
        }
    };

    // local storage
    var get = function() {
        var s = localStorage.getItem("sweetSpot");
        return s ? JSON.parse(s) : {
            x1: 0.0, y1: 0.0,
            x2: 1.0, y2: 0.0,
            x3: 1.0, y3: 1.0,
            x4: 0.0, y4: 1.0
        };
    };

    var set = function(s) {
        localStorage.setItem("sweetSpot", JSON.stringify(s));
        homography = computeHomography(s);
        return s;
    };

    // constructor
    (function() {
        set(get()); // computes the homography and saves stuff in local storage
    })();

    // public attributes
    this.get = get;
    this.set = set;
    this.i2w = function(x, y) { return inputToWall({x: x, y: y}); }; // give a (x,y) input point to receive a 2D {x: ... , y: ...} wall point
}
