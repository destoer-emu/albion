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
        switch(running_type)
        {
            case emu_type::gameboy: gb_instance.key_pressed(event->key()); break;
            case emu_type::gba: gba_instance.key_pressed(event->key()); break;
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
        switch(running_type)
        {
            case emu_type::gameboy: gb_instance.key_released(event->key()); break;
            case emu_type::gba: gba_instance.key_released(event->key()); break;
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
        start_emu();
        return;
    }

    try
    {

        running_type = get_emulator_type(file_name);

        switch(running_type)
        {
            case emu_type::gameboy:
            {
                gb_instance.reset(file_name);
                setMinimumSize(gb_instance.X,(gb_instance.Y) + menuBar()->height());
                resize(gb_instance.X * 2,(gb_instance.Y * 2) + menuBar()->height());    
                framebuffer.init(gb_instance.X,gb_instance.Y);
                gb_instance.init(&framebuffer);                            
                break;
            }

            case emu_type::gba:
            {
                gba_instance.reset(file_name);
                setMinimumSize(gba_instance.X,(gba_instance.Y) + menuBar()->height());
                resize(gba_instance.X * 2,(gba_instance.Y * 2) + menuBar()->height());
                framebuffer.init(gba_instance.X,gba_instance.Y); 
                gba_instance.init(&framebuffer);                                 
                break;
            }
        }

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
        try
        {
            switch(running_type)
            {        
                case emu_type::gameboy:
                {
                    gb_instance.load_state(file_name);
                    break;
                }

                case emu_type::gba:
                {
                    gba_instance.load_state(file_name);
                    break;
                }
            }
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
            switch(running_type)
            {        
                case emu_type::gameboy:
                {
                    // save a state just before incase they somehow
                    gb_instance.save_state(file_name);
                    break;
                }

                case emu_type::gba:
                {
                    gba_instance.save_state(file_name);
                    break;
                }
            }
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
        switch(running_type)
        {
            case emu_type::gameboy:
            {
                gb_instance.stop();
                gb_instance.wait(); // end the thread
                emu_running = false;
                gb_instance.save_backup_ram();
                break;
            }

            case emu_type::gba:
            {
                gba_instance.stop();
                gba_instance.wait(); // end the thread
                emu_running = false;
                gba_instance.save_backup_ram();                
                break;
            }
        }

    }
}

void QtMainWindow::start_emu()
{
    emu_running = true;
    switch(running_type)
    {
        case emu_type::gameboy:
        {
            gb_instance.start();
            break;
        }

        case emu_type::gba:
        {
            gba_instance.start();            
            break;
        }
    }
}
#endif
