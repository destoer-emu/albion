#pragma once
#ifdef FRONTEND_QT
#include <QThread>
#include <QMutex>
#include <gb/gb.h>
#include "framebuffer.h"

class EmuInstance : public QThread
{
    Q_OBJECT
public:
    EmuInstance(QObject *parent = nullptr, gameboy::GB *g = nullptr, FrameBuffer *f = nullptr);
    void run();

private:
    gameboy::GB *gb;
    FrameBuffer *framebuffer;
};
#endif

