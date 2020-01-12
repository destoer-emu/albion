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
    stop_emu();
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

    load_state_act = new QAction(tr("&load state"), this);
    load_state_act->setShortcuts(QKeySequence::New);
    load_state_act->setStatusTip(tr("load a save state"));
    connect(load_state_act,&QAction::triggered, this, &QtMainWindow::load_state);

    save_state_act = new QAction(tr("&save state"), this);
    save_state_act->setShortcuts(QKeySequence::New);
    save_state_act->setStatusTip(tr("save a save state"));
    connect(save_state_act,&QAction::triggered, this, &QtMainWindow::save_state);

    alignment_group = new QActionGroup(this);
    alignment_group->addAction(open_act);
    alignment_group->addAction(load_state_act);
    alignment_group->addAction(save_state_act);
}


// create our top bar menu
void QtMainWindow::create_menus()
{
    emu_menu = menuBar()->addMenu(tr("&File"));
    emu_menu->addAction(open_act);
    emu_menu->addAction(load_state_act);
    emu_menu->addAction(save_state_act);
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

            case Qt::Key_Plus:
            {
                gb.apu.stop_audio();
                gb.throttle_emu = false;
                break;
            }

            case Qt::Key_Minus:
            {
                gb.apu.start_audio();
                gb.throttle_emu = true;
                break;
            }

        }
    }
}




// open up a file dialog to load a rom
// will need handling for if its allready open we will neglect this for now
void QtMainWindow::open()
{
    // if any emulator instance is running kill it
    stop_emu(); 

    std::string file_name = QFileDialog::getOpenFileName(this,tr("open rom"),".","").toStdString();
    
    // we quit out
    if(file_name == "" || !std::filesystem::is_regular_file(file_name))
    {
        return;
    }

    try
    {
        // construct our main gb class
        gb.reset(file_name);

        setMinimumSize(gb.ppu.X,(gb.ppu.Y) + menuBar()->height());
        resize(gb.ppu.X * 2,(gb.ppu.Y * 2) + menuBar()->height());
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
        setWindowTitle("destoer-emu: no rom");        
        return;
    }
    start_emu();
}




void QtMainWindow::load_state()
{
    if(!emu_running)
    { 
        return;
    }

    stop_emu();
    std::string file_name = QFileDialog::getOpenFileName(this,tr("load state"),"").toStdString();
    
    // we didnt quit out
    if(file_name != "" && std::filesystem::is_regular_file(file_name))
    {
        std::string backup = std::filesystem::current_path().string() + "/backup.state";

        // save a state just before incase they somehow
        gb.save_state(backup);

        try
        {
            gb.load_state(file_name);
        }

        catch(std::exception &ex)
        {
            QMessageBox messageBox;
            messageBox.critical(nullptr,"Error",ex.what());
            messageBox.setFixedSize(500,200);
            gb.load_state(backup);
        }
    }

    
    start_emu();
}


void QtMainWindow::save_state()
{
    if(!emu_running)
    { 
        return;
    }

    stop_emu();
    std::string file_name = QFileDialog::getSaveFileName(this,tr("load state"),"").toStdString();
    
    // we didnt quit out
    if(file_name != "")
    {
        try
        {
            gb.save_state(file_name);
        }

        catch(std::exception &ex)
        {
            QMessageBox messageBox;
            messageBox.critical(nullptr,"Error",ex.what());
            messageBox.setFixedSize(500,200);
        }
    }

    start_emu();
}

void QtMainWindow::stop_emu()
{
    if(emu_running)
    {
        gb.quit = true;
        emu_instance->wait(); // end the thread
        delete(emu_instance);
        emu_running = false;
        gb.quit = false;
        gb.mem.save_cart_ram();
    }
}

void QtMainWindow::start_emu()
{
    emu_running = true;
    emu_instance = new EmuInstance(nullptr,&gb,&framebuffer);
    emu_instance->start();
}
#endif
