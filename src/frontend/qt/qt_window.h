#ifdef FRONTEND_QT
#include <QMainWindow>
#include <QApplication>
#include <QTextStream>
#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QVBoxLayout>
#include <albion/emulator.h>
#include "framebuffer.h"
#include "gameboy_instance.h"
#include "gba_instance.h"

class QtMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QtMainWindow();
    ~QtMainWindow();

    FrameBuffer framebuffer;
private slots:
    void open();
    void load_state();
    void save_state();
    //void disassembler(); <-- disassembler implemented in imgui first for now!

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void create_actions();
    void create_menus();


    QMenu *file_menu;
    QAction *open_act;
    QAction *load_state_act;
    QAction *save_state_act;

    QMenu *emu_menu;
    QAction *pause_emulator_act;
    QAction *continue_emulator_act;
    QAction *disable_audio_act;
    QAction *enable_audio_act;

    QLabel *info_label;
    QThread *emu_thread;


    void stop_emu();
    void start_emu();
    void disable_audio();
    void enable_audio();

    bool emu_running = false;
    GameboyInstance gb_instance;
    GbaInstance gba_instance;
    emu_type running_type = emu_type::none;
};
#endif
