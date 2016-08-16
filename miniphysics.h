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

#include "common_features/rectf.h"

//Private Type obj
//    id As Long
//    dx As Double
//    dy As Double
//    x As Double
//    y As Double
//    w As Double
//    h As Double
//    spx As Double
//    spy As Double
//    st As Boolean
//End Type
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

class obj_DEMOONLY
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

    obj_DEMOONLY():
        m_drawSpeed(false),
        m_bumped(false),
        m_touch(Contact_None),
        m_jumpPressed(false),
        m_keys()
    {}

    obj_DEMOONLY(const obj_DEMOONLY& o) :
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


class obj: public obj_DEMOONLY
{
public:
    enum Type {
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

    obj(int x=0, int y=0, int id=0) :
        obj_DEMOONLY(),
        m_id(id),
        m_posRect(x, y, 32.0, 32.0),
        m_posRectOld(x, y, 32.0, 32.0),
        m_x(x),
        m_y(y),
        m_oldx(x),
        m_oldy(y),
        m_w(32.0),
        m_h(32.0),
        m_oldw(32.0),
        m_oldh(32.0),
        m_velocityX(0.0),
        m_velocityXsrc(0.0),
        m_velocityY(0.0),
        m_touchLeftWall(false),
        m_touchRightWall(false),
        m_stand(false),
        m_standOnYMovable(false),
        m_crushed(false),
        m_crushedOld(false),
        m_crushedHard(false),
        m_crushedHardDelay(0),
        m_cliff(false),
        m_allowHoleRuning(false),
        m_onSlopeFloor(false),
        m_onSlopeFloorOld(false),
        m_onSlopeFloorTopAlign(false),
        m_onSlopeFloorShape(0),
        m_onSlopeFloorRect{0.0, 0.0, 0.0, 0.0},
        m_onSlopeCeiling(false),
        m_onSlopeCeilingOld(false),
        m_onSlopeCeilingShape(0),
        m_onSlopeCeilingRect{0.0, 0.0, 0.0, 0.0},
        m_onSlopeYAdd(0.0),
        m_blocked{Block_ALL, Block_ALL},
        m_filterID(0)
    {}
    obj(const obj& o) :
        obj_DEMOONLY(o),
        m_id(o.m_id),
        m_posRect(o.m_posRect),
        m_posRectOld(o.m_posRectOld),
        m_x(o.m_x),
        m_y(o.m_y),
        m_oldx(o.m_oldx),
        m_oldy(o.m_oldy),
        m_w(o.m_w),
        m_h(o.m_h),
        m_oldw(o.m_oldw),
        m_oldh(o.m_oldh),
        m_velocityX(o.m_velocityX),
        m_velocityXsrc(o.m_velocityXsrc),
        m_velocityY(o.m_velocityY),
        m_touchLeftWall(o.m_touchLeftWall),
        m_touchRightWall(o.m_touchRightWall),
        m_stand(o.m_stand),
        m_standOnYMovable(o.m_standOnYMovable),
        m_crushed(o.m_crushed),
        m_crushedOld(o.m_crushedOld),
        m_crushedHard(o.m_crushedHard),
        m_crushedHardDelay(o.m_crushedHardDelay),
        m_cliff(o.m_cliff),
        m_allowHoleRuning(o.m_allowHoleRuning),
        m_onSlopeFloor(o.m_onSlopeFloor),
        m_onSlopeFloorOld(o.m_onSlopeFloorOld),
        m_onSlopeFloorTopAlign(o.m_onSlopeFloorTopAlign),
        m_onSlopeFloorShape(o.m_onSlopeFloorShape),
        m_onSlopeFloorRect(o.m_onSlopeFloorRect),
        m_onSlopeCeiling(o.m_onSlopeCeiling),
        m_onSlopeCeilingOld(o.m_onSlopeCeilingOld),
        m_onSlopeCeilingShape(o.m_onSlopeCeilingShape),
        m_onSlopeCeilingRect(o.m_onSlopeCeilingRect),
        m_onSlopeYAdd(o.m_onSlopeYAdd),
        m_blocked{o.m_blocked[0], o.m_blocked[1]},
        m_filterID(o.m_filterID)

    {}

    void paint(QPainter &p, double cameraX, double cameraY)
    {
        double x = round(m_x-cameraX);
        double y = round(m_y-cameraY);
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

        double w = m_w-1.0;
        double h = m_h-1.0;

        QPolygonF poly;
        switch(m_id)
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
            p.drawText(x-20, y-5, QString("%1 %2").arg(m_velocityX, 7).arg(m_velocityY, 7) );
        if(m_stand || m_cliff)
            p.drawText(x+m_w+10, y+2, QString("%1 %2").arg(m_stand?"[G]":"   ").arg(m_cliff?"[CLIFF]":""));
        if(m_touchLeftWall)
            p.drawText(x+m_w+10, y+10, QString("L"));
        if(m_touchRightWall)
            p.drawText(x+m_w+10, y+10, QString("R"));
    }

    int     m_id;
    inline  double top()    { return m_y; }
    inline  double left()   { return m_x; }
    inline  double right()  { return m_x+m_w; }
    inline  double bottom() { return m_y+m_h; }
    inline  double topOld()    { return m_oldy; }
    inline  double leftOld()   { return m_oldx; }
    inline  double rightOld()  { return m_oldx+m_oldw; }
    inline  double bottomOld() { return m_oldy+m_oldh; }
    inline  double centerX() { return m_x+(m_w/2.0); }
    inline  double centerY() { return m_y+(m_h/2.0); }
    inline  double centerXold() { return m_oldx+(m_oldw/2.0); }
    inline  double centerYold() { return m_oldy+(m_oldy/2.0); }
    inline  bool   betweenH(double left, double right) { if(right < this->left()) return false; if(left > this->right()) return false; return true; }
    inline  bool   betweenH(double X) { return (X >= left()) && (X <= right()); }
    inline  bool   betweenV(double top, double bottom) { if(bottom < this->top()) return false; if(top > this->bottom()) return false; return true; }
    inline  bool   betweenV(double Y) { return (Y >= top()) && (Y <= bottom()); }
    inline  objRect rect() { return {m_x, m_y, m_w, m_h}; }
    //! Real body geometry and position
    PGE_RectF m_posRect;
    PGE_RectF m_posRectOld;

    double  m_x;
    double  m_y;
    double  m_oldx;
    double  m_oldy;
    double  m_w;
    double  m_h;
    double  m_oldw;
    double  m_oldh;
    double  m_velocityX;
    double  m_velocityXsrc;
    double  m_velocityY;
    bool    m_touchLeftWall;
    bool    m_touchRightWall;
    bool    m_stand;
    bool    m_standOnYMovable;
    bool    m_crushed;
    bool    m_crushedOld;
    bool    m_crushedHard;
    int     m_crushedHardDelay;
    bool    m_cliff;
    //! Allow running over floor holes
    bool    m_allowHoleRuning;
    bool    m_onSlopeFloor;
    bool    m_onSlopeFloorOld;
    //! Enable automatical aligning of position while staying on top corner of slope
    bool    m_onSlopeFloorTopAlign;
    //! Shape of recently contacted floor slope block
    int     m_onSlopeFloorShape;
    objRect m_onSlopeFloorRect;
    bool    m_onSlopeCeiling;
    bool    m_onSlopeCeilingOld;
    int     m_onSlopeCeilingShape;
    objRect m_onSlopeCeilingRect;
    double  m_onSlopeYAdd;
    int     m_blocked[2];
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
    void processCollisions();
    void loop();
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void initializeGL();
    void paintEvent(QPaintEvent *e);
private:
    double cameraX;
    double cameraY;
    int lastTest;
    QHash<int, bool> keyMap;
    QList<obj>  objs;
    QList<obj*> movingBlock;
    obj     pl;
    QTimer looper;
    QOpenGLFunctions *f;
    QFont m_font;
};

#endif // MINIPHYSICS_H
