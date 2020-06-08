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

    setCentralWidget(&framebuffer);
    setMinimumSize(gb_instance.X,(gb_instance.Y) + menuBar()->height());
    resize(gb_instance.X * 2,(gb_instance.Y * 2) + menuBar()->height());  
}

// if any of our emulator threads are still running terminate them gracefully
QtMainWindow::~QtMainWindow()
{
    stop_emu();
}


// create our actions for our menu
void QtMainWindow::create_actions()
{
    // FILE
    open_act = new QAction(tr("&open rom"), this);
    open_act->setShortcuts(QKeySequence::New);
    open_act->setStatusTip(tr("open a rom file"));
    connect(open_act,&QAction::triggered, this, &QtMainWindow::open);

    load_state_act = new QAction(tr("&load state"), this);
    load_state_act->setShortcuts(QKeySequence::New);
    load_state_act->setStatusTip(tr("load a save state"));
    connect(load_state_act,&QAction::triggered, this, &QtMainWindow::load_state);

    save_state_act = new QAction(tr("&save state"), this);
    save_state_act->setShortcuts(QKeySequence::New);
    save_state_act->setStatusTip(tr("save a save state"));
    connect(save_state_act,&QAction::triggered, this, &QtMainWindow::save_state);


    // emulator
    pause_emulator_act = new QAction(tr("&pause"), this);
    pause_emulator_act->setShortcuts(QKeySequence::New);
    pause_emulator_act->setStatusTip(tr("pause the emulator"));
    connect(pause_emulator_act,&QAction::triggered, this, &QtMainWindow::stop_emu);

    continue_emulator_act = new QAction(tr("&continue"), this);
    continue_emulator_act->setShortcuts(QKeySequence::New);
    continue_emulator_act->setStatusTip(tr("resume emulation"));
    connect(continue_emulator_act,&QAction::triggered, this, &QtMainWindow::start_emu);

    disable_audio_act = new QAction(tr("&disable audio"), this);
    disable_audio_act->setShortcuts(QKeySequence::New);
    disable_audio_act->setStatusTip(tr("turn off audio playback"));
    connect(disable_audio_act,&QAction::triggered, this, &QtMainWindow::disable_audio);

    enable_audio_act = new QAction(tr("&enable audio"), this);
    enable_audio_act->setShortcuts(QKeySequence::New);
    enable_audio_act->setStatusTip(tr("turn on audio playback"));
    connect(enable_audio_act,&QAction::triggered, this, &QtMainWindow::enable_audio);


}

void QtMainWindow::enable_audio()
{
    switch(running_type)
    {
        case emu_type::gameboy:
        {
            gb_instance.enable_audio();
            break;
        }

        case emu_type::gba:
        {
            break;
        }

        case emu_type::none:
        {
            break;
        }

    }    
}


void QtMainWindow::disable_audio()
{
    switch(running_type)
    {
        case emu_type::gameboy:
        {
            gb_instance.disable_audio();
            break;
        }

        case emu_type::gba:
        {
            break;
        }

        case emu_type::none:
        {
            break;
        }

    }    
}

// create our top bar menu
void QtMainWindow::create_menus()
{
    file_menu = menuBar()->addMenu(tr("&File"));
    file_menu->addAction(open_act);
    file_menu->addAction(load_state_act);
    file_menu->addAction(save_state_act);

    emu_menu = menuBar()->addMenu(tr("&Emulator"));
    emu_menu->addAction(pause_emulator_act);
    emu_menu->addAction(continue_emulator_act);
    emu_menu->addAction(disable_audio_act);
    emu_menu->addAction(enable_audio_act);
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
            case emu_type::none: break;
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
            case emu_type::none: break;
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
    if(!std::filesystem::is_regular_file(file_name))
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
                break;
            }

            case emu_type::gba:
            {
                gba_instance.reset(file_name);
                setMinimumSize(gba_instance.X,(gba_instance.Y) + menuBar()->height());
                resize(gba_instance.X * 2,(gba_instance.Y * 2) + menuBar()->height());                               
                break;
            }

            case emu_type::none:
            {
                return;
            }

        }

        // set window name with the rom title
        QString window_tile = QString::fromStdString(fmt::format("destoer-emu: {}",
            file_name.substr(file_name.find_last_of("/\\") + 1)));
        setWindowTitle(window_tile);
        start_emu();
    }

    catch(std::exception &ex)
    {
        QMessageBox messageBox;
        messageBox.critical(nullptr,"Error",ex.what());
        messageBox.setFixedSize(500,200);
        setWindowTitle("destoer-emu: no rom");        
        return;
    }
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

                case emu_type::none:
                {
                    return;
                }
            }
            start_emu();
        }

        catch(std::exception &ex)
        {
            QMessageBox messageBox;
            messageBox.critical(nullptr,"Error",ex.what());
            messageBox.setFixedSize(500,200);
        }
    }
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

                case emu_type::none:
                {
                    return;
                }

            }
            start_emu();
        }

        catch(std::exception &ex)
        {
            QMessageBox messageBox;
            messageBox.critical(nullptr,"Error",ex.what());
            messageBox.setFixedSize(500,200);
        }
    }
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

            case emu_type::none:
            {
                break;
            }
        }
    }
}

void QtMainWindow::start_emu()
{
    
    switch(running_type)
    {
        case emu_type::gameboy:
        {
            framebuffer.init(gb_instance.X,gb_instance.Y); 
            gb_instance.init(&framebuffer);  
            gb_instance.start();
            emu_running = true;
            break;
        }

        case emu_type::gba:
        {
            framebuffer.init(gba_instance.X,gba_instance.Y); 
            gba_instance.init(&framebuffer);  
            gba_instance.start();
            emu_running = true;            
            break;
        }

        case emu_type::none:
        {
            break;
        }
    }
}
#endif
