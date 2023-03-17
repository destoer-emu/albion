#pragma once
#ifdef FRONTEND_QT
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QPainter>
#include <QGLWidget>
#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <albion/lib.h>


class FrameBuffer : public QOpenGLWidget
{
    Q_OBJECT

public:
    void init(int x, int y);
    void redraw(std::vector<uint32_t> &other);

protected:
    void paintGL() override;

private:
    void swap_buffer(std::vector<uint32_t> &other);

    int X = 0;
    int Y = 0;
    std::vector<uint32_t> screen;
    std::mutex screen_mutex;
};
#endif

