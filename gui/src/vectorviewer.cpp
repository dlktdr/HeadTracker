#include <QGLWidget>
#include "vectorviewer.h"
#include "ui_vectorviewer.h"
#include <math.h>

static const GLfloat light_ambient[4]  = { 0.0f, 0.0f, 0.0f, 1.0f };
//static const GLfloat light_ambient[4]  = { 0.4f, 0.4f, 0.4f, 1.0f };
static const GLfloat light_diffuse[4]  = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat light_specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat light_position[4] = { 5.0f, 5.0f, -3.0f, 0.0f };

static const GLfloat mat_ambient[4]    = { 0.7f, 0.7f, 0.7f, 1.0f };
static const GLfloat mat_diffuse[4]    = { 0.8f, 0.8f, 0.8f, 1.0f };
static const GLfloat mat_specular[4]   = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat high_shininess[1] = { 100.0f };

VectorOpenGL::VectorOpenGL(TrackerSettings *trk, QWidget *parent)
  : QOpenGLWidget(parent), trkset(trk)
{

}

void VectorOpenGL::dataUpdate()
{

}

void VectorOpenGL::resizeGL(int width, int height)
{
  const float ar = (float) width / (float) height;

  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity() ;
}

void draw_cone(float height, float radius, int slices)
{      // draw the upper part of the cone
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0, 0, height);

    for (int i = 0; i < slices; i++) {
      float angle = 360/slices * i * M_PI / 180;
      float x = sin(angle) * radius;
      float y = cos(angle) * radius;
      float coneAngle = atan(radius / height);
      glNormal3f(cos(coneAngle) * cos(angle), sin(coneAngle), cos(coneAngle) * sin(angle));
      glVertex3f(x, y, 0);
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0, 0, 0);
    for (int i = 0; i < slices; i++) {
      float angle = 360/slices * (slices-i) * M_PI / 180;
      float x = sin(angle) * radius;
      float y = cos(angle) * radius;
      glNormal3f(0, -1, 0);
      glVertex3f(x, y, 0);
    }
    glEnd();
}


void draw_cylinder(GLfloat radius,
                   GLfloat height)
{
    GLfloat x              = 0.0;
    GLfloat y              = 0.0;
    GLfloat angle          = 0.0;
    GLfloat angle_stepsize = 0.1;

    glBegin(GL_QUAD_STRIP);
    angle = 0.0;
        while( angle < 2*M_PI ) {
            x = radius * cos(angle);
            y = radius * sin(angle);
            glVertex3f(x, y , height);
            glVertex3f(x, y , 0.0);
            angle = angle + angle_stepsize;
        }
        glVertex3f(radius, 0.0, height);
        glVertex3f(radius, 0.0, 0.0);
    glEnd();

    /*glBegin(GL_POLYGON);
    angle = 0.0;
        while( angle < 2*M_PI ) {
            x = radius * cos(angle);
            y = radius * sin(angle);
            glVertex3f(x, y , height);
            angle = angle + angle_stepsize;
        }
        glVertex3f(radius, 0.0, height);
    glEnd();*/
}




void drawArrow(float height, float offset=0)
{
  glPushMatrix();
  glRotatef(90, 0.0, 1.0, 0.0);
  glRotatef(180, 1.0, 0.0, 0.0);
  glTranslatef(0,0,-height-offset);
  glScalef(1,1,-1.0);
  draw_cone(2.0,0.75,180);
  glRotatef(180, 1.0, 0.0, 0.0);
  draw_cylinder(0.25,height);
  glPopMatrix();
}

void drawSphere(double r, int lats, int longs) {
    int i, j;
    for(i = 0; i <= lats; i++) {
        double lat0 = M_PI * (-0.5 + (double) (i - 1) / lats);
        double z0  = sin(lat0);
        double zr0 =  cos(lat0);

        double lat1 = M_PI * (-0.5 + (double) i / lats);
        double z1 = sin(lat1);
        double zr1 = cos(lat1);

        glBegin(GL_QUAD_STRIP);
        for(j = 0; j <= longs; j++) {
            double lng = 2 * M_PI * (double) (j - 1) / longs;
            double x = cos(lng);
            double y = sin(lng);

            glNormal3f(x * zr0, y * zr0, z0);
            glVertex3f(r * x * zr0, r * y * zr0, r * z0);
            glNormal3f(x * zr1, y * zr1, z1);
            glVertex3f(r * x * zr1, r * y * zr1, r * z1);
        }
        glEnd();
    }
}

void drawOrigin()
{
  float orginrad = 0.8;
  glColor3f(1, 1, 1);
  drawSphere(orginrad,18,18);
  glColor3f(1, 0, 0);
  drawArrow(5,orginrad);
  glRotatef(90, 0.0, 0.0, 1.0);
  glColor3f(0, 1, 0);
  drawArrow(5,orginrad);
  glColor3f(0, 0, 1);
  glRotatef(90, 0.0, -1.0, 0.0);
  drawArrow(5,orginrad);
}



void VectorOpenGL::initializeGL()
{
    glClearColor(0.0, 0.0, 0.0, 1.0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    static GLfloat lightPosition[4] = { 0, 0, 10, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);

    // Set up the rendering context, load shaders and other resources, etc.:
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void VectorOpenGL::paintGL()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -60.0);
  glRotatef(-80, 1.0, 0.1, 0.0);

  drawOrigin();



}


VectorViewer::VectorViewer(TrackerSettings *trk, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::VectorViewer),
  trkset(trk)
{
  ui->setupUi(this);
  QVBoxLayout *mylay = new QVBoxLayout();
  mylay->setContentsMargins(0,0,0,0);
  this->setLayout(mylay);
  vectGLWidget = new VectorOpenGL(trk);
  mylay->addWidget(vectGLWidget);

  // Disable all data
  QMap<QString, bool> dataitems;
  dataitems.insert("off_accx",true);
  dataitems.insert("off_accy",true);
  dataitems.insert("off_accy",true);
  trkset->setDataItemSend(dataitems);

  connect(trkset,&TrackerSettings::liveDataChanged,vectGLWidget,&VectorOpenGL::dataUpdate);

}

VectorViewer::~VectorViewer()
{
  delete ui;
}
