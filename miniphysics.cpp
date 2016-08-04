#include "miniphysics.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMessageBox>
#include <QApplication>
#include <QFile>

#include "PGE_File_Formats/file_formats.h"

QList<objRect> g_paintRectsAndPause;
bool           g_globalPause = false;

static double brick1Passed = 0.0;
static double brick2Passed = 0.0;
static double brick3Passed = 0.0;
static double brick4Passed = 0.0;

MiniPhysics::MiniPhysics(QWidget* parent):
    QOpenGLWidget(parent),
    m_font("Courier New", 10, 2)
{
    connect(&looper, &QTimer::timeout, this, &MiniPhysics::loop);
    looper.setTimerType(Qt::PreciseTimer);
    initTest1();
}

void MiniPhysics::initTest1()
{
    LevelData file;
    QFile physFile(":/test.lvlx");
    physFile.open(QIODevice::ReadOnly);
    QString rawdata = QString::fromUtf8(physFile.readAll());
    if( !FileFormats::ReadExtendedLvlFileRaw(rawdata, ".", file) )
    {
        QMessageBox::critical(nullptr, "SHIT", file.meta.ERROR_info);
        return;
    }
    initTestCommon(&file);
}

void MiniPhysics::initTest2()
{
    LevelData file;
    QFile physFile(":/test2.lvlx");
    physFile.open(QIODevice::ReadOnly);
    QString rawdata = QString::fromUtf8(physFile.readAll());
    if( !FileFormats::ReadExtendedLvlFileRaw(rawdata, ".", file) )
    {
        QMessageBox::critical(nullptr, "SHIT", file.meta.ERROR_info);
        return;
    }
    initTestCommon(&file);
}

void MiniPhysics::initTestCommon(LevelData *fileP)
{
    objs.clear();
    movingBlock.clear();
    looper.stop();

    brick1Passed = 0.0;
    brick2Passed = 0.0;
    brick3Passed = 0.0;
    brick4Passed = 0.0;

    keyMap[Qt::Key_Left]  = false;
    keyMap[Qt::Key_Right] = false;
    keyMap[Qt::Key_Space] = false;

    LevelData &file = *fileP;

    pl.m_id = obj::SL_Rect;
    pl.m_x = file.players[0].x;
    pl.m_y = file.players[0].y;
    pl.m_oldx = pl.m_x;
    pl.m_oldy = pl.m_y;
    pl.m_w = 32;
    pl.m_h = 32;
    pl.m_drawSpeed = true;
    //Allow hole-running
    pl.m_allowHoleRuning = false;
    //Align character while staying on top corner of the slope (
    pl.m_onSlopeFloorTopAlign = true;

    for(int i=0; i<file.blocks.size(); i++)
    {
        int id = 0;
        LevelBlock& blk = file.blocks[i];
        switch(blk.id)
        {
            case 358: case 357: case 321: id = obj::SL_RightBottom;   break;
            case 359: case 360: case 319: id = obj::SL_LeftBottom;    break;
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
    looper.start(25);
}


template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

template <class TArray> void findHorizontalBoundaries(TArray &array, double &lefter, double &righter)
{
    if(array.isEmpty())
        return;
    for(int i=0; i < array.size(); i++)
    {
        obj* x = array[i];
        if(x->m_x < lefter)
            lefter = x->m_x;
        if(x->m_x+x->m_w > righter)
            righter = x->m_x + x->m_w;
    }
}

template <class TArray> void findVerticalBoundaries(TArray &array, double &higher, double &lower,
                                                    obj**highest=nullptr, obj**lowerest=nullptr)
{
    if(array.isEmpty())
        return;
    for(int i=0; i < array.size(); i++)
    {
        obj* x = array[i];
        if(x->m_y < higher)
        {
            higher = x->m_y;
            if(highest)
                *highest = x;
        }
        if(x->m_y + x->m_h > lower)
        {
            lower = x->m_y + x->m_h;
            if(lowerest)
                *lowerest = x;
        }
    }
}

void MiniPhysics::iterateStep()
{
    if(keyMap[Qt::Key_F1])
        looper.setInterval(25);
    else if(keyMap[Qt::Key_F2])
        looper.setInterval(100);
    else if(keyMap[Qt::Key_F3])
        looper.setInterval(250);
    else if(keyMap[Qt::Key_F4])
        looper.setInterval(500);
    else if(keyMap[Qt::Key_F11])
        initTest1();
    else if(keyMap[Qt::Key_F12])
        initTest2();

    if(g_globalPause)
    {
        if(keyMap[Qt::Key_Enter])
        {
            g_paintRectsAndPause.clear();
            g_globalPause = false;
        }
        return;
    }

    bool lk=false, rk=false;
    {
        int lastID  =  movingBlock.size()-1;
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
        if( (pl.m_velY < 8) && (!pl.m_stand ||
                                 pl.m_standOnYMovable ||
                                (!pl.m_allowHoleRuning && pl.m_cliff && (pl.m_velX_source != 0.0))
                                )
                )
            pl.m_velY += 0.4;

        if(pl.m_stand && keyMap[Qt::Key_Space] && !pl.m_jumpPressed)
        {
            pl.m_velY = -10; //'8
            pl.m_jumpPressed = true;
        }
        pl.m_jumpPressed = keyMap[Qt::Key_Space];

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
    int i=0, /*ck=0,*/ tm=0, td=0;
    obj::ContactAt contactAt = obj::Contact_None;
    bool colH=false, colV=false;
    tm = -1;
    td = 0;

    //Return player to top back on fall down
    if(pl.m_y > this->height())
        pl.m_y = 64;
    bool doHit = false;
    bool doCliffCheck = false;
    bool xSpeedWasReversed=false;
    QVector<obj*> l_clifCheck;
    QVector<obj*> l_toBump;
    QVector<obj*> l_slopeFloor;
    QVector<obj*> l_slopeCeiling;
    QVector<obj*> l_possibleCrushers;
    obj* standingOn = nullptr;
    obj* ceilingOn  = nullptr;
    double speedNum = 0.0;
    double speedSum = 0.0;

    //! DEBUG ONLY!!
    g_paintRectsAndPause.clear();

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
        else if(pl.m_onSlopeFloorOld) //Collect extra candidates for a cliff detection on the slope
        {
            objRect &r1 = pl.m_onSlopeFloorRect;
            objRect  r2 = objs[i].rect();
            if( (pl.m_onSlopeFloorShape == obj::SL_LeftBottom) && (pl.m_velX >= 0.0) )
            {
                if( recttouch(pl.m_x + pl.m_w, pl.centerY(), pl.m_w, r2.h, objs[i].m_x, objs[i].m_y, objs[i].m_w, objs[i].m_h)
                    &&
                      //is touching corners
                      ( ( (r1.x+r1.w) >= (r2.x-1.0) ) &&
                        ( (r1.x+r1.w) <= (r2.x+r2.w) ) &&
                        ( (r1.y+r1.h) >= (r2.y-1.0) ) &&
                        ( (r1.y+r1.h) <= (r2.y+1.0) ) ) &&
                        ( objs[i].top() > pl.bottom() ) )
                    l_clifCheck.push_back(&objs[i]);
            }
            else
            if( (pl.m_onSlopeFloorShape == obj::SL_RightBottom) && (pl.m_velX <= 0.0) )
            {
                if( recttouch(pl.m_x, pl.centerY(), pl.m_w, r2.h, objs[i].m_x, objs[i].m_y, objs[i].m_w, objs[i].m_h)
                        &&
                          //is touching corners
                          ( ( (r1.x) >= (r2.x) ) &&
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
                            /*
                            if(pl.m_onSlopeFloor || pl.m_onSlopeFloorOld)
                            {
                                if(pl.left() >= objs[i].right()-2.0 )
                                    goto tipRectT_Skip;
                                if(pl.right() <= objs[i].left()+2.0 )
                                    goto tipRectT_Skip;
                            }*/
                    tipRectT://Impacted at top
                            pl.m_y = objs[i].m_y - pl.m_h;
                            pl.m_velY   = objs[i].m_velY;
                            pl.m_stand  = true;
                            pl.m_standOnYMovable = (objs[i].m_velY != 0.0);
                            pl.m_velX   = pl.m_velX_source + objs[i].m_velX;
                            speedSum += objs[i].m_velX;
                            if(objs[i].m_velX != 0.0)
                                speedNum += 1.0;
                            contactAt = obj::Contact_Top;
                            doCliffCheck = true;
                            standingOn = &objs[i];
                            objs[i].m_touch = contactAt;
                    //tipRectT_Skip:;
                        }
                    } else {
                        //'bottom
                        if( (objs[i].m_id == obj::SL_Rect) ||
                            (objs[i].m_id == obj::SL_LeftBottom) ||
                            (objs[i].m_id == obj::SL_RightBottom) )
                        {
                            /*
                            if(pl.m_onSlopeCeiling || pl.m_onSlopeCeilingOld)
                            {
                                if(pl.left() >= objs[i].right()-2.0 )
                                    goto tipRectB_Skip;
                                if(pl.right() <= objs[i].left()+2.0 )
                                    goto tipRectB_Skip;
                            }*/
                    tipRectB://Impacted at bottom
                            pl.m_y = objs[i].m_y + objs[i].m_h;
                            pl.m_velY = objs[i].m_velY;
                            contactAt = obj::Contact_Bottom;
                            doHit = true;
                            ceilingOn = &objs[i];
                            objs[i].m_touch = contactAt;
                    //tipRectB_Skip:;
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
                            if(pl.m_onSlopeCeilingOld || pl.m_onSlopeFloorOld)
                            {
                                objRect& rF = pl.m_onSlopeFloorRect;
                                objRect& rC = pl.m_onSlopeCeilingRect;
                                if( (objs[i].top() == rF.top())  &&
                                    (objs[i].right() <= (rF.left()+1.0)) &&
                                    (pl.m_onSlopeFloorShape == obj::SL_LeftBottom))
                                    goto tipRectL_Skip;
                                if( (objs[i].bottom() == rC.bottom()) &&
                                    (objs[i].right() <= (rF.left()+1.0)) &&
                                    (pl.m_onSlopeCeilingShape == obj::SL_LeftTop)  )
                                    goto tipRectL_Skip;
                            }
                    tipRectL://Impacted at left
                            {
                                if(pl.m_allowHoleRuning)
                                {
                                    if( (pl.bottom() < (objs[i].top() + 2.0)) &&
                                            (pl.m_velY > 0.0) && (pl.m_velX > 0.0 ) &&
                                            (fabs(pl.m_velX) > fabs(pl.m_velY)) )
                                        goto tipRectT;
                                }
                                pl.m_x = objs[i].m_x - pl.m_w;
                                double &splr = pl.m_velX;
                                double &sbox = objs[i].m_velX;
                                xSpeedWasReversed = splr <= sbox;
                                splr = std::min( splr, sbox );
                                pl.m_velX_source = splr;
                                speedSum = 0.0;
                                speedNum = 0.0;
                                contactAt = obj::Contact_Left;
                                objs[i].m_touch = contactAt;
                            }
                    tipRectL_Skip:;
                        }
                    } else {
                        //'right
                        if( (objs[i].m_id == obj::SL_Rect) ||
                            (objs[i].m_id == obj::SL_RightBottom) ||
                            (objs[i].m_id == obj::SL_RightTop) )
                        {
                            if(pl.m_onSlopeCeilingOld || pl.m_onSlopeFloorOld)
                            {
                                objRect& rF = pl.m_onSlopeFloorRect;
                                objRect& rC = pl.m_onSlopeCeilingRect;
                                if( (objs[i].top() == rF.top()) &&
                                    (objs[i].left() >= (rF.right()-1.0)) &&
                                    (pl.m_onSlopeFloorShape == obj::SL_RightBottom) )
                                    goto tipRectR_Skip;
                                if( (objs[i].bottom() == rC.bottom()) &&
                                    (objs[i].left() >= (rF.right()-1.0)) &&
                                    (pl.m_onSlopeCeilingShape == obj::SL_RightTop) )
                                    goto tipRectR_Skip;
                            }
                    tipRectR://Impacted at right
                            {
                                if(pl.m_allowHoleRuning)
                                {
                                    if( (pl.bottom() < (objs[i].top() + 2.0)) &&
                                            (pl.m_velY > 0.0) && (pl.m_velX < 0.0 ) &&
                                            (fabs(pl.m_velX) > fabs(pl.m_velY)) )
                                        goto tipRectT;
                                }
                                pl.m_x = objs[i].m_x + objs[i].m_w;
                                double &splr = pl.m_velX;
                                double &sbox = objs[i].m_velX;
                                xSpeedWasReversed = splr >= sbox;
                                splr = std::max( splr, sbox );
                                pl.m_velX_source = splr;
                                speedSum = 0.0;
                                speedNum = 0.0;
                                contactAt = obj::Contact_Right;
                                objs[i].m_touch = contactAt;
                            }
                    tipRectR_Skip:;
                        }
                    }
                }
                if( (pl.m_stand) || (pl.m_velX_source == 0.0) || xSpeedWasReversed)
                    tm = -2;
        tipTriangleShape://Check triangular collision
                if(contactAt == obj::Contact_None)
                {
                    k = objs[i].m_h / objs[i].m_w;
                    switch(objs[i].m_id)
                    {
                    case obj::SL_LeftBottom:
                        if( (pl.left() <= objs[i].left()) && (pl.m_onSlopeFloorTopAlign || (pl.m_velY >= 0.0) ) )
                        {
                            if( pl.bottom() > objs[i].bottom())
                                goto tipRectB;
                            if( (pl.bottom() >= objs[i].top()) && ((pl.left() < objs[i].left()) || (pl.m_velX <= 0.0)) )
                                goto tipRectT;
                        }
                        else if( ( pl.bottom() > objs[i].bottom() ) &&
                                    ((!pl.m_onSlopeFloorOld && (pl.bottomOld() > objs[i].bottomOld())) ||
                                      (pl.m_onSlopeFloorShape != objs[i].m_id)))
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
                            if( (pl.m_velX > 0.0) || !pl.m_onSlopeFloorTopAlign)
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
                            if(objs[i].m_velX != 0.0)
                                speedNum += 1.0;
                            contactAt = obj::Contact_Top;
                            doCliffCheck = true;
                            standingOn = &objs[i];
                            l_slopeFloor.push_back(&objs[i]);
                            objs[i].m_touch = contactAt;
                        }
                        break;
                    case obj::SL_RightBottom:
                        if( pl.right() >= objs[i].right() && (pl.m_onSlopeFloorTopAlign || (pl.m_velY >= 0.0) ))
                        {
                            if( pl.bottom() > objs[i].bottom())
                                goto tipRectB;
                            if( (pl.bottom() >= objs[i].top()) && ((pl.right() > objs[i].right()) || (pl.m_velX >= 0.0)) )
                                goto tipRectT;
                        }
                        else if( ( pl.bottom() > objs[i].bottom() ) &&
                                    ((!pl.m_onSlopeFloorOld && (pl.bottomOld() > objs[i].bottomOld())) ||
                                      (pl.m_onSlopeFloorShape != objs[i].m_id)))
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
                            if( (pl.m_velX < 0.0) || !pl.m_onSlopeFloorTopAlign)
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
                            if(objs[i].m_velX != 0.0)
                                speedNum += 1.0;
                            contactAt = obj::Contact_Top;
                            doCliffCheck = true;
                            standingOn = &objs[i];
                            l_slopeFloor.push_back(&objs[i]);
                            objs[i].m_touch = contactAt;
                        }
                        break;
                    case obj::SL_LeftTop:
                        if( pl.left() <= objs[i].left() )
                        {
                            if( pl.top() < objs[i].top() )
                                goto tipRectT;
                            if( (pl.top() < objs[i].bottom()) && ((pl.left() < objs[i].left()) || (pl.m_velX <= 0.0)) )
                                goto tipRectB;
                        }
                        else if( ( pl.top() < objs[i].top() )  &&
                                 ((!pl.m_onSlopeCeilingOld && (pl.topOld() < objs[i].topOld())) ||
                                   (pl.m_onSlopeCeilingShape != objs[i].m_id)) )
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
                            double oldY = pl.m_oldy;
                            pl.m_y    = objs[i].bottom() - ((pl.m_x - objs[i].m_x) * k);
                            pl.m_velY = fabs(oldY-pl.m_y);
                            if( pl.m_velX < 0.0)
                            {
                                pl.m_velY = fabs(oldY-pl.m_y);
                            } else {
                                pl.m_velY = objs[i].m_velY;
                            }
                            pl.m_onSlopeCeiling = true;
                            pl.m_onSlopeCeilingShape = objs[i].m_id;
                            pl.m_onSlopeCeilingRect  = objs[i].rect();
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
                            objs[i].m_touch = contactAt;
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
                        else if( ( pl.top() < objs[i].top() )  &&
                                 ((!pl.m_onSlopeCeilingOld && (pl.topOld() < objs[i].topOld())) ||
                                   (pl.m_onSlopeCeilingShape != objs[i].m_id)) )
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
                            double oldY = pl.m_oldy;
                            pl.m_y    = objs[i].bottom() - ((objs[i].right() - pl.m_x - pl.m_w) * k);
                            if( pl.m_velX > 0.0)
                            {
                                pl.m_velY = fabs(oldY-pl.m_y);
                            } else {
                                pl.m_velY = objs[i].m_velY;
                            }
                            pl.m_onSlopeCeiling = true;
                            pl.m_onSlopeCeilingShape = objs[i].m_id;
                            pl.m_onSlopeCeilingRect  = objs[i].rect();
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
                            objs[i].m_touch = contactAt;
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

        if( (objs[i].m_id == obj::SL_Rect) && recttouch(pl.m_oldx, pl.m_oldy, pl.m_w, pl.m_h, objs[i].m_oldx, objs[i].m_oldy, objs[i].m_w, objs[i].m_h) )
        {
            l_possibleCrushers.push_back(&objs[i]);
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
        obj*candidate = nullptr;
        for(int bump=0; bump<l_toBump.size(); bump++)
        {
            obj* x = l_toBump[bump];
            if(candidate == x)
                continue;
            if(
                (x->bottom() > pl.top()) && (x->bottomOld() > pl.topOld()) &&
                (x->m_id == obj::SL_Rect || x->m_id == obj::SL_LeftBottom || x->m_id == obj::SL_RightBottom)
            ) {
                l_toBump.removeAt(bump); bump--;
                continue;
            }
            if(!candidate)
                candidate = x;
            if( fabs(x->centerX() - pl.centerX()) <
                fabs(candidate->centerX() - pl.centerX()) )
            {
                candidate = x;
            }
        }
        if(candidate)
            candidate->m_bumped = true;
    }

    //Detect a cliff
    if(doCliffCheck && !l_clifCheck.isEmpty())
    {
        obj* candidate = l_clifCheck.first();
        double lefter  = candidate->m_x;
        double righter = candidate->m_x+candidate->m_w;
        findHorizontalBoundaries(l_clifCheck, lefter, righter);
//        for(int i=0; i<l_clifCheck.size(); i++)
//        {
//            obj* x = l_clifCheck[i];
//            if(x->m_x < lefter)
//                lefter = x->m_x;
//            if(x->m_x+x->m_w > righter)
//                righter = x->m_x + x->m_w;
//        }
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
    if((pl.m_stand || !l_clifCheck.isEmpty()) && pl.m_onSlopeCeiling &&
            (standingOn || !l_clifCheck.isEmpty()) && !l_slopeCeiling.isEmpty())
    {
        obj& floor      = standingOn ? *standingOn : *l_clifCheck[0];
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
    if(pl.m_onSlopeFloor && (ceilingOn || !l_toBump.isEmpty()) && !l_slopeFloor.isEmpty())
    {
        obj* floorT = l_slopeFloor[0];
        for(int j=0; j<l_clifCheck.size(); j++)
        {
            if( (l_clifCheck[j]->m_id != obj::SL_Rect) && !l_slopeFloor.contains(l_clifCheck[j]) )
                l_slopeFloor.push_back(l_clifCheck[j]);
        }

        for(int j=0; j<l_toBump.size(); j++)
        {
            bool colV=false;
            bool colH=false;
            obj& block = *l_toBump[j];
            if( pt(pl.m_x, pl.m_y, pl.m_w, pl.m_h, block.m_x, block.m_y, block.m_w, block.m_h) )
            {

                colH = pt(pl.m_x,     pl.m_oldy,  pl.m_w, pl.m_h, block.m_x, block.m_oldy, block.m_w, block.m_h);
                colV = pt(pl.m_oldx,  pl.m_y, pl.m_w, pl.m_h, block.m_oldx, block.m_y, block.m_w, block.m_h);
                if( colH )
                {
                    if( pl.centerX() < block.centerX() )
                    {
                        //'left
                        if( (block.m_id == obj::SL_Rect) ||
                            (block.m_id == obj::SL_LeftBottom)||
                            (block.m_id == obj::SL_LeftTop) )
                        {
                            pl.m_x = block.m_x - pl.m_w;
                            double &splr = pl.m_velX;
                            double &sbox = block.m_velX;
                            splr = std::min( splr, sbox );
                            pl.m_velX_source = splr;
                            speedSum = 0.0;
                            speedNum = 0.0;
                        }
                    }
                    else
                    {
                        //'right
                        if( (block.m_id == obj::SL_Rect) ||
                            (block.m_id == obj::SL_RightBottom) ||
                            (block.m_id == obj::SL_RightTop) )
                        {
                            pl.m_x = block.m_x + block.m_w;
                            double &splr = pl.m_velX;
                            double &sbox = block.m_velX;
                            splr = std::max( splr, sbox );
                            pl.m_velX_source = splr;
                            speedSum = 0.0;
                            speedNum = 0.0;
                        }
                    }
                    goto kill;
                }
                else if(colV)
                {
                    if( pl.centerY() > l_toBump[j]->centerY())
                        continue;
                }
                if(!colV && !colH)
                    goto kill;
            }
        kill:
            l_toBump.removeAt(j);
            j--;
        }

        if(!ceilingOn && l_toBump.isEmpty())
            goto applySpeedAdd;

        for(int j=0; j<l_slopeFloor.size(); j++)
        {
            g_paintRectsAndPause.push_back(l_slopeFloor[j]->rect());
            // LEFT!!
            if( (l_slopeFloor[j]->centerX() < pl.centerX()) && (l_slopeFloor[j]->right() < pl.centerX()) &&
                ( (l_slopeFloor[j]->bottom() >= pl.bottom()) || (l_slopeFloor[j]->bottomOld() >= pl.bottomOld()) ) && (pl.m_velX < 0.0) )
            {
                pl.m_x = l_slopeFloor[j]->m_x + l_slopeFloor[j]->m_w;
                double &splr = pl.m_velX;
                double &sbox = l_slopeFloor[j]->m_velX;
                splr = std::max( splr, sbox );
                pl.m_velX_source = splr;
                speedSum = 0.0;
                speedNum = 0.0;
            }
            // RIGHT!!
            if( (l_slopeFloor[j]->centerX() > pl.centerX()) && (l_slopeFloor[j]->left() > pl.centerX()) &&
                ( (l_slopeFloor[j]->bottom() >= pl.bottom()) || (l_slopeFloor[j]->bottomOld() >= pl.bottomOld()) ) && (pl.m_velX > 0.0) )
            {
                pl.m_x = l_slopeFloor[j]->m_x - pl.m_w;
                double &splr = pl.m_velX;
                double &sbox = l_slopeFloor[j]->m_velX;
                splr = std::min( splr, sbox );
                pl.m_velX_source = splr;
                speedSum = 0.0;
                speedNum = 0.0;
            }
        }

        obj* ceilB = ceilingOn ? ceilingOn : l_toBump[0];
        if( (l_slopeFloor[0] != ceilB) )
        {
            double ceil_lefter  = ceilB->left();
            double ceil_righter = ceilB->right();
            double floor_higher = floorT->top();
            double floor_lower  = floorT->bottom();
            findHorizontalBoundaries(l_toBump, ceil_lefter, ceil_righter);
            findVerticalBoundaries(l_slopeFloor, floor_higher, floor_lower, &floorT);
            obj& floor      = *floorT;
            obj& ceiling    = *ceilB;

            double k1   = floor.m_h / floor.m_w;
            //double k2   = ceiling.m_h / ceiling.m_w;
            double h    = pl.m_h;
            double posX = pl.m_x;
            // LEFT!!!
            if( (floor.m_id == obj::SL_LeftBottom) && (ceiling.m_id == obj::SL_Rect ||
                                                       ceiling.m_id == obj::SL_LeftBottom ||
                                                       ceiling.m_id == obj::SL_RightBottom) && (pl.m_velX <= 0.00)
                && (ceiling.bottom() > pl.top() ) && (ceiling.centerYold() < pl.centerYold()) )
            {
                double Y1 = floor.m_y + ( (posX - floor.m_x) * k1 ) - h;
                double Y2 = ceiling.bottom();
                while( (Y1 < Y2) && (posX <= ceil_righter) )
                {
                    posX += 1.0;
                    Y1 = floor.m_y + ( (posX - floor.m_x) * k1 ) - h;
                }
                pl.m_x = posX;
                pl.m_y = Y2;
                //pl.m_velX = ceiling.m_velX;
                double &splr = pl.m_velX;
                double &sbox = ceiling.m_velX;
                splr = std::min( splr, sbox );
                pl.m_velX_source = splr /*ceiling.m_velX*/;
                speedSum = 0.0;
                speedNum = 0.0;
            } else
            // RIGHT!!!
            if( (floor.m_id == obj::SL_RightBottom) && (ceiling.m_id == obj::SL_Rect ||
                                                        ceiling.m_id == obj::SL_LeftBottom ||
                                                        ceiling.m_id == obj::SL_RightBottom) && (pl.m_velX >= 0.00)
                    && (ceiling.bottom() > pl.top() ) && (ceiling.centerYold() < pl.centerYold()) )
            {
                double Y1 = floor.m_y + ( (floor.right() - posX - pl.m_w) * k1) - h;
                double Y2 = ceiling.bottom();
                while( (Y1 < Y2) && ( (posX + pl.m_w) >= ceil_lefter) )
                {
                    posX -= 1.0;
                    Y1 = floor.m_y + ( (floor.right() - posX - pl.m_w) * k1) - h;
                }
                pl.m_x = posX;
                pl.m_y = Y2;
                //pl.m_velX = ceiling.m_velX;
                double &splr = pl.m_velX;
                double &sbox = ceiling.m_velX;
                splr = std::max( splr, sbox );
                pl.m_velX_source = splr /*ceiling.m_velX*/;
                speedSum = 0.0;
                speedNum = 0.0;
            }
        }
    }

applySpeedAdd:
    if( (speedNum > 1.0) && (speedSum != 0.0) )
    {
        pl.m_velX = pl.m_velX_source + (speedSum/speedNum);
    }

}

void MiniPhysics::loop()
{
    iterateStep();
    if(!g_globalPause)
        processCollisions();
    repaint();
    if(pl.m_crushed)
        printf("ouch!\n");
    if(pl.m_crushed && pl.m_crushedOld)
        printf("OOOOOOOOOUCH!\n");
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
    for(int i=0; i<g_paintRectsAndPause.size(); i++)
    {
        objRect& r = g_paintRectsAndPause[i];
        p.setBrush(Qt::transparent);
        p.setPen(Qt::red);
        p.drawRect(r.x, r.y, r.w, r.h);
    }
}

