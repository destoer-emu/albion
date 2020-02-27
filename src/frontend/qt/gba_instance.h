#pragma once
#include <gba/gba.h>
#include <QKeyEvent>
#include <QString>
#include <QMessageBox>
#include <QThread>
#include "framebuffer.h"


class GbaInstance : public QThread
{
    Q_OBJECT
public:

    void init(FrameBuffer *f);

    void reset(const std::string &filename);
    void run(); // called on thread start
    void stop();
    void key_pressed(int key);
    void key_released(int key);

    // all of these will pause the emulation
    // and then resume it
    void load_state(const std::string &filename);
    void save_state(const std::string &filename);
    void save_backup_ram();

    static constexpr auto X = gameboyadvance::SCREEN_WIDTH;
    static constexpr auto Y = gameboyadvance::SCREEN_HEIGHT;

private:
    gameboyadvance::GBA gba;
    FrameBuffer *framebuffer = nullptr;
};