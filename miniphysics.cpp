#include "miniphysics.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMessageBox>
#include <QApplication>
#include <QFile>

#include "PGE_File_Formats/file_formats.h"

static double brick1Passed = 0.0;
static double brick2Passed = 0.0;
static double brick3Passed = 0.0;
static double brick4Passed = 0.0;

#ifdef STOP_LOOP_ON_CRUSH
bool    alive = true;
#endif

MiniPhysics::MiniPhysics(QWidget* parent):
    QOpenGLWidget(parent),
    m_font("Courier New", 10, 2)
{
    m_font.setStyleStrategy(QFont::OpenGLCompatible);
    connect(&looper, &QTimer::timeout, this, &MiniPhysics::loop);
    looper.setTimerType(Qt::PreciseTimer);
    pl.m_w = 24;
    pl.m_h = 30;
    //Allow hole-running
    pl.m_allowHoleRuning = false;
    //Align character while staying on top corner of the slope (
    pl.m_onSlopeFloorTopAlign = true;

    initTest1();
}

void MiniPhysics::initTest1()
{
    lastTest = 1;
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
    lastTest = 2;
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

    cameraX = fileP->sections[0].size_left;
    cameraY = fileP->sections[0].size_top;

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
    pl.m_drawSpeed = true;

    for(int i=0; i<file.blocks.size(); i++)
    {
        int id = 0;
        int blockSide = obj::Block_ALL;
        LevelBlock& blk = file.blocks[i];
        switch(blk.id)
        {
            case 358: case 357: case 321: id = obj::SL_RightBottom;   break;
            case 359: case 360: case 319: id = obj::SL_LeftBottom;    break;
            case 363: case 364: id = obj::SL_LeftTop;       break;
            case 362: case 361: id = obj::SL_RightTop;      break;
            case 366:
            id = obj::SL_LeftBottom;
            blockSide = obj::Block_TOP; break;
            case 495:
            blockSide = obj::Block_RIGHT;
            id = obj::SL_Rect; break;
            case 25: case 575:
            blockSide = obj::Block_TOP;
            id = obj::SL_Rect; break;
            case 500:
            blockSide = obj::Block_BOTTOM;
            id = obj::SL_Rect; break;
            default:    id = obj::SL_Rect;                  break;
        }
        obj box(blk.x, blk.y, id);
        box.m_w = blk.w;
        box.m_h = blk.h;
        box.m_blocked[0] = blockSide;
        box.m_blocked[1] = blockSide;
        objs.push_back(box);
        if(blk.id == 159)
            movingBlock.push_back(&objs.last());
    }


    {
        int lastID = movingBlock.size()-1;
        obj &brick1 = *movingBlock[lastID];
        brick1.m_velX = 0.8;
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
    {
        keyMap[Qt::Key_F11] = false;
        initTest1();
    }
    else if(keyMap[Qt::Key_F12])
    {
        keyMap[Qt::Key_F12] = false;
        initTest2();
    }

    if(keyMap[Qt::Key_Q])
    {
        keyMap[Qt::Key_Q] = false;
        pl.m_allowHoleRuning = !pl.m_allowHoleRuning;
    }
    if(keyMap[Qt::Key_W])
    {
        keyMap[Qt::Key_W]=false;
        pl.m_onSlopeFloorTopAlign = !pl.m_onSlopeFloorTopAlign;
    }

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
        if(brick2Passed > 8.0*32.0 || brick2Passed < 0.0)
            brick2.m_velY *= -1.0;

        obj &brick3 = *movingBlock[lastID-2];
        brick3.m_oldx = brick3.m_x;
        brick3.m_x   += brick3.m_velX;
        brick3Passed += brick3.m_velX;
        if(brick3Passed > 9.0*32.0 || brick3Passed < 0.0)
            brick3.m_velX *= -1.0;

        obj &brick4 = *movingBlock[lastID-3];
        brick4.m_oldx = brick4.m_x;
        brick4.m_x   += brick4.m_velX;
        brick4Passed += brick4.m_velX;
        if(brick4Passed > 10.0*32.0 || brick4Passed < 0.0)
            brick4.m_velX *= -1.0;
    }

    { //With pl
        pl.m_keys.left  = keyMap[Qt::Key_Left];
        pl.m_keys.right = keyMap[Qt::Key_Right];
        double Xmod = 0;
        if(pl.m_keys.left ^ pl.m_keys.right)
        {
            if( (pl.m_keys.left && pl.m_velX_source > -6) && !pl.m_touchLeftWall )
                Xmod -= 0.4;
            if( (pl.m_keys.right && pl.m_velX_source < 6) && !pl.m_touchRightWall )
                Xmod += 0.4;
        }
        else if( !pl.m_keys.left && !pl.m_keys.right)
        {
            if(fabs(pl.m_velX_source) > 0.4)
            {
                Xmod -= sgn(pl.m_velX_source)*0.4;
            }
            else
            {
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

        pl.m_touchLeftWall  = false;
        pl.m_touchRightWall = false;
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
        if(!pl.m_onSlopeFloorOld)
            pl.m_onSlopeFloorShape = -1;
        if(!pl.m_onSlopeCeilingOld)
            pl.m_onSlopeCeilingShape = -1;

        if(keyMap[Qt::Key_1])
        {
            keyMap[Qt::Key_1]=false;
            if(pl.m_stand)
                pl.m_oldy += pl.m_h-30;
            pl.m_y += pl.m_h-30;
            pl.m_oldx = pl.m_oldx-(24-pl.m_w);
            pl.m_w = 24;
            pl.m_h = 30;
        }
        if(keyMap[Qt::Key_2])
        {
            keyMap[Qt::Key_2]=false;
            if(pl.m_stand)
                pl.m_oldy += pl.m_h-32;
            pl.m_y += pl.m_h-32;
            pl.m_oldx = pl.m_oldx-(32-pl.m_w);
            pl.m_w = 32;
            pl.m_h = 32;
        }
        if(keyMap[Qt::Key_3])
        {
            keyMap[Qt::Key_3]=false;
            if(pl.m_stand)
                pl.m_oldy += pl.m_h-50;
            pl.m_y += pl.m_h-50;
            pl.m_oldx = pl.m_oldx-(24-pl.m_w);
            pl.m_w = 24;
            pl.m_h = 50;
        }

        pl.m_stand      = false;
        pl.m_standOnYMovable = false;
    }
}




template <class TArray> void findHorizontalBoundaries(TArray &array, double &lefter, double &righter,
                                                      obj**leftest=nullptr, obj**rightest=nullptr)
{
    if(array.empty())
        return;
    for(unsigned int i=0; i < array.size(); i++)
    {
        obj* x = array[i];
        if(x->left() < lefter)
        {
            lefter = x->left();
            if(leftest)
                *leftest = x;
        }
        if(x->right() > righter)
        {
            righter = x->right();
            if(rightest)
                *rightest = x;
        }
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

static inline bool figureTouch(obj &pl, obj& block, double marginV = 0.0, double marginH = 0.0)
{
    return recttouch(pl.m_x+marginH, pl.m_y+marginV, pl.m_w-(marginH*2.0), pl.m_h-(marginV*2.0),
                     block.m_x,      block.m_y,      block.m_w,            block.m_h);
}

inline bool isBlockFloor(int id)
{
    return (id == obj::SL_Rect) ||
           (id == obj::SL_LeftTop) ||
           (id == obj::SL_RightTop);
}

inline bool isBlockCeiling(int id)
{
    return (id == obj::SL_Rect) ||
           (id == obj::SL_LeftBottom) ||
           (id == obj::SL_RightBottom);
}

inline bool isBlockLeftWall(int id)
{
    return (id == obj::SL_Rect) ||
           (id == obj::SL_LeftBottom) ||
           (id == obj::SL_LeftTop);
}

inline bool isBlockRightWall(int id)
{
    return (id == obj::SL_Rect) ||
           (id == obj::SL_RightBottom) ||
           (id == obj::SL_RightTop);
}

inline bool isBlockFloor(obj* block)
{
    return (block->m_id == obj::SL_Rect) ||
           (block->m_id == obj::SL_LeftTop) ||
           (block->m_id == obj::SL_RightTop);
}

inline bool isBlockCeiling(obj* block)
{
    return (block->m_id == obj::SL_Rect) ||
           (block->m_id == obj::SL_LeftBottom) ||
           (block->m_id == obj::SL_RightBottom);
}

inline bool isBlockLeftWall(obj* block)
{
    return (block->m_id == obj::SL_Rect) ||
           (block->m_id == obj::SL_LeftBottom) ||
           (block->m_id == obj::SL_LeftTop);
}

inline bool isBlockRightWall(obj* block)
{
    return (block->m_id == obj::SL_Rect) ||
           (block->m_id == obj::SL_RightBottom) ||
           (block->m_id == obj::SL_RightTop);
}

/**
 * @brief Finds minimal available height between slope floor and slope ceiling (and between horizontal block and one of slopes)
 * @param idF shape of floor
 * @param sF rectangle of floor
 * @param vF velocity of floor
 * @param idC shape of ceiling
 * @param sC rectangle of ceilig
 * @param w width of player
 * @param h height of player
 * @param [INOUT] x X position of player
 * @param [INOUT] y Y position of player
 * @param [INOUT] newSpeedX X-Speed of the player
 * @param slT is slope top
 * @param slB is slope bottom
 * @return true if slope floor<->ceiling collision has been detected and resolved. False if found nothing
 */
inline bool findMinimalHeight(int idF, objRect sF, double vF,
                              int idC, objRect sC,
                              double w, double h, double *x, double *y, double *newSpeedX,
                              bool slT=false, bool slB=false)
{
    double &posX = *x;
    double &posY = *y;
    double &velX = *newSpeedX;
    double k1   = sF.h / sF.w;
    double k2   = sC.h / sC.w;
    /***************************Ceiling and floor slopes*******************************/
    if( slT && slB && (idF==obj::SL_LeftBottom) && (idC==obj::SL_LeftTop) )
    {
        //LeftBottom
        double Y1 = sF.y + ( (posX - sF.x) * k1 ) - h;
        //LeftTop
        double Y2 = sC.bottom() - ((posX - sC.x) * k2);
        while( (Y1 < Y2) && (posX <= sC.right()) )
        {
            posX += 1.0;
            Y1 = sF.y + ( (posX - sF.x) * k1 ) - h;
            Y2 = sC.bottom() - ((posX - sC.x) * k2);
        }
        posY = Y1;
        velX = std::max(velX, vF);
        return true;
    }
    else
    if( slT && slB && (idF==obj::SL_RightBottom) && (idC==obj::SL_RightTop) )
    {
        //RightBottom
        double Y1 = sF.y + ( (sF.right() - posX - w) * k1) - h;
        //RightTop
        double Y2 = sC.bottom() - ((sC.right() - posX - w) * k2);
        while( (Y1 < Y2) && ( (posX + w) >= sC.left()) )
        {
            posX -= 1.0;
            Y1 = sF.y + ( (sF.right() - posX - w) * k1) - h;
            Y2 = sC.bottom() - ((sC.right() - posX - w) * k2);
        }
        posY = Y1;
        velX = std::min(velX, vF);
        return true;
    }

    else
    if( slT && slB && (idF==obj::SL_RightBottom) && (idC==obj::SL_LeftTop) && (k1 != k2) )
    {
        //RightBottom
        double Y1 = sF.y + ( (sF.right() - posX - w) * k1) - h;
        //LeftTop
        double Y2 = sC.bottom() - ((posX - sC.x) * k2);
        while( (Y1 < Y2) /*&& ( (posX + w) >= sC.left())*/ )
        {
            posX += (k1 > k2) ? -1.0 : 1.0;
            Y1 = sF.y + ( (sF.right() - posX - w) * k1) - h;
            Y2 = sC.bottom() - ((posX - sC.x) * k2);
        }
        posY = Y2;
        velX = (k1 > k2) ? std::min(velX, vF) : std::max(velX, vF);
        return true;
    }
    else
    if( slT && slB && (idF==obj::SL_LeftBottom) && (idC==obj::SL_RightTop) && (k1 != k2) )
    {
        //LeftBottom
        double Y1 = sF.y + ( (posX - sF.x) * k1 ) - h;
        //RightTop
        double Y2 = sC.bottom() - ((sC.right() - posX - w) * k2);
        while( (Y1 < Y2) /*&& ( (posX + w) >= sC.left())*/ )
        {
            posX += (k1 < k2) ? -1.0 : 1.0;
            Y1 = sF.y + ( (posX - sF.x) * k1 ) - h;
            Y2 = sC.bottom() - ((sC.right() - posX - w) * k2);
        }
        posY = Y2;
        velX = (k1 < k2) ? std::min(velX, vF) : std::max(velX, vF);
        return true;
    }
    /***************************Ceiling slope and horizontal floor*******************************/
    else
    if( slT && isBlockFloor(idF) && (idC==obj::SL_LeftTop) )
    {
        double Y1 = sF.y - h;
        double Y2 = sC.bottom() - ((posX - sC.x) * k2);
        while( (Y1 < Y2) && (posX <= sC.right()) )
        {
            posX += 1.0;
            Y2 = sC.bottom() - ((posX - sC.x) * k2);
        }
        posY = Y1;
        velX = std::max(velX, vF);
        return true;
    }
    else
    if( slT && isBlockFloor(idF) && (idC==obj::SL_RightTop) )
    {
        double Y1 = sF.y - h;
        double Y2 = sC.bottom() - ((sC.right() - posX - w) * k2);
        while( (Y1 < Y2) && ( (posX + w) >= sC.left()) )
        {
            posX -= 1.0;
            Y2 = sC.bottom() - ((sC.right() - posX - w) * k2);
        }
        posY = Y1;
        velX = std::min(velX, vF);
        return true;
    }
    /***************************Floor slope and horizontal ceiling*******************************/
    else
    if( slB && (idF==obj::SL_LeftBottom) && isBlockCeiling(idC) )
    {
        double Y1 = sF.y + ( (posX - sF.x) * k1 ) - h;
        double Y2 = sC.bottom();
        while( (Y1 < Y2) && (posX < sC.right()) )
        {
            posX += 1.0;
            Y1 = sF.y + ( (posX - sF.x) * k1 ) - h;
        }
        posY = Y2;
        velX = std::max(velX, vF);
        return true;
    }
    else
    if( slB && (idF==obj::SL_RightBottom) && isBlockCeiling(idC) )
    {
        double Y1 = sF.y + ( (sF.right() - posX - w) * k1) - h;
        double Y2 = sC.bottom();
        while( (Y1 < Y2) && ( (posX + w) > sC.left()) )
        {
            posX -= 1.0;
            Y1 = sF.y + ( (sF.right() - posX - w) * k1) - h;
        }
        posY = Y2;
        velX = std::min(velX, vF);
        return true;
    }

    return false;
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
    if(pl.m_y > this->height()-cameraY)
        pl.m_y = 64;

    bool doHit = false;
    bool doCliffCheck = false;
    bool xSpeedWasReversed=false;
    std::vector<obj*> l_clifCheck;
    std::vector<obj*> l_toBump;

    std::vector<obj*> l_possibleCrushers;

    QVector<obj*> l_contactL;
    QVector<obj*> l_contactR;
    QVector<obj*> l_contactT;
    QVector<obj*> l_contactB;

    obj* collideAtTop  = nullptr;
    obj* collideAtBottom = nullptr;
    obj* collideAtLeft  = nullptr;
    obj* collideAtRight = nullptr;

    bool doSpeedStack = true;
    double speedNum = 0.0;
    double speedSum = 0.0;

    bool blockSkip = false;
    int  blockSkipStartFrom = 0;
    int  blockSkipI = 0;
    double oldSpeedX = pl.m_velX;
    double oldSpeedY = pl.m_velY;

    for(i=0; i<objs.size(); i++)
    {
        if(blockSkip && (blockSkipI==i))
        {
            blockSkip = false;
            continue;
        }

        objs[i].m_bumped = false;
        contactAt = obj::Contact_None;
        /* ********************Collect blocks to hit************************ */
        if( figureTouch(pl, objs[i], -1.0, 0.0) )
        {
            if(pl.centerY() < objs[i].centerY())
            {
                l_clifCheck.push_back(&objs[i]);
            } else {
                l_toBump.push_back(&objs[i]);
            }
        }

        /* ****Collect extra candidates for a cliff detection on the slope******** */
        if(pl.m_onSlopeFloor)
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
        /* ************************************************************************* */

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
                        if( isBlockFloor(&objs[i]))
                        {
                    tipRectT://Impacted at top of block (bottom of player)
                            if((objs[i].m_blocked[pl.m_filterID]&obj::Block_TOP) == 0)
                                goto tipRectT_Skip;
                            pl.m_y = objs[i].m_y - pl.m_h;
                            pl.m_velY   = objs[i].m_velY;
                            pl.m_stand  = true;
                            pl.m_standOnYMovable = (objs[i].m_velY != 0.0);
                            if(doSpeedStack)
                            {
                                pl.m_velX   = pl.m_velX_source + objs[i].m_velX;
                                speedSum += objs[i].m_velX;
                                if(objs[i].m_velX != 0.0)
                                    speedNum += 1.0;
                            }
                            contactAt = obj::Contact_Top;
                            doCliffCheck = true;
                            l_clifCheck.push_back(&objs[i]);
                            l_contactB.push_back(&objs[i]);
                            collideAtBottom = &objs[i];
                            //objs[i].m_touch = contactAt;
                            if(pl.m_onSlopeCeiling)
                            {
                                if( findMinimalHeight(objs[i].m_id, objs[i].rect(), objs[i].m_velX,
                                                  pl.m_onSlopeCeilingShape, pl.m_onSlopeCeilingRect,
                                                  pl.m_w, pl.m_h, &pl.m_x, &pl.m_y, &pl.m_velX_source,
                                                  pl.m_onSlopeCeiling, pl.m_onSlopeFloor) )
                                {
                                    pl.m_velX = pl.m_velX_source;
                                    doSpeedStack = false;
                                    speedSum = 0.0;
                                    speedNum = 0.0;
                                }
                            }
                        }
                    tipRectT_Skip:;
                    } else {
                        //'bottom
                        if( isBlockCeiling(&objs[i]) )
                        {
                    tipRectB://Impacted at bottom of block (top of player)
                            if((objs[i].m_blocked[pl.m_filterID]&obj::Block_BOTTOM) == 0)
                                goto tipRectB_Skip;

                            /* ************************************************************
                             * Aligned contact check to allow catching hole on the ceiling
                             * by thrown up NPCs. This makes inability to catch hole on the
                             * wall while flying up. But still be able catch hole on the wall
                             * while falling down.
                             * Ignore this code part when gravitation is directed to up
                             **************************************************************/
                            if(round(objs[i].right())-1.0 < round(pl.left()))
                            {
                                pl.m_x = round(pl.m_x);
                                goto tipRectB_Skip;
                            }
                            if(round(objs[i].left())+1.0 > round(pl.right()))
                            {
                                pl.m_x = round(pl.m_x);
                                goto tipRectB_Skip;
                            }
                            /* *************************************************************/

                            pl.m_y = objs[i].m_y + objs[i].m_h;
                            pl.m_velY = objs[i].m_velY;
                            contactAt = obj::Contact_Bottom;
                            doHit = true;
                            collideAtTop = &objs[i];
                            l_contactT.push_back(&objs[i]);
                            if(pl.m_onSlopeFloor)
                            {
                                if( findMinimalHeight(pl.m_onSlopeFloorShape, pl.m_onSlopeFloorRect, 0.0,
                                                      objs[i].m_id, objs[i].rect(),
                                                      pl.m_w, pl.m_h, &pl.m_x, &pl.m_y, &pl.m_velX_source,
                                                      pl.m_onSlopeCeiling, pl.m_onSlopeFloor) )
                                {
                                    doSpeedStack = false;
                                    pl.m_velX = pl.m_velX_source;
                                    speedSum = 0.0;
                                    speedNum = 0.0;
                                }
                            }
                    tipRectB_Skip:;
                        }
                    }
                } else {
                tipRectH://Check horisontal sides collision
                    if( pl.centerX() < objs[i].centerX() )
                    {
                        //'left
                        if( isBlockLeftWall(&objs[i]) )
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
                    tipRectL://Impacted at left (right of player)
                            {
                                if((objs[i].m_blocked[pl.m_filterID]&obj::Block_LEFT) == 0)
                                    goto tipRectL_Skip;

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
                                doSpeedStack = false;
                                speedSum = 0.0;
                                speedNum = 0.0;
                                contactAt = obj::Contact_Left;
                                collideAtRight = &objs[i];
                                l_contactR.push_back(&objs[i]);
                            }
                    tipRectL_Skip:;
                        }
                    } else {
                        //'right
                        if( isBlockRightWall(&objs[i]) )
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
                    tipRectR://Impacted at right (left of player)
                            {
                                if((objs[i].m_blocked[pl.m_filterID]&obj::Block_RIGHT) == 0)
                                    goto tipRectR_Skip;

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
                                doSpeedStack = false;
                                speedSum = 0.0;
                                speedNum = 0.0;
                                contactAt = obj::Contact_Right;
                                collideAtLeft = &objs[i];
                                l_contactL.push_back(&objs[i]);
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
                    //Avoid infinite loop for case of skipped collision resolving
                    contactAt=obj::Contact_Skipped;

                    k = objs[i].m_h / objs[i].m_w;
                    switch(objs[i].m_id)
                    {
                    case obj::SL_LeftBottom:
                        /* *************** Resolve collision with corner on the top ********************************* */
                        if( (pl.left() <= objs[i].left()) && (pl.m_onSlopeFloorTopAlign || (pl.m_velY >= 0.0) ) )
                        {
                            if( pl.bottom() > objs[i].bottom())
                                goto tipRectB;
                            if( (pl.bottom() >= objs[i].top()) && ((pl.left() < objs[i].left()) || (pl.m_velX <= 0.0)) )
                                goto tipRectT;
                        }
                        /* *************** Resolve collision with footer corner on right bottom side **************** */
                        else if( ( pl.bottom() > objs[i].bottom() ) &&
                                ((!pl.m_onSlopeFloorOld && (pl.bottomOld() > objs[i].bottomOld())) ||
                                  ((pl.m_onSlopeFloorShape != objs[i].m_id)&&(pl.m_onSlopeFloorShape >=0))
                                 ))
                        {
                            if(!colH)
                            {
                                if( objs[i].betweenH(pl.left(), pl.right()) && (pl.m_velY >= 0.0) )
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
                        /* ********************* Resolve collision with top slope surface *************************** */
                        else if( pl.bottom() > objs[i].m_y + ( (pl.m_x - objs[i].m_x) * k) - 1 )
                        {
                            if((objs[i].m_blocked[pl.m_filterID]&obj::Block_TOP) == 0)
                                goto skipTriangleResolving;

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
                            if(doSpeedStack)
                            {
                                pl.m_velX = pl.m_velX_source + objs[i].m_velX;
                                speedSum += objs[i].m_velX;
                                if(objs[i].m_velX != 0.0)
                                    speedNum += 1.0;
                            }
                            contactAt = obj::Contact_Top;
                            doCliffCheck = true;
                            l_clifCheck.push_back(&objs[i]);
                            l_contactB.push_back(&objs[i]);
                            if(pl.m_onSlopeCeiling)
                            {
                                if( findMinimalHeight(objs[i].m_id, objs[i].rect(), objs[i].m_velX,
                                                  pl.m_onSlopeCeilingShape, pl.m_onSlopeCeilingRect,
                                                  pl.m_w, pl.m_h, &pl.m_x, &pl.m_y, &pl.m_velX_source,
                                                  pl.m_onSlopeCeiling, pl.m_onSlopeFloor) )
                                {
                                    doSpeedStack = false;
                                    pl.m_velX = pl.m_velX_source;
                                    speedSum = 0.0;
                                    speedNum = 0.0;
                                }
                            }
                        }
                        break;
                    case obj::SL_RightBottom:
                        /* *************** Resolve collision with corner on the right top ************************* */
                        if( pl.right() >= objs[i].right() && (pl.m_onSlopeFloorTopAlign || (pl.m_velY >= 0.0) ))
                        {
                            if( pl.bottom() > objs[i].bottom())
                                goto tipRectB;
                            if( (pl.bottom() >= objs[i].top()) && ((pl.right() > objs[i].right()) || (pl.m_velX >= 0.0)) )
                                goto tipRectT;
                        }
                        /* ************** Resolve collision with footer corner on left bottom side **************** */
                        else if( ( pl.bottom() > objs[i].bottom() ) &&
                                ((!pl.m_onSlopeFloorOld && (pl.bottomOld() > objs[i].bottomOld())) ||
                                  ((pl.m_onSlopeFloorShape != objs[i].m_id)&&(pl.m_onSlopeFloorShape >=0))
                                 ))
                        {
                            if(!colH)
                            {
                                if( objs[i].betweenH(pl.left(), pl.right()) && (pl.m_velY >= 0.0) )
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
                        /* ********************* Resolve collision with top slope surface *************************** */
                        else if(pl.bottom() > objs[i].m_y + ((objs[i].right() - pl.m_x - pl.m_w) * k) - 1 )
                        {
                            if((objs[i].m_blocked[pl.m_filterID]&obj::Block_TOP) == 0)
                                goto skipTriangleResolving;

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
                            if(doSpeedStack)
                            {
                                pl.m_velX = pl.m_velX_source + objs[i].m_velX;
                                speedSum += objs[i].m_velX;
                                if(objs[i].m_velX != 0.0)
                                    speedNum += 1.0;
                            }
                            contactAt = obj::Contact_Top;
                            doCliffCheck = true;
                            l_clifCheck.push_back(&objs[i]);
                            l_contactB.push_back(&objs[i]);
                            if(pl.m_onSlopeCeiling)
                            {
                                if( findMinimalHeight(objs[i].m_id, objs[i].rect(), objs[i].m_velX,
                                                  pl.m_onSlopeCeilingShape, pl.m_onSlopeCeilingRect,
                                                  pl.m_w, pl.m_h, &pl.m_x, &pl.m_y, &pl.m_velX_source,
                                                  pl.m_onSlopeCeiling, pl.m_onSlopeFloor) )
                                {
                                    //pl.m_velX_source = 0.0;
                                    doSpeedStack = false;
                                    pl.m_velX = pl.m_velX_source;
                                    speedSum = 0.0;
                                    speedNum = 0.0;
                                }
                            }
                        }
                        break;
                    case obj::SL_LeftTop:
                        /* *************** Resolve collision with corner on the left bottom ************************* */
                        if( pl.left() <= objs[i].left() )
                        {
                            if( pl.top() < objs[i].top() )
                                goto tipRectT;
                            if( (pl.top() < objs[i].bottom()) && ((pl.left() < objs[i].left()) || (pl.m_velX <= 0.0)) )
                                goto tipRectB;
                        }
                        /* ****************** Resolve collision with upper corner on left top side ****************** */
                        else if( ( pl.top() < objs[i].top() )  &&
                                 ((!pl.m_onSlopeCeilingOld && (pl.topOld() < objs[i].topOld())) ||
                                   (pl.m_onSlopeCeilingShape != objs[i].m_id)) )
                        {
                            if(!colH)
                            {
                                if( objs[i].betweenH(pl.left(), pl.right()) && (pl.m_velY <= 0.0) )
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
                        /* ********************* Resolve collision with bottom slope surface *************************** */
                        else if(pl.m_y < objs[i].bottom() - ((pl.m_x - objs[i].m_x) * k) )
                        {
                            if((objs[i].m_blocked[pl.m_filterID]&obj::Block_BOTTOM) == 0)
                                goto skipTriangleResolving;

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
                            contactAt = obj::Contact_Bottom;
                            doHit = true;
                            l_contactT.push_back(&objs[i]);
                            if(pl.m_onSlopeFloor)
                            {
                                if( findMinimalHeight(pl.m_onSlopeFloorShape, pl.m_onSlopeFloorRect, 0.0,
                                                      objs[i].m_id, objs[i].rect(),
                                                      pl.m_w, pl.m_h, &pl.m_x, &pl.m_y, &pl.m_velX_source,
                                                      pl.m_onSlopeCeiling, pl.m_onSlopeFloor) )
                                {
                                    //pl.m_velX_source = 0.0;
                                    doSpeedStack = false;
                                    pl.m_velX = pl.m_velX_source;
                                    speedSum = 0.0;
                                    speedNum = 0.0;
                                }
                            }
                        }
                        break;
                    case obj::SL_RightTop:
                        /* *************** Resolve collision with corner on the right bottom ************************* */
                        if(pl.right() >= objs[i].right())
                        {
                            if( pl.top() < objs[i].top())
                                goto tipRectT;
                            if( (pl.m_y < objs[i].bottom()) && ((pl.right() > objs[i].right()) || (pl.m_velX >= 0.0)) )
                                goto tipRectB;
                        }
                        /* ****************** Resolve collision with upper corner on right top side ****************** */
                        else if( ( pl.top() < objs[i].top() )  &&
                                 ((!pl.m_onSlopeCeilingOld && (pl.topOld() < objs[i].topOld())) ||
                                   (pl.m_onSlopeCeilingShape != objs[i].m_id)) )
                        {
                            if(!colH)
                            {
                                if( objs[i].betweenH(pl.left(), pl.right()) && (pl.m_velY <= 0.0) )
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
                        /* ********************* Resolve collision with bottom slope surface *************************** */
                        else if(pl.m_y < objs[i].bottom() - ((objs[i].right() - pl.m_x - pl.m_w) * k))
                        {
                            if((objs[i].m_blocked[pl.m_filterID]&obj::Block_BOTTOM) == 0)
                                goto skipTriangleResolving;

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
                            contactAt = obj::Contact_Bottom;
                            doHit = true;
                            l_contactT.push_back(&objs[i]);
                            if(pl.m_onSlopeFloor)
                            {
                                if( findMinimalHeight(pl.m_onSlopeFloorShape, pl.m_onSlopeFloorRect, 0.0f,
                                                      objs[i].m_id, objs[i].rect(),
                                                      pl.m_w, pl.m_h, &pl.m_x, &pl.m_y, &pl.m_velX_source,
                                                      pl.m_onSlopeCeiling, pl.m_onSlopeFloor) )
                                {
                                    //pl.m_velX_source = 0.0;
                                    doSpeedStack = false;
                                    pl.m_velX = pl.m_velX_source;
                                    speedSum = 0.0;
                                    speedNum = 0.0;
                                }
                            }
                        }
                        break;
                    default:
                        break;
                    }

                    /* ***** If slope block being detected, restart loop from begin and then skip this element *******
                     * this is VERY important, because just resolved ceiling or floor collision will be missed
                     * after resolving of the slope collision which makes glitch.
                     * To have accurate result need re-check collisions with all previous elements
                     * after resolving slope collision
                     * ***********************************************************************************************/
                    if( (contactAt != obj::Contact_None) && (contactAt != obj::Contact_Skipped) )
                    {
                        blockSkipI = i;
                        i = blockSkipStartFrom;
                        blockSkipStartFrom = blockSkipI;
                        blockSkip = true;
                    }
                }
            skipTriangleResolving:;

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

        /* ********************Find wall touch blocks************************ */
        if(  figureTouch(pl, objs[i], 0.0, -1.0) &&
            !figureTouch(pl, objs[i], 0.0, 0.0) )
        {
            if( pl.betweenV( objs[i].centerY() ) )
            {
                if((pl.right() <= objs[i].left()) &&
                   isBlockLeftWall(objs[i].m_id) &&
                    ((objs[i].m_blocked[pl.m_filterID]&obj::Block_LEFT) != 0) &&
                    (oldSpeedX >= objs[i].m_velX) )
                {
                    //objs[i].m_touch = obj::Contact_Left;
                    pl.m_touchRightWall = true;
                }
                else
                if( (pl.left() >= objs[i].right()) &&
                    isBlockRightWall(objs[i].m_id) &&
                    ((objs[i].m_blocked[pl.m_filterID]&obj::Block_RIGHT) != 0) &&
                    (oldSpeedX <= objs[i].m_velX))
                {
                    //objs[i].m_touch = obj::Contact_Right;
                    pl.m_touchLeftWall = true;
                }
            }

            if( ((contactAt == obj::Contact_None) || (contactAt == obj::Contact_Skipped)) &&//Don't push duplicates
                pl.betweenV(pl.top(), pl.bottom()))
            {
                if( (pl.right() <= objs[i].left()) &&
                     isBlockLeftWall(objs[i].m_id) &&
                    ((objs[i].m_blocked[pl.m_filterID]&obj::Block_LEFT) != 0) &&
                    (oldSpeedX >= objs[i].m_velX) &&
                    pl.m_keys.right)
                {
                    l_contactR.push_back(&objs[i]);
                }
                else
                if( (pl.left() >= objs[i].right()) &&
                    isBlockRightWall(objs[i].m_id) &&
                    ((objs[i].m_blocked[pl.m_filterID]&obj::Block_RIGHT) != 0) &&
                    (oldSpeedX <= objs[i].m_velX) &&
                    pl.m_keys.left)
                {
                    l_contactL.push_back(&objs[i]);
                }
            }
        }

        /* ***********Find touching blocks (needed to process danger surfaces)*********** */
        if( ((contactAt == obj::Contact_None) || (contactAt == obj::Contact_Skipped)) &&//Don't push duplicates
            figureTouch(pl, objs[i], -1.0, 0.0) &&
            !figureTouch(pl, objs[i], 0.0, 0.0) )
        {
            if(pl.betweenH(pl.left(), pl.right()))
            {
                if( (pl.bottom() <= objs[i].top()) &&
                    isBlockFloor(objs[i].m_id) &&
                    ((objs[i].m_blocked[pl.m_filterID]&obj::Block_TOP) != 0))
                {
                    l_contactB.push_back(&objs[i]);
                }
                else
                if( (pl.top() >= objs[i].bottom()) &&
                    ( isBlockCeiling(objs[i].m_id) /*||
                      ( (objs[i].m_id == obj::SL_RightTop) && pl.betweenH(objs[i].left()) )||
                      ( (objs[i].m_id == obj::SL_LeftTop) && pl.betweenH(objs[i].right()) )*/
                     ) &&
                    ((objs[i].m_blocked[pl.m_filterID]&obj::Block_BOTTOM) != 0) &&
                    (oldSpeedY < objs[i].m_velY) )
                {
                    l_contactT.push_back(&objs[i]);
                }
            }
        }

        if(td == 1)
        {
            tm = -1;
            break;
        }

        if( (objs[i].m_id == obj::SL_Rect) &&
            (objs[i].m_blocked[pl.m_filterID]==obj::Block_ALL) &&
            recttouch(pl.m_oldx,      pl.m_oldy,        pl.m_w,      pl.m_h,
                      objs[i].m_oldx, objs[i].m_oldy,   objs[i].m_w, objs[i].m_h) )
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

    if(pl.m_crushed && pl.m_crushedOld )
    {
        /*HELP ME TO AVOID THIS CRAP!!!!*/
        if(pl.m_stand)
        {
            doSpeedStack = false;
            pl.m_velX_source = 0.0;
            pl.m_velX = 0.0;
            pl.m_x += 8.0;
        }
    }

    /* ***********************Hit a block********************************** */
    if(doHit && !l_toBump.empty())
    {
        obj*candidate = nullptr;
        for(unsigned int bump=0; bump<l_toBump.size(); bump++)
        {
            obj* x = l_toBump[bump];
            if(candidate == x)
                continue;
            if(!candidate)
                candidate = x;
            if( x->betweenH(pl.centerX()) )
            {
                candidate = x;
            }
        }
        if(candidate)
            candidate->m_bumped = true;
    }

    /* ***********************Detect a cliff********************************** */
    if(doCliffCheck && !l_clifCheck.empty())
    {
        obj* candidate = l_clifCheck[0];
        double lefter  = candidate->left();
        double righter = candidate->right();
        findHorizontalBoundaries(l_clifCheck, lefter, righter);
        if((pl.m_velX_source <= 0.0) && (lefter >= pl.centerX()) )
            pl.m_cliff = true;
        if((pl.m_velX_source >= 0.0) && (righter <= pl.centerX()) )
            pl.m_cliff = true;
    } else {
        if(!pl.m_stand)
            l_clifCheck.clear();
    }

    /* *********************** Check all collided sides ***************************** */
    for(int i=0; i<l_contactL.size(); i++)
    {
        if(l_contactL[i]->betweenV(pl.top()+1.0, pl.bottom()-1.0))
           l_contactL[i]->m_touch = obj::Contact_Right;
        else
        {
            l_contactL.erase(l_contactL.begin()+i);
            i--;
        }
    }
    for(int i=0; i<l_contactR.size(); i++)
    {
        if(l_contactR[i]->betweenV(pl.top()+1.0, pl.bottom()-1.0))
           l_contactR[i]->m_touch = obj::Contact_Left;
        else
        {
            l_contactR.erase(l_contactR.begin()+i);
            i--;
        }
    }
    for(int i=0; i<l_contactT.size(); i++)
    {
        if(l_contactT[i]->betweenH(pl.left()+1.0, pl.right()-1.0))
           l_contactT[i]->m_touch = obj::Contact_Bottom;
        else
        {
            l_contactT.erase(l_contactT.begin()+i);
            i--;
        }
    }
    for(int i=0; i<l_contactB.size(); i++)
    {
        if(l_contactB[i]->betweenH(pl.left()+1.0, pl.right()-1.0))
           l_contactB[i]->m_touch = obj::Contact_Top;
        else
        {
            l_contactB.erase(l_contactB.begin()+i);
            i--;
        }
    }

    /* ****************************Detection of the crush****************************** */
    if(collideAtBottom && collideAtTop)
    {
        //If character got crushed between moving layers
        if(collideAtBottom->m_velY < collideAtTop->m_velY )
        {
            pl.m_stand = false;
            pl.m_y = collideAtTop->bottom();
            pl.m_velY = collideAtTop->m_velY;
            doSpeedStack = false;
            speedNum = 0.0;
            speedSum = 0.0;
            #ifdef STOP_LOOP_ON_CRUSH
            alive = false;
            #endif
            if( (collideAtTop->m_blocked[pl.m_filterID]==obj::Block_ALL) &&
                (collideAtBottom->m_blocked[pl.m_filterID]==obj::Block_ALL) )
            {
                printf("CRUSHED BETWEEN VERTICAL!!!\n");
                fflush(stdout);
            }
        }
    }

    if(collideAtLeft && collideAtRight)
    {
        //If character got crushed between moving layers
        if(collideAtRight->m_velX < collideAtLeft->m_velX )
        {
            #ifdef STOP_LOOP_ON_CRUSH
            alive = false;
            #endif
            if( (collideAtLeft->m_blocked[pl.m_filterID]==obj::Block_ALL) &&
                (collideAtRight->m_blocked[pl.m_filterID]==obj::Block_ALL) )
            {
                printf("CRUSHED BETWEEN HORIZONTAL!!!\n");
                fflush(stdout);
            }
        }
    }
    /* ****************************************************************************** */

    if( doSpeedStack && (speedNum > 1.0) && (speedSum != 0.0) )
    {
        pl.m_velX = pl.m_velX_source + (speedSum/speedNum);
    }

}

void MiniPhysics::loop()
{
    #ifdef STOP_LOOP_ON_CRUSH
    if(alive)
    {
    #endif
        iterateStep();
        processCollisions();
    #ifdef STOP_LOOP_ON_CRUSH
    }
    #endif
    cameraX = pl.centerX()-width()/2.0;
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
    p.setPen(QPen(QColor(Qt::black), 1,
                  Qt::SolidLine,
                  Qt::SquareCap,
                  Qt::BevelJoin));
    p.setFont(m_font);
    for(int i=0; i<objs.size(); i++)
    {
        objs[i].paint(p, cameraX, cameraY);
    }
    pl.paint(p, cameraX, cameraY);
    p.drawText(10, 10, QString("Hole-Run = %1").arg(pl.m_allowHoleRuning) );
    p.drawText(10, 30, QString("Align on top-slope = %1").arg(pl.m_onSlopeFloorTopAlign) );
}

