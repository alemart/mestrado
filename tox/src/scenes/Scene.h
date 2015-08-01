#ifndef _SCENE_H
#define _SCENE_H

class MyApplication;

// abstract scene
class Scene
{
public:
    Scene(MyApplication* app) : _application(app) { }
    virtual ~Scene() { }

    virtual void update() = 0;
    virtual void draw() = 0;

    virtual void keyPressed(int key) { }
    virtual void keyReleased(int key) { }
    virtual void mouseMoved(int x, int y) { }
    virtual void mouseDragged(int x, int y, int button) { }
    virtual void mousePressed(int x, int y, int button) { }
    virtual void mouseReleased(int x, int y, int button) { }

    inline MyApplication* application() const { return _application; }

private:
    MyApplication* _application;
};

#endif
