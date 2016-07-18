#include "miniphysics.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMessageBox>
#include <QApplication>

#include "PGE_File_Formats/file_formats.h"

MiniPhysics::MiniPhysics(QWidget* parent):
    QOpenGLWidget(parent)
{
    keyMap[Qt::Key_Left]  = false;
    keyMap[Qt::Key_Right] = false;
    keyMap[Qt::Key_Space] = false;

    LevelData file;
    if( !FileFormats::OpenLevelFile(QApplication::applicationDirPath()+"/test.lvlx", file) )
        QMessageBox::critical(nullptr, "SHIT", file.ERROR_info);

    pl.m_id = obj::SL_Rect;
    pl.m_x = file.players[0].x;
    pl.m_y = file.players[0].y;
    pl.m_dx = pl.m_x;
    pl.m_dy = pl.m_y;
    pl.m_w = 25;
    pl.m_h = 30;

    for(int i=0; i<file.blocks.size(); i++)
    {
        int id = 0;
        LevelBlock& blk = file.blocks[i];
        switch(blk.id)
        {
            case 358: case 357: id = obj::SL_RightBottom;   break;
            case 359: case 360: id = obj::SL_LeftBottom;    break;
            case 363: case 364: id = obj::SL_LeftTop;       break;
            case 362: case 361: id = obj::SL_RightTop;      break;
            default:    id = obj::SL_Rect;                  break;
        }
        obj box(blk.x, blk.y, id);
        box.m_w = blk.w;
        box.m_h = blk.h;
        objs.push_back(box);
    }
    /*
    objs.push_back(obj(7,   6, obj::SL_RightBottom));
    objs.push_back(obj(8,   5, obj::SL_RightBottom));
    objs.push_back(obj(1,   5, obj::SL_LeftBottom));
    objs.push_back(obj(2,   6, obj::SL_LeftBottom));
    objs.push_back(obj(1,   2, obj::SL_LeftTop));
    objs.push_back(obj(2,   1, obj::SL_LeftTop));
    objs.push_back(obj(7,   1, obj::SL_RightTop));
    objs.push_back(obj(8,   2, obj::SL_RightTop));
    objs.push_back(obj(2,   0, obj::SL_Rect));
    objs.push_back(obj(3,   0, obj::SL_Rect));
    objs.push_back(obj(4,   0, obj::SL_Rect));
    objs.push_back(obj(6,   0, obj::SL_Rect));
    objs.push_back(obj(2,   7, obj::SL_Rect));
    objs.push_back(obj(3,   7, obj::SL_Rect));
    objs.push_back(obj(4,   7, obj::SL_Rect));
    objs.push_back(obj(6,   7, obj::SL_Rect));
    objs.push_back(obj(0,   2, obj::SL_Rect));
    objs.push_back(obj(0,   3, obj::SL_Rect));
    objs.push_back(obj(0,   4, obj::SL_Rect));
    objs.push_back(obj(0,   5, obj::SL_Rect));
    objs.push_back(obj(9,   2, obj::SL_Rect));
    objs.push_back(obj(10,  3, obj::SL_Rect));
    objs.push_back(obj(10,  4, obj::SL_Rect));
    objs.push_back(obj(9,   5, obj::SL_Rect));
    objs.push_back(obj(1,   1, obj::SL_Rect));
    objs.push_back(obj(1,   6, obj::SL_Rect));
    objs.push_back(obj(8,   1, obj::SL_Rect));
    objs.push_back(obj(8,   6, obj::SL_Rect));

    objs.push_back(obj(4, 4, obj::SL_Rect));
    //objs.push_back(obj(2, 5, obj::SL_RIGHT_TOP));
    //objs.push_back(obj(3, 4, obj::SL_RIGHT_TOP));
    */
    {
        obj &brick = objs.last();
        brick.m_velX = 0.8;
    }

    connect(&looper, &QTimer::timeout, this, &MiniPhysics::loop);
    looper.start(15);
}



template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

void MiniPhysics::iterateStep()
{
    bool lk, rk;
    {
        obj &brick = objs.last();
        brick.m_dx = brick.m_x;
        brick.m_x += brick.m_velX;
        //brick.y += brick.spy;
        if(brick.m_x > 8*32 || brick.m_x < 2*32)
            brick.m_velX *= -1.0;
    }

    { //With pl

/*
 * With pl
    lk = GetAsyncKeyState(vbKeyLeft) < 0
    rk = GetAsyncKeyState(vbKeyRight) < 0
    If lk Xor rk Then
        If lk And .spx > -6 Then .spx = .spx - 0.4
        If rk And .spx < 6 Then .spx = .spx + 0.4
    ElseIf lk = False And rk = False Then
        If Abs(.spx) > 0.4 Then .spx = .spx - Sgn(.spx) * 0.4 Else .spx = 0
    End If
    If .spy < 8 And .st = False Then .spy = .spy + 0.4
    If .st And GetAsyncKeyState(vbKeyUp) < 0 Then .spy = -10 '8
    .st = False
    .dx = .x
    .dy = .y
    .x = .x + .spx
    .y = .y + .spy
End With
*/
        lk = keyMap[Qt::Key_Left];
        rk = keyMap[Qt::Key_Right];
        if(lk ^ rk)
        {
            if(lk && pl.m_velX > -6)
                pl.m_velX -= 0.4;
            if(rk && pl.m_velX < 6)
                pl.m_velX += 0.4;
        }
        else if( !lk && !rk) {
            if(fabs(pl.m_velX) > 0.4) {
                pl.m_velX -= sgn(pl.m_velX)*0.4;
            } else {
                pl.m_velX = 0;
            }
        }
        if(pl.m_velY < 8 && !pl.m_st)
            pl.m_velY += 0.4;
        if(pl.m_st && keyMap[Qt::Key_Space])
            pl.m_velY = -10; //'8
        pl.m_st = false;
        pl.m_dx = pl.m_x;
        pl.m_dy = pl.m_y;
        pl.m_x += pl.m_velX;
        pl.m_y += pl.m_velY;
    }

}

static inline bool pt(double x1, double y1, double w1, double h1,
        double x2, double y2, double w2, double h2)
{
    if( (y1 + h1 > y2) && (y2 + h2 > y1) )
        return ((x1 + w1 > x2) && (x2 + w2 > x1));
    return false;
}

void MiniPhysics::processCollisions()
{
    double k = 0;
    int i=0, contactAt=obj::Contact_None, /*ck=0,*/ tm=0, td=0;
    bool lk, rk;
    tm = -1;
    td = 0;

    //Return player to top back on fall down
    //if(pl.y > 8*32)
    //    pl.y = 64;

    for(i=0; i<objs.size(); i++)
    {
        contactAt = obj::Contact_None;
        if( (pl.m_x + pl.m_w > objs[i].m_x) && (objs[i].m_x + objs[i].m_w > pl.m_x) )
        {
            if(pl.m_y + pl.m_h == objs[i].m_y)
                goto tipa;
        }

    tipc:
        if( pt(pl.m_x, pl.m_y, pl.m_w, pl.m_h, objs[i].m_x, objs[i].m_y, objs[i].m_w, objs[i].m_h))
        {
            lk = pt(pl.m_x, pl.m_dy, pl.m_w, pl.m_h, objs[i].m_x, objs[i].m_y - objs[i].m_velY, objs[i].m_w, objs[i].m_h);
            rk = pt(pl.m_dx, pl.m_y, pl.m_w, pl.m_h, objs[i].m_x - objs[i].m_velX, objs[i].m_y, objs[i].m_w, objs[i].m_h);
            if( lk ^ rk )
            {
                if(!lk)
                {
    tipa:
                    if( pl.m_y + pl.m_h / 2 < objs[i].m_y + objs[i].m_h / 2 )
                    {
                        //'top
                        if( (objs[i].m_id == obj::SL_Rect) ||
                            (objs[i].m_id == obj::SL_LeftTop) ||
                            (objs[i].m_id == obj::SL_RightTop))
                        {
    tipd:
                            pl.m_y = objs[i].m_y - pl.m_h;
                            pl.m_velY = objs[i].m_velY;
                            pl.m_st = true;
                            contactAt = obj::Contact_Top;
                        }
                    } else {
                        //'bottom
                        if( (objs[i].m_id == obj::SL_Rect) ||
                            (objs[i].m_id == obj::SL_LeftBottom) ||
                            (objs[i].m_id == obj::SL_RightBottom) )
                        {
    tipe:
                            pl.m_y = objs[i].m_y + objs[i].m_h;
                            pl.m_velY = objs[i].m_velY;
                            contactAt = obj::Contact_Bottom;
                        }
                    }
                } else {
    tipb:
                    if( pl.m_x + pl.m_w/2 < objs[i].m_x + objs[i].m_w/2 )
                    {
                        //'left
                        if( (objs[i].m_id == obj::SL_Rect) ||
                            (objs[i].m_id == obj::SL_LeftBottom)||
                            (objs[i].m_id == obj::SL_LeftTop) )
                        {
                            pl.m_x = objs[i].m_x - pl.m_w;
                            pl.m_velX = objs[i].m_velX;
                            contactAt = obj::Contact_Left;
                        }
                    } else {
                        //'right
                        if( (objs[i].m_id == obj::SL_Rect) ||
                            (objs[i].m_id == obj::SL_RightBottom) ||
                            (objs[i].m_id == obj::SL_RightTop) )
                        {
                            pl.m_x = objs[i].m_x + objs[i].m_w;
                            pl.m_velX = objs[i].m_velX;
                            contactAt = obj::Contact_Right;
                        }
                    }
                }
                tm = -2;
    TipF:
                if(contactAt == obj::Contact_None)
                {
                    k = objs[i].m_h / objs[i].m_w;
                    switch(objs[i].m_id)
                    {
                    case obj::SL_LeftBottom:
                        if(pl.m_x <= objs[i].m_x)
                        {
                            if( pl.m_y + pl.m_h > objs[i].m_y ) goto tipd;
                        }
                        else if( pl.m_y + pl.m_h > objs[i].m_y + (pl.m_x - objs[i].m_x) * k )
                        {
                            pl.m_y = objs[i].m_y + (pl.m_x - objs[i].m_x) * k - pl.m_h;
                            pl.m_velY = objs[i].m_velY;
                            if( pl.m_velX > 0)
                                pl.m_velY = pl.m_velX * k;
                            pl.m_st = true;
                            contactAt = obj::Contact_Top;
                        }
                        break;
                    case obj::SL_RightBottom:
                        if( pl.m_x + pl.m_w >= objs[i].m_x + objs[i].m_w)
                        {
                            if(pl.m_y + pl.m_h > objs[i].m_y) goto tipd;
                        }
                        else if(pl.m_y + pl.m_h > objs[i].m_y + (objs[i].m_x + objs[i].m_w - pl.m_x - pl.m_w) * k)
                        {
                            pl.m_y = objs[i].m_y + (objs[i].m_x + objs[i].m_w - pl.m_x - pl.m_w) * k - pl.m_h;
                            pl.m_velY = objs[i].m_velY;
                            if(pl.m_velX < 0)
                                pl.m_velY = -pl.m_velX * k;
                            pl.m_st = true;
                            contactAt = obj::Contact_Top;
                        }
                        break;
                    case obj::SL_LeftTop:
                        if(pl.m_x <= objs[i].m_x)
                        {
                            if(pl.m_y < objs[i].m_y + objs[i].m_h) goto tipe;
                        }
                        else if(pl.m_y < objs[i].m_y + objs[i].m_h - (pl.m_x - objs[i].m_x) * k)
                        {
                            pl.m_y = objs[i].m_y + objs[i].m_h - (pl.m_x - objs[i].m_x) * k;
                            pl.m_velY = objs[i].m_velY;
                            contactAt = obj::Contact_Bottom;
                        }
                        break;
                    case obj::SL_RightTop:
                        if(pl.m_x + pl.m_w >= objs[i].m_x + objs[i].m_w)
                        {
                            if(pl.m_y < objs[i].m_y + objs[i].m_h) goto tipe;
                        }
                        else if(pl.m_y < objs[i].m_y + objs[i].m_h - (objs[i].m_x + objs[i].m_w - pl.m_x - pl.m_w) * k)
                        {
                            pl.m_y = objs[i].m_y + objs[i].m_h - (objs[i].m_x + objs[i].m_w - pl.m_x - pl.m_w) * k;
                            pl.m_velY = objs[i].m_velY;
                            contactAt = obj::Contact_Bottom;
                        }
                        break;
                    default:
                        break;
                    }
                }
            } else {
                if( objs[i].m_id != obj::SL_Rect )
                    goto TipF;
                if( !lk && !rk )
                {
                    if(tm == i)
                    {
                        if( fabs(pl.m_velY) > fabs(pl.m_velX) )
                            goto tipa;
                        else
                            goto tipb;
                    } else {
                        if(tm != -2)
                            tm = i;
                    }
                }
            }
        }
        if(td == 1)
        {
            tm = -1;
            break;
        }
    }
    if(tm >= 0)
    {
        i = tm;
        td = 1;
        goto tipc;
    }
}

void MiniPhysics::loop()
{
    iterateStep();
    processCollisions();
    repaint();
}

void MiniPhysics::keyPressEvent(QKeyEvent *event)
{
    keyMap[event->key()] = true;
}

void MiniPhysics::keyReleaseEvent(QKeyEvent *event)
{
    keyMap[event->key()] = false;
}

void MiniPhysics::initializeGL()
{
    f = QOpenGLContext::currentContext()->functions();
    f->glClearColor(1.0, 1.0, 1.0, 1.0);
}

void MiniPhysics::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(this->rect(), Qt::white);
    p.setBrush(Qt::gray);
    p.setPen(QColor(Qt::black));
    for(int i=0; i<objs.size(); i++)
        objs[i].paint(p);
    pl.paint(p);
}
