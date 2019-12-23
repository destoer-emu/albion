#pragma once
#ifdef FRONTEND_QT
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QPainter>
#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../headers/lib.h"


class FrameBuffer : public QWidget
{
    Q_OBJECT

public:
    void init(int x, int y);
    void redraw(std::vector<uint32_t> &other);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void swap_buffer(std::vector<uint32_t> &other);

    int X;
    int Y;
    bool ready = false;
    std::vector<uint32_t> screen;
    std::mutex screen_mutex;
};
#endif

