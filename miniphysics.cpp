#include "miniphysics.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMessageBox>
#include <QApplication>
#include <QFile>

#include "PGE_File_Formats/file_formats.h"

MiniPhysics::MiniPhysics(QWidget* parent):
    QOpenGLWidget(parent),
    m_font("Courier New", 10, 2)
{
    keyMap[Qt::Key_Left]  = false;
    keyMap[Qt::Key_Right] = false;
    keyMap[Qt::Key_Space] = false;

    LevelData file;
    {
        QFile physFile(":/test.lvlx");
        physFile.open(QIODevice::ReadOnly);
        QString rawdata = QString::fromUtf8(physFile.readAll());
        if( !FileFormats::ReadExtendedLvlFileRaw(rawdata, ".", file) )
        {
            QMessageBox::critical(nullptr, "SHIT", file.meta.ERROR_info);
            return;
        }
    }

    pl.m_id = obj::SL_Rect;
    pl.m_x = file.players[0].x;
    pl.m_y = file.players[0].y;
    pl.m_oldx = pl.m_x;
    pl.m_oldy = pl.m_y;
    pl.m_w = 24;
    pl.m_h = 30;
    pl.m_drawSpeed = true;
    //Allow hole-running
    pl.m_allowHoleRuning = true;

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
        if(blk.id == 159)
            movingBlock.push_back(&objs.last());
    }


    {
        int lastID = movingBlock.size()-1;
        obj &brick1 = *movingBlock[lastID];
        brick1.m_velX = 0.8;
        //brick1.drawSpeed = true;
        obj &brick2 = *movingBlock[lastID-1];
        brick2.m_velY = 1.0;
        obj &brick3 = *movingBlock[lastID-2];
        brick3.m_velX = 0.8;
        obj &brick4 = *movingBlock[lastID-3];
        brick4.m_velX = 1.6;
    }

    connect(&looper, &QTimer::timeout, this, &MiniPhysics::loop);
    looper.setTimerType(Qt::PreciseTimer);
    looper.start(25);
}



template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

void MiniPhysics::iterateStep()
{
    bool lk=false, rk=false;
    {
        static double brick1Passed = 0.0;
        static double brick2Passed = 0.0;
        static double brick3Passed = 0.0;
        static double brick4Passed = 0.0;

        int lastID = movingBlock.size()-1;
        obj &brick1 = *movingBlock[lastID];

        brick1.m_oldx = brick1.m_x;
        brick1.m_oldy = brick1.m_y;
        brick1.m_x   += brick1.m_velX;
        brick1.m_y   += brick1.m_velY;

        brick1.m_velX = sin(brick1Passed)*2.0;
        brick1.m_velY = cos(brick1Passed)*2.0;
        brick1Passed += 0.07;

        obj &brick2 = *movingBlock[lastID-1];
        brick2.m_oldy = brick2.m_y;
        brick2.m_y   += brick2.m_velY;
        brick2Passed += brick2.m_velY;
        //brick.y += brick.spy;
        if(brick2Passed > 8.0*32.0 || brick2Passed < 0.0)
            brick2.m_velY *= -1.0;

        obj &brick3 = *movingBlock[lastID-2];
        brick3.m_oldx = brick3.m_x;
        brick3.m_x   += brick3.m_velX;
        brick3Passed += brick3.m_velX;
        //brick.y += brick.spy;
        if(brick3Passed > 9.0*32.0 || brick3Passed < 0.0)
            brick3.m_velX *= -1.0;

        obj &brick4 = *movingBlock[lastID-3];
        brick4.m_oldx = brick4.m_x;
        brick4.m_x   += brick4.m_velX;
        brick4Passed += brick4.m_velX;
        //brick.y += brick.spy;
        if(brick4Passed > 10.0*32.0 || brick4Passed < 0.0)
            brick4.m_velX *= -1.0;
    }

    { //With pl
        lk = keyMap[Qt::Key_Left];
        rk = keyMap[Qt::Key_Right];
        double Xmod = 0;
        if(lk ^ rk)
        {
            if(lk && pl.m_velX_source > -6)
                Xmod -= 0.4;
            if(rk && pl.m_velX_source < 6)
                Xmod += 0.4;
        }
        else if( !lk && !rk)
        {
            if(fabs(pl.m_velX_source) > 0.4)
            {
                Xmod -= sgn(pl.m_velX_source)*0.4;
            }
            else
            {
                //pl.m_velX_source = 0;
                Xmod = -pl.m_velX_source;
            }
        }
        pl.m_velX        += Xmod;
        pl.m_velX_source += Xmod;

        if(!pl.m_stand)
            pl.m_velX = pl.m_velX_source;

        /*
         * For NPC's: ignore "stand" flag is "on-cliff" is true.
         * to allow catch floor holes Y velocity must not be zero!
         * However, non-zero velocity may cause corner stumbling on slopes
         *
         * For playables: need to allow runnung over floor holes
         * even width is smaller than hole
         */
        if( (pl.m_velY < 8) && (!pl.m_stand || pl.m_standOnYMovable || (!pl.m_allowHoleRuning && pl.m_cliff) ) )
            pl.m_velY += 0.4;

        if(pl.m_stand && keyMap[Qt::Key_Space] && !pl.m_jumpPressed)
        {
            pl.m_velY = -10; //'8
            pl.m_jumpPressed = true;
        }
        if(pl.m_jumpPressed && !keyMap[Qt::Key_Space])
            pl.m_jumpPressed = false;

        pl.m_stand      = false;
        pl.m_standOnYMovable = false;
        pl.m_crushedOld = pl.m_crushed;
        pl.m_crushed    = false;
        pl.m_cliff      = false;

        pl.m_oldx = pl.m_x;
        pl.m_oldy = pl.m_y;
        pl.m_x += pl.m_velX;
        pl.m_y += pl.m_velY;

        if(pl.m_onSlopeFloor)
            pl.m_y += pl.m_onSlopeYAdd;

        pl.m_onSlopeFloorOld = pl.m_onSlopeFloor;
        pl.m_onSlopeFloor = false;
        pl.m_onSlopeCeilingOld = pl.m_onSlopeCeiling;
        pl.m_onSlopeCeiling = false;
    }

}

static inline bool pt(double x1, double y1, double w1, double h1,
        double x2, double y2, double w2, double h2)
{
    if( (y1 + h1 > y2) && (y2 + h2 > y1) )
        return ((x1 + w1 > x2) && (x2 + w2 > x1));
    return false;
}

static inline bool recttouch(double X,  double Y,   double w,   double h,
                             double x2, double y2,  double w2,  double h2)
{
    return ( (X + w > x2) && (x2 + w2 > X) && (Y + h > y2) && (y2 + h2 > Y));
}

void MiniPhysics::processCollisions()
{
    double k = 0;
    int i=0, contactAt=obj::Contact_None, /*ck=0,*/ tm=0, td=0;
    bool colH=false, colV=false;
    tm = -1;
    td = 0;

    //Return player to top back on fall down
    if(pl.m_y > this->height())
        pl.m_y = 64;
    bool doHit = false;
    bool doCliffCheck = false;
    QVector<obj*> l_clifCheck;
    QVector<obj*> l_toBump;
    QVector<obj*> l_slopeFloor;
    QVector<obj*> l_slopeCeiling;
    obj* standingOn = nullptr;
    obj* ceilingOn  = nullptr;
    double speedNum = 0.0;
    double speedSum = 0.0;

    for(i=0; i<objs.size(); i++)
    {
        objs[i].m_bumped = false;
        contactAt = obj::Contact_None;
        /* ********************Collect blocks to hit************************ */
        if( recttouch(pl.m_x, pl.m_y-1, pl.m_w, pl.m_h+2, objs[i].m_x, objs[i].m_y, objs[i].m_w, objs[i].m_h) )
        {
            if(pl.m_y + pl.m_h/2.0 < objs[i].m_y+objs[i].m_h/2.0)
            {
                l_clifCheck.push_back(&objs[i]);
            } else {
                l_toBump.push_back(&objs[i]);
            }
        }
        else if(pl.m_onSlopeFloorOld) //Needed to detect cliff on the floor
        {
            objRect &r1 = pl.m_onSlopeFloorRect;
            objRect  r2 = objs[i].rect();
            if( (pl.m_onSlopeFloorShape == obj::SL_LeftBottom) && (pl.m_velX >= 0.0) )
            {
                if( recttouch(pl.m_x + pl.m_w, pl.centerY(), pl.m_w, pl.m_h, objs[i].m_x, objs[i].m_y, objs[i].m_w, objs[i].m_h)
                    &&
                      //is touching corners
                      ( ( (r1.x+r1.w) >= (r2.x-1.0) ) &&
                        ( (r1.x+r1.w) <= (r2.x+1.0) ) &&
                        ( (r1.y+r1.h) >= (r2.y-1.0) ) &&
                        ( (r1.y+r1.h) <= (r2.y+1.0) ) ) )
                    l_clifCheck.push_back(&objs[i]);
            }
            else
            if( (pl.m_onSlopeFloorShape == obj::SL_RightBottom) && (pl.m_velX <= 0.0) )
            {
                if( recttouch(pl.m_x - 16.0, pl.m_y + 16.0, pl.m_w, pl.m_h+2, objs[i].m_x, objs[i].m_y, objs[i].m_w, objs[i].m_h)
                        &&
                          //is touching corners
                          ( ( (r1.x) >= (r2.x+r2.w-1.0) ) &&
                            ( (r1.x) <= (r2.x+r2.w+1.0) ) &&
                            ( (r1.y+r1.h) >= (r2.y-1.0) ) &&
                            ( (r1.y+r1.h) <= (r2.y+1.0) ) ) )
                    l_clifCheck.push_back(&objs[i]);
            }
        }
        /* ********************Collect blocks to hit************************ */

        if( (pl.m_x + pl.m_w > objs[i].m_x) && (objs[i].m_x + objs[i].m_w > pl.m_x) )
        {
            if(pl.m_y + pl.m_h == objs[i].m_y)
                goto tipRectV;
        }

    tipRectShape://Recheck rectangular collision
        if( pt(pl.m_x, pl.m_y, pl.m_w, pl.m_h, objs[i].m_x, objs[i].m_y, objs[i].m_w, objs[i].m_h))
        {
            colH = pt(pl.m_x,     pl.m_oldy,  pl.m_w, pl.m_h,     objs[i].m_x,    objs[i].m_oldy, objs[i].m_w, objs[i].m_h);
            colV = pt(pl.m_oldx,  pl.m_y,     pl.m_w, pl.m_h,     objs[i].m_oldx, objs[i].m_y,    objs[i].m_w, objs[i].m_h);
            if( colH ^ colV )
            {
                if(!colH)
                {
                tipRectV://Check vertical sides colllisions
                    if( pl.centerY() < objs[i].centerY() )
                    {
                        //'top
                        if( (objs[i].m_id == obj::SL_Rect) ||
                            (objs[i].m_id == obj::SL_LeftTop) ||
                            (objs[i].m_id == obj::SL_RightTop))
                        {
                            if(pl.m_onSlopeFloor || pl.m_onSlopeFloorOld)
                            {
                                if(pl.left() >= objs[i].right()-2.0 )
                                    goto tipRectT_Skip;
                                if(pl.right() <= objs[i].left()+2.0 )
                                    goto tipRectT_Skip;
                            }
                    tipRectT://Impacted at top
                            pl.m_y = objs[i].m_y - pl.m_h;
                            pl.m_velY   = objs[i].m_velY;
                            pl.m_stand  = true;
                            pl.m_standOnYMovable = (objs[i].m_velY != 0.0);
                            pl.m_velX   = pl.m_velX_source + objs[i].m_velX;
                            speedSum += objs[i].m_velX;
                            speedNum += 1.0;
                            contactAt = obj::Contact_Top;
                            doCliffCheck = true;
                            standingOn = &objs[i];
                    tipRectT_Skip:;
                        }
                    } else {
                        //'bottom
                        if( (objs[i].m_id == obj::SL_Rect) ||
                            (objs[i].m_id == obj::SL_LeftBottom) ||
                            (objs[i].m_id == obj::SL_RightBottom) )
                        {
                            if(pl.m_onSlopeCeiling || pl.m_onSlopeCeilingOld)
                            {
                                if(pl.left() >= objs[i].right()-2.0 )
                                    goto tipRectB_Skip;
                                if(pl.right() <= objs[i].left()+2.0 )
                                    goto tipRectB_Skip;
                            }
                    tipRectB://Impacted at bottom
                            pl.m_y = objs[i].m_y + objs[i].m_h;
                            pl.m_velY = objs[i].m_velY;
                            contactAt = obj::Contact_Bottom;
                            doHit = true;
                            ceilingOn = &objs[i];
                    tipRectB_Skip:;
                        }
                    }
                } else {
                tipRectH://Check horisontal sides collision
                    if( pl.centerX() < objs[i].centerX() )
                    {
                        //'left
                        if( (objs[i].m_id == obj::SL_Rect) ||
                            (objs[i].m_id == obj::SL_LeftBottom)||
                            (objs[i].m_id == obj::SL_LeftTop) )
                        {
                    tipRectL://Impacted at left
                            if(pl.m_allowHoleRuning)
                            {
                                if( (pl.bottom() < (objs[i].top() + 2.0)) &&
                                        (pl.m_velY > 0.0) && (pl.m_velX > 0.0 ) &&
                                        (fabs(pl.m_velX) > fabs(pl.m_velY)) )
                                    goto tipRectT;
                            }
                            pl.m_x = objs[i].m_x - pl.m_w;
                            pl.m_velX = objs[i].m_velX;
                            pl.m_velX_source = objs[i].m_velX;
                            speedSum = 0.0;
                            speedNum = 0.0;
                            contactAt = obj::Contact_Left;
                        }
                    } else {
                        //'right
                        if( (objs[i].m_id == obj::SL_Rect) ||
                            (objs[i].m_id == obj::SL_RightBottom) ||
                            (objs[i].m_id == obj::SL_RightTop) )
                        {
                    tipRectR://Impacted at right
                            if(pl.m_allowHoleRuning)
                            {
                                if( (pl.bottom() < (objs[i].top() + 2.0)) &&
                                        (pl.m_velY > 0.0) && (pl.m_velX < 0.0 ) &&
                                        (fabs(pl.m_velX) > fabs(pl.m_velY)) )
                                    goto tipRectT;
                            }
                            pl.m_x = objs[i].m_x + objs[i].m_w;
                            pl.m_velX = objs[i].m_velX;
                            pl.m_velX_source = objs[i].m_velX;
                            speedSum = 0.0;
                            speedNum = 0.0;
                            contactAt = obj::Contact_Right;
                        }
                    }
                }
                tm = -2;
        tipTriangleShape://Check triangular collision
                if(contactAt == obj::Contact_None)
                {
                    k = objs[i].m_h / objs[i].m_w;
                    switch(objs[i].m_id)
                    {
                    case obj::SL_LeftBottom:
                        if( (pl.left() <= objs[i].left()) )
                        {
                            if( pl.bottom() > objs[i].bottom())
                                goto tipRectB;
                            if( (pl.bottom() >= objs[i].top()) && ((pl.left() < objs[i].left()) || (pl.m_velX <= 0.0)) )
                                goto tipRectT;
                        }
                        else if( ( pl.bottom() > objs[i].bottom() ) )
                        {
                            if(!colH)
                            {
                                if( objs[i].betweenH(pl.left()) || objs[i].betweenH(pl.right()))
                                    goto tipRectB;
                            } else {
                                if( pl.centerX() < objs[i].centerX() )
                                {
                                    if(pl.m_velX >= 0.0)
                                        goto tipRectL;
                                }
                                else
                                {
                                    if(pl.m_velX <= 0.0)
                                        goto tipRectR;
                                }
                            }
                        }
                        else if( pl.bottom() > objs[i].m_y + ( (pl.m_x - objs[i].m_x) * k) - 1 )
                        {
                            pl.m_y = objs[i].m_y + ( (pl.m_x - objs[i].m_x) * k ) - pl.m_h;
                            pl.m_velY = objs[i].m_velY;
                            pl.m_onSlopeFloor = true;
                            pl.m_onSlopeFloorShape = objs[i].m_id;
                            pl.m_onSlopeFloorRect  = objs[i].rect();
                            if( pl.m_velX > 0.0)
                            {
                                pl.m_velY = pl.m_velX * k;
                                pl.m_onSlopeYAdd = 0.0;
                            }
                            else
                            {
                                pl.m_onSlopeYAdd = pl.m_velX * k;
                                if((pl.m_onSlopeYAdd < 0.0) && (pl.bottom() + pl.m_onSlopeYAdd < objs[i].m_y))
                                    pl.m_onSlopeYAdd = -fabs(pl.bottom() - objs[i].m_y);
                            }
                            pl.m_stand = true;
                            pl.m_standOnYMovable = (objs[i].m_velY != 0.0);
                            pl.m_velX = pl.m_velX_source + objs[i].m_velX;
                            speedSum += objs[i].m_velX;
                            speedNum += 1.0;
                            contactAt = obj::Contact_Top;
                            doCliffCheck = true;
                            standingOn = &objs[i];
                            l_slopeFloor.push_back(&objs[i]);
                        }
                        break;
                    case obj::SL_RightBottom:
                        if( pl.right() >= objs[i].right())
                        {
                            if( pl.bottom() > objs[i].bottom())
                                goto tipRectB;
                            if( (pl.bottom() >= objs[i].top()) && ((pl.right() > objs[i].right()) || (pl.m_velX >= 0.0)) )
                                goto tipRectT;
                        }
                        else if( ( pl.bottom() > objs[i].bottom() ) )
                        {
                            if(!colH)
                            {
                                if( objs[i].betweenH(pl.left()) || objs[i].betweenH(pl.right()))
                                    goto tipRectB;
                            } else {
                                if( pl.centerX() < objs[i].centerX() )
                                {
                                    if(pl.m_velX >= 0.0)
                                        goto tipRectL;
                                }
                                else
                                {
                                    if(pl.m_velX <= 0.0)
                                        goto tipRectR;
                                }
                            }
                        }
                        else if(pl.bottom() > objs[i].m_y + ((objs[i].right() - pl.m_x - pl.m_w) * k) - 1 )
                        {
                            pl.m_y = objs[i].m_y + ( (objs[i].right() - pl.m_x - pl.m_w) * k) - pl.m_h;
                            pl.m_velY = objs[i].m_velY;
                            pl.m_onSlopeFloor = true;
                            pl.m_onSlopeFloorShape = objs[i].m_id;
                            pl.m_onSlopeFloorRect  = objs[i].rect();
                            if(pl.m_velX < 0.0)
                            {
                                pl.m_velY = -pl.m_velX * k;
                                pl.m_onSlopeYAdd = 0.0;
                            } else {
                                pl.m_onSlopeYAdd = -pl.m_velX * k;
                                if((pl.m_onSlopeYAdd < 0.0) && (pl.bottom() + pl.m_onSlopeYAdd < objs[i].m_y))
                                    pl.m_onSlopeYAdd = -fabs(pl.bottom() - objs[i].m_y);
                            }
                            pl.m_stand = true;
                            pl.m_standOnYMovable = (objs[i].m_velY != 0.0);
                            pl.m_velX = pl.m_velX_source + objs[i].m_velX;
                            speedSum += objs[i].m_velX;
                            speedNum += 1.0;
                            contactAt = obj::Contact_Top;
                            doCliffCheck = true;
                            standingOn = &objs[i];
                            l_slopeFloor.push_back(&objs[i]);
                        }
                        break;
                    case obj::SL_LeftTop:
                        if( pl.left() <= objs[i].left() )
                        {
                            if( pl.top() < objs[i].top())
                                goto tipRectT;
                            if( (pl.top() < objs[i].bottom()) && ((pl.left() < objs[i].left()) || (pl.m_velX <= 0.0)) )
                                goto tipRectB;
                        }
                        else if( ( pl.top() < objs[i].top() ) )
                        {
                            if(!colH)
                            {
                                if( objs[i].betweenH(pl.left()) || objs[i].betweenH(pl.right()))
                                    goto tipRectT;
                            } else {
                                if( pl.centerX() < objs[i].centerX() )
                                {
                                    if(pl.m_velX >= 0.0)
                                        goto tipRectL;
                                }
                                else
                                {
                                    if(pl.m_velX <= 0.0)
                                        goto tipRectR;
                                }
                            }
                        }
                        else if(pl.m_y < objs[i].bottom() - ((pl.m_x - objs[i].m_x) * k) )
                        {
                            pl.m_y    = objs[i].bottom() - ((pl.m_x - objs[i].m_x) * k);
                            pl.m_velY = objs[i].m_velY;
                            pl.m_onSlopeCeiling = true;
                            #ifdef RESOLVE_CEILING_AND_FLOOR
                            if(pl.m_stand || pl.m_onSlope || pl.m_onSlopeOld)
                            {
                                pl.m_x = pl.m_x+fabs(pl.m_velX);//objs[i].right();
                                pl.m_velX_source = objs[i].m_velX;
                            }
                            #endif
                            contactAt = obj::Contact_Bottom;
                            doHit = true;
                            ceilingOn = &objs[i];
                            l_slopeCeiling.push_back(&objs[i]);
                        }
                        break;
                    case obj::SL_RightTop:
                        if(pl.right() >= objs[i].right())
                        {
                            if( pl.top() < objs[i].top())
                                goto tipRectT;
                            if( (pl.m_y < objs[i].bottom()) && ((pl.right() > objs[i].right()) || (pl.m_velX >= 0.0)) )
                                goto tipRectB;
                        }
                        else if( ( pl.top() < objs[i].top() ) )
                        {
                            if(!colH)
                            {
                                if( objs[i].betweenH(pl.left()) || objs[i].betweenH(pl.right()))
                                    goto tipRectT;
                            } else {
                                if( pl.centerX() < objs[i].centerX() )
                                {
                                    if(pl.m_velX >= 0.0)
                                        goto tipRectL;
                                }
                                else
                                {
                                    if(pl.m_velX <= 0.0)
                                        goto tipRectR;
                                }
                            }
                        }
                        else if(pl.m_y < objs[i].bottom() - ((objs[i].right() - pl.m_x - pl.m_w) * k))
                        {
                            pl.m_y    = objs[i].bottom() - ((objs[i].right() - pl.m_x - pl.m_w) * k);
                            pl.m_velY = objs[i].m_velY;
                            pl.m_onSlopeCeiling = true;
                            #ifdef RESOLVE_CEILING_AND_FLOOR
                            if(pl.m_stand || pl.m_onSlope || pl.m_onSlopeOld)
                            {
                                pl.m_x = pl.m_x-fabs(pl.m_velX);//pl.m_x = objs[i].left() - pl.m_w;
                                pl.m_velX_source = objs[i].m_velX;
                            }
                            #endif
                            contactAt = obj::Contact_Bottom;
                            doHit = true;
                            ceilingOn = &objs[i];
                            l_slopeCeiling.push_back(&objs[i]);
                        }
                        break;
                    default:
                        break;
                    }
                }
            } else {
                //If shape is not rectangle - check triangular collision
                if( objs[i].m_id != obj::SL_Rect )
                    goto tipTriangleShape;
                //Catching 90-degree angle impacts of corners with rectangles
                if( !colH && !colV )
                {
                    if(tm == i)
                    {
                        if( fabs(pl.m_velY) > fabs(pl.m_velX) )
                            goto tipRectV;
                        else
                            goto tipRectH;
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

        if( (objs[i].m_id == obj::SL_Rect) && recttouch(pl.m_x, pl.m_y, pl.m_w, pl.m_h, objs[i].m_x, objs[i].m_y, objs[i].m_w, objs[i].m_h) )
        {
            pl.m_crushed = true;
        }
    }
    if(tm >= 0)
    {
        i = tm;
        td = 1;
        goto tipRectShape;
    }

    /*
    if(doAlignY)
        pl.m_y = alignYto;
    */

    /*
if( fabs(blocks[i]->posRect.center().x()-posRect.center().x())<
    fabs(nearest->posRect.center().x()-posRect.center().x()) )
*/

//#define centerOfObj(obj) (obj->m_x + obj->m_w / 2.0)
//#define centerOfObjR(obj) (obj.m_x + obj.m_w / 2.0)

    //Hit a block
    if(doHit && !l_toBump.isEmpty())
    {
        obj*candidate = l_toBump.first();
        foreach(obj* x, l_toBump)
        {
            if(candidate == x)
                continue;
            if( fabs(x->centerX() - pl.centerX()) <
                fabs(candidate->centerX() - pl.centerX()) )
            {
                candidate = x;
            }
        }
        candidate->m_bumped = true;
    }

    //Detect a cliff
    if(doCliffCheck && !l_clifCheck.isEmpty())
    {
        obj* candidate = l_clifCheck.first();
        double lefter  = candidate->m_x;
        double righter = candidate->m_x+candidate->m_w;
        foreach(obj* x, l_clifCheck)
        {
            if(candidate == x)
                continue;
            if(x->m_x < lefter)
                lefter = x->m_x;
            if(x->m_x+x->m_w > righter)
                righter = x->m_x + x->m_w;
        }
        if((pl.m_velX_source <= 0.0) && (lefter > pl.centerX()) )
            pl.m_cliff = true;
        if((pl.m_velX_source >= 0.0) && (righter < pl.centerX()) )
            pl.m_cliff = true;
    }

    //If both celling and floor slope
    if(pl.m_onSlopeFloor && pl.m_onSlopeCeiling && !l_slopeFloor.isEmpty() && !l_slopeCeiling.isEmpty())
    {
        obj& floor      = *l_slopeFloor.first();
        obj& ceiling    = *l_slopeCeiling.first();
        double k1   = floor.m_h / floor.m_w;
        double k2   = ceiling.m_h / ceiling.m_w;
        double h    = pl.m_h;
        double posX = pl.m_x;
        if( (floor.m_id == obj::SL_LeftBottom) && (ceiling.m_id == obj::SL_LeftTop) && (pl.m_velX <= 0.00) )
        {
            double Y1 = floor.m_y + ( (posX - floor.m_x) * k1 ) - h;
            double Y2 = ceiling.bottom() - ((posX - ceiling.m_x) * k2);
            while( (Y1 <= Y2) && (posX <= ceiling.right()) )
            {
                posX += 1.0;
                Y1 = floor.m_y + ( (posX - floor.m_x) * k1 ) - h;
                Y2 = ceiling.bottom() - ((posX - ceiling.m_x) * k2);
            }
            pl.m_x = posX;
            pl.m_y = Y1;
            pl.m_velX = ceiling.m_velX;
            pl.m_velX_source = ceiling.m_velX;
            speedSum = 0.0;
            speedNum = 0.0;
        } else
        if( (floor.m_id == obj::SL_RightBottom) && (ceiling.m_id == obj::SL_RightTop) && (pl.m_velX >= 0.00) )
        {
            double Y1 = floor.m_y + ( (floor.right() - posX - pl.m_w) * k1) - h;
            double Y2 = ceiling.bottom() - ((ceiling.right() - posX - pl.m_w) * k2);
            while( (Y1 <= Y2) && ( (posX + pl.m_w) >= ceiling.left()) )
            {
                posX -= 1.0;
                Y1 = floor.m_y + ( (floor.right() - posX - pl.m_w) * k1) - h;
                Y2 = ceiling.bottom() - ((ceiling.right() - posX - pl.m_w) * k2);
            }
            pl.m_x = posX;
            pl.m_y = Y1;
            pl.m_velX = ceiling.m_velX;
            pl.m_velX_source = ceiling.m_velX;
            speedSum = 0.0;
            speedNum = 0.0;
        }
    } else
    if(pl.m_stand && pl.m_onSlopeCeiling && standingOn && !l_slopeCeiling.isEmpty())
    {
        obj& floor      = *standingOn;
        obj& ceiling    = *l_slopeCeiling.first();
        //double k1   = floor.m_h / floor.m_w; //No needed for a floor detection
        double k2   = ceiling.m_h / ceiling.m_w;
        double h    = pl.m_h;
        double posX = pl.m_x;
        if( (floor.m_id == obj::SL_Rect ||
             floor.m_id == obj::SL_LeftTop ||
             floor.m_id == obj::SL_RightTop ) && (ceiling.m_id == obj::SL_LeftTop) && (pl.m_velX <= 0.00) )
        {
            double Y1 = floor.m_y - h;
            double Y2 = ceiling.bottom() - ((posX - ceiling.m_x) * k2);
            while( (Y1 <= Y2) && (posX <= ceiling.right()) )
            {
                posX += 1.0;
                Y2 = ceiling.bottom() - ((posX - ceiling.m_x) * k2);
            }
            pl.m_x = posX;
            pl.m_y = Y1;
            pl.m_velX = ceiling.m_velX;
            pl.m_velX_source = ceiling.m_velX;
            speedSum = 0.0;
            speedNum = 0.0;
        } else
        if( (floor.m_id == obj::SL_Rect ||
             floor.m_id == obj::SL_LeftTop ||
             floor.m_id == obj::SL_RightTop ) && (ceiling.m_id == obj::SL_RightTop) && (pl.m_velX >= 0.00) )
        {
            double Y1 = floor.m_y - h;
            double Y2 = ceiling.bottom() - ((ceiling.right() - posX - pl.m_w) * k2);
            while( (Y1 <= Y2) && ( (posX + pl.m_w) >= ceiling.left()) )
            {
                posX -= 1.0;
                Y2 = ceiling.bottom() - ((ceiling.right() - posX - pl.m_w) * k2);
            }
            pl.m_x = posX;
            pl.m_y = Y1;
            pl.m_velX = ceiling.m_velX;
            pl.m_velX_source = ceiling.m_velX;
            speedSum = 0.0;
            speedNum = 0.0;
        }
    } else
    if(pl.m_onSlopeFloor && ceilingOn && !l_slopeFloor.isEmpty())
    {
        obj& floor      = *l_slopeFloor.first();
        obj& ceiling    = *ceilingOn;
        double k1   = floor.m_h / floor.m_w;
        //double k2   = ceiling.m_h / ceiling.m_w;
        double h    = pl.m_h;
        double posX = pl.m_x;
        if( (floor.m_id == obj::SL_LeftBottom) && (ceiling.m_id == obj::SL_Rect ||
                                                   ceiling.m_id == obj::SL_LeftBottom ||
                                                   ceiling.m_id == obj::SL_RightBottom) && (pl.m_velX <= 0.00) )
        {
            double Y1 = floor.m_y + ( (posX - floor.m_x) * k1 ) - h;
            double Y2 = ceiling.bottom();
            while( (Y1 <= Y2) && (posX <= ceiling.right()) )
            {
                posX += 1.0;
                Y1 = floor.m_y + ( (posX - floor.m_x) * k1 ) - h;
            }
            pl.m_x = posX;
            pl.m_y = Y1;
            pl.m_velX = ceiling.m_velX;
            pl.m_velX_source = ceiling.m_velX;
            speedSum = 0.0;
            speedNum = 0.0;
        } else
        if( (floor.m_id == obj::SL_RightBottom) && (ceiling.m_id == obj::SL_Rect ||
                                                    ceiling.m_id == obj::SL_LeftBottom ||
                                                    ceiling.m_id == obj::SL_RightBottom) && (pl.m_velX >= 0.00) )
        {
            double Y1 = floor.m_y + ( (floor.right() - posX - pl.m_w) * k1) - h;
            double Y2 = ceiling.bottom();
            while( (Y1 <= Y2) && ( (posX + pl.m_w) >= ceiling.left()) )
            {
                posX -= 1.0;
                Y1 = floor.m_y + ( (floor.right() - posX - pl.m_w) * k1) - h;
            }
            pl.m_x = posX;
            pl.m_y = Y1;
            pl.m_velX = ceiling.m_velX;
            pl.m_velX_source = ceiling.m_velX;
            speedSum = 0.0;
            speedNum = 0.0;
        }
    }


    if( (speedNum > 1.0) && (speedSum != 0.0) )
    {
        pl.m_velX = pl.m_velX_source + (speedSum/speedNum);
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
    p.setPen(QColor(Qt::black));
    p.setFont(m_font);
    for(int i=0; i<objs.size(); i++)
    {
        objs[i].paint(p);
    }
    pl.paint(p);
}
