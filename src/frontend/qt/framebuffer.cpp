#ifdef FRONTEND_QT
#include "framebuffer.h"
#include <QSurface>

void FrameBuffer::swap_buffer(std::vector<uint32_t> &other)
{
    // have to lock this here as paintevent can be called
    // from our emu thread or our main window
    std::scoped_lock<std::mutex> guard(screen_mutex);
    std::swap(screen,other);
}

void FrameBuffer::init(int x, int y)
{
    X = x;
    Y = y;
    screen.resize(x * y);
    resize(x*2,y*2);

    makeCurrent();

    // setup our texture
    glEnable(GL_TEXTURE_2D); 
    glBindTexture(GL_TEXTURE_2D,context()->defaultFramebufferObject());    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,X,Y,0,GL_RGBA, GL_UNSIGNED_BYTE,screen.data());
    glBindTexture(GL_TEXTURE_2D,0);


    doneCurrent();       
}



void FrameBuffer::paintGL()
{
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,X,Y,GL_RGBA, GL_UNSIGNED_BYTE,screen.data());

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0, 1.0);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0, 1.0);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0, -1.0);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0, -1.0);
    glEnd();
}

void FrameBuffer::redraw(std::vector<uint32_t> &other)
{
    swap_buffer(other);
    update();
}

#endif
