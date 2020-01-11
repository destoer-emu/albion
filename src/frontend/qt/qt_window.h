#ifdef FRONTEND_QT
#include <QMainWindow>
#include <QApplication>
#include <QTextStream>
#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QVBoxLayout>
#include "framebuffer.h"
#include "emu_instance.h"
#include "../../headers/gb.h"
#include "../../headers/lib.h"

class QtMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QtMainWindow();
    ~QtMainWindow();

    FrameBuffer framebuffer;
private slots:
    void open();
    //void disassembler(); <-- disassembler implemented in imgui first for now!

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void create_actions();
    void create_menus();


    QMenu *emu_menu;
    QActionGroup *alignment_group;
    QAction *open_act;
    QAction *disassembler_act;
    QLabel *info_label;
    QThread *emu_thread;


    void destroy_emu();

    GB gb;
    bool emu_running = false;
    EmuInstance *emu_instance;
};
#endif
