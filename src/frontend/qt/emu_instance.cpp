#ifdef FRONTEND_QT
#include "emu_instance.h"
#include <QMessageBox>

EmuInstance::EmuInstance(QObject *parent, GB *g, FrameBuffer *f)
{
    UNUSED(parent);
    gb = g;
    framebuffer = f;
}



void EmuInstance::run()
{
    constexpr uint32_t fps = 60;
    constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
    uint64_t next_time = current_time() + screen_ticks_per_frame;

    framebuffer->init(gb->ppu.X,gb->ppu.Y);
    try
    {
        while(!gb->quit)
        {
            gb->run();


            // update the screen!
            framebuffer->redraw(gb->ppu.screen);

            // throttle the emulation
            this->msleep(time_left(next_time));
            next_time += screen_ticks_per_frame;
        }
    }

    catch(std::exception &ex)
    {
        QMessageBox messageBox;
        messageBox.critical(nullptr,"Error",ex.what());
        messageBox.setFixedSize(500,200);
    }
}
#endif