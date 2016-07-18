#ifndef MINIPHYSICS_H
#define MINIPHYSICS_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_1>
#include <QTimer>
#include <QList>
#include <QHash>
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
        m_dx(0.0),
        m_dy(0.0),
        m_x(x),
        m_y(y),
        m_w(32.0),
        m_h(32.0),
        m_velX(0.0),
        m_velY(0.0),
        m_st(false)
    {}
    obj(const obj& o) :
        m_id(o.m_id),
        m_dx(o.m_dx),
        m_dy(o.m_dy),
        m_x(o.m_x),
        m_y(o.m_y),
        m_w(o.m_w),
        m_h(o.m_h),
        m_velX(o.m_velX),
        m_velY(o.m_velY),
        m_st(o.m_st)
    {}

    void paint(QPainter &p)
    {
        QPolygonF poly;
        switch(m_id)
        {
        case SL_RightBottom:
            poly.append(QPointF(m_x, m_y+m_h));
            poly.append(QPointF(m_x+m_w, m_y));
            poly.append(QPointF(m_x+m_w, m_y+m_h));
            p.drawPolygon(poly);
            break;
        case SL_LeftBottom:
            poly.append(QPointF(m_x, m_y));
            poly.append(QPointF(m_x, m_y+m_h));
            poly.append(QPointF(m_x+m_w, m_y+m_h));
            p.drawPolygon(poly);
            break;
        case SL_RightTop:
            poly.append(QPointF(m_x, m_y));
            poly.append(QPointF(m_x+m_w, m_y));
            poly.append(QPointF(m_x+m_w, m_y+m_h));
            p.drawPolygon(poly);
            break;
        case SL_LeftTop:
            poly.append(QPointF(m_x, m_y));
            poly.append(QPointF(m_x+m_w, m_y));
            poly.append(QPointF(m_x, m_y+m_h));
            p.drawPolygon(poly);
            break;
        default:
        case SL_Rect:
            p.drawRect(m_x, m_y, m_w, m_h); break;
        }
    }
    int     m_id;
    double  m_dx;
    double  m_dy;
    double  m_x;
    double  m_y;
    double  m_w;
    double  m_h;
    double  m_velX;
    double  m_velY;
    bool    m_st;
};

class MiniPhysics : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit MiniPhysics(QWidget* parent=nullptr);
protected:
    void iterateStep();
    void processCollisions();
    void loop();
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void initializeGL();
    void paintEvent(QPaintEvent *e);
private:
    QHash<int, bool> keyMap;
    QList<obj> objs;
    obj     pl;
    QTimer looper;
    QOpenGLFunctions *f;
};

#endif // MINIPHYSICS_H
