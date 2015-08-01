#ifndef _CANVAS_H
#define _CANVAS_H

#include <vector>
#include <map>
#include "ofMain.h"
#include "../WallApp.h"

class Canvas : public WallApp
{
public:
    Canvas(WallOS* system);
    ~Canvas();

    void setup();
    void update();
    void draw();

private:
    struct Camera
    {
        ofVec2f position; // topleft corner of the screen, in [0,1]^2 coordinates
        ofVec2f speed;
    } _camera;

    struct Point
    {
        ofVec2f position; // in [0,1]^2 normalized coords
        ofColor color;

        // path data
        int tuioSessionId;
        int next; // an index of _points
    };
    std::vector<Point> _points;

    struct EraserSpot {
        ofVec2f position; // [0,1]^2
        int tuioSessionId;
    };
    std::vector<EraserSpot> _eraserSpots;

    static const char* _filepath;
    void _loadFromDisk();
    void _saveToDisk();

    int _findIndexOfPreviousPointHavingIndex(int idx);
    ofColor _touchType2color(int touchType) const;
    bool _isMarker(int touchType) const;

    //
    // Renderers
    // (for delayed rendering)
    //
    class Renderer {
    public:
        Renderer(int zindex = 0) : _zindex(zindex) { }
        virtual ~Renderer() { }
        int zindex() const { return _zindex; }
        virtual void render(WallOS* system, const Camera& cam) = 0;

    protected:
        int _zindex;
    };

    class CurveRenderer : public Renderer {
    public:
        CurveRenderer(int zindex, std::vector<ofVec2f> pointsInNormalizedCoords, ofColor color) : Renderer(zindex), _points(pointsInNormalizedCoords), _color(color) { }
        ~CurveRenderer() { }
        void render(WallOS* system, const Camera& cam);

    private:
        std::vector<ofVec2f> _points;
        ofColor _color;
    };

    class EraserRenderer : public Renderer {
    public:
        EraserRenderer(int zindex, ofVec2f eraserSpotInNormalizedCoords, int size) : Renderer(zindex), _spot(eraserSpotInNormalizedCoords), _size(size) { }
        ~EraserRenderer() { }
        void render(WallOS* system, const Camera& cam);

    private:
        ofVec2f _spot;
        int _size;
    };

    class RendererComparator {
    public:
        inline bool operator()(const Renderer* a, const Renderer* b) const { return a->zindex() > b->zindex(); }
    };
};

#endif
