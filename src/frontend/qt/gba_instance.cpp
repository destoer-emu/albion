#include "gba_instance.h"

using namespace gameboyadvance;

void GbaInstance::init(FrameBuffer *f)
{
    if(f == nullptr)
    {
        throw std::runtime_error("[gb-init] Warning framebuffer is invalid (we likely have a bug somewhere)\n");
    }

    framebuffer = f;    
}

// the qt build is being incredibly slow somewhere and i have no idea why yet
void GbaInstance::run()
{
    constexpr uint32_t fps = 60;
    constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
    uint64_t next_time = current_time() + screen_ticks_per_frame;

    gba.quit = false;

    try
    {

        if(framebuffer == nullptr)
        {
            throw std::runtime_error("[gba-run] Warning framebuffer is invalid (we likely have a bug somewhere)\n");
        }


        while(!gba.quit)
        {
            gba.run();


            // update the screen!
            // this really needs to be optimised...
            framebuffer->redraw(gba.disp.screen);

            
            //if(gba.throttle_emu)
            {
                // throttle the emulation
                this->msleep(time_left(next_time));
            }
            
            /*
            else // run at 8x speed
            {
                this->msleep(time_left(next_time) / 8);
            }
            */

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


void GbaInstance::stop()
{
    gba.quit = true;
}

// we are gonna define our own custom key set that is done at compile time based on the frontend if 
// we generalize this
void GbaInstance::key_pressed(int key)
{
    gba.key_input(key,true);
}

void GbaInstance::key_released(int key)
{
    gba.key_input(key,false);
}


// here we need to code to throw if hte instance is still running as these operations require 
// that the emulation is not running

// unimpl
void GbaInstance::load_state(const std::string &filename)
{
    UNUSED(filename);
}

void GbaInstance::save_state(const std::string &filename)
{
    UNUSED(filename);
}

void GbaInstance::save_backup_ram()
{
    gba.mem.save_cart_ram();
}

void GbaInstance::reset(const std::string &filename)
{
    gba.reset(filename);
}