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

class obj
{
public:
    enum Type {
        SL_Rect = 0,
        SL_RightBottom,
        SL_LeftBottom,
        SL_LeftTop,
        SL_RightTop
    };
    enum ContactAt{
        Contact_None = 0,
        Contact_Top,
        Contact_Bottom,
        Contact_Left,
        Contact_Right
    };

    obj(int x=0, int y=0, int id=0) :
        m_id(id),
        m_x(x),
        m_y(y),
        m_oldx(x),
        m_oldy(y),
        m_w(32.0),
        m_h(32.0),
        m_velX(0.0),
        m_velX_source(0.0),
        m_velY(0.0),
        m_stand(false),
        m_standOnYMovable(false),
        m_crushed(false),
        m_crushedOld(false),
        m_drawSpeed(false),
        m_bumped(false),
        m_cliff(false),
        m_touch(Contact_None),
        m_jumpPressed(false),
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
        m_onSlopeYAdd(0.0)
    {}
    obj(const obj& o) :
        m_id(o.m_id),
        m_x(o.m_x),
        m_y(o.m_y),
        m_oldx(o.m_oldx),
        m_oldy(o.m_oldy),
        m_w(o.m_w),
        m_h(o.m_h),
        m_velX(o.m_velX),
        m_velX_source(o.m_velX_source),
        m_velY(o.m_velY),
        m_stand(o.m_stand),
        m_standOnYMovable(o.m_standOnYMovable),
        m_crushed(o.m_crushed),
        m_crushedOld(o.m_crushedOld),
        m_drawSpeed(o.m_drawSpeed),
        m_bumped(o.m_bumped),
        m_cliff(o.m_cliff),
        m_touch(o.m_touch),
        m_jumpPressed(o.m_jumpPressed),
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
        m_onSlopeYAdd(o.m_onSlopeYAdd)
    {}

    void paint(QPainter &p, double cameraX, double cameraY)
    {
        double x = round(-cameraX+m_x);
        double y = round(-cameraY+m_y);
        p.setBrush(Qt::gray);
        if(m_crushed && m_crushedOld)
        {
            p.setBrush(Qt::red);
        }
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

        QPolygonF poly;
        switch(m_id)
        {
        case SL_RightBottom:
            poly.append(QPointF(x, y+m_h));
            poly.append(QPointF(x+m_w, y));
            poly.append(QPointF(x+m_w, y+m_h));
            p.drawPolygon(poly);
            break;
        case SL_LeftBottom:
            poly.append(QPointF(x, y));
            poly.append(QPointF(x, y+m_h));
            poly.append(QPointF(x+m_w, y+m_h));
            p.drawPolygon(poly);
            break;
        case SL_RightTop:
            poly.append(QPointF(x, y));
            poly.append(QPointF(x+m_w, y));
            poly.append(QPointF(x+m_w, y+m_h));
            p.drawPolygon(poly);
            break;
        case SL_LeftTop:
            poly.append(QPointF(x, y));
            poly.append(QPointF(x+m_w, y));
            poly.append(QPointF(x, y+m_h));
            p.drawPolygon(poly);
            break;
        default:
        case SL_Rect:
            p.drawRect(x, y, m_w, m_h); break;
        }
        if(m_drawSpeed)
            p.drawText(x-20, y-5, QString("%1 %2").arg(m_velX, 7).arg(m_velY, 7) );
        if(m_stand || m_cliff)
            p.drawText(x+m_w+10, y+2, QString("%1 %2").arg(m_stand?"[G]":"   ").arg(m_cliff?"[CLIFF]":""));
    }
    int     m_id;
    inline  double x()      { return m_x; }
    inline  double y()      { return m_y; }
    inline  double top()    { return m_y; }
    inline  double left()   { return m_x; }
    inline  double right()  { return m_x + m_w; }
    inline  double bottom() { return m_y + m_h; }
    inline  double topOld()    { return m_oldy; }
    inline  double leftOld()   { return m_oldx; }
    inline  double rightOld()  { return m_oldx + m_w; }
    inline  double bottomOld() { return m_oldy + m_h; }
    inline  double centerX() { return m_x + m_w/2.0; }
    inline  double centerY() { return m_y + m_h/2.0; }
    inline  double centerXold() { return m_oldx + m_w/2.0; }
    inline  double centerYold() { return m_oldy + m_h/2.0; }
    inline  bool   betweenH(double left, double right) { if(right < m_x) return false; if(left > m_x+m_w) return false; return true; }
    inline  bool   betweenH(double X) { return (X >= m_x) && (X <= m_x+m_w); }
    inline  bool   betweenV(double top, double bottom) { if(bottom < m_y) return false; if(top > m_y+m_h) return false; return true; }
    inline  bool   betweenV(double Y) { return (Y >= m_y) && (Y <= m_y+m_h); }
    inline  objRect rect() { return {m_x, m_y, m_w, m_h}; }
    double  m_x;
    double  m_y;
    double  m_oldx;
    double  m_oldy;
    double  m_w;
    double  m_h;
    double  m_velX;
    double  m_velX_source;
    double  m_velY;
    bool    m_stand;
    bool    m_standOnYMovable;
    bool    m_crushed;
    bool    m_crushedOld;
    bool    m_drawSpeed;
    bool    m_bumped;
    bool    m_cliff;
    ContactAt m_touch;
    //! Is jump key pressed
    bool    m_jumpPressed;
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
