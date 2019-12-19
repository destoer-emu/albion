#ifdef FRONTEND_QT
#include "framebuffer.h"
#include <QSurface>


void FrameBuffer::init(std::vector<uint32_t> *buf,int x, int y)
{
    if(buf == nullptr)
    {
        puts("nullptr for screen when constructing framebuffer");
        exit(1);
    }
    screen = buf;
    width = x;
    height = y;
    ready = true;
}


void FrameBuffer::initializeGL()
{
    // clear the screen
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // gl texture
    glEnable(GL_TEXTURE_2D); 

}

// called when we need to draw this
void FrameBuffer::paintGL()
{
    // not ready to draw yet
    if(!ready || screen == nullptr)
    {
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    // setup our texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // create the texture supposedly theres a faster way to do this with subimage but i cant get
    // it working?
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,screen->data());

    // render the texture
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0, 1.0);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0, 1.0);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0, -1.0);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0, -1.0);
    glEnd();
}

//why the heck is this so slow
void FrameBuffer::redraw()
{
    update();
}

#endif
