#ifdef FRONTEND_QT
#include <QVBoxLayout>
#include <QStatusBar>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeyEvent>
#include <QString>
#include <iostream>
#include <thread>
#include <string.h>
#include "framebuffer.h"
#include "qt_window.h"

// main window ctor 
QtMainWindow::QtMainWindow()
{
    create_actions();
    create_menus();


    setWindowTitle(tr("Menus"));
    setMinimumSize(160,144);
    setCentralWidget(&framebuffer);
}

// if any of our emulator threads are still running terminate them gracefully
QtMainWindow::~QtMainWindow()
{
    destroy_emu();
}


// create our actions for our menu
void QtMainWindow::create_actions()
{
    open_act = new QAction(tr("&open rom"), this);
    open_act->setShortcuts(QKeySequence::New);
    open_act->setStatusTip(tr("open a rom file"));
    connect(open_act,&QAction::triggered, this, &QtMainWindow::open);

/*
    disassembler_act = new QAction(tr("&open disassembler"),this);
    open_act->setShortcuts(QKeySequence::New);
    open_act->setStatusTip(tr("open the debugger"));
    connect(disassembler_act,&QAction::triggered, this, &QtMainWindow::disassembler);
*/

    alignment_group = new QActionGroup(this);
    alignment_group->addAction(open_act);
}


// create our top bar menu
void QtMainWindow::create_menus()
{
    emu_menu = menuBar()->addMenu(tr("&File"));
    emu_menu->addAction(open_act);
/*
    emu_menu = menuBar()->addMenu(tr("&Debug"));
    emu_menu->addAction(disassembler_act);
*/
}


//void QtMainWindow::disassembler() {}

void QtMainWindow::keyPressEvent(QKeyEvent *event)
{
    // not interested in keys pushed while we aernt running an emu instance
    if(!emu_running)
    {
        event->ignore();
        return;
    }


    // not interested in a held down key
    if(event->isAutoRepeat())
    {
        event->ignore();
        return;
    }

    if(event->type() == QEvent::KeyPress)
    {
        switch(event->key())
        {
            case Qt::Key_A: gb.key_pressed(4); break;
            case Qt::Key_S: gb.key_pressed(5); break;
            case Qt::Key_Return: gb.key_pressed(7); break;
            case Qt::Key_Space: gb.key_pressed(6); break;
            case Qt::Key_Right: gb.key_pressed(0); break;
            case Qt::Key_Left: gb.key_pressed(1); break;
            case Qt::Key_Up: gb.key_pressed(2);break;
            case Qt::Key_Down: gb.key_pressed(3); break;
        }
    }
}

void QtMainWindow::keyReleaseEvent(QKeyEvent *event)
{
    // not interested in keys pushed while we aernt running an emu instance
    if(!emu_running)
    {
        event->ignore();
        return;
    }


    // not interested in a held down key
    if(event->isAutoRepeat())
    {
        event->ignore();
        return;
    }


    if(event->type() == QEvent::KeyRelease)
    {
        switch(event->key())
        {
            case Qt::Key_A: gb.key_released(4); break;
            case Qt::Key_S: gb.key_released(5); break;
            case Qt::Key_Return: gb.key_released(7); break;
            case Qt::Key_Space: gb.key_released(6); break;
            case Qt::Key_Right: gb.key_released(0); break;
            case Qt::Key_Left: gb.key_released(1); break;
            case Qt::Key_Up: gb.key_released(2);break;
            case Qt::Key_Down: gb.key_released(3); break;
        }
    }
}




// open up a file dialog to load a rom
// will need handling for if its allready open we will neglect this for now
void QtMainWindow::open()
{
    std::string file_name = QFileDialog::getOpenFileName(this,tr("open rom"),".","").toStdString();
    
    // we quit out
    if(file_name == "")
    {
        return;
    }


    // if any emulator instance is running kill it
    destroy_emu(); 

    try
    {
        // construct our main gb class
        gb.reset(file_name);

        setMinimumSize(gb.ppu.X,(gb.ppu.Y) + emu_menu->height());
        resize(gb.ppu.X * 2,(gb.ppu.Y * 2) + emu_menu->height());
        // set window name with the rom title
        QString window_tile = QString::fromStdString(fmt::format("destoer-emu: {}",
            file_name.substr(file_name.find_last_of("/\\") + 1)));
        setWindowTitle(window_tile);
    }

    catch(std::exception &ex)
    {
        QMessageBox messageBox;
        messageBox.critical(nullptr,"Error",ex.what());
        messageBox.setFixedSize(500,200);        
        return;
    }

    emu_running = true;
    emu_instance = new EmuInstance(nullptr,&gb,&framebuffer);
    emu_instance->start();
}


void QtMainWindow::destroy_emu()
{
    if(emu_running)
    {
        gb.quit = true;
        emu_instance->wait(); // end the thread
        delete(emu_instance);
        emu_running = false;
        gb.quit = false;
        gb.mem.save_cart_ram();
        setWindowTitle("destoer-emu: no rom");
    }
}
#endif
