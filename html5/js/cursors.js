//
// Whenever you touch the wall, a nice (usually black) cursor appears
//

function Cursors()
{
    this._cursors = [];
}

Cursors.prototype = {

    addAt: function(x, y, color, radius) {
        this._cursors.push({x: x, y: y, color: color || 'rgba(0,0,0,0.5)', radius: radius || 80});
    }

};

$(function() {
    window.cursors = new Cursors();

    $('<div id="cursor_wrapper"></div>').appendTo('body');
    setInterval(function() {
        $('.cursor').remove();
        var v = cursors._cursors;
        while(v.length > 0) {
            var p = v[v.length - 1];
            $('<div class="cursor"></div>').css({
                'top': p.y - 90,
                'left': p.x - 40,
                'backgroundColor': p.color,
                'width': p.radius * 2,
                'height': p.radius * 2
            }).appendTo($('#cursor_wrapper'));
            v.pop();
        }
    }, 1000.0 / 30.0);
});
