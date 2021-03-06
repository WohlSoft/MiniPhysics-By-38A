#include "miniphysics.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMessageBox>
#include <QApplication>
#include <QFile>
#include <assert.h>
#include <vector>

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
    pl.m_momentum.w = 24;
    pl.m_momentum.h = 30;
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

    pl.m_shape = physBody::SL_Rect;
    pl.m_momentum.x = file.players[0].x;
    pl.m_momentum.y = file.players[0].y;
    pl.m_momentum.oldx = pl.m_momentum.x;
    pl.m_momentum.oldy = pl.m_momentum.y;
    //pl.m_posRect.setPos(file.players[0].x, file.players[0].y);
    //pl.m_posRectOld = pl.m_posRect;
    pl.m_drawSpeed = true;

    for(int i=0; i<file.blocks.size(); i++)
    {
        int id = 0;
        int blockSide = physBody::Block_ALL;
        LevelBlock& blk = file.blocks[i];
        switch(blk.id)
        {
            case 358: case 357: case 321: id = physBody::SL_RightBottom;   break;
            case 359: case 360: case 319: id = physBody::SL_LeftBottom;    break;
            case 363: case 364: id = physBody::SL_LeftTop;       break;
            case 362: case 361: id = physBody::SL_RightTop;      break;
            case 365:
                id = physBody::SL_RightBottom;
                blockSide = physBody::Block_TOP; break;
            case 366:
                id = physBody::SL_LeftBottom;
                blockSide = physBody::Block_TOP; break;
            case 367:
                id = physBody::SL_LeftTop;
                blockSide = physBody::Block_BOTTOM; break;
            case 368:
                id = physBody::SL_RightTop;
                blockSide = physBody::Block_BOTTOM; break;
            case 495:
                blockSide = physBody::Block_RIGHT;
                id = physBody::SL_Rect; break;
            case 25: case 575:
                blockSide = physBody::Block_TOP;
                id = physBody::SL_Rect; break;
            case 500:
                blockSide = physBody::Block_BOTTOM;
                id = physBody::SL_Rect; break;
            default:    id = physBody::SL_Rect;                  break;
        }
        physBody box(blk.x, blk.y, id);
        box.m_momentum.w = blk.w;
        box.m_momentum.h = blk.h;
        box.m_momentum.oldw = blk.w;
        box.m_momentum.oldh = blk.h;
        box.m_blocked[0] = blockSide;
        box.m_blocked[1] = blockSide;
        objs.push_back(box);
        if(blk.id == 159)
            movingBlock.push_back(objs.size()-1);
    }


    {
        physBody &brick1 = objs[movingBlock[3]];
        brick1.m_momentum.velX = 0.8;
        physBody &brick2 = objs[movingBlock[2]];
        brick2.m_momentum.velY = 1.0;
        physBody &brick3 = objs[movingBlock[1]];
        brick3.m_momentum.velX = 0.8;
        physBody &brick4 = objs[movingBlock[0]];
        brick4.m_momentum.velX = 1.6;
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
        //unsigned int lastID  =  movingBlock.size()-1;
        physBody &brick1 = objs[movingBlock[3]];

        brick1.m_momentum.oldx = brick1.m_momentum.x;
        brick1.m_momentum.oldy = brick1.m_momentum.y;
        brick1.m_momentum.x   += brick1.m_momentum.velX;
        brick1.m_momentum.y   += brick1.m_momentum.velY;

        brick1.m_momentum.velX = sin(brick1Passed)*2.0;
        brick1.m_momentum.velY = cos(brick1Passed)*2.0;
        brick1Passed += 0.07;

        physBody &brick2 = objs[movingBlock[2]];
        brick2.m_momentum.oldy = brick2.m_momentum.y;
        brick2.m_momentum.y   += brick2.m_momentum.velY;
        brick2Passed += brick2.m_momentum.velY;
        if(brick2Passed > 8.0*32.0 || brick2Passed < 0.0)
            brick2.m_momentum.velY *= -1.0;

        physBody &brick3 = objs[movingBlock[1]];
        brick3.m_momentum.oldx = brick3.m_momentum.x;
        brick3.m_momentum.x   += brick3.m_momentum.velX;
        brick3Passed += brick3.m_momentum.velX;
        if(brick3Passed > 9.0*32.0 || brick3Passed < 0.0)
            brick3.m_momentum.velX *= -1.0;

        physBody &brick4 = objs[movingBlock[0]];
        brick4.m_momentum.oldx = brick4.m_momentum.x;
        brick4.m_momentum.x   += brick4.m_momentum.velX;
        brick4Passed += brick4.m_momentum.velX;
        if(brick4Passed > 10.0*32.0 || brick4Passed < 0.0)
            brick4.m_momentum.velX *= -1.0;
    }

    { //With pl
        //Send controlls
        pl.m_keys.left  = keyMap[Qt::Key_Left];
        pl.m_keys.right = keyMap[Qt::Key_Right];
        pl.m_keys.jump  = keyMap[Qt::Key_Space];

        //Iterate step
        pl.iterateStep();

        //Process transforms and game logic
        if(keyMap[Qt::Key_1])
        {
            keyMap[Qt::Key_1]=false;
            pl.m_momentum.y += pl.m_momentum.h-30;
            pl.m_momentum.w = 24;
            pl.m_momentum.h = 30;
        }
        if(keyMap[Qt::Key_2])
        {
            keyMap[Qt::Key_2]=false;
            pl.m_momentum.y += pl.m_momentum.h-32;
            pl.m_momentum.w = 32;
            pl.m_momentum.h = 32;
        }
        if(keyMap[Qt::Key_3])
        {
            keyMap[Qt::Key_3]=false;
            pl.m_momentum.y += pl.m_momentum.h-50;
            pl.m_momentum.w = 24;
            pl.m_momentum.h = 50;
        }
    }
}

void physBody::iterateStep()
{
    /*****************Apply accelerations and iterate movement********************/
    double Xmod = 0;
    if(m_keys.left ^ m_keys.right)
    {
        if( (m_keys.left && m_momentum.velXsrc > -6) && !m_touchLeftWall )
            Xmod -= 0.4;
        if( (m_keys.right && m_momentum.velXsrc < 6) && !m_touchRightWall )
            Xmod += 0.4;
    }
    else if( !m_keys.left && !m_keys.right)
    {
        if(fabs(m_momentum.velXsrc) > 0.4)
        {
            Xmod -= sgn(m_momentum.velXsrc)*0.4;
        }
        else
        {
            Xmod = -m_momentum.velXsrc;
        }
    }
    m_momentum.velX    += Xmod;
    m_momentum.velXsrc += Xmod;

    if(!m_stand)
        m_momentum.velX = m_momentum.velXsrc;


    /*****************Iteration game logic*******************/
    /*
     * For NPC's: ignore "stand" flag is "on-cliff" is true.
     * to allow catch floor holes Y velocity must not be zero!
     * However, non-zero velocity may cause corner stumbling on slopes
     *
     * For playables: need to allow runnung over floor holes
     * even width is smaller than hole
     */
    if( (m_momentum.velY < 8) &&
        (!m_stand || m_standOnYMovable ||
        (!m_allowHoleRuning && m_cliff && (m_momentum.velXsrc != 0.0)) )
       )
        m_momentum.velY += 0.4;

    m_momentum.saveOld();
    m_momentum.x += m_momentum.velX;
    m_momentum.y += m_momentum.velY;

    if(m_slopeFloor.has)
        m_momentum.y += m_onSlopeYAdd;

    /*****************Post-Iteration game logic*****************************/
    if(m_stand && m_keys.jump && !m_jumpPressed)
    {
        m_momentum.velY = -10; //'8
        m_jumpPressed = true;
        m_momentum.y += m_momentum.velY;
    }
    m_jumpPressed = m_keys.jump;

    m_moveLeft = m_keys.left;
    m_moveRight = m_keys.right;
}


typedef physBody PhysObject;

template <class TArray> void findHorizontalBoundaries(TArray &array, double &lefter, double &righter,
                                                      PhysObject**leftest=nullptr, PhysObject**rightest=nullptr)
{
    if(array.empty())
        return;
    for(unsigned int i=0; i < array.size(); i++)
    {
        PhysObject* x = array[i];
        if(x->m_momentum.left() < lefter)
        {
            lefter = x->m_momentum.left();
            if(leftest)
                *leftest = x;
        }
        if(x->m_momentum.right() > righter)
        {
            righter = x->m_momentum.right();
            if(rightest)
                *rightest = x;
        }
    }
}

template <class TArray> void findVerticalBoundaries(TArray &array, double &higher, double &lower,
                                                    PhysObject**highest=nullptr, PhysObject**lowerest=nullptr)
{
    if(array.isEmpty())
        return;
    for(int i=0; i < array.size(); i++)
    {
        PhysObject* x = array[i];
        if(x->m_momentum.y < higher)
        {
            higher = x->m_momentum.y;
            if(highest)
                *highest = x;
        }
        if(x->m_momentum.y + x->m_momentum.h > lower)
        {
            lower = x->m_momentum.y + x->m_momentum.h;
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

static inline bool figureTouch(PhysObject &pl, PhysObject& block, double marginV = 0.0, double marginH = 0.0)
{
    return recttouch(pl.m_momentum.x+marginH, pl.m_momentum.y+marginV, pl.m_momentum.w-(marginH*2.0), pl.m_momentum.h-(marginV*2.0),
                     block.m_momentum.x,      block.m_momentum.y,      block.m_momentum.w,            block.m_momentum.h);
}

inline bool isBlockFloor(int shape)
{
    return (shape == PhysObject::SL_Rect) ||
           (shape == PhysObject::SL_LeftTop) ||
           (shape == PhysObject::SL_RightTop);
}

inline bool isBlockCeiling(int shape)
{
    return (shape == PhysObject::SL_Rect) ||
           (shape == PhysObject::SL_LeftBottom) ||
           (shape == PhysObject::SL_RightBottom);
}

inline bool isBlockLeftWall(int shape)
{
    return (shape == PhysObject::SL_Rect) ||
           (shape == PhysObject::SL_LeftBottom) ||
           (shape == PhysObject::SL_LeftTop);
}

inline bool isBlockRightWall(int shape)
{
    return (shape == PhysObject::SL_Rect) ||
           (shape == PhysObject::SL_RightBottom) ||
           (shape == PhysObject::SL_RightTop);
}

inline bool isBlockFloor(PhysObject* block)
{
    return (block->m_shape == PhysObject::SL_Rect) ||
           (block->m_shape == PhysObject::SL_LeftTop) ||
           (block->m_shape == PhysObject::SL_RightTop);
}

inline bool isBlockCeiling(PhysObject* block)
{
    return (block->m_shape == PhysObject::SL_Rect) ||
           (block->m_shape == PhysObject::SL_LeftBottom) ||
           (block->m_shape == PhysObject::SL_RightBottom);
}

inline bool isBlockLeftWall(PhysObject* block)
{
    return (block->m_shape == PhysObject::SL_Rect) ||
           (block->m_shape == PhysObject::SL_LeftBottom) ||
           (block->m_shape == PhysObject::SL_LeftTop);
}

inline bool isBlockRightWall(PhysObject* block)
{
    return (block->m_shape == PhysObject::SL_Rect) ||
           (block->m_shape == PhysObject::SL_RightBottom) ||
           (block->m_shape == PhysObject::SL_RightTop);
}

/**
 * @brief Finds minimal available height between slope floor and slope ceiling (and between horizontal block and one of slopes)
 * @param idF shape of floor
 * @param sF rectangle of floor
 * @param vF velocity of floor
 * @param idC shape of ceiling
 * @param sC rectangle of ceilig
 * @param player Pointer to the player
 * @return true if slope floor<->ceiling collision has been detected and resolved. False if found nothing
 */
inline bool findMinimalHeight(int idF, PhysObject::objRect sF, double vF,
                              int idC, PhysObject::objRect sC, PhysObject* player)
{
    bool slT  =     player->m_slopeCeiling.has;
    bool slB  =     player->m_slopeFloor.has;
    bool &wallL =   player->m_blockedAtLeft;
    bool &wallR =   player->m_blockedAtRight;
    double &w =     player->m_momentum.w;
    double &h =     player->m_momentum.h;
    double &posX =  player->m_momentum.x;
    double &posY =  player->m_momentum.y;
    double &velX =  player->m_momentum.velXsrc;
    double k1   =   sF.h / sF.w;
    double k2   =   sC.h / sC.w;
    /***************************Ceiling and floor slopes*******************************/
    if( slT && slB && (idF==PhysObject::SL_LeftBottom) && (idC==PhysObject::SL_LeftTop) )
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
        wallL=true;
        return true;
    }
    else
    if( slT && slB && (idF==PhysObject::SL_RightBottom) && (idC==PhysObject::SL_RightTop) )
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
        wallR=true;
        return true;
    }

    else
    if( slT && slB && (idF==PhysObject::SL_RightBottom) && (idC==PhysObject::SL_LeftTop) && (k1 != k2) )
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
        wallL = (k1 <= k2);
        wallR = (k1 > k2);
        return true;
    }
    else
    if( slT && slB && (idF==PhysObject::SL_LeftBottom) && (idC==PhysObject::SL_RightTop) && (k1 != k2) )
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
        wallL = (k1 >= k2);
        wallR = (k1 < k2);
        return true;
    }
    /***************************Ceiling slope and horizontal floor*******************************/
    else
    if( slT && isBlockFloor(idF) && (idC==PhysObject::SL_LeftTop) )
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
        wallL=true;
        return true;
    }
    else
    if( slT && isBlockFloor(idF) && (idC==PhysObject::SL_RightTop) )
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
        wallR=true;
        return true;
    }
    /***************************Floor slope and horizontal ceiling*******************************/
    else
    if( slB && (idF==PhysObject::SL_LeftBottom) && isBlockCeiling(idC) )
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
        wallL=true;
        return true;
    }
    else
    if( slB && (idF==PhysObject::SL_RightBottom) && isBlockCeiling(idC) )
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
        wallR=true;
        return true;
    }

    return false;
}


void physBody::processCollisions(PGE_RenderList &objs)
{
    double k = 0;
    int tm=0, td=0;
    PhysObject::ContactAt contactAt = PhysObject::Contact_None;
    bool colH=false, colV=false;
    tm = -1;
    td = 0;

    bool doHit = false;
    bool doCliffCheck = false;
    bool xSpeedWasReversed = false;
    std::vector<PhysObject*> l_clifCheck;
    std::vector<PhysObject*> l_toBump;

    resetEvents();

    PhysObject* collideAtTop  = nullptr;
    PhysObject* collideAtBottom = nullptr;
    PhysObject* collideAtLeft  = nullptr;
    PhysObject* collideAtRight = nullptr;

    bool    doSpeedStack = true;
    double  speedNum = 0.0;
    double  speedSum = 0.0;

    bool blockSkip = false;
    PGE_SizeT   blockSkipStartFrom = 0;
    PGE_SizeT   blockSkipI = 0;
    double oldSpeedX = m_momentum.velX;
    double oldSpeedY = m_momentum.velY;

    PhysObject* CUR=nullptr;
    PGE_SizeT i = 0;

    for(i=0; i<objs.size(); i++)
    {
        CUR = &objs[i];
        if(blockSkip && (blockSkipI==i))
        {
            blockSkip = false;
            continue;
        }

        #ifdef IS_MINIPHYSICS_DEMO_PROGRAM
        CUR->m_bumped = false;
        #endif
        contactAt = PhysObject::Contact_None;
        /* ********************Collect blocks to hit************************ */
        if( figureTouch(*this, objs[i], -1.0, 0.0) )
        {
            if(m_momentum.centerY() < CUR->m_momentum.centerY())
            {
                l_clifCheck.push_back(CUR);
            } else {
                l_toBump.push_back(CUR);
            }
        }

        /* ****Collect extra candidates for a cliff detection on the slope******** */
        if(m_slopeFloor.has || m_slopeFloor.hasOld)
        {
            PhysObject::Momentum &mPlr = m_momentum;
            PhysObject::Momentum &mBlk = CUR->m_momentum;
            PhysObject::objRect &r1 = m_slopeFloor.rect;
            PhysObject::objRect  r2 = CUR->m_momentum.rect();
            if( (m_slopeFloor.shape == PhysObject::SL_LeftBottom) && (mPlr.velX >= 0.0) )
            {
                if( recttouch(mPlr.x + mPlr.w, mPlr.centerY(), mPlr.w, r2.h,
                              mBlk.x, mBlk.y, mBlk.w, mBlk.h)
                    &&
                      //is touching corners
                      ( ( (r1.x+r1.w) >= (r2.x-1.0) ) &&
                        ( (r1.x+r1.w) <= (r2.x+r2.w) ) &&
                        ( (r1.y+r1.h) >= (r2.y-1.0) ) &&
                        ( (r1.y+r1.h) <= (r2.y+1.0) ) ) &&
                        ( mBlk.top() > mPlr.bottom() ) )
                    l_clifCheck.push_back(CUR);
            }
            else
            if( (m_slopeFloor.shape == PhysObject::SL_RightBottom) && (mPlr.velX <= 0.0) )
            {
                if( recttouch(mPlr.x, mPlr.centerY(), mPlr.w, r2.h,
                              mBlk.x, mBlk.y, mBlk.w, mBlk.h)
                        &&
                          //is touching corners
                          ( ( (r1.x) >= (r2.x) ) &&
                            ( (r1.x) <= (r2.x+r2.w+1.0) ) &&
                            ( (r1.y+r1.h) >= (r2.y-1.0) ) &&
                            ( (r1.y+r1.h) <= (r2.y+1.0) ) ) )
                    l_clifCheck.push_back(CUR);
            }
        }
        /* ************************************************************************* */

        if( (m_momentum.x + m_momentum.w > CUR->m_momentum.x) && (CUR->m_momentum.x + CUR->m_momentum.w > m_momentum.x) )
        {
            if(m_momentum.y + m_momentum.h == CUR->m_momentum.y)
                goto tipRectV;
        }

    tipRectShape://Recheck rectangular collision
        if( pt(m_momentum.x, m_momentum.y, m_momentum.w, m_momentum.h,
               CUR->m_momentum.x, CUR->m_momentum.y, CUR->m_momentum.w, CUR->m_momentum.h))
        {
            colH = pt(m_momentum.x,     m_momentum.oldy,  m_momentum.w, m_momentum.oldh,     CUR->m_momentum.x,    CUR->m_momentum.oldy, CUR->m_momentum.w, CUR->m_momentum.oldh);
            colV = pt(m_momentum.oldx,  m_momentum.y,     m_momentum.oldw, m_momentum.h,     CUR->m_momentum.oldx, CUR->m_momentum.y,    CUR->m_momentum.oldw, CUR->m_momentum.h);
            if( colH ^ colV )
            {
                if(!colH)
                {
                tipRectV://Check vertical sides colllisions
                    if( m_momentum.centerY() < CUR->m_momentum.centerY() )
                    {
                        //'top
                        if( isBlockFloor(CUR))
                        {
                    tipRectT://Impacted at top of block (bottom of player)
                            if((CUR->m_blocked[m_filterID]&PhysObject::Block_TOP) == 0)
                                goto tipRectT_Skip;
                            m_momentum.y = CUR->m_momentum.y - m_momentum.h;
                            m_momentum.velY   = CUR->m_momentum.velY;
                            m_stand  = true;
                            m_standOnYMovable = (CUR->m_momentum.velY != 0.0);
                            if(doSpeedStack)
                            {
                                m_momentum.velX   = m_momentum.velXsrc + CUR->m_momentum.velX;
                                speedSum += CUR->m_momentum.velX;
                                if(CUR->m_momentum.velX != 0.0)
                                    speedNum += 1.0;
                            }
                            contactAt = PhysObject::Contact_Top;
                            doCliffCheck = true;
                            l_clifCheck.push_back(CUR);
                            l_contactB.push_back(CUR);
                            collideAtBottom = CUR;
                            //CUR->m_momentum.touch = contactAt;
                            if(m_slopeCeiling.has)
                            {
                                if( findMinimalHeight(CUR->m_shape, CUR->m_momentum.rect(), CUR->m_momentum.velX,
                                                  m_slopeCeiling.shape, m_slopeCeiling.rect, this) )
                                {
                                    m_momentum.velX = m_momentum.velXsrc;
                                    doSpeedStack = false;
                                    speedSum = 0.0;
                                    speedNum = 0.0;
                                }
                            }
                        }
                    tipRectT_Skip:;
                    } else {
                        //'bottom
                        if( isBlockCeiling(CUR) )
                        {
                    tipRectB://Impacted at bottom of block (top of player)
                            if((CUR->m_blocked[m_filterID]&PhysObject::Block_BOTTOM) == 0)
                                goto tipRectB_Skip;

                            /* ************************************************************
                             * Aligned contact check to allow catching hole on the ceiling
                             * by thrown up NPCs. This makes inability to catch hole on the
                             * wall while flying up. But still be able catch hole on the wall
                             * while falling down.
                             * Ignore this code part when gravitation is directed to up
                             **************************************************************/
                            if(round(CUR->m_momentum.right())-1.0 < round(m_momentum.left()))
                            {
                                m_momentum.x = round(m_momentum.x);
                                goto tipRectB_Skip;
                            }
                            if(round(CUR->m_momentum.left())+1.0 > round(m_momentum.right()))
                            {
                                m_momentum.x = round(m_momentum.x);
                                goto tipRectB_Skip;
                            }
                            /* *************************************************************/

                            m_momentum.y = CUR->m_momentum.y + CUR->m_momentum.h;
                            m_momentum.velY = CUR->m_momentum.velY;
                            contactAt = PhysObject::Contact_Bottom;
                            doHit = true;
                            collideAtTop = CUR;
                            l_contactT.push_back(CUR);
                            if(m_slopeFloor.has)
                            {
                                if( findMinimalHeight(m_slopeFloor.shape, m_slopeFloor.rect, 0.0,
                                                      CUR->m_shape, CUR->m_momentum.rect(), this) )
                                {
                                    doSpeedStack = false;
                                    m_momentum.velX = m_momentum.velXsrc;
                                    speedSum = 0.0;
                                    speedNum = 0.0;
                                }
                            }
                    tipRectB_Skip:;
                        }
                    }
                } else {
                tipRectH://Check horisontal sides collision
                    if( m_momentum.centerX() < CUR->m_momentum.centerX() )
                    {
                        //'left
                        if( isBlockLeftWall(CUR) )
                        {
                            if(m_slopeCeiling.hasOld || m_slopeFloor.hasOld)
                            {
                                PhysObject::objRect& rF = m_slopeFloor.rect;
                                PhysObject::objRect& rC = m_slopeCeiling.rect;
                                if( (CUR->m_momentum.top() == rF.top())  &&
                                    (CUR->m_momentum.right() <= (rF.left()+1.0)) &&
                                    (m_slopeFloor.shape == PhysObject::SL_LeftBottom))
                                    goto tipRectL_Skip;
                                if( (CUR->m_momentum.bottom() == rC.bottom()) &&
                                    (CUR->m_momentum.right() <= (rF.left()+1.0)) &&
                                    (m_slopeCeiling.shape == PhysObject::SL_LeftTop)  )
                                    goto tipRectL_Skip;
                            }
                    tipRectL://Impacted at left (right of player)
                            {
                                if((CUR->m_blocked[m_filterID]&PhysObject::Block_LEFT) == 0)
                                    goto tipRectL_Skip;

                                if(m_allowHoleRuning)
                                {
                                    if( (m_momentum.bottom() < (CUR->m_momentum.top() + 2.0)) &&
                                            (m_momentum.velY > 0.0) && (m_momentum.velX > 0.0 ) &&
                                            (fabs(m_momentum.velX) > fabs(m_momentum.velY)) )
                                        goto tipRectT;
                                }
                                m_momentum.x = CUR->m_momentum.x - m_momentum.w;
                                double &splr = m_momentum.velX;
                                double &sbox = CUR->m_momentum.velX;
                                xSpeedWasReversed = splr <= sbox;
                                splr = std::min( splr, sbox );
                                m_momentum.velXsrc = splr;
                                doSpeedStack = false;
                                speedSum = 0.0;
                                speedNum = 0.0;
                                contactAt = PhysObject::Contact_Left;
                                collideAtRight = CUR;
                                l_contactR.push_back(CUR);
                                m_blockedAtRight=true;
                            }
                    tipRectL_Skip:;
                        }
                    } else {
                        //'right
                        if( isBlockRightWall(CUR) )
                        {
                            if(m_slopeCeiling.hasOld || m_slopeFloor.hasOld)
                            {
                                objRect& rF = m_slopeFloor.rect;
                                objRect& rC = m_slopeCeiling.rect;
                                if( (CUR->m_momentum.top() == rF.top()) &&
                                    (CUR->m_momentum.left() >= (rF.right()-1.0)) &&
                                    (m_slopeFloor.shape == PhysObject::SL_RightBottom) )
                                    goto tipRectR_Skip;
                                if( (CUR->m_momentum.bottom() == rC.bottom()) &&
                                    (CUR->m_momentum.left() >= (rF.right()-1.0)) &&
                                    (m_slopeCeiling.shape == PhysObject::SL_RightTop) )
                                    goto tipRectR_Skip;
                            }
                    tipRectR://Impacted at right (left of player)
                            {
                                if((CUR->m_blocked[m_filterID]&PhysObject::Block_RIGHT) == 0)
                                    goto tipRectR_Skip;

                                if(m_allowHoleRuning)
                                {
                                    if( (m_momentum.bottom() < (CUR->m_momentum.top() + 2.0)) &&
                                            (m_momentum.velY > 0.0) && (m_momentum.velX < 0.0 ) &&
                                            (fabs(m_momentum.velX) > fabs(m_momentum.velY)) )
                                        goto tipRectT;
                                }
                                m_momentum.x = CUR->m_momentum.x + CUR->m_momentum.w;
                                double &splr = m_momentum.velX;
                                double &sbox = CUR->m_momentum.velX;
                                xSpeedWasReversed = splr >= sbox;
                                splr = std::max( splr, sbox );
                                m_momentum.velXsrc = splr;
                                doSpeedStack = false;
                                speedSum = 0.0;
                                speedNum = 0.0;
                                contactAt = PhysObject::Contact_Right;
                                collideAtLeft = CUR;
                                l_contactL.push_back(CUR);
                                m_blockedAtLeft=true;
                            }
                    tipRectR_Skip:;
                        }
                    }
                }

                if( (m_stand) || (m_momentum.velXsrc == 0.0) || xSpeedWasReversed)
                    tm = -2;

        tipTriangleShape://Check triangular collision
                if(contactAt == PhysObject::Contact_None)
                {
                    //Avoid infinite loop for case of skipped collision resolving
                    contactAt=PhysObject::Contact_Skipped;

                    k = CUR->m_momentum.h / CUR->m_momentum.w;
                    switch(CUR->m_shape)
                    {
                    case PhysObject::SL_LeftBottom:
                        /* *************** Resolve collision with corner on the top ********************************* */
                        if( (m_momentum.left() <= CUR->m_momentum.left()) && (m_onSlopeFloorTopAlign || (m_momentum.velY >= 0.0) ) )
                        {
                            if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_BOTTOM) != 0 ) &&
                                  (m_momentum.bottom() > CUR->m_momentum.bottom()) )
                                goto tipRectB;
                            if( CUR->m_blocked[m_filterID] == PhysObject::Block_ALL )
                            {
                                if(  (m_momentum.bottom() >= CUR->m_momentum.top()) &&
                                    ((m_momentum.left() < CUR->m_momentum.left()) || (m_momentum.velX <= 0.0)) )
                                    goto tipRectT;
                            } else {
                                if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_TOP) != 0 ) &&
                                    ( m_momentum.bottom() >= CUR->m_momentum.top() ) && ( m_momentum.bottomOld() <= CUR->m_momentum.topOld() ) &&
                                    ( (m_momentum.left() < CUR->m_momentum.left()) || (m_momentum.velX <= 0.0)) )
                                    goto tipRectT;
                            }
                        }
                        /* *************** Resolve collision with footer corner on right bottom side **************** */
                        else if( ( m_momentum.bottom() > CUR->m_momentum.bottom() ) &&
                                ((!m_slopeFloor.hasOld && (m_momentum.bottomOld() > CUR->m_momentum.bottomOld())) ||
                                  ((m_slopeFloor.shape != CUR->m_shape)&&(m_slopeFloor.shape >=0))
                                 ))
                        {
                            if(!colH)
                            {
                                if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_BOTTOM) != 0 ) &&
                                       CUR->m_momentum.betweenH(m_momentum.left(), m_momentum.right()) && (m_momentum.velY >= 0.0) )
                                    goto tipRectB;
                            } else {
                                if( m_momentum.centerX() < CUR->m_momentum.centerX() )
                                {
                                    if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_LEFT) != 0 ) &&
                                          (m_momentum.velX >= 0.0) )
                                        goto tipRectL;
                                }
                                else
                                {
                                    if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_RIGHT) != 0 ) &&
                                          (m_momentum.velX <= 0.0) )
                                        goto tipRectR;
                                }
                            }
                        }
                        /* ********************* Resolve collision with top slope surface *************************** */
                        else if( m_momentum.bottom() > CUR->m_momentum.y + ( (m_momentum.x - CUR->m_momentum.x) * k) - 1 )
                        {
                            if( (CUR->m_blocked[m_filterID] != PhysObject::Block_ALL) && (!m_slopeFloor.hasOld) )
                            {
                                if( (CUR->m_blocked[m_filterID]&PhysObject::Block_TOP) == 0 )
                                    goto skipTriangleResolving;
                                if( (CUR->m_blocked[m_filterID]&PhysObject::Block_BOTTOM) == 0 )
                                {
                                    if(m_momentum.velY < CUR->m_momentum.velY)
                                        goto skipTriangleResolving;
                                    if( (m_momentum.bottomOld() > CUR->m_momentum.oldy + ( (m_momentum.oldx - CUR->m_momentum.oldx) * k) - 0) )
                                        goto skipTriangleResolving;
                                }
                            }

                            m_momentum.y = CUR->m_momentum.y + ( (m_momentum.x - CUR->m_momentum.x) * k ) - m_momentum.h;
                            m_momentum.velY = CUR->m_momentum.velY;
                            m_slopeFloor.has = true;
                            m_slopeFloor.shape = CUR->m_shape;
                            m_slopeFloor.rect  = CUR->m_momentum.rect();
                            if( (m_momentum.velX > 0.0) || !m_onSlopeFloorTopAlign)
                            {
                                m_momentum.velY = CUR->m_momentum.velY +m_momentum.velX * k;
                                m_onSlopeYAdd = 0.0;
                            }
                            else
                            {
                                m_onSlopeYAdd = m_momentum.velX * k;
                                if((m_onSlopeYAdd < 0.0) && (m_momentum.bottom() + m_onSlopeYAdd < CUR->m_momentum.y))
                                    m_onSlopeYAdd = -fabs(m_momentum.bottom() - CUR->m_momentum.y);
                            }
                            m_stand = true;
                            m_standOnYMovable = (CUR->m_momentum.velY != 0.0);
                            if(doSpeedStack)
                            {
                                m_momentum.velX = m_momentum.velXsrc + CUR->m_momentum.velX;
                                speedSum += CUR->m_momentum.velX;
                                if(CUR->m_momentum.velX != 0.0)
                                    speedNum += 1.0;
                            }
                            contactAt = PhysObject::Contact_Top;
                            doCliffCheck = true;
                            l_clifCheck.push_back(CUR);
                            l_contactB.push_back(CUR);
                            if(m_slopeCeiling.has)
                            {
                                if( findMinimalHeight(CUR->m_shape, CUR->m_momentum.rect(), CUR->m_momentum.velX,
                                                  m_slopeCeiling.shape, m_slopeCeiling.rect, this) )
                                {
                                    doSpeedStack = false;
                                    m_momentum.velX = m_momentum.velXsrc;
                                    speedSum = 0.0;
                                    speedNum = 0.0;
                                }
                            }
                        }
                        break;
                    case PhysObject::SL_RightBottom:
                        /* *************** Resolve collision with corner on the right top ************************* */
                        if( m_momentum.right() >= CUR->m_momentum.right() && (m_onSlopeFloorTopAlign || (m_momentum.velY >= 0.0) ))
                        {
                            if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_BOTTOM) != 0 ) &&
                                  (m_momentum.bottom() > CUR->m_momentum.bottom()) )
                                goto tipRectB;
                            if( CUR->m_blocked[m_filterID] == PhysObject::Block_ALL )
                            {
                                if(  (m_momentum.bottom() >= CUR->m_momentum.top()) &&
                                    ((m_momentum.right() > CUR->m_momentum.right()) || (m_momentum.velX >= 0.0)) )
                                    goto tipRectT;
                            } else {
                                if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_TOP) != 0 ) &&
                                    ( m_momentum.bottom() >= CUR->m_momentum.top() ) && ( m_momentum.bottomOld() <= CUR->m_momentum.topOld() ) &&
                                    ((m_momentum.right() > CUR->m_momentum.right()) || (m_momentum.velX >= 0.0)) )
                                    goto tipRectT;
                            }
                        }
                        /* ************** Resolve collision with footer corner on left bottom side **************** */
                        else if( ( m_momentum.bottom() > CUR->m_momentum.bottom() ) &&
                                ((!m_slopeFloor.hasOld && (m_momentum.bottomOld() > CUR->m_momentum.bottomOld())) ||
                                  ((m_slopeFloor.shape != CUR->m_shape)&&(m_slopeFloor.shape >=0))
                                 ))
                        {
                            if(!colH)
                            {
                                if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_BOTTOM) != 0 ) &&
                                       CUR->m_momentum.betweenH(m_momentum.left(), m_momentum.right()) && (m_momentum.velY >= 0.0) )
                                    goto tipRectB;
                            } else {
                                if( m_momentum.centerX() < CUR->m_momentum.centerX() )
                                {
                                    if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_LEFT) != 0 ) &&
                                          (m_momentum.velX >= 0.0) )
                                        goto tipRectL;
                                }
                                else
                                {
                                    if(( (CUR->m_blocked[m_filterID]&PhysObject::Block_RIGHT) != 0 ) &&
                                         (m_momentum.velX <= 0.0) )
                                        goto tipRectR;
                                }
                            }
                        }
                        /* ********************* Resolve collision with top slope surface *************************** */
                        else if(m_momentum.bottom() > CUR->m_momentum.y + ((CUR->m_momentum.right() - m_momentum.x - m_momentum.w) * k) - 1 )
                        {
                            if((CUR->m_blocked[m_filterID] & PhysObject::Block_TOP) == 0)
                                goto skipTriangleResolving;

                            if( (CUR->m_blocked[m_filterID] != PhysObject::Block_ALL) && (!m_slopeFloor.hasOld) )
                            {
                                if( (CUR->m_blocked[m_filterID]&PhysObject::Block_TOP) == 0 )
                                    goto skipTriangleResolving;
                                if( (CUR->m_blocked[m_filterID]&PhysObject::Block_BOTTOM) == 0 )
                                {
                                    if(m_momentum.velY < CUR->m_momentum.velY)
                                        goto skipTriangleResolving;
                                    if( (m_momentum.bottomOld() > CUR->m_momentum.oldy + ((CUR->m_momentum.rightOld() - m_momentum.oldx - m_momentum.oldw) * k) - 0) )
                                        goto skipTriangleResolving;
                                }
                            }

                            m_momentum.y = CUR->m_momentum.y + ( (CUR->m_momentum.right() - m_momentum.x - m_momentum.w) * k) - m_momentum.h;
                            m_momentum.velY = CUR->m_momentum.velY;
                            m_slopeFloor.has = true;
                            m_slopeFloor.shape = CUR->m_shape;
                            m_slopeFloor.rect  = CUR->m_momentum.rect();
                            if( (m_momentum.velX < 0.0) || !m_onSlopeFloorTopAlign)
                            {
                                m_momentum.velY = CUR->m_momentum.velY -m_momentum.velX * k;
                                m_onSlopeYAdd = 0.0;
                            } else {
                                m_onSlopeYAdd = -m_momentum.velX * k;
                                if((m_onSlopeYAdd < 0.0) && (m_momentum.bottom() + m_onSlopeYAdd < CUR->m_momentum.y))
                                    m_onSlopeYAdd = -fabs(m_momentum.bottom() - CUR->m_momentum.y);
                            }
                            m_stand = true;
                            m_standOnYMovable = (CUR->m_momentum.velY != 0.0);
                            if(doSpeedStack)
                            {
                                m_momentum.velX = m_momentum.velXsrc + CUR->m_momentum.velX;
                                speedSum += CUR->m_momentum.velX;
                                if(CUR->m_momentum.velX != 0.0)
                                    speedNum += 1.0;
                            }
                            contactAt = PhysObject::Contact_Top;
                            doCliffCheck = true;
                            l_clifCheck.push_back(CUR);
                            l_contactB.push_back(CUR);
                            if(m_slopeCeiling.has)
                            {
                                if( findMinimalHeight(CUR->m_shape, CUR->m_momentum.rect(), CUR->m_momentum.velX,
                                                  m_slopeCeiling.shape, m_slopeCeiling.rect, this) )
                                {
                                    //m_velX_source = 0.0;
                                    doSpeedStack = false;
                                    m_momentum.velX = m_momentum.velXsrc;
                                    speedSum = 0.0;
                                    speedNum = 0.0;
                                }
                            }
                        }
                        break;
                    case PhysObject::SL_LeftTop:
                        /* *************** Resolve collision with corner on the left bottom ************************* */
                        if( m_momentum.left() <= CUR->m_momentum.left() )
                        {
                            if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_TOP) != 0 ) &&
                                (m_momentum.top() < CUR->m_momentum.top()) )
                                goto tipRectT;
                            if( CUR->m_blocked[m_filterID] == PhysObject::Block_ALL )
                            {
                                if( (m_momentum.top() <= CUR->m_momentum.bottom()) &&
                                   ((m_momentum.left() < CUR->m_momentum.left()) || (m_momentum.velX <= 0.0)) )
                                    goto tipRectB;
                            } else {
                                if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_BOTTOM) != 0 ) &&
                                   (m_momentum.top() < CUR->m_momentum.bottom()) && ( m_momentum.topOld() >= CUR->m_momentum.bottomOld() ) &&
                                   ((m_momentum.left() < CUR->m_momentum.left()) || (m_momentum.velX <= 0.0)) )
                                    goto tipRectB;
                            }
                        }
                        /* ****************** Resolve collision with upper corner on left top side ****************** */
                        else if( ( m_momentum.top() < CUR->m_momentum.top() )  &&
                                 ((!m_slopeCeiling.hasOld && (m_momentum.topOld() < CUR->m_momentum.topOld())) ||
                                   (m_slopeCeiling.shape != CUR->m_shape)) )
                        {
                            if(!colH)
                            {
                                if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_TOP) != 0 ) &&
                                    CUR->m_momentum.betweenH(m_momentum.left(), m_momentum.right()) && (m_momentum.velY <= 0.0) )
                                    goto tipRectT;
                            } else {
                                if( m_momentum.centerX() < CUR->m_momentum.centerX() )
                                {
                                    if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_LEFT) != 0 ) &&
                                        (m_momentum.velX >= 0.0) )
                                        goto tipRectL;
                                }
                                else
                                {
                                    if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_RIGHT) != 0 ) &&
                                        (m_momentum.velX <= 0.0))
                                        goto tipRectR;
                                }
                            }
                        }
                        /* ********************* Resolve collision with bottom slope surface *************************** */
                        else if(m_momentum.y < CUR->m_momentum.bottom() - ((m_momentum.x - CUR->m_momentum.x) * k) )
                        {
                            if( (CUR->m_blocked[m_filterID] != PhysObject::Block_ALL) && (!m_slopeCeiling.hasOld) )
                            {
                                if((CUR->m_blocked[m_filterID]&PhysObject::Block_BOTTOM) == 0)
                                    goto skipTriangleResolving;
                                if( (CUR->m_blocked[m_filterID]&PhysObject::Block_TOP) == 0 )
                                {
                                    if(m_momentum.velY > CUR->m_momentum.velY)
                                        goto skipTriangleResolving;
                                    if( m_momentum.topOld() < CUR->m_momentum.bottomOld() - ((m_momentum.oldx - CUR->m_momentum.oldx) * k) )
                                        goto skipTriangleResolving;
                                }
                            }

                            m_momentum.y    = CUR->m_momentum.bottom() - ((m_momentum.x - CUR->m_momentum.x) * k);
                            if( m_momentum.velX < 0.0)
                            {
                                m_momentum.velY = CUR->m_momentum.velY - m_momentum.velX * k;
                            } else {
                                m_momentum.velY = CUR->m_momentum.velY;
                            }
                            m_slopeCeiling.has = true;
                            m_slopeCeiling.shape = CUR->m_shape;
                            m_slopeCeiling.rect  = CUR->m_momentum.rect();
                            contactAt = PhysObject::Contact_Bottom;
                            doHit = true;
                            l_contactT.push_back(CUR);
                            if(m_slopeFloor.has)
                            {
                                if( findMinimalHeight(m_slopeFloor.shape, m_slopeFloor.rect, 0.0,
                                                      CUR->m_shape, CUR->m_momentum.rect(), this) )
                                {
                                    //m_velX_source = 0.0;
                                    doSpeedStack = false;
                                    m_momentum.velX = m_momentum.velXsrc;
                                    speedSum = 0.0;
                                    speedNum = 0.0;
                                }
                            }
                        }
                        break;
                    case PhysObject::SL_RightTop:
                        /* *************** Resolve collision with corner on the right bottom ************************* */
                        if(m_momentum.right() >= CUR->m_momentum.right())
                        {
                            if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_TOP) != 0 ) &&
                                (m_momentum.top() < CUR->m_momentum.top()) )
                                goto tipRectT;
                            if( CUR->m_blocked[m_filterID] == PhysObject::Block_ALL )
                            {
                                if( (m_momentum.y < CUR->m_momentum.bottom()) &&
                                    ((m_momentum.right() > CUR->m_momentum.right()) || (m_momentum.velX >= 0.0)) )
                                    goto tipRectB;
                            } else {
                                if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_BOTTOM) != 0 ) &&
                                   (m_momentum.top() < CUR->m_momentum.bottom()) && ( m_momentum.topOld() >= CUR->m_momentum.bottomOld() ) &&
                                   ((m_momentum.right() > CUR->m_momentum.right()) || (m_momentum.velX >= 0.0)) )
                                    goto tipRectB;
                            }
                        }
                        /* ****************** Resolve collision with upper corner on right top side ****************** */
                        else if( ( m_momentum.top() < CUR->m_momentum.top() )  &&
                                 ((!m_slopeCeiling.hasOld && (m_momentum.topOld() < CUR->m_momentum.topOld())) ||
                                   (m_slopeCeiling.shape != CUR->m_shape)) )
                        {
                            if(!colH)
                            {
                                if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_TOP) != 0 ) &&
                                    CUR->m_momentum.betweenH(m_momentum.left(), m_momentum.right()) && (m_momentum.velY <= 0.0) )
                                    goto tipRectT;
                            } else {
                                if( m_momentum.centerX() < CUR->m_momentum.centerX() )
                                {
                                    if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_LEFT) != 0 ) &&
                                        (m_momentum.velX >= 0.0))
                                        goto tipRectL;
                                }
                                else
                                {
                                    if( ( (CUR->m_blocked[m_filterID]&PhysObject::Block_RIGHT) != 0 ) &&
                                        (m_momentum.velX <= 0.0))
                                        goto tipRectR;
                                }
                            }
                        }
                        /* ********************* Resolve collision with bottom slope surface *************************** */
                        else if(m_momentum.y < CUR->m_momentum.bottom() - ((CUR->m_momentum.right() - m_momentum.x - m_momentum.w) * k))
                        {
                            if( (CUR->m_blocked[m_filterID] != PhysObject::Block_ALL) && (!m_slopeCeiling.hasOld) )
                            {
                                if((CUR->m_blocked[m_filterID]&PhysObject::Block_BOTTOM) == 0)
                                    goto skipTriangleResolving;
                                if( (CUR->m_blocked[m_filterID]&PhysObject::Block_TOP) == 0 )
                                {
                                    if(m_momentum.velY > CUR->m_momentum.velY)
                                        goto skipTriangleResolving;
                                    if( m_momentum.oldy < CUR->m_momentum.bottomOld() - ((CUR->m_momentum.rightOld() - m_momentum.oldx - m_momentum.oldw) * k) )
                                        goto skipTriangleResolving;
                                }
                            }

                            m_momentum.y    = CUR->m_momentum.bottom() - ((CUR->m_momentum.right() - m_momentum.x - m_momentum.w) * k);
                            if( m_momentum.velX > 0.0)
                            {
                                m_momentum.velY = CUR->m_momentum.velY + m_momentum.velX * k ;
                            } else {
                                m_momentum.velY = CUR->m_momentum.velY;
                            }
                            m_slopeCeiling.has = true;
                            m_slopeCeiling.shape = CUR->m_shape;
                            m_slopeCeiling.rect  = CUR->m_momentum.rect();
                            contactAt = PhysObject::Contact_Bottom;
                            doHit = true;
                            l_contactT.push_back(CUR);
                            if(m_slopeFloor.has)
                            {
                                if( findMinimalHeight(m_slopeFloor.shape, m_slopeFloor.rect, 0.0f,
                                                      CUR->m_shape, CUR->m_momentum.rect(), this) )
                                {
                                    doSpeedStack = false;
                                    m_momentum.velX = m_momentum.velXsrc;
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
                    if( (contactAt != PhysObject::Contact_None) && (contactAt != PhysObject::Contact_Skipped) )
                    {
                        blockSkipI = i;
                        i = blockSkipStartFrom;
                        CUR = &objs[i];
                        blockSkipStartFrom = blockSkipI;
                        blockSkip = true;
                    }
                }
            skipTriangleResolving:;

            } else {
                //If shape is not rectangle - check triangular collision
                if( CUR->m_shape != PhysObject::SL_Rect )
                    goto tipTriangleShape;

                //Catching 90-degree angle impacts of corners with rectangles
                if( !colH && !colV )
                {
                    if(tm == signed(i))
                    {
                        if( fabs(m_momentum.velY) > fabs(m_momentum.velX) )
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
        if(  figureTouch(*this, objs[i], 0.0, -1.0) &&
            !figureTouch(*this, objs[i], 0.0, 0.0) )
        {
            if( m_momentum.betweenV( CUR->m_momentum.centerY() ) )
            {
                if((m_momentum.right() <= CUR->m_momentum.left()) &&
                   isBlockLeftWall(CUR->m_shape) &&
                    ((CUR->m_blocked[m_filterID]&PhysObject::Block_LEFT) != 0) &&
                    (oldSpeedX >= CUR->m_momentum.velX) )
                {
                    //CUR->m_touch = obj::Contact_Left;
                    m_touchRightWall = true;
                    if(!collideAtRight)
                        collideAtRight = CUR;
                }
                else
                if( (m_momentum.left() >= CUR->m_momentum.right()) &&
                    isBlockRightWall(CUR->m_shape) &&
                    ((CUR->m_blocked[m_filterID]&PhysObject::Block_RIGHT) != 0) &&
                    (oldSpeedX <= CUR->m_momentum.velX))
                {
                    //CUR->m_touch = obj::Contact_Right;
                    m_touchLeftWall = true;
                    if(!collideAtLeft)
                        collideAtLeft = CUR;
                }
            }

            if( ((contactAt == PhysObject::Contact_None) || (contactAt == PhysObject::Contact_Skipped)) &&//Don't push duplicates
                m_momentum.betweenV(m_momentum.top(), m_momentum.bottom()))
            {
                if( (m_momentum.right() <= CUR->m_momentum.left()) &&
                     isBlockLeftWall(CUR->m_shape) &&
                    ((CUR->m_blocked[m_filterID]&PhysObject::Block_LEFT) != 0) &&
                    (oldSpeedX >= CUR->m_momentum.velX) &&
                    m_moveRight)
                {
                    l_contactR.push_back(CUR);
                }
                else
                if( (m_momentum.left() >= CUR->m_momentum.right()) &&
                    isBlockRightWall(CUR->m_shape) &&
                    ((CUR->m_blocked[m_filterID]&PhysObject::Block_RIGHT) != 0) &&
                    (oldSpeedX <= CUR->m_momentum.velX) &&
                    m_moveLeft)
                {
                    l_contactL.push_back(CUR);
                }
            }
        }

        /* ***********Find touching blocks (needed to process danger surfaces)*********** */
        if( ((contactAt == PhysObject::Contact_None) || (contactAt == PhysObject::Contact_Skipped)) &&//Don't push duplicates
            figureTouch(*this, objs[i], -1.0, 0.0) &&
            !figureTouch(*this, objs[i], 0.0, 0.0) )
        {
            if(m_momentum.betweenH(m_momentum.left(), m_momentum.right()))
            {
                if( (m_momentum.bottom() <= CUR->m_momentum.top()) &&
                    isBlockFloor(CUR->m_shape) &&
                    ((CUR->m_blocked[m_filterID]&PhysObject::Block_TOP) != 0))
                {
                    l_contactB.push_back(CUR);
                }
                else
                if( (m_momentum.top() >= CUR->m_momentum.bottom()) &&
                    ( isBlockCeiling(CUR->m_shape) /*||
                      ( (CUR->m_id == obj::SL_RightTop) && betweenH(CUR->left()) )||
                      ( (CUR->m_id == obj::SL_LeftTop) && betweenH(CUR->right()) )*/
                     ) &&
                    ((CUR->m_blocked[m_filterID]&PhysObject::Block_BOTTOM) != 0) &&
                    (oldSpeedY < CUR->m_momentum.velY) )
                {
                    l_contactT.push_back(CUR);
                }
            }
        }

        if(td == 1)
        {
            tm = -1;
            break;
        }

        if( (CUR->m_shape == PhysObject::SL_Rect) &&
            (CUR->m_blocked[m_filterID]==PhysObject::Block_ALL) &&
            recttouch(m_momentum.oldx,      m_momentum.oldy,        m_momentum.oldw,      m_momentum.oldh,
                      CUR->m_momentum.oldx, CUR->m_momentum.oldy,   CUR->m_momentum.oldw, CUR->m_momentum.oldh) )
        {
            //l_possibleCrushers.push_back(CUR);
            m_crushed = true;
        }
    }
    if(tm >= 0)
    {
        i = tm;
        CUR = &objs[i];
        td = 1;
        goto tipRectShape;
    }

    if(m_crushed && m_crushedOld )
    {
        if(m_stand)
        {
            /*HELP ME TO AVOID THIS CRAP!!!!*/
            doSpeedStack = false;
            m_momentum.velXsrc = 0.0;
            m_momentum.velX = 0.0;
            m_momentum.x += 8.0;
        }
    }

    /* ***********************Hit a block********************************** */
    if(doHit && !l_toBump.empty())
    {
        PhysObject*candidate = nullptr;
        for(unsigned int bump=0; bump<l_toBump.size(); bump++)
        {
            PhysObject* x = l_toBump[bump];
            if(candidate == x)
                continue;
            if(!candidate)
                candidate = x;
            if( x->m_momentum.betweenH(m_momentum.centerX()) )
            {
                candidate = x;
            }
        }
        if(candidate)
        {
            #ifdef IS_MINIPHYSICS_DEMO_PROGRAM
            candidate->m_bumped = true;
            #else
            if(CUR->type == LVLBlock)
            {
                LVL_Block* nearest = static_cast<LVL_Block*>(CUR);
                if(type==LVLPlayer)
                {
                    LVL_Player* plr = static_cast<LVL_Player*>(this);
                    processCharacterSwitchBlock(plr, nearest);//Do transformation if needed
                    long npcid=nearest->data.npc_id;
                    nearest->hit(LVL_Block::up, this, 1);
                    if( nearest->setup->setup.hitable ||
                        (npcid !=0 ) ||
                        (nearest->destroyed) ||
                        (nearest->setup->setup.bounce) )
                    {
                        plr->bump(false, speedY());
                    }
                }
            } else {
                PGE_Audio::playSoundByRole(obj_sound_role::BlockHit);
            }
            #endif
        }
    }

    /* ***********************Detect a cliff********************************** */
    if(doCliffCheck && !l_clifCheck.empty())
    {
        PhysObject* candidate = l_clifCheck[0];
        double lefter  = candidate->m_momentum.left();
        double righter = candidate->m_momentum.right();
        findHorizontalBoundaries(l_clifCheck, lefter, righter);
        if((m_momentum.velXsrc <= 0.0) && (lefter >= m_momentum.centerX()) )
            m_cliff = true;
        if((m_momentum.velXsrc >= 0.0) && (righter <= m_momentum.centerX()) )
            m_cliff = true;
    } else {
        if(!m_stand)
            l_clifCheck.clear();
    }

    /* *********************** Check all collided sides ***************************** */
    for(int i=0; i<l_contactL.size(); i++)
    {
        if(!l_contactL[i]->m_momentum.betweenV(m_momentum.top()+1.0, m_momentum.bottom()-1.0))
        {
            l_contactL.erase(l_contactL.begin()+i);
            i--;
        }
        #ifdef IS_MINIPHYSICS_DEMO_PROGRAM
        else l_contactL[i]->m_touch = PhysObject::Contact_Right;
        #endif
    }
    for(int i=0; i<l_contactR.size(); i++)
    {
        if(!l_contactR[i]->m_momentum.betweenV(m_momentum.top()+1.0, m_momentum.bottom()-1.0))
        {
            l_contactR.erase(l_contactR.begin()+i);
            i--;
        }
        #ifdef IS_MINIPHYSICS_DEMO_PROGRAM
        else l_contactR[i]->m_touch = PhysObject::Contact_Left;
        #endif
    }
    for(int i=0; i<l_contactT.size(); i++)
    {
        if(!l_contactT[i]->m_momentum.betweenH(m_momentum.left()+1.0, m_momentum.right()-1.0))
        {
            l_contactT.erase(l_contactT.begin()+i);
            i--;
        }
        #ifdef IS_MINIPHYSICS_DEMO_PROGRAM
           else l_contactT[i]->m_touch = PhysObject::Contact_Bottom;
        #endif
    }
    for(int i=0; i<l_contactB.size(); i++)
    {
        if(!l_contactB[i]->m_momentum.betweenH(m_momentum.left()+1.0, m_momentum.right()-1.0))
        {
            l_contactB.erase(l_contactB.begin()+i);
            i--;
        }
        #ifdef IS_MINIPHYSICS_DEMO_PROGRAM
           else l_contactB[i]->m_touch = PhysObject::Contact_Top;
        #endif
    }

    /* ****************************Detection of the crush****************************** */
    if(collideAtBottom && collideAtTop)
    {
        //If character got crushed between moving layers
        if(collideAtBottom->m_momentum.velY < collideAtTop->m_momentum.velY )
        {
            m_stand = false;
            m_momentum.y = collideAtTop->m_momentum.bottom();
            m_momentum.velY = collideAtTop->m_momentum.velY;
            doSpeedStack = false;
            speedNum = 0.0;
            speedSum = 0.0;
            #ifdef STOP_LOOP_ON_CRUSH
            alive = false;
            #endif
            if( (collideAtTop->m_shape == PhysObject::SL_RightTop) &&
                ( round(m_momentum.right()) == round(collideAtTop->m_momentum.right()) ) )
            {
                m_momentum.x = collideAtTop->m_momentum.right() - m_momentum.w - 1.0;
            } else
            if( (collideAtTop->m_shape == PhysObject::SL_LeftTop) &&
                ( round(m_momentum.left()) == round(collideAtTop->m_momentum.left()) ) )
            {
                m_momentum.x = collideAtTop->m_momentum.left() + 1.0;
            } else
            if( (collideAtBottom->m_shape == PhysObject::SL_RightBottom) &&
                ( round(m_momentum.right()) == round(collideAtBottom->m_momentum.right()) ) )
            {
                m_momentum.x = collideAtBottom->m_momentum.right() - m_momentum.w - 1.0;
            } else
            if( (collideAtBottom->m_shape == PhysObject::SL_LeftBottom) &&
                ( round(m_momentum.left()) == round(collideAtBottom->m_momentum.left()) ) )
            {
                m_momentum.x = collideAtBottom->m_momentum.left() + 1.0;
            } else
            if( (collideAtTop->m_blocked[m_filterID]==PhysObject::Block_ALL) &&
                (collideAtBottom->m_blocked[m_filterID]==PhysObject::Block_ALL) &&
                  (m_momentum.h > fabs(collideAtTop->m_momentum.bottom() - collideAtBottom->m_momentum.top())) )
            {
                m_crushedHardDelay = 30;
                m_crushedHard = true;
                printf("CRUSHED BETWEEN VERTICAL!!!\n");
                fflush(stdout);
            }
        }
    }

    if(collideAtLeft && collideAtRight)
    {
        //If character got crushed between moving layers
        if(collideAtRight->m_momentum.velX < collideAtLeft->m_momentum.velX )
        {
            #ifdef STOP_LOOP_ON_CRUSH
            alive = false;
            #endif
            if( (collideAtLeft->m_blocked[m_filterID]==PhysObject::Block_ALL) &&
                (collideAtRight->m_blocked[m_filterID]==PhysObject::Block_ALL) &&
                 (m_momentum.w > fabs(collideAtLeft->m_momentum.right() - collideAtRight->m_momentum.left())) )
            {
                m_crushedHardDelay = 30;
                m_crushedHard = true;
                printf("CRUSHED BETWEEN HORIZONTAL!!!\n");
                fflush(stdout);
            }
        }
    }
    /* ****************************************************************************** */

    if( doSpeedStack && (speedNum > 1.0) && (speedSum != 0.0) )
    {
        m_momentum.velX = m_momentum.velXsrc + (speedSum/speedNum);
    }
}


void MiniPhysics::loop()
{
    #ifdef STOP_LOOP_ON_CRUSH
    if(alive)
    {
    #endif
        iterateStep();
        //processCollisions();
        //Return player to top back on fall down
        if(pl.m_momentum.y > height()-cameraY)
        {
            pl.m_momentum.y = 64;
            pl.m_momentum.oldy = 64;
        }
        pl.processCollisions(objs);
    #ifdef STOP_LOOP_ON_CRUSH
    }
    #endif
    cameraX = pl.m_momentum.centerX()-width()/2.0;
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
    for(PGE_SizeT i=0; i<objs.size(); i++)
    {
        objs[i].paint(p, cameraX, cameraY);
    }
    pl.paint(p, cameraX, cameraY);
    p.drawText(10, 10, QString("Hole-Run = %1").arg(pl.m_allowHoleRuning) );
    p.drawText(10, 30, QString("Align on top-slope = %1").arg(pl.m_onSlopeFloorTopAlign) );
}
