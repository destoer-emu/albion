#pragma once
#include <QThread>
#include "../headers/gb.h"
#include "framebuffer.h"

class EmuInstance : public QThread
{
    Q_OBJECT
public:
    EmuInstance(QObject *parent = nullptr, GB *g = nullptr, FrameBuffer *f = nullptr);
    void run();

private:
     GB *gb;
     FrameBuffer *framebuffer;
};


