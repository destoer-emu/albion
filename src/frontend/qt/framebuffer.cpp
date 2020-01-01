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
    ready = true;
    X = x;
    Y = y;
    screen.resize(x * y);
}

void FrameBuffer::paintEvent(QPaintEvent*)
{
    if(!ready)
    {
        return;
    }

    std::scoped_lock<std::mutex> guard(screen_mutex);

    QPainter painter(this);
    QImage image((uchar*)screen.data(), X, Y, QImage::Format_RGBA8888);
    painter.drawImage(0,0,image.scaled(width(),height()));
}

void FrameBuffer::redraw(std::vector<uint32_t> &other)
{
    swap_buffer(other);
    update();
}

#endif
