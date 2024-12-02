#ifndef WIDGET_H
#define WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QVector>
#include <QStringList>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QTimer>
#include <QTime>
#include <QMatrix4x4>
#include <QMatrix3x3>
#include <QQuaternion>
#include <QVector2D>
#include <QTimer>
#include <QtOpenGL>
#include <QTime>
#include "camera.h"
#include <QMenuBar>
#include <gl/GLU.h>
#include <gl/GL.h>
#include <QWheelEvent>

struct point{
    double x,y,z;
};

struct linestruct{
    point p1,p2;
    point centralp;
    point on1at4,on3at4;
};

struct FACET{
    float normal[3];
    float vertex[3][3];

};

struct vtx
{
    QVector<point> pt;
    point norm;
};

struct surf{
    QVector<vtx> vtxinfo;
    point surnorm;
    //曲线用
    QVector<point> sphereNorm;
};

struct body{
    QVector<surf> surfinfo;
};

enum selectType{node,line,surface};

class Vector3
{
public:
    Vector3(float fx, float fy, float fz)
        :x(fx), y(fy), z(fz)
    {
    }

    // Subtract
    Vector3 operator - (const Vector3& v) const
    {
        return Vector3(x - v.x, y - v.y, z - v.z) ;
    }

    Vector3 operator + (const Vector3& v) const
    {
        return Vector3(x + v.x, y + v.y, z + v.z) ;
    }

    // Dot product
    float Dot(const Vector3& v) const
    {
        return x * v.x + y * v.y + z * v.z ;
    }

    // Cross product
    Vector3 Cross(const Vector3& v) const
    {
        return Vector3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x ) ;
    }

public:
    float x, y, z ;
};


class Widget : public QOpenGLWidget,public QOpenGLExtraFunctions
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    virtual void initializeGL() override;
    virtual void paintGL() override;
    virtual void resizeGL(int w,int h) override;
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

signals:
    void mousePickingPos(QVector3D worldpos);
public slots:
    void on_timeout();
private:
    QVector<float> vertices;
    QVector<float> tmp11;
    QVector<float> Position;
    QVector<float> Normal;//读文件时的俩个临时变量顶点位置，法向量

    //原图形的绘制
    QOpenGLShaderProgram ashaderprogram;
    QOpenGLVertexArrayObject aVAO;//声明VAO顶点数组对象
    QOpenGLBuffer aVBO;//声明VBO数组缓冲对象

    //用于点选标记的绘制
    QOpenGLShaderProgram select_shaderprogram;
    QOpenGLVertexArrayObject sVAO;//声明sVAO顶点数组对象
    QOpenGLBuffer sVBO;//声明sVBO数组缓冲对象

    QMatrix4x4 model;
    QMatrix4x4 view;
    QMatrix4x4 projection;

    Camera m_camera;

    QTimer m_timer;
    QTime m_time;

    GLfloat xtrans, ytrans, ztrans; // translation on x,y,z-axis
    QVector2D mousePos;
    QQuaternion rotation;
    QVector4D worldPosFromViewPort(float posX, float posY);
    QVector3D newWorldPos(float posX,float posY);
    bool IntersectTriangle(const Vector3& orig, const Vector3& dir,
         Vector3& v0, Vector3& v1, Vector3& v2,
         float* t, float* u, float* v);
    //bool hasPickingFace(QVector3D d, QVector3D cameraPos, QVector3D pos, unsigned *indicess, unsigned indlen, float* vaGrps);
    bool PointinTriangle(Vector3 A, Vector3 B, Vector3 C, Vector3 P);

    bool shiftPress = false;
    bool ctrlPress = false;

    QVector3D modelPos = {0.0f,0.0f,0.0f};

    //储存所有三角形片数据
    QVector<vtx> vertInfo;
    void getVtxInfo(std::vector<FACET> FACETINFO);

    QVector<QVector<float>> selectItems;
    QVector<QVector<float>> modelpush;
    QVector<QVector<float>> tests;

    //记录面的信息
    QVector<surf> surfacesInfo;
    void getSurfaceInfo();

    //记录体的信息（未完成）
    QVector<body> bodyInfo;
    void getBodyInfo();

    //判断三角形片是否与面上任意三角形片有至少两个共同点，若无则不是共面
    bool equaPoint2w(vtx vt1, surf surf1);

    bool vtxAllPointsOnSurface(vtx vt1, surf surf1);

    //获取面上的中心点
    point getCentralPoint(surf surf1);

    //获取面上xyz最大的点
    QVector<point> maxAxisOnSurface(surf surf1);

    //获取模型中所有的关键点
    QVector<point> getALLKP();
    QVector<point> allKp;

    //获取线
    QVector<linestruct> getALLLine();
    QVector<linestruct> allLine;
    //判断向量和两个点之间连线是否垂直
    bool pointLineisVerticaltoSurface(point p1,point p2,surf surf1);

    selectType seletype = surface;

    //画显示点的正方体
    void paintKeyPoint(float xro, float yro, float zro);

    //画显示线的边线
    QVector<linestruct> drawline;
    QVector<linestruct> drawline_blue;
    QVector<linestruct> drawline_green;

    //判断射线是否交于点的包络球
    bool isOnSphere(point kp,QVector3D wp);

    std::vector<FACET> fVec;

    bool isAsciiFileType(QString path);

    void loadAsciiSTL(std::string path, std::vector<FACET> &facetVec);

    void loadBinarySTL(std::string path,std::vector<FACET> &facetVec);

    float vecAndVecCos(point p1,point p2);



    //界面ui相关
    QMenuBar * m_Menubar;
    QString filePath;


    //绘制坐标系
    void paintAxis();




};

#endif // WIDGET_H

