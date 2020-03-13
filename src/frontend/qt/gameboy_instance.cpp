#include <frontend/qt/gameboy_instance.h>
#include <frontend/gb/controller.h>

void GameboyInstance::init(FrameBuffer *f)
{
    if(f == nullptr)
    {
        throw std::runtime_error("[gb-init] Warning framebuffer is invalid (we likely have a bug somewhere)\n");
    }

    framebuffer = f;    
}

void GameboyInstance::run()
{
    constexpr uint32_t fps = 60;
    constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
    uint64_t next_time = current_time() + screen_ticks_per_frame;

    gb.quit = false;


    try
    {

        if(framebuffer == nullptr)
        {
            throw std::runtime_error("[gb-run] Warning framebuffer is invalid (we likely have a bug somewhere)\n");
        }


        GbControllerInput controller;
	    controller.init();

        while(!gb.quit)
        {
            controller.update(gb);

            gb.run();


            // update the screen!
            // this really needs to be optimised...
            framebuffer->redraw(gb.ppu.screen);


            if(gb.throttle_emu)
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


void GameboyInstance::stop()
{
    gb.quit = true;
}


void GameboyInstance::disable_audio()
{
    gb.apu.playback.stop();
}

void GameboyInstance::enable_audio()
{
    gb.apu.playback.start();
}


// we are gonna define our own custom key set that is done at compile time based on the frontend if 
// we generalize this
void GameboyInstance::key_pressed(int key)
{
    gb.key_input(key,true);
}

void GameboyInstance::key_released(int key)
{
    gb.key_input(key,false);
}


// here we need to code to throw if hte instance is still running as these operations require 
// that the emulation is not running


void GameboyInstance::load_state(const std::string &filename)
{
    std::string backup = std::filesystem::current_path().string() + path_seperator + "backup.state";

    // save a state just before incase they somehow
    gb.save_state(backup);

    try
    {
        gb.load_state(filename);
    }

    // reload a clean state and rethrow so the frotend can handle the error
    catch(std::exception &ex)
    {
        gb.load_state(backup);
        throw ex;
    }
}

void GameboyInstance::save_state(const std::string &filename)
{
    // nothing to clean up here at this level
    // so just let exceptions go str8 to the frontend
    gb.save_state(filename);
}

void GameboyInstance::save_backup_ram()
{
    gb.mem.save_cart_ram();
}

void GameboyInstance::reset(const std::string &filename)
{
    gb.reset(filename);
}