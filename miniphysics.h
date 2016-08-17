#ifndef MINIPHYSICS_H
#define MINIPHYSICS_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_1>
#include <QTimer>
#include <QList>
#include <QHash>
#include <QFont>
#include <QPainter>
#include <vector>

#include "common_features/rectf.h"

#define IS_MINIPHYSICS_DEMO_PROGRAM

class physBody;
typedef QVector<physBody> PGE_RenderList;
typedef int               PGE_SizeT;

struct objControlls
{
    objControlls() :
        left(false),
        right(false),
        jump(false)
    {}
    bool left;
    bool right;
    bool jump;
};

class physBody_DEMOONLY
{
public:
    enum ContactAt{
        Contact_None = 0,
        Contact_Top,
        Contact_Bottom,
        Contact_Left,
        Contact_Right,
        Contact_Skipped
    };

    physBody_DEMOONLY():
        m_drawSpeed(false),
        m_bumped(false),
        m_touch(Contact_None),
        m_jumpPressed(false),
        m_keys()
    {}

    physBody_DEMOONLY(const physBody_DEMOONLY& o) :
        m_drawSpeed(o.m_drawSpeed),
        m_bumped(o.m_bumped),
        m_touch(o.m_touch),
        m_jumpPressed(o.m_jumpPressed),
        m_keys(o.m_keys)
    {}

    bool    m_drawSpeed;
    bool    m_bumped;
    ContactAt m_touch;
    //! Is jump key pressed
    bool    m_jumpPressed;
    objControlls m_keys;
};

class physBody: public physBody_DEMOONLY
{
public:
    enum ShapeType {
        SL_Rect = 0,
        SL_RightBottom,
        SL_LeftBottom,
        SL_LeftTop,
        SL_RightTop
    };
    /*
    enum ContactAt{
        Contact_None = 0,
        Contact_Top,
        Contact_Bottom,
        Contact_Left,
        Contact_Right,
        Contact_Skipped
    };*/
    enum BlockedSides{
        Block_NONE      = 0,
        Block_LEFT      = 0x1,
        Block_TOP       = 0x2,
        Block_RIGHT     = 0x4,
        Block_BOTTOM    = 0x8,
        Block_ALL       = 0xF,
    };

    struct objRect
    {
        double x;
        double y;
        double w;
        double h;
        inline double left() {return x;}
        inline double right() {return x+w;}
        inline double top() {return y;}
        inline double bottom() {return y+h;}
    };

    struct Momentum {
        Momentum() :
            x(0.0),     y(0.0),     w(32.0),    h(32.0),
            oldx(0.0),  oldy(0.0),  oldw(32.0), oldh(32.0),
            velX(0.0),  velY(0.0),  velXsrc(0.0)
        {}
        Momentum(double _x, double _y) :
            x(_x),     y(_y),     w(32.0),    h(32.0),
            oldx(_x),  oldy(_y),  oldw(32.0), oldh(32.0),
            velX(0.0),  velY(0.0),  velXsrc(0.0)
        {}

        //! Position X
        double x;
        //! Position Y
        double y;
        //! Width
        double w;
        //! Height
        double h;
        //! Previous position X
        double oldx;
        //! Previous position Y
        double oldy;
        //! Previous width
        double oldw;
        //! Previous height
        double oldh;
        //! Real (sum of the own and floor speeds) horizontal speed (velocity)
        double velX;
        //! Real Vertical speed (velocity)
        double velY;
        //! Source (own) horizontal speed (velocity).
        double velXsrc;

        inline objRect   rect() {return {x, y, w, h}; }
        inline objRect   rectOld() {return {oldx, oldy, oldw, oldh}; }
        inline PGE_RectF rectF() {return PGE_RectF(x, y, w, h); }
        inline PGE_RectF rectOldF() {return PGE_RectF(oldx, oldy, oldw, oldh); }

        /**
         * @brief Copy all current values to the "old"
         */
        inline void saveOld() { oldx = x; oldy=y; oldw=w; oldh=h; }

        inline void setXatLeft(double newx) { x = newx; }
        inline void setXatRight(double newx) { x = newx-w; }
        inline void setYatTop(double newy) { y = newy; }
        inline void setYatBottom(double newy) { y = newy-h; }
        inline void setLeft(double newx) { w = fabs(newx-(x+w)); x = newx; }
        inline void setRight(double newx) { w = fabs(x-newx); }
        inline void setTop(double newy) { h = fabs(newy-(y+h)); y = newy; }
        inline void setBottom(double newy) { h = fabs(y-newy); }

        inline double left(){return x;}
        inline double top(){return y;}
        inline double right(){return x+w;}
        inline double bottom(){return y+h;}
        inline double centerX(){return x+(w/2.0);}
        inline double centerY(){return y+(h/2.0);}

        inline double leftOld(){return oldx;}
        inline double topOld(){return oldy;}
        inline double rightOld(){return oldx+oldw;}
        inline double bottomOld(){return oldy+oldh;}
        inline double centerXold(){return oldx+(oldw/2.0);}
        inline double centerYold(){return oldy+(oldh/2.0);}

        inline  bool   betweenH(double left, double right) { if(right < x) return false; if(left > x+w) return false; return true; }
        inline  bool   betweenH(double X) { return (X >= x) && (X <= x+w); }
        inline  bool   betweenV(double top, double bottom) { if(bottom < y) return false; if(top > y+h) return false; return true; }
        inline  bool   betweenV(double Y) { return (Y >= y) && (Y <= y+h); }
    };


    struct SlopeState {
        SlopeState():
            has(false),
            hasOld(false),
            shape(physBody::SL_Rect)
        {}
        bool    has;
        bool    hasOld;
        objRect rect;
        int     shape;
    };

    physBody(int x=0, int y=0, int id=0) :
        physBody_DEMOONLY(),
        m_shape(id),
        m_momentum(double(x), double(y)),
        m_touchLeftWall(false),
        m_touchRightWall(false),
        m_blockedAtLeft(false),
        m_blockedAtRight(false),
        m_stand(false),
        m_standOnYMovable(false),
        m_crushed(false),
        m_crushedOld(false),
        m_crushedHard(false),
        m_crushedHardDelay(0),
        m_cliff(false),
        m_onSlopeYAdd(0.0),
        m_allowHoleRuning(false),
        m_onSlopeFloorTopAlign(false),
        m_blocked{Block_ALL, Block_ALL},
        m_filterID(0)
    {}

    physBody(const physBody& o) :
        physBody_DEMOONLY(o),
//        l_contactL(o.l_contactL),
//        l_contactR(o.l_contactR),
//        l_contactT(o.l_contactT),
//        l_contactB(o.l_contactB),
        m_shape(o.m_shape),
        m_momentum(o.m_momentum),
        m_touchLeftWall(o.m_touchLeftWall),
        m_touchRightWall(o.m_touchRightWall),
        m_blockedAtLeft(o.m_blockedAtLeft),
        m_blockedAtRight(o.m_blockedAtRight),
        m_stand(o.m_stand),
        m_standOnYMovable(o.m_standOnYMovable),
        m_crushed(o.m_crushed),
        m_crushedOld(o.m_crushedOld),
        m_crushedHard(o.m_crushedHard),
        m_crushedHardDelay(o.m_crushedHardDelay),
        m_cliff(o.m_cliff),
        m_slopeFloor(o.m_slopeFloor),
        m_slopeCeiling(o.m_slopeCeiling),
        m_onSlopeYAdd(o.m_onSlopeYAdd),
        m_allowHoleRuning(o.m_allowHoleRuning),
        m_onSlopeFloorTopAlign(o.m_onSlopeFloorTopAlign),
        m_blocked{o.m_blocked[0], o.m_blocked[1]},
        m_filterID(o.m_filterID)
    {}

    void paint(QPainter &p, double cameraX, double cameraY)
    {
        double x = round(m_momentum.x-cameraX);
        double y = round(m_momentum.y-cameraY);
        p.setBrush(Qt::gray);
        p.setOpacity(m_blocked[0]==Block_ALL ? 1.0 : 0.5 );
        if(m_crushed && m_crushedOld)
            p.setBrush(QColor("#FFA500"));
        if(m_crushedHard)
            p.setBrush(Qt::red);
        if(m_touch != Contact_None)
        {
            switch(m_touch)
            {
            case Contact_Left:
                {
                    p.setBrush(Qt::cyan);
                    break;
                }
            case Contact_Right:
                {
                    p.setBrush(Qt::yellow);
                    break;
                }
            case Contact_Top:
                {
                    p.setBrush(Qt::darkCyan);
                    break;
                }
            case Contact_Bottom:
                {
                    p.setBrush(Qt::darkYellow);
                    break;
                }
            default:break;
            }
            m_touch = Contact_None;
        }

        if(m_bumped)
        {
            p.setBrush(Qt::green);
        }

        double w = m_momentum.w-1.0;
        double h = m_momentum.h-1.0;

        QPolygonF poly;
        switch(m_shape)
        {
        case SL_RightBottom:
            poly.append(QPointF(x, y+h));
            poly.append(QPointF(x+w, y));
            poly.append(QPointF(x+w, y+h));
            p.drawPolygon(poly);
            break;
        case SL_LeftBottom:
            poly.append(QPointF(x, y));
            poly.append(QPointF(x, y+h));
            poly.append(QPointF(x+w, y+h));
            p.drawPolygon(poly);
            break;
        case SL_RightTop:
            poly.append(QPointF(x, y));
            poly.append(QPointF(x+w, y));
            poly.append(QPointF(x+w, y+h));
            p.drawPolygon(poly);
            break;
        case SL_LeftTop:
            poly.append(QPointF(x, y));
            poly.append(QPointF(x+w, y));
            poly.append(QPointF(x, y+h));
            p.drawPolygon(poly);
            break;
        default:
        case SL_Rect:
            p.drawRect(x, y, w, h); break;
        }
        if(m_drawSpeed)
            p.drawText(x-20, y-5, QString("%1 %2").arg(m_momentum.velX, 7).arg(m_momentum.velY, 7) );
        if(m_stand || m_cliff)
            p.drawText(x+m_momentum.w+10, y+2, QString("%1 %2").arg(m_stand?"[G]":"   ").arg(m_cliff?"[CLIFF]":""));
        if(m_touchLeftWall)
            p.drawText(x+m_momentum.w+10, y+10, QString("L"));
        if(m_touchRightWall)
            p.drawText(x+m_momentum.w+10, y+10, QString("R"));
        if(m_blockedAtLeft)
            p.drawText(x+2, y+15, QString("|<-"));
        if(m_blockedAtRight)
            p.drawText(x+2, y+15, QString("->|"));
    }

    QVector<physBody*> l_contactL;
    QVector<physBody*> l_contactR;
    QVector<physBody*> l_contactT;
    QVector<physBody*> l_contactB;

    void iterateStep();

    void processCollisions(PGE_RenderList &objs);

    int         m_shape;
    Momentum    m_momentum;
    /***************Events****************/
    inline void    resetEvents()
    {
        m_cliff      = false;
        m_touchLeftWall  = false;
        m_touchRightWall = false;
        m_blockedAtLeft = false;
        m_blockedAtRight = false;
        m_crushedOld = m_crushed;
        m_crushed    = false;
        m_stand      = false;
        m_standOnYMovable = false;
        #ifdef IS_MINIPHYSICS_DEMO_PROGRAM
        if(m_crushedHardDelay > 0)
            m_crushedHardDelay -= 1;
        else
        #endif
            m_crushedHard = false;

        l_contactL.clear();
        l_contactR.clear();
        l_contactT.clear();
        l_contactB.clear();
    }
    bool    m_touchLeftWall;
    bool    m_touchRightWall;
    bool    m_blockedAtLeft;
    bool    m_blockedAtRight;
    bool    m_stand;
    bool    m_standOnYMovable;
    bool    m_crushed;
    bool    m_crushedOld;
    bool    m_crushedHard;
    int     m_crushedHardDelay;
    bool    m_cliff;
    /*************************************/
    SlopeState m_slopeFloor;
    SlopeState m_slopeCeiling;
    //! Y-speed add while standing on the slope
    double  m_onSlopeYAdd;

    //! Allow running over floor holes
    bool    m_allowHoleRuning;
    //! Enable automatical aligning of position while staying on top corner of slope
    bool    m_onSlopeFloorTopAlign;
    //! Blocking filters (0 - playable characters, 1 - NPCs)
    int     m_blocked[2];
    //! Type of self (0 - playable characters, 1 - NPCs)
    int     m_filterID;
};

struct LevelData;
class MiniPhysics : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit MiniPhysics(QWidget* parent=nullptr);

    void initTest1();
    void initTest2();
    void initTestCommon(LevelData *file);
protected:
    void iterateStep();
    void loop();
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void initializeGL();
    void paintEvent(QPaintEvent *e);
private:
    double cameraX;
    double cameraY;
    int lastTest;
    QHash<int, bool>    keyMap;
    PGE_RenderList      objs;
    std::vector<unsigned int> movingBlock;
    physBody     pl;
    QTimer looper;
    QOpenGLFunctions *f;
    QFont m_font;
};

#endif // MINIPHYSICS_H
