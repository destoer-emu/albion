#ifdef FRONTEND_QT
#include "emu_instance.h"
#include <QMessageBox>

EmuInstance::EmuInstance(QObject *parent, GB *g, FrameBuffer *f)
{
    UNUSED(parent);

    if(g == nullptr || f == nullptr)
    {
        throw std::runtime_error("invalid emu instance!");
    }


    gb = g;
    framebuffer = f;
}



void EmuInstance::run()
{
    constexpr uint32_t fps = 60;
    constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
    uint64_t next_time = current_time() + screen_ticks_per_frame;

    try
    {
        while(!gb->quit)
        {
            gb->run();


            // update the screen!
            // this really needs to be optimised...
            framebuffer->redraw(gb->ppu.screen);


            if(gb->throttle_emu)
            {
                // throttle the emulation
                this->msleep(time_left(next_time));
            }

            else // run at 8x speed
            {
                this->msleep(time_left(next_time) / 8);
            }

            next_time = current_time() + screen_ticks_per_frame;
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