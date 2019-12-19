#pragma once
#ifdef FRONTEND_QT
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../headers/lib.h"


class FrameBuffer : public QOpenGLWidget
{

    Q_OBJECT

public:
    // holds a mutable ref to a buffer we can write to
    void init(std::vector<uint32_t> *buf, int x, int y);
    void redraw();

protected:

    void initializeGL() override;

    // called when we need to draw this
    void paintGL() override;

private:
    bool ready = false;
    int width;
    int height;
    std::vector<uint32_t> *screen = nullptr;
    QTimer *timer;

    GLuint screen_texture;
};
#endif
