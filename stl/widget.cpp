#include "widget.h"
#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QtMath>
#include <QOpenGLShaderProgram>
#include <QMouseEvent>
#include <QWheelEvent>


float _near=0.1f,_far=100.0f;
QPoint lastPos = {0,0};
const unsigned int timeOutmSec=100;
QVector3D viewInitPos(0.0f,5.0f,50.0f);

//模型比例
float ratio = 1;

Widget::Widget(QWidget *parent)
    : QOpenGLWidget(parent),xtrans(0),ytrans(0),ztrans(0.0)
{
    this->setFixedSize(QSize(1000,600));
    this->setWindowTitle("STL_Show");
    m_Menubar = new QMenuBar(this);
    QAction * loadmodel = new QAction(u8"导入模型",this);
    QAction * testaction = new QAction(u8"测试按钮",this);
    QAction * axisaction = new QAction(u8"绘制坐标系",this);
    m_Menubar->addAction(loadmodel);
    m_Menubar->addAction(testaction);
    m_Menubar->addAction(axisaction);
    connect(loadmodel,&QAction::triggered,this,[=](){
        filePath = QFileDialog::getOpenFileName(this,u8"请选择stl模型文件","","STL(*.stl)");

//        fVec.clear();

//        if(filePath!="")
//        {
//            if(isAsciiFileType(filePath))
//                loadAsciiSTL(filePath.toStdString(),fVec);
//            else
//                loadBinarySTL(filePath.toStdString(),fVec);

//            getVtxInfo(fVec);
//            getSurfaceInfo();
//            allKp = getALLKP();
//            qDebug()<<"allKP num"<<allKp.size();
//            allLine = getALLLine();
//            modelpush.push_back(vertices);
//            update();
//        }
    });

    connect(axisaction,&QAction::triggered,this,[=](){this->paintAxis();});



    connect(&m_timer,SIGNAL(timeout()),this,SLOT(on_timeout()));
    m_timer.start(timeOutmSec);
    m_time.start();

    QSurfaceFormat format;
    format.setAlphaBufferSize(24);  //设置alpha缓冲大小
    format.setVersion(3,3);         //设置版本号
    format.setSamples(10);          //设置重采样次数，用于反走样

    this->setFormat(format);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    fVec.clear();
    filePath = "D:\\model_stl\\RobotSolid5.stl";

    if(isAsciiFileType(filePath))
        loadAsciiSTL(filePath.toStdString(),fVec);
    else
        loadBinarySTL(filePath.toStdString(),fVec);

    //判定ratio 取第一个三角形片的坐标平均值
    float maxaxis = 0;
    float averx = (fVec.at(0).vertex[0][0]+fVec.at(0).vertex[1][0]+fVec.at(0).vertex[2][0])/3;
    float avery = (fVec.at(0).vertex[0][1]+fVec.at(0).vertex[1][1]+fVec.at(0).vertex[2][1])/3;
    float averz = (fVec.at(0).vertex[0][2]+fVec.at(0).vertex[1][2]+fVec.at(0).vertex[2][2])/3;
    if(averx>maxaxis)
    {
        maxaxis = averx;
    }
    if(avery>maxaxis)
    {
        maxaxis = avery;
    }
    if(averz>maxaxis)
    {
        maxaxis = averz;
    }


    ratio = 1 / (maxaxis / 10);
    qDebug()<<"ratio = " << ratio;
    //qDebug()<<"result = "<<vecAndVecCos({0.00334,-0.00815,0},{0.00288,-0.00833,0});

    getVtxInfo(fVec);

    getSurfaceInfo();
    allKp = getALLKP();

    qDebug()<<"allKP num"<<allKp.size();

    //allLine = getALLLine();
    //modelpush.push_back(vertices);
    update();

}

Widget::~Widget()
{
    makeCurrent();
}

void Widget::initializeGL()
{
    this->initializeOpenGLFunctions();//初始化opengl函数
    ashaderprogram.create();//生成着色器程序
    select_shaderprogram.create();
    if(!ashaderprogram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/stl.vert")){
        qDebug()<<"ERROR:"<<ashaderprogram.log();    //如果编译出错,打印报错信息
    }
    if(!ashaderprogram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/stl.frag")){
        qDebug()<<"ERROR:"<<ashaderprogram.log();    //如果编译出错,打印报错信息
    }
    if(!select_shaderprogram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/stl.vert")){
        qDebug()<<"ERROR:"<<select_shaderprogram.log();    //如果编译出错,打印报错信息
    }
    if(!select_shaderprogram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/secondcolor.frag")){
        qDebug()<<"ERROR:"<<select_shaderprogram.log();    //如果编译出错,打印报错信息
    }
    //将添加到此程序的着色器与addshader链接在一起
     if(!ashaderprogram.link()){
         qDebug()<<"ERROR:"<<ashaderprogram.log();    //如果链接出错,打印报错信息
     }
     if(!select_shaderprogram.link()){
         qDebug()<<"ERROR:"<<select_shaderprogram.log();    //如果链接出错,打印报错信息
     }
//    QOpenGLVertexArrayObject::Binder{&VAO};

    aVAO.create();// 创建一个VAO对象，OpenGL会给它（顶点数组缓存对象）分配一个id
    aVAO.bind();//将RC中的当前顶点数组缓存对象Id设置为VAO的id
    aVBO.create();
    aVBO.bind();
    aVBO.allocate(vertices.data(),sizeof(float)*vertices.size());
    ashaderprogram.setAttributeBuffer("aPos", GL_FLOAT, 0, 3, sizeof(GLfloat) * 6);
    ashaderprogram.enableAttributeArray("aPos");
    ashaderprogram.setAttributeBuffer("aNormal", GL_FLOAT,sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 6);
    ashaderprogram.enableAttributeArray("aNormal");

    sVAO.create();
    sVAO.bind();
    sVBO.create();
    sVBO.bind();

    select_shaderprogram.setAttributeBuffer("aPos", GL_FLOAT, 0, 3, sizeof(GLfloat) * 6);
    select_shaderprogram.enableAttributeArray("aPos");
    select_shaderprogram.setAttributeBuffer("aNormal", GL_FLOAT,sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 6);
    select_shaderprogram.enableAttributeArray("aNormal");

    this->glEnable(GL_DEPTH_TEST);

    //glDisable(GL_POLYGON_OFFSET_FILL)

}
void Widget::resizeGL(int w,int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);

}

void Widget::paintGL()
{
    model.setToIdentity();
    view.setToIdentity();
    projection.setToIdentity();
    projection.perspective(m_camera.Zoom,(float)width()/height(),_near,_far);
    view=m_camera.GetViewMatrix();
    m_camera.Position = viewInitPos;
    this->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);//清空颜色缓冲区
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    QVector3D lightColor(1.0f,1.0f,1.0f);
    QVector3D objectColor(1.0f,0.5f,0.31f);
    QVector3D lightPos(20.0f,15.0f,50.0f);

    //foreach(QVector<float> newmodel,modelpush)
    //{
        //ashaderprogram.link();
        //aVAO.bind();
        //aVBO.bind();
        //aVBO.allocate(newmodel.data(),sizeof(float)*newmodel.size());
        ashaderprogram.bind();
        //将此着色器程序绑定到活动的qopenglcontext，并使其成为当前着色器程序。任何先前绑定的着色器程序都将被释放
        //成功绑定返回ture,反之，返回false.

        ashaderprogram.setUniformValue("objectColor",objectColor);
        ashaderprogram.setUniformValue("lightColor",lightColor);
        ashaderprogram.setUniformValue("lightPos", lightPos);

        model.setToIdentity();
        model.translate(modelPos);
        model.rotate(rotation);
        //glPushMatrix();
        //glMultMatrixf(m_transform.constData());
        ashaderprogram.setUniformValue("view", view);
        ashaderprogram.setUniformValue("projection", projection);
        ashaderprogram.setUniformValue("model", model);
        ashaderprogram.bind();
        int n = vertices.capacity()/sizeof(float);
        QOpenGLVertexArrayObject::Binder bind(&aVAO);//绑定VAO

        //offset>1 拉远 <1拉近 深度测试会自动屏蔽远处被遮挡的物体
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 0.0f);
        this->glDrawArrays(GL_TRIANGLES,0,n);
        glDisable(GL_POLYGON_OFFSET_FILL);
    //}

    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    foreach(auto te,tests)
    {
        //select_shaderprogram.link();
        sVAO.bind();
        sVBO.bind();
        sVBO.allocate(te.data(),sizeof(float)*te.size());
        select_shaderprogram.bind();

        select_shaderprogram.setUniformValue("view", view);
        select_shaderprogram.setUniformValue("projection", projection);
        select_shaderprogram.setUniformValue("model", model);
        int n1 = te.capacity()/sizeof(float);
        QOpenGLVertexArrayObject::Binder bind0(&sVAO);//绑定VAO
        this->glDrawArrays(GL_TRIANGLES,0,n1);
    }

    //glPopMatrix();
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 0.0f);
    for(int i=0;i<drawline.size();i++)
    {
        glColor3f(1.0f,0.0f,0.0f);
        select_shaderprogram.bind();
        select_shaderprogram.setUniformValue("view", view);
        select_shaderprogram.setUniformValue("projection", projection);
        select_shaderprogram.setUniformValue("model", model);
        glLineWidth(3);
        glBegin(GL_LINES);  //画线
        glVertex3d(drawline.at(i).p1.x,drawline.at(i).p1.y,drawline.at(i).p1.z);  //开始点(x,y)
        glVertex3d(drawline.at(i).p2.x,drawline.at(i).p2.y,drawline.at(i).p2.z);  //结束点(x,y)
        glEnd();
    }
    for(int i=0;i<drawline_blue.size();i++)
    {
        glColor3f(0.0f,0.0f,1.0f);
        select_shaderprogram.bind();
        select_shaderprogram.setUniformValue("view", view);
        select_shaderprogram.setUniformValue("projection", projection);
        select_shaderprogram.setUniformValue("model", model);
        glLineWidth(3);
        glBegin(GL_LINES);  //画线
        glVertex3d(drawline_blue.at(i).p1.x,drawline_blue.at(i).p1.y,drawline_blue.at(i).p1.z);  //开始点(x,y)
        glVertex3d(drawline_blue.at(i).p2.x,drawline_blue.at(i).p2.y,drawline_blue.at(i).p2.z);  //结束点(x,y)
        glEnd();
    }
    for(int i=0;i<drawline_green.size();i++)
    {
        glColor3f(0.0f,1.0f,0.0f);
        select_shaderprogram.bind();
        select_shaderprogram.setUniformValue("view", view);
        select_shaderprogram.setUniformValue("projection", projection);
        select_shaderprogram.setUniformValue("model", model);
        glLineWidth(3);
        glBegin(GL_LINES);  //画线
        glVertex3d(drawline_green.at(i).p1.x,drawline_green.at(i).p1.y,drawline_green.at(i).p1.z);  //开始点(x,y)
        glVertex3d(drawline_green.at(i).p2.x,drawline_green.at(i).p2.y,drawline_green.at(i).p2.z);  //结束点(x,y)
        glEnd();
    }
    glDisable(GL_POLYGON_OFFSET_FILL);


}


void Widget::mousePressEvent(QMouseEvent *event)
{
    makeCurrent();
    //qDebug()<<u8"摄像机初始位置："<<m_camera.Position.x()<<m_camera.Position.y()<<m_camera.Position.z();
    lastPos = event->pos();
    mousePos = QVector2D(event->pos());
    if(event->buttons()&Qt::LeftButton && seletype == surface){
        double mint = 1e5;int indexoftri = -1;
        QMap<int,float> Tcompare;

        if(!shiftPress && !ctrlPress)
        {
            tests.clear();
            drawline.clear();
            Tcompare.clear();
            mint = 1e5;
            indexoftri = -1;
        }

        QVector3D wp = newWorldPos(event->pos().x(),event->pos().y());
        qDebug() <<u8"新世界坐标:"<<wp.x()<<wp.y()<<wp.z();

        //遍历所有三角形片
        QVector<float> tritmp;
        for(int i=0;i<vertInfo.size();i++)
        {
            /********************************* 2024.03.20 11:36 *******************************/
            vtx tmp = vertInfo.at(i);

            Vector3 v1(tmp.pt.at(0).x,tmp.pt.at(0).y,tmp.pt.at(0).z);
            Vector3 v2(tmp.pt.at(1).x,tmp.pt.at(1).y,tmp.pt.at(1).z);
            Vector3 v3(tmp.pt.at(2).x,tmp.pt.at(2).y,tmp.pt.at(2).z);

            //左乘一个model矩阵进行旋转变换
            QMatrix4x4 v123 = {v1.x,v2.x,v3.x,0,v1.y,v2.y,v3.y,0,v1.z,v2.z,v3.z,0,0,0,0,1};
            QMatrix4x4 res = model * v123;

            v1 = Vector3(res(0,0)+modelPos.x(),res(1,0)+modelPos.y(),res(2,0)+modelPos.z());
            v2 = Vector3(res(0,1)+modelPos.x(),res(1,1)+modelPos.y(),res(2,1)+modelPos.z());
            v3 = Vector3(res(0,2)+modelPos.x(),res(1,2)+modelPos.y(),res(2,2)+modelPos.z());

            Vector3 d(wp.x()-m_camera.Position.x(),wp.y()-m_camera.Position.y(),wp.z()-m_camera.Position.z());
            Vector3 camerapos(m_camera.Position.x(),m_camera.Position.y(),m_camera.Position.z());

            //v1,v2,v3是三个顶点，d是dir，射线方向
            //O+dT O是摄像机起点，d是射线的方向向量，T是计算值
            //(1-u-v)V0+uV1+vV2  三角形上所有点均满足的方程，v0,v1,v2是三个顶点，uv是可计算的值
            float T, U, V;

            if (IntersectTriangle(camerapos, d, v1, v2, v3, &T, &U, &V)) {
                Vector3 intpos = Vector3(camerapos.x+d.x*T,camerapos.y+d.y*T,camerapos.z+d.z*T);
                if (PointinTriangle(v1, v2, v3, intpos)) {
                    qDebug() << u8"当前选取到的三角形片编号为:" << i;
                    qDebug()<<"modelpos :"<<modelPos.x()<<modelPos.y()<<modelPos.z();
                    //qDebug()<<u8"tri before："<<tmp.pt.at(0).x<<tmp.pt.at(0).y<<tmp.pt.at(0).z<<tmp.pt.at(1).x<<tmp.pt.at(1).y<<
                    //          tmp.pt.at(1).z<<tmp.pt.at(2).x<<tmp.pt.at(2).y<<tmp.pt.at(2).z;
                    //qDebug()<<u8"tri after："<<tmp.pt.at(0).x+modelPos.x()<<tmp.pt.at(0).y+modelPos.y()<<tmp.pt.at(0).z+modelPos.z()<<tmp.pt.at(1).x+modelPos.x()<<
                    //          tmp.pt.at(1).y+modelPos.y()<<tmp.pt.at(1).z+modelPos.z()<<tmp.pt.at(2).x+modelPos.x()<<tmp.pt.at(2).y+modelPos.y()<<tmp.pt.at(2).z+modelPos.z();
                    qDebug()<<u8"射线方向向量："<<d.x<<d.y<<d.z;
                    qDebug()<<u8"与三角形片交点为："<<intpos.x<<intpos.y<<intpos.z;
                    qDebug()<<u8"此时的T值为："<<T;
                    qDebug()<<endl;

                    tritmp.push_back(tmp.pt.at(0).x);tritmp.push_back(tmp.pt.at(0).y);tritmp.push_back(tmp.pt.at(0).z);
                    tritmp.push_back(tmp.norm.x);tritmp.push_back(tmp.norm.y);tritmp.push_back(tmp.norm.z);
                    tritmp.push_back(tmp.pt.at(1).x);tritmp.push_back(tmp.pt.at(1).y);tritmp.push_back(tmp.pt.at(1).z);
                    tritmp.push_back(tmp.norm.x);tritmp.push_back(tmp.norm.y);tritmp.push_back(tmp.norm.z);
                    tritmp.push_back(tmp.pt.at(2).x);tritmp.push_back(tmp.pt.at(2).y);tritmp.push_back(tmp.pt.at(2).z);
                    tritmp.push_back(tmp.norm.x);tritmp.push_back(tmp.norm.y);tritmp.push_back(tmp.norm.z);

                    Tcompare[i] = T;

                    if(shiftPress)
                      tests.push_back(tritmp);

                }

            }

        }

        if(!shiftPress)
        {
            //选取最近的面,判断t值大小

            for(QMap<int,float>::iterator it=Tcompare.begin();it!=Tcompare.end();it++)
            {
                if(it.value()<mint && it.value()!=0)
                {
                    mint = it.value();
                    indexoftri = it.key();
                }
            }
            if(indexoftri != -1)
            {
                vtx tp = vertInfo.at(indexoftri);

                //通过当前选择的三角形判断属于哪个面
                for(int i=0;i<surfacesInfo.size();i++)
                {
                    //if((tp.norm.x == surfacesInfo.at(i).surnorm.x && tp.norm.y == surfacesInfo.at(i).surnorm.y &&tp.norm.z == surfacesInfo.at(i).surnorm.z ) && equaPoint2w(tp,surfacesInfo.at(i)))
                    //这里应该是只要和面内任一vtx的法向量夹角小于一定值即可
                    bool sphereNormEqual = false;
                    for(int k=0;k<surfacesInfo.at(i).sphereNorm.size();k++)
                    {
                        if(vecAndVecCos(tp.norm,surfacesInfo.at(i).sphereNorm.at(k))>0.9)
                        {
                            sphereNormEqual = true;
                            break;
                        }
                    }

                    if(sphereNormEqual && equaPoint2w(tp,surfacesInfo.at(i)))
                    {
                        for(int j=0;j<surfacesInfo.at(i).vtxinfo.size();j++)
                        {
                            QVector<float> restmp;
                            for(int k=0;k<3;k++)
                            {
                                restmp.push_back(surfacesInfo.at(i).vtxinfo.at(j).pt.at(k).x);
                                restmp.push_back(surfacesInfo.at(i).vtxinfo.at(j).pt.at(k).y);
                                restmp.push_back(surfacesInfo.at(i).vtxinfo.at(j).pt.at(k).z);
                                restmp.push_back(surfacesInfo.at(i).surnorm.x);
                                restmp.push_back(surfacesInfo.at(i).surnorm.y);
                                restmp.push_back(surfacesInfo.at(i).surnorm.z);
                            }
                            qDebug()<<u8"当前选取的面的法向量为："<<surfacesInfo.at(i).surnorm.x<<surfacesInfo.at(i).surnorm.y<<surfacesInfo.at(i).surnorm.z;
                            tests.push_back(restmp);
                        }
                        //getCentralPoint(surfacesInfo.at(i));
                        //maxAxisOnSurface(surfacesInfo.at(i));
                    }
                }


            }
        }




    }


    //选取点的模式
    if(event->buttons()&Qt::LeftButton && seletype == node)
    {

        if(!shiftPress && !ctrlPress)
        {
            tests.clear();
            drawline.clear();

        }
        QVector3D wp = newWorldPos(event->pos().x(),event->pos().y());
        qDebug() <<u8"新世界坐标:"<<wp.x()<<wp.y()<<wp.z();

        //球 kp到wp的距离小于半径r即可
        //allkp应该移动位置了，经过model变换了
        for(int i=0;i<allKp.size();i++)
        {
            point kpi = allKp.at(i);
            //QVector4D v123 = {float(kpi.x),float(kpi.y),float(kpi.z),1};
            QMatrix4x4 v123 = {float(kpi.x),0,0,0,float(kpi.y),0,0,0,float(kpi.z),0,0,0,0,0,0,1};
            QMatrix4x4 res = model * v123;
            float r = 4 * ratio; //r应该有合适算法

            //射线法判断
            QVector3D camerapos(m_camera.Position.x(),m_camera.Position.y(),m_camera.Position.z());
            float xc = camerapos.x(),yc =  camerapos.y(),zc =  camerapos.z();
            float xw = wp.x()-xc,yw=wp.y()-yc,zw=wp.z()-zc;
            float xr = res(0,0)+modelPos.x(),yr=res(1,0)+modelPos.y(),zr=res(2,0)+modelPos.z();

            //一元二次方程判断b^2-4ac
            float a = xw*xw+yw*yw+zw*zw;
            float b = 2*xc*xw - 2*xr*xw +2*yc*yw -2*yr*yw +2*zc*zw -2*zr*zw;
            float c = (xc-xr)*(xc-xr) + (yc-yr)*(yc-yr) + (zc-zr)*(zc-zr) - r*r;


            if(b*b-4*a*c>=0)
            {
                qDebug()<<u8"变化前的坐标" << kpi.x<<kpi.y<<kpi.z;
                //qDebug()<<u8"变化后的坐标" << xr<<yr<<zr;
                //绘图，选取点的绘图，画一个正方体，距离中心点距离均设成固定值，如0.5 / 1

                paintKeyPoint(kpi.x,kpi.y,kpi.z);

            }

        }

    }

    if(event->buttons()&Qt::LeftButton && seletype == line)
    {
        if(!shiftPress && !ctrlPress)
        {
            tests.clear();
            drawline.clear();

        }
        QVector3D wp = newWorldPos(event->pos().x(),event->pos().y());
        qDebug() <<u8"新世界坐标:"<<wp.x()<<wp.y()<<wp.z();

        //包络球 kp到wp的距离小于半径r即可
        //allkp应该移动位置了，经过model变换了
        for(int i=0;i<allLine.size();i++)
        {
            linestruct kpl = allLine.at(i);
            int touchnum = 0;

            point kpi = kpl.centralp;
            if(isOnSphere(kpi,wp))
            {
                touchnum++;
            }

            kpi = kpl.on1at4;
            if(isOnSphere(kpi,wp))
            {
                touchnum++;
            }

            kpi = kpl.on3at4;
            if(isOnSphere(kpi,wp))
            {
                touchnum++;
            }

            if(touchnum!=0)
            {
                drawline.push_back(kpl);
            }



        }

    }

    doneCurrent();
    event->accept();

}

void Widget::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Shift)
    {
        shiftPress = true;
        //qDebug()<<"shiftpress 1";
    }

    if(event->key() == Qt::Key_Control)
    {
        ctrlPress = true;
        //qDebug()<<"shiftpress 1";
    }

    float deltaTime=timeOutmSec/250.0f;

    switch (event->key()) {
    case Qt::Key_W: m_camera.ProcessKeyboard(FORWARD,deltaTime);break;
    case Qt::Key_S: m_camera.ProcessKeyboard(BACKWARD,deltaTime);break;
    case Qt::Key_D: m_camera.ProcessKeyboard(RIGHT,deltaTime);break;
    case Qt::Key_A: m_camera.ProcessKeyboard(LEFT,deltaTime);break;
    case Qt::Key_Q: m_camera.ProcessKeyboard(DOWN,deltaTime);break;
    case Qt::Key_E: m_camera.ProcessKeyboard(UP,deltaTime);break;
    case Qt::Key_Space: m_camera.Position=viewInitPos;break;

    //切换选取模式
    case Qt::Key_K:
    {
        seletype = node;
        qDebug()<<u8"切换成了选取点模式";
        break;
    }
    case Qt::Key_L: {
        seletype = line;
        qDebug()<<u8"切换成了选取线模式";
        break;
    }
    case Qt::Key_M: {
        seletype = surface;
        qDebug()<<u8"切换成了选取面模式";
        break;
    }

    default:break;
    }

}

void Widget::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Shift)
    {
        shiftPress = false;
    }
    if(event->key() == Qt::Key_Control)
    {
        ctrlPress = false;
    }
}


void Widget::mouseMoveEvent(QMouseEvent *event)
{

    if(event->buttons() == Qt::RightButton)
    {
        QVector2D newPos = (QVector2D)event->pos();
        QVector2D diff = newPos - mousePos;
        qreal angle = (diff.length())/3.6;
        // Rotation axis is perpendicular to the mouse position difference
        // vector
        QVector3D normalizeAxis = QVector3D(diff.y(), diff.x(), 0.0).normalized();
        QVector3D rotationAxis = normalizeAxis;
        rotation = QQuaternion::fromAxisAndAngle(rotationAxis, angle) * rotation;
        //QMatrix4x4 tmp = fromQuaternion(rotation);
        //glRotatef(angle,diff.y(),diff.x(),0.0);
        /*float c = cos(angle);
        float s = sin(angle);
        node nor = {normalizeAxis.x(),normalizeAxis.y(),normalizeAxis.z()};
        node old = {modelPos.x(),modelPos.y(),modelPos.z()};
        float newx,newy,newz;
        float x = nor.x,y=nor.y,z=nor.z;


        newx = (x*x*(1-c)+c) * old.x + (x*y*(1-c)-z*s) * old.y + (x*z*(1-c)+y*s) * old.z;

        newy = (y*x*(1-c)+z*s) * old.x + (y*y*(1-c)+c) * old.y + (y*z*(1-c)-x*s) * old.z;

        newz = (x*z*(1-c)-y*s) * old.x + (y*z*(1-c)+x*s) * old.y + (z*z*(1-c)+c) * old.z;

        modelPos = QVector3D(newx,newy,newz);*/
        mousePos = newPos;

        this->update();
    }
    if(event->buttons()&Qt::LeftButton && ctrlPress)
    {
        auto currentPos = event->pos();
        QPoint deltaPos = currentPos-lastPos;
        lastPos = currentPos;
        //qDebug()<<u8"modelpos改变前位置："<<modelPos.x()<<modelPos.y()<<modelPos.z();
        modelPos = QVector3D(modelPos.x()+deltaPos.x()/5,modelPos.y()-deltaPos.y()/5,modelPos.z());

        //qDebug()<<u8"modelpos改变后位置："<<modelPos.x()<<modelPos.y()<<modelPos.z();
        //m_camera.ProcessMouseMovement(deltaPos.x(),-deltaPos.y());

        this->update();
    }

    update();
    lastPos = event->pos();
    event->accept();
}

void Widget::wheelEvent(QWheelEvent *event)
{
    m_camera.ProcessMouseScroll(event->angleDelta().y()/60);
}

QVector4D Widget::worldPosFromViewPort(float posX, float posY)
{
    float winZ;
    glReadPixels(
                posX,
                this->height()-posY
                ,1,1
                ,GL_DEPTH_COMPONENT,GL_FLOAT
                ,&winZ);
    float x=(2.0f*posX)/this->width()-1.0f;
    float y=1.0f-(2.0f*posY)/this->height();
    float z=winZ*2.0-1.0f;

    float w = (2.0 * _near * _far) / (_far + _near - z * (_far - _near));
    //float w= _near*_far/(_near*winZ-_far*winZ+_far);
    QVector4D wolrdPostion(x,y,z,1);
    wolrdPostion=w*wolrdPostion;
    return view.inverted()*projection.inverted()*wolrdPostion;

}

void Widget::getVtxInfo(std::vector<FACET> FACETINFO)
{
    //往vertices里面存数据，不然OpenGL画不出来
    for(int i=0;i<FACETINFO.size();i++)
    {
        vtx tp;
        tp.norm.x = FACETINFO.at(i).normal[0]*ratio;
        tp.norm.y = FACETINFO.at(i).normal[1]*ratio;
        tp.norm.z = FACETINFO.at(i).normal[2]*ratio;
        point tmp;
        tmp.x = FACETINFO.at(i).vertex[0][0]*ratio;
        tmp.y = FACETINFO.at(i).vertex[0][1]*ratio;
        tmp.z = FACETINFO.at(i).vertex[0][2]*ratio;
        tp.pt.push_back(tmp);
        tmp.x = FACETINFO.at(i).vertex[1][0]*ratio;
        tmp.y = FACETINFO.at(i).vertex[1][1]*ratio;
        tmp.z = FACETINFO.at(i).vertex[1][2]*ratio;
        tp.pt.push_back(tmp);
        tmp.x = FACETINFO.at(i).vertex[2][0]*ratio;
        tmp.y = FACETINFO.at(i).vertex[2][1]*ratio;
        tmp.z = FACETINFO.at(i).vertex[2][2]*ratio;
        tp.pt.push_back(tmp);
        vertInfo.push_back(tp);

        vertices.push_back(FACETINFO.at(i).vertex[0][0]*ratio);
        vertices.push_back(FACETINFO.at(i).vertex[0][1]*ratio);
        vertices.push_back(FACETINFO.at(i).vertex[0][2]*ratio);
        vertices.push_back(FACETINFO.at(i).normal[0]*ratio);
        vertices.push_back(FACETINFO.at(i).normal[1]*ratio);
        vertices.push_back(FACETINFO.at(i).normal[2]*ratio);
        vertices.push_back(FACETINFO.at(i).vertex[1][0]*ratio);
        vertices.push_back(FACETINFO.at(i).vertex[1][1]*ratio);
        vertices.push_back(FACETINFO.at(i).vertex[1][2]*ratio);
        vertices.push_back(FACETINFO.at(i).normal[0]*ratio);
        vertices.push_back(FACETINFO.at(i).normal[1]*ratio);
        vertices.push_back(FACETINFO.at(i).normal[2]*ratio);
        vertices.push_back(FACETINFO.at(i).vertex[2][0]*ratio);
        vertices.push_back(FACETINFO.at(i).vertex[2][1]*ratio);
        vertices.push_back(FACETINFO.at(i).vertex[2][2]*ratio);
        vertices.push_back(FACETINFO.at(i).normal[0]*ratio);
        vertices.push_back(FACETINFO.at(i).normal[1]*ratio);
        vertices.push_back(FACETINFO.at(i).normal[2]*ratio);
    }


}

void Widget::on_timeout()
{
    update();
}

QVector3D Widget::newWorldPos(float posX,float posY)
{
    float x = (2.0f * posX) / width() - 1.0f;
    float y = 1.0f - (2.0f * posY) / height();
    float z = 1.0f;

    QVector3D ray_nds = QVector3D(x, y, z);
    QVector4D ray_clip = QVector4D(ray_nds.x(), ray_nds.y(), ray_nds.z(), 1.0f);
    QVector4D ray_eye = projection.inverted() * ray_clip;
    QVector4D ray_world = view.inverted() * ray_eye;
    float x1,y1,z1,w1 = ray_world.w();
    //qDebug()<<"ray_world before"<<ray_world.x()<<ray_world.y()<<ray_world.z()<<ray_world.w();
    if (ray_world.w() != 0.0)
    {
        x1 = ray_world.x() / ray_world.w();
        y1 = ray_world.y() / ray_world.w();
        z1 = ray_world.z() / ray_world.w();
    }
    ray_world = QVector4D(x1,y1,z1,w1);
    //qDebug()<<"ray_world after"<<ray_world.x()<<ray_world.y()<<ray_world.z()<<ray_world.w();

    QVector3D p = QVector3D(ray_world.x(), ray_world.y(), ray_world.z());
    return p;
}

bool Widget::IntersectTriangle(const Vector3& orig, const Vector3& dir,
     Vector3& v0, Vector3& v1, Vector3& v2,
     float* t, float* u, float* v)
 {
     // E1
     Vector3 E1 = v1 - v0;

     // E2
     Vector3 E2 = v2 - v0;

     // P
     Vector3 P = dir.Cross(E2);

     // determinant
     float det = E1.Dot(P);

     // keep det > 0, modify T accordingly
     Vector3 T(0,0,0);
     if( det >0 )
     {
         T = orig - v0;
     }
     else
     {
         T = v0 - orig;
         det = -det;
     }

     // If determinant is near zero, ray lies in plane of triangle
     if( det < 0.0001f )
         return false;

     // Calculate u and make sure u <= 1
     *u = T.Dot(P);
     if( *u < 0.0f || *u > det )
         return false;

     // Q
     Vector3 Q = T.Cross(E1);

     // Calculate v and make sure u + v <= 1
     *v = dir.Dot(Q);
     if( *v < 0.0f || *u + *v > det )
         return false;

     // Calculate t, scale parameters, ray intersects triangle
     *t = E2.Dot(Q);

     float fInvDet = 1.0f / det;
     *t *= fInvDet;
     *u *= fInvDet;
     *v *= fInvDet;

     return true;
 }

// Determine whether point P in triangle ABC
bool Widget::PointinTriangle(Vector3 A, Vector3 B, Vector3 C, Vector3 P)
{
    Vector3 v0 = C - A ;
    Vector3 v1 = B - A ;
    Vector3 v2 = P - A ;

    float dot00 = v0.Dot(v0) ;
    float dot01 = v0.Dot(v1) ;
    float dot02 = v0.Dot(v2) ;
    float dot11 = v1.Dot(v1) ;
    float dot12 = v1.Dot(v2) ;

    float inverDeno = 1 / (dot00 * dot11 - dot01 * dot01) ;

    float u = (dot11 * dot02 - dot01 * dot12) * inverDeno ;
    if (u < 0 || u > 1) // if u out of range, return directly
    {
        return false ;
    }

    float v = (dot00 * dot12 - dot01 * dot02) * inverDeno ;
    if (v < 0 || v > 1) // if v out of range, return directly
    {
        return false ;
    }

    return u + v <= 1 ;
}

////面片拣选算法
//bool Widget::hasPickingFace(QVector3D d, QVector3D cameraPos, QVector3D pos, unsigned *indicess, unsigned indlen, float* vaGrps)
//{
//    int index = 0;
//    /*  首先遍历顶点索引数组，获取每个面片的顶点位置信息  因为采用的三角网格模型，所以每次遍历3个顶点  */
//    for (int i = 0; i < indlen; i += 3) {

//        /*注意因为默认都是模型在(0,0,0)时的坐标，所以都要加上pos，变换到模型所在位置*/
//        index = indicess[i] * 3;
//        QVector3D v1 = QVector3D(vaGrps[index], vaGrps[index + 1], vaGrps[index + 2]) + pos;
//        index = indicess[i + 1] * 3;
//        QVector3D v2 = QVector3D(vaGrps[index], vaGrps[index + 1], vaGrps[index + 2]) + pos;
//        index = indicess[i + 2] * 3;
//        QVector3D v3 = QVector3D(vaGrps[index], vaGrps[index + 1], vaGrps[index + 2]) + pos;

//        /*std::cout << "..................................cur test  ：" << i / 3 << std::endl;
//        std::cout << "V1： " << v1[0] << "," << v1[1] << "," << v1[2] << std::endl;
//        std::cout << "V2： " << v2[0] << "," << v2[1] << "," << v2[2] << std::endl;
//        std::cout << "V3： " << v3[0] << "," << v3[1] << "," << v3[2] << std::endl;*/


//        float T, U, V;
//        if (IntersectTriangle(cameraPos, d, v1, v2, v3, &T, &U, &V)) {

//            QVector3D intpos = cameraPos + QVector3D(d.x() * T, d.y() * T, d.z() * T);
//            if (PointinTriangle(v1, v2, v3, intpos)) {
//                return true;
//            }
//        }
//    }
//    //遍历所有面片仍未返回说明这个网格与鼠标无交点，返回false
//    return false;
//}

void Widget::getSurfaceInfo()
{
    //把法向量相同的三角形片归到一个面上
    for(int i=0;i<vertInfo.size();i++)
    {
        point trii = vertInfo.at(i).norm;
        surf tmpsurf;
        tmpsurf.surnorm = trii;
        tmpsurf.vtxinfo.push_back(vertInfo.at(i));
        tmpsurf.sphereNorm.push_back(trii);
        surfacesInfo.push_back(tmpsurf);
    }

    for(QVector<surf>::iterator it = surfacesInfo.begin();it!=surfacesInfo.end();it++)
    {
        for(QVector<surf>::iterator itt = it+1;itt!=surfacesInfo.end();)
        {
            bool find = false;
            for(int i=0;i<itt->vtxinfo.size();i++)
            {
                if(equaPoint2w(itt->vtxinfo.at(i),*it))
                {
                    find = true;
                    break;
                }
            }
            //设置两向量夹角小于60度归为同一平面
            if(vecAndVecCos(itt->surnorm,it->surnorm) > 0.9 && find)
            //if((itt->surnorm.x == it->surnorm.x &&itt->surnorm.y == it->surnorm.y&&itt->surnorm.z == it->surnorm.z) && find)
            {
                for(int j=0;j<itt->vtxinfo.size();j++)
                {
                    it->vtxinfo.push_back(itt->vtxinfo.at(j));
                }
                it->surnorm = itt->surnorm;
                for(int j=0;j<itt->sphereNorm.size();j++)
                {
                    it->sphereNorm.push_back(itt->sphereNorm.at(j));
                }
                surfacesInfo.erase(itt);
            }
            else
            {
                itt++;
            }
        }
    }

    //再去遍历判定一次？所有点都在面上则合并
    for(QVector<surf>::iterator it = surfacesInfo.begin();it!=surfacesInfo.end();it++)
    {
        for(QVector<surf>::iterator itt = it+1;itt!=surfacesInfo.end();)
        {
            bool flag = false;
            for(int k =0;k<itt->vtxinfo.size();k++)
            {
                if(vtxAllPointsOnSurface(itt->vtxinfo.at(k),*it))
                {
                    flag = true;
                    break;
                }
            }
            if(flag && vecAndVecCos(itt->surnorm,it->surnorm) > 0.9)
            {
                for(int i=0;i<itt->vtxinfo.size();i++)
                {
                    it->vtxinfo.push_back(itt->vtxinfo.at(i));
                }
                surfacesInfo.erase(itt);
            }
            else
            {
                itt++;
            }
        }
    }

    //再遍历一次，去除意外情况
    for(QVector<surf>::iterator it = surfacesInfo.begin();it!=surfacesInfo.end();it++)
    {
        for(QVector<surf>::iterator itt = it+1;itt!=surfacesInfo.end();)
        {
            bool find = false;
            for(int i=0;i<itt->vtxinfo.size();i++)
            {
                if(equaPoint2w(itt->vtxinfo.at(i),*it))
                {
                    find = true;
                    break;
                }
            }
            //设置两向量夹角小于60度归为同一平面
            if(vecAndVecCos(itt->surnorm,it->surnorm) > 0.9 && find)
            {
                for(int j=0;j<itt->vtxinfo.size();j++)
                {
                    it->vtxinfo.push_back(itt->vtxinfo.at(j));
                }
                it->surnorm = itt->surnorm;
                for(int j=0;j<itt->sphereNorm.size();j++)
                {
                    it->sphereNorm.push_back(itt->sphereNorm.at(j));
                }
                surfacesInfo.erase(itt);
            }
            else
            {
                itt++;
            }
        }
    }




    /*for(int i=0;i<surfacesInfo.size();i++)
    {
        qDebug()<<"surfaceinfo norm"<<surfacesInfo.at(i).surnorm.x<<surfacesInfo.at(i).surnorm.y<<surfacesInfo.at(i).surnorm.z;
        qDebug()<<"surfaceinfo vtxsize"<<surfacesInfo.at(i).vtxinfo.size();
    }*/

}



void Widget::getBodyInfo()
{
    //面的法向量相同，但顶点不同？ 或者是所有vtx的顶点都不同？
    //在面区分完之后，法向量相同的面，若不是同一个，则是不同体
    //可以效仿面的创立，先把所有的面都假设成一个体，然后erase和push

    for(int i=0;i<surfacesInfo.size();i++)
    {
        surf surf1 = surfacesInfo.at(i);
        body bo;
        bo.surfinfo.push_back(surf1);
        bodyInfo.push_back(bo);
    }
/*
    //目前是每个面都分配了一个体
    for(QVector<body>::iterator it = bodyInfo.begin();it!=bodyInfo.end();it++)
    {
        for(QVector<body>::iterator itt = it+1;itt!=bodyInfo.end();)
        {
            //面法向量若相同，则是不同体的。
            //同体怎么判断呢？
            //法向量不同，且不是相反的，并且没有共同点
            if(it->surfinfo.at(0))
            {
                for(int i=0;i<itt->vtxinfo.size();i++)
                {
                    it->vtxinfo.push_back(itt->vtxinfo.at(i));
                }
                surfacesInfo.erase(itt);
            }
            else
            {
                itt++;
            }
        }
    }
*/
}

bool Widget::equaPoint2w(vtx vt1, surf surf1)
{
    vtx vtj = vt1;

    for(int i=0;i<surf1.vtxinfo.size();i++)
    {
        int num = 0;//重复的点个数
        vtx vtk = surf1.vtxinfo.at(i);
        if((vtj.pt.at(0).x == vtk.pt.at(0).x && vtj.pt.at(0).y == vtk.pt.at(0).y && vtj.pt.at(0).z == vtk.pt.at(0).z)||
                (vtj.pt.at(0).x == vtk.pt.at(1).x && vtj.pt.at(0).y == vtk.pt.at(1).y && vtj.pt.at(0).z == vtk.pt.at(1).z)||
                (vtj.pt.at(0).x == vtk.pt.at(2).x && vtj.pt.at(0).y == vtk.pt.at(2).y && vtj.pt.at(0).z == vtk.pt.at(2).z))
        {
            num++;
        }
        if((vtj.pt.at(1).x == vtk.pt.at(0).x && vtj.pt.at(1).y == vtk.pt.at(0).y && vtj.pt.at(1).z == vtk.pt.at(0).z)||
                (vtj.pt.at(1).x == vtk.pt.at(1).x && vtj.pt.at(1).y == vtk.pt.at(1).y && vtj.pt.at(1).z == vtk.pt.at(1).z)||
                (vtj.pt.at(1).x == vtk.pt.at(2).x && vtj.pt.at(1).y == vtk.pt.at(2).y && vtj.pt.at(1).z == vtk.pt.at(2).z))
        {
            num++;
        }
        if((vtj.pt.at(2).x == vtk.pt.at(0).x && vtj.pt.at(2).y == vtk.pt.at(0).y && vtj.pt.at(2).z == vtk.pt.at(0).z)||
                (vtj.pt.at(2).x == vtk.pt.at(1).x && vtj.pt.at(2).y == vtk.pt.at(1).y && vtj.pt.at(2).z == vtk.pt.at(1).z)||
                (vtj.pt.at(2).x == vtk.pt.at(2).x && vtj.pt.at(2).y == vtk.pt.at(2).y && vtj.pt.at(2).z == vtk.pt.at(2).z))
        {
            num++;
        }
        if(num>=2)
        {
            return true;
        }
    }

    return false;

}

bool Widget::vtxAllPointsOnSurface(vtx vt1, surf surf1)
{
    //换一种思路，三个点都在面上
    vtx vtj = vt1;
    int num = 0;
    for(int i=0;i<surf1.vtxinfo.size();i++)
    {
        vtx vtk = surf1.vtxinfo.at(i);
        if((vtj.pt.at(0).x == vtk.pt.at(0).x && vtj.pt.at(0).y == vtk.pt.at(0).y && vtj.pt.at(0).z == vtk.pt.at(0).z)||
                (vtj.pt.at(0).x == vtk.pt.at(1).x && vtj.pt.at(0).y == vtk.pt.at(1).y && vtj.pt.at(0).z == vtk.pt.at(1).z)||
                (vtj.pt.at(0).x == vtk.pt.at(2).x && vtj.pt.at(0).y == vtk.pt.at(2).y && vtj.pt.at(0).z == vtk.pt.at(2).z))
        {
            num++;
        }
        if((vtj.pt.at(1).x == vtk.pt.at(0).x && vtj.pt.at(1).y == vtk.pt.at(0).y && vtj.pt.at(1).z == vtk.pt.at(0).z)||
                (vtj.pt.at(1).x == vtk.pt.at(1).x && vtj.pt.at(1).y == vtk.pt.at(1).y && vtj.pt.at(1).z == vtk.pt.at(1).z)||
                (vtj.pt.at(1).x == vtk.pt.at(2).x && vtj.pt.at(1).y == vtk.pt.at(2).y && vtj.pt.at(1).z == vtk.pt.at(2).z))
        {
            num++;
        }
        if((vtj.pt.at(2).x == vtk.pt.at(0).x && vtj.pt.at(2).y == vtk.pt.at(0).y && vtj.pt.at(2).z == vtk.pt.at(0).z)||
                (vtj.pt.at(2).x == vtk.pt.at(1).x && vtj.pt.at(2).y == vtk.pt.at(1).y && vtj.pt.at(2).z == vtk.pt.at(1).z)||
                (vtj.pt.at(2).x == vtk.pt.at(2).x && vtj.pt.at(2).y == vtk.pt.at(2).y && vtj.pt.at(2).z == vtk.pt.at(2).z))
        {
            num++;
        }
    }
    if(num>=3)
    {
        return true;
    }
    else
    {
        return false;
    }
}

point Widget::getCentralPoint(surf surf1)
{
    double sumx=0,sumy=0,sumz=0;
    for(int i=0;i<surf1.vtxinfo.size();i++)
    {
        for(int j=0;j<surf1.vtxinfo.at(i).pt.size();j++)
        {
            sumx += surf1.vtxinfo.at(i).pt.at(j).x;
            sumy += surf1.vtxinfo.at(i).pt.at(j).y;
            sumz += surf1.vtxinfo.at(i).pt.at(j).z;
        }
    }

    sumx /= (3*surf1.vtxinfo.size());
    sumy /= (3*surf1.vtxinfo.size());
    sumz /= (3*surf1.vtxinfo.size());

    if(abs(sumx)<1e-5)
        sumx = 0;
    if(abs(sumy)<1e-5)
        sumy = 0;
    if(abs(sumz)<1e-5)
        sumz = 0;

    //qDebug()<<"central point:"<<sumx<<sumy<<sumz;

    return point{sumx,sumy,sumz};
}

QVector<point> Widget::maxAxisOnSurface(surf surf1)
{
    double maxx = -1e5,maxy = -1e5,maxz = -1e5;
    QVector<point> xvec,yvec,zvec;
    QVector<point> res;
    for(int i=0;i<surf1.vtxinfo.size();i++)
    {
        for(int j=0;j<surf1.vtxinfo.at(i).pt.size();j++)
        {
            point tmp = surf1.vtxinfo.at(i).pt.at(j);
            if(tmp.x > maxx)
            {
                xvec.clear();
                maxx = tmp.x;
                xvec.push_back(tmp);
            }
            if(tmp.x == maxx)
            {
                xvec.push_back(tmp);
            }
            if(tmp.y > maxy)
            {
                yvec.clear();
                maxy = tmp.y;
                yvec.push_back(tmp);
            }
            if(tmp.y == maxy)
            {
                yvec.push_back(tmp);
            }
            if(tmp.z > maxz)
            {
                zvec.clear();
                maxz = tmp.z;
                zvec.push_back(tmp);
            }
            if(tmp.z == maxz)
            {
                zvec.push_back(tmp);
            }
        }
    }

    foreach(auto x,xvec)
    {
        res.push_back(x);
    }
    foreach(auto y,yvec)
    {
        res.push_back(y);
    }
    foreach(auto z,zvec)
    {
        res.push_back(z);
    }


    //去重
    for(QVector<point>::iterator it = res.begin();it!=res.end();it++)
    {
        for(QVector<point>::iterator itt = it+1;itt!=res.end();)
        {
            if(itt->x == it->x && itt->y == it->y && itt->z == it->z)
            {
                res.erase(itt);
            }
            else
                itt++;
        }
    }

    /*
    for(int i=0;i<res.size();i++)
    {
         qDebug()<<"key point"<<res.at(i).x<<res.at(i).y<<res.at(i).z;
    }
    */

    return res;

}


QVector<point> Widget::getALLKP()
{
    QVector<point> res;
    for(int i=0;i<surfacesInfo.size();i++)
    {
        QVector<point> tmp = maxAxisOnSurface(surfacesInfo.at(i));
        foreach (auto x, tmp) {
            res.push_back(x);
        }
    }

    //去重
    for(QVector<point>::iterator it = res.begin();it!=res.end();it++)
    {
        for(QVector<point>::iterator itt = it+1;itt!=res.end();)
        {
            if(itt->x == it->x && itt->y == it->y && itt->z == it->z)
            {
                res.erase(itt);
            }
            else
                itt++;
        }
    }
/*
    for(int i=0;i<res.size();i++)
    {
        qDebug()<<"kp info"<<i+1<< res.at(i).x<<res.at(i).y<<res.at(i).z;
    }
*/

    return res;

}

QVector<linestruct> Widget::getALLLine()
{
    //连接所有关键点，但是需要排除穿过模型内部的线
    QVector<linestruct> res;
    for(int i=0;i<allKp.size();i++)
    {
        for(int j=i+1;j<allKp.size();j++)
        {
            linestruct tmp;
            tmp.centralp.x = (allKp.at(i).x + allKp.at(j).x)/2;
            tmp.centralp.y = (allKp.at(i).y + allKp.at(j).y)/2;
            tmp.centralp.z = (allKp.at(i).z + allKp.at(j).z)/2;
            tmp.p1 = allKp.at(i);
            tmp.p2 = allKp.at(j);
            tmp.on1at4.x = (tmp.centralp.x + tmp.p1.x)/2;
            tmp.on1at4.y = (tmp.centralp.y + tmp.p1.y)/2;
            tmp.on1at4.z = (tmp.centralp.z + tmp.p1.z)/2;
            tmp.on3at4.x = (tmp.centralp.x + tmp.p2.x)/2;
            tmp.on3at4.y = (tmp.centralp.y + tmp.p2.y)/2;
            tmp.on3at4.z = (tmp.centralp.z + tmp.p2.z)/2;
            res.push_back(tmp);
        }
    }

    //16个点有120条线
    //C16 15 = 120

    //试图排除掉穿过模型内部的线
    //线不属于任何一个面内，则排除。那如何确定线在面内呢？用所有法向量判断是否垂直（可能只能排除一部分）
    //若都不垂直，则排除
    //两个kp一定要是同一个面内的  也就是和面内法向量垂直的同时，面内的三角形片中含有这两个点即可

    for(QVector<linestruct>::iterator it = res.begin();it!=res.end();)
    {
        bool flag = false;
        for(int k=0;k<surfacesInfo.size();k++)
        {
            int equanum1 = 0;
            int equanum2 = 0;
            //用于判断当前面上的三角形片中有几个和it相同的，若是2个则可以确定在该面。
            for(int x=0;x<surfacesInfo.at(k).vtxinfo.size();x++)
            {
                for(int q=0;q<3;q++)
                {
                    point tmp = surfacesInfo.at(k).vtxinfo.at(x).pt.at(q);
                    if(tmp.x == it->p1.x && tmp.y == it->p1.y && tmp.z ==it->p1.z)
                    {
                        equanum1++;
                        //qDebug()<<"equanum++"<<tmp.x<<tmp.y<<tmp.z;
                    }
                    if(tmp.x == it->p2.x && tmp.y == it->p2.y && tmp.z ==it->p2.z)
                    {
                        equanum2++;
                        //qDebug()<<"equanum++"<<tmp.x<<tmp.y<<tmp.z;
                    }
                }
            }
            //判断法向量是否垂直
            if(pointLineisVerticaltoSurface(it->p1,it->p2,surfacesInfo.at(k)) && equanum1+equanum2 >= 2 && (equanum1!=0 && equanum2!=0))
            {
                flag = true;
                //qDebug()<<u8"it此时信息为"<<it->p1.x<<it->p1.y<<it->p1.z<<it->p2.x<<it->p2.y<<it->p2.z<<u8" 在第"<<k<<u8"个面上,该面的信息为："<<surfacesInfo.at(k).surnorm.x<<surfacesInfo.at(k).surnorm.y
                //       <<surfacesInfo.at(k).surnorm.z<<u8"面上的vtx信息为："<<surfacesInfo.at(k).vtxinfo.at(0).pt.at(0).x<<surfacesInfo.at(k).vtxinfo.at(0).pt.at(0).y<<surfacesInfo.at(k).vtxinfo.at(0).pt.at(0).z;
            }

        }
        if(!flag)
        {
            res.erase(it);
        }
        else
        {
            it++;
        }
    }

    //去重
    for(QVector<linestruct>::iterator it = res.begin();it!=res.end();it++)
    {
        for(QVector<linestruct>::iterator itt = it+1;itt!=res.end();)
        {
            if((itt->p1.x == it->p1.x && itt->p1.y == it->p1.y && itt->p1.z == it->p1.z)&&(itt->p2.x == it->p2.x && itt->p2.y == it->p2.y && itt->p2.z == it->p2.z))
            {
                res.erase(itt);
            }
            else
                itt++;
        }
    }


/*
    for(int i=0;i<res.size();i++)
    {
        qDebug()<<"line info"<<i+1<< res.at(i).p1.x<<res.at(i).p1.y<<res.at(i).p1.z<<res.at(i).p2.x<<res.at(i).p2.y<<res.at(i).p2.z;
    }
*/


    return res;
}

bool Widget::pointLineisVerticaltoSurface(point p1, point p2, surf surf1)
{
    bool res = false;

    point vec = {p1.x-p2.x,p1.y-p2.y,p1.z-p2.z};
    point surfv = surf1.surnorm;

    if(vec.x*surfv.x+vec.y*surfv.y+vec.z*surfv.z == 0)
    {
        res = true;
        //qDebug()<<"vec surfv"<<vec.x<<vec.y<<vec.z<<surfv.x<<surfv.y<<surfv.z;
    }



    return res;
}

void Widget::paintKeyPoint(float xro, float yro, float zro)
{
    //绘制选取面的时候，用的是原始坐标。
    //所有绘制都用原始坐标，涉及到考虑射线是否和目标有交点时，原坐标才需要转换。


    QVector<int> rec = {1,2,4,2,3,4,5,8,6,6,8,7,4,8,5,5,1,4,6,7,3,2,6,3,1,6,2,5,6,1,3,7,4,7,8,4};

    //124 234 586 687
    //485 514 673 263
    //162 561 374 784
    double showl = 0.2;
    int index = 0;

    float tpx;
    float tpy;
    float tpz;
    int n=36;
    int curindex = -1;
    QVector<float> tri;

    while(n--)
    {
        curindex = 36-n;
        index = rec.at(36-n-1);

        switch (index) {
        case 1:
            tpx = xro+showl;
            tpy = yro+showl;
            tpz = zro+showl;
            break;
        case 2:
            tpx = xro-showl;
            tpy = yro+showl;
            tpz = zro+showl;
            break;
        case 3:
            tpx = xro-showl;
            tpy = yro-showl;
            tpz = zro+showl;
            break;
        case 4:
            tpx = xro+showl;
            tpy = yro-showl;
            tpz = zro+showl;
            break;
        case 5:
            tpx = xro+showl;
            tpy = yro+showl;
            tpz = zro-showl;
            break;
        case 6:
            tpx = xro-showl;
            tpy = yro+showl;
            tpz = zro-showl;
            break;
        case 7:
            tpx = xro-showl;
            tpy = yro-showl;
            tpz = zro-showl;
            break;
        case 8:
            tpx = xro+showl;
            tpy = yro-showl;
            tpz = zro-showl;
            break;
        default:
            break;
        }
        float xr =tpx,yr=tpy,zr=tpz;


        if(curindex<7)
        {
            tri.push_back(xr);tri.push_back(yr);tri.push_back(zr);
            tri.push_back(0);tri.push_back(0);tri.push_back(1);
        }
        if(curindex >= 7 && curindex<13)
        {
            tri.push_back(xr);tri.push_back(yr);tri.push_back(zr);
            tri.push_back(0);tri.push_back(0);tri.push_back(-1);
        }
        if(curindex >= 13 && curindex<19)
        {
            tri.push_back(xr);tri.push_back(yr);tri.push_back(zr);
            tri.push_back(1);tri.push_back(0);tri.push_back(0);
        }
        if(curindex >= 19 && curindex<25)
        {
            tri.push_back(xr);tri.push_back(yr);tri.push_back(zr);
            tri.push_back(-1);tri.push_back(0);tri.push_back(0);
        }
        if(curindex >= 25 && curindex<31)
        {
            tri.push_back(xr);tri.push_back(yr);tri.push_back(zr);
            tri.push_back(0);tri.push_back(1);tri.push_back(0);
        }
        if(curindex >= 31 && curindex<37)
        {
            tri.push_back(xr);tri.push_back(yr);tri.push_back(zr);
            tri.push_back(0);tri.push_back(-1);tri.push_back(0);
        }


    }
    tests.push_back(tri);





}

bool Widget::isOnSphere(point kp, QVector3D wp)
{
    point kpi = kp;
    QMatrix4x4 v123 = {float(kpi.x),0,0,0,float(kpi.y),0,0,0,float(kpi.z),0,0,0,0,0,0,1};
    QMatrix4x4 res = model * v123;
    float r = 1; //r应该有合适算法

    //射线法判断
    QVector3D camerapos(m_camera.Position.x(),m_camera.Position.y(),m_camera.Position.z());
    float xc = camerapos.x(),yc =  camerapos.y(),zc =  camerapos.z();
    float xw = wp.x()-xc,yw=wp.y()-yc,zw=wp.z()-zc;
    float xr = res(0,0)+modelPos.x(),yr=res(1,0)+modelPos.y(),zr=res(2,0)+modelPos.z();

    //一元二次方程判断b^2-4ac
    float a = xw*xw+yw*yw+zw*zw;
    float b = 2*xc*xw - 2*xr*xw +2*yc*yw -2*yr*yw +2*zc*zw -2*zr*zw;
    float c = (xc-xr)*(xc-xr) + (yc-yr)*(yc-yr) + (zc-zr)*(zc-zr) - r*r;


    if(b*b-4*a*c>=0)
    {
        return true;
    }
    else
    {
        return false;
    }
}


bool Widget::isAsciiFileType(QString path)
{
    QFile file(path);

    char ch[80];

    if(file.open(QIODevice::ReadOnly))
    {
        file.read(ch,80);

        file.close();
        //读取80个字节 如果是ascii 就会换行了 如果是二进制的不会有换行
        for(auto i = 0; i< 80;i++)
        {
            if(ch[i] == '\n')
            {
                qDebug()<<"this is ascii";
                return true;
            }
        }
        return false;
    }
    qDebug()<<"Open File Failed!";

    return false;
}


void Widget::loadBinarySTL(std::string path, std::vector<FACET> &facetVec)
{
    FILE* file = fopen(path.c_str(),"r");

    /*80字节文件头*/
    char header[80];
    fread(header,80,1,file);

    uint32_t triangleNum;
    fread(&triangleNum,sizeof(uint32_t),1,file);

    for(auto i = 0; i< triangleNum;i++)
    {
        /*12字节法向量*/
        FACET temp;

        fread(&temp.normal[0],sizeof(float),1,file);
        fread(&temp.normal[1],sizeof(float),1,file);
        fread(&temp.normal[2],sizeof(float),1,file);

        /*36字节顶点*/
        for(auto j = 0; j< 3;j++)
        {
            fread(&temp.vertex[j][0],sizeof(float),1,file);
            fread(&temp.vertex[j][1],sizeof(float),1,file);
            fread(&temp.vertex[j][2],sizeof(float),1,file);
        }
        /*2字节三角型面片的属性信息*/
        char c[2];
        fread(c,2,1,file);

        facetVec.push_back(temp);
    }

    fclose(file);
    delete file;

}

void Widget::loadAsciiSTL(std::string path, std::vector<FACET> &facetVec)
{
    /*          ASCII STL 格式
     *
     *
     * solid
     *      facet normal +0.0000000E+00 +1.0000000E+00 +0.0000000E+00
                outer loop
                    vertex   +8.0000000E+01 +1.0000000E+02 +6.0000000E+01
                    vertex   +8.0000000E+01 +1.0000000E+02 +0.0000000E+00
                    vertex   +0.0000000E+00 +1.0000000E+02 +0.0000000E+00
                endloop
            endfacet
          ......
        endsolid

     * 共计7行
     *
     *
     *
     *
     *
     * */

    FILE* file = fopen(path.c_str(),"r");

    //把文件指针移动到文件末尾,并计算文件大小
    fseek(file,0L,SEEK_END);
    long fileSize = ftell(file);

    fclose(file);

    file = fopen(path.c_str(),"r");
    //计算文件有多少行
    int fileLines = 0;

    for(long i = 0;i<fileSize;i++)
       //一个字符一个字符的读取 当读到换行符说明读到了一行 直到结束
        if(getc(file) == '\n')
            fileLines++;

    //计算三角形的个数
    int triangleCount = 0;
    //一段三角形的描述是7行 stl文件格式决定的
    triangleCount = fileLines / 7;

    //文件指针移到文件头
    rewind(file);

    //跳过文件头 solid
    while(getc(file) != '\n');


    for(auto i = 0; i< triangleCount;i++)
    {
        FACET temp;

        //读法向量
        fscanf(file,"%*s %*s %f %f %f\n",&temp.normal[0],&temp.normal[1],&temp.normal[2]);
        //跳过 outer loop
        fscanf(file,"%*s %*s");
        //读顶点数据
        fscanf(file,"%*s %f %f %f\n",&temp.vertex[0][0],&temp.vertex[0][1],&temp.vertex[0][2]);
        fscanf(file,"%*s %f %f %f\n",&temp.vertex[1][0],&temp.vertex[1][1],&temp.vertex[1][2]);
        fscanf(file,"%*s %f %f %f\n",&temp.vertex[2][0],&temp.vertex[2][1],&temp.vertex[2][2]);
        //跳过 endloop
        fscanf(file,"%*s");
        //跳过 endfacet
        fscanf(file,"%*s");

        facetVec.push_back(temp);
    }
    //跳过文件尾部 endsolid
    while(getc(file) != '\n');

    fclose(file);

    delete file;

}

float Widget::vecAndVecCos(point p1, point p2)
{
    float dot = p1.x*p2.x + p1.y*p2.y + p1.z*p2.z;
    float lengthp1 = sqrt(p1.x*p1.x + p1.y*p1.y + p1.z*p1.z);
    float lengthp2 = sqrt(p2.x*p2.x + p2.y*p2.y + p2.z*p2.z);

    return abs(dot / (lengthp1 * lengthp2));
}

void Widget::paintAxis()
{
    QWidget * newaxis = new QWidget();
    newaxis->resize(300,150);
    newaxis->show();
    linestruct t;
    t.p1.x = 0.0;
    t.p1.y = 0.0;
    t.p1.z = 0.0;
    t.p2.x = 30.0;
    t.p2.y = 0.0;
    t.p2.z = 0.0;
drawline.push_back(t);
    t.p1.x = 30.0;
    t.p1.y = 0.0;
    t.p1.z = 0.0;
    t.p2.x = 29.0;
    t.p2.y = 1.0;
    t.p2.z = 0.0;
drawline.push_back(t);
    t.p1.x = 30.0;
    t.p1.y = 0.0;
    t.p1.z = 0.0;
    t.p2.x = 29.0;
    t.p2.y = -1.0;
    t.p2.z = 0.0;
drawline.push_back(t);


    t.p1.x = 0.0;
    t.p1.y = 0.0;
    t.p1.z = 0.0;
    t.p2.x = 0.0;
    t.p2.y = 30.0;
    t.p2.z = 0.0;
    drawline_blue.push_back(t);
    t.p1.x = 0.0;
    t.p1.y = 30.0;
    t.p1.z = 0.0;
    t.p2.x = 1.0;
    t.p2.y = 29.0;
    t.p2.z = 0.0;
drawline_blue.push_back(t);
    t.p1.x = 0.0;
    t.p1.y = 30.0;
    t.p1.z = 0.0;
    t.p2.x = -1.0;
    t.p2.y = 29.0;
    t.p2.z = 0.0;
drawline_blue.push_back(t);


    t.p1.x = 0.0;
    t.p1.y = 0.0;
    t.p1.z = 0.0;
    t.p2.x = 0.0;
    t.p2.y = 0.0;
    t.p2.z = 30.0;
    drawline_green.push_back(t);
    t.p1.x = 0.0;
    t.p1.y = 0.0;
    t.p1.z = 30.0;
    t.p2.x = 1.0;
    t.p2.y = 0.0;
    t.p2.z = 29.0;
drawline_green.push_back(t);
    t.p1.x = 0.0;
    t.p1.y = 0.0;
    t.p1.z = 30.0;
    t.p2.x = -1.0;
    t.p2.y = 0.0;
    t.p2.z = 29.0;
drawline_green.push_back(t);

}




