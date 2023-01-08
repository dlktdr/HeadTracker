/* This work is from https://github.com/PaulStoffregen/MotionCal
 *    _ Gives a visual representation of the current calibration
 */

#include <QOpenGLFunctions>
#include <QtOpenGL>
#include "calibrate/imuread.h"
#include "magcalwidget.h"

static const GLfloat light_ambient[4]  = { 0.0f, 0.0f, 0.0f, 1.0f };
//static const GLfloat light_ambient[4]  = { 0.4f, 0.4f, 0.4f, 1.0f };
static const GLfloat light_diffuse[4]  = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat light_specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat light_position[4] = { 5.0f, 5.0f, -3.0f, 0.0f };

static const GLfloat mat_ambient[4]    = { 0.7f, 0.7f, 0.7f, 1.0f };
static const GLfloat mat_diffuse[4]    = { 0.8f, 0.8f, 0.8f, 1.0f };
static const GLfloat mat_specular[4]   = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat high_shininess[1] = { 100.0f };

MagCalibration_t magcal;
Quaternion_t current_orientation;

static void rotate(const Point_t *in, Point_t *out, const float *rmatrix)
{
    out->x = in->x * rmatrix[0] + in->y * rmatrix[1] + in->z * rmatrix[2];
    out->y = in->x * rmatrix[3] + in->y * rmatrix[4] + in->z * rmatrix[5];
    out->z = in->x * rmatrix[6] + in->y * rmatrix[7] + in->z * rmatrix[8];
}

static void quad_to_rotation(const Quaternion_t *quat, float *rmatrix)
{
    float qw = quat->q0;
    float qx = quat->q1;
    float qy = quat->q2;
    float qz = quat->q3;
    rmatrix[0] = 1.0f - 2.0f * qy * qy - 2.0f * qz * qz;
    rmatrix[1] = 2.0f * qx * qy - 2.0f * qz * qw;
    rmatrix[2] = 2.0f * qx * qz + 2.0f * qy * qw;
    rmatrix[3] = 2.0f * qx * qy + 2.0f * qz * qw;
    rmatrix[4] = 1.0f  - 2.0f * qx * qx - 2.0f * qz * qz;
    rmatrix[5] = 2.0f * qy * qz - 2.0f * qx * qw;
    rmatrix[6] = 2.0f * qx * qz - 2.0f * qy * qw;
    rmatrix[7] = 2.0f * qy * qz + 2.0f * qx * qw;
    rmatrix[8] = 1.0f  - 2.0f * qx * qx - 2.0f * qy * qy;
}

void apply_calibration(int16_t rawx, int16_t rawy, int16_t rawz, Point_t *out)
{
    float x, y, z;

    x = ((float)rawx * UT_PER_COUNT) - magcal.V[0];
    y = ((float)rawy * UT_PER_COUNT) - magcal.V[1];
    z = ((float)rawz * UT_PER_COUNT) - magcal.V[2];
    out->x = x * magcal.invW[0][0] + y * magcal.invW[0][1] + z * magcal.invW[0][2];
    out->y = x * magcal.invW[1][0] + y * magcal.invW[1][1] + z * magcal.invW[1][2];
    out->z = x * magcal.invW[2][0] + y * magcal.invW[2][1] + z * magcal.invW[2][2];
}

void calibration_confirmed(void)
{
    //show_calibration_confirmed = true;
}

MagCalWidget::MagCalWidget(QWidget *parent) : QOpenGLWidget(parent)
{

    raw_data_reset();

    //visualize_init();
}


void MagCalWidget::rawMagChanged(float x, float y, float z)
{
    int16_t data[9] = {0,0,0,0,0,0,0,0,0};
    points=(points+1)%MAGBUFFSIZE;
    data[6] = x*10;
    data[7] = y*10;
    data[8] = z*10;

    // Do the stuff
    raw_data(data);
    update();
}

void MagCalWidget::setTracker(TrackerSettings *trk)
{
    trkset = trk;
    connect(trkset,&TrackerSettings::rawMagChanged,this,&MagCalWidget::rawMagChanged);
}

void MagCalWidget::resetDataPoints()
{
    raw_data_reset();
}

void MagCalWidget::drawSphere(double r, int lats, int longs) {
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

void MagCalWidget::initializeGL()
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);


    spherelist = glGenLists(1);
    glNewList(spherelist, GL_COMPILE);
    drawSphere(0.08, 16, 14);
    glEndList();
    spherelowreslist = glGenLists(1);
    glNewList(spherelowreslist, GL_COMPILE);
    drawSphere(0.08, 12, 10);
    glEndList();

    // Set up the rendering context, load shaders and other resources, etc.:
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void MagCalWidget::resizeGL(int width, int height)
{
    const float ar = (float) width / (float) height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity() ;
}

void MagCalWidget::paintGL()
{
    int i;
    float xscale, yscale, zscale;
    float xoff, yoff, zoff;
    float rotation[9];
    Point_t point, draw;
    Quaternion_t orientation;

    quality_reset();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(1, 0, 0);	// set current color to red
    glLoadIdentity();
    xscale = 0.05f;
    yscale = 0.05f;
    zscale = 0.05f;
    xoff = 0.0;
    yoff = 0.0;
    zoff = -7.0;

    //if (hard_iron.valid) {
    if (1) {
        memcpy(&orientation, &current_orientation, sizeof(orientation));
        // TODO: this almost but doesn't perfectly seems to get the
        //  real & screen axes in sync....
        if (invert_q0) orientation.q0 *= -1.0f;
        if (invert_q1) orientation.q1 *= -1.0f;
        if (invert_q2) orientation.q2 *= -1.0f;
        if (invert_q3) orientation.q3 *= -1.0f;
        quad_to_rotation(&orientation, rotation);

        //rotation[0] *= -1.0f;
        //rotation[1] *= -1.0f;
        //rotation[2] *= -1.0f;
        //rotation[3] *= -1.0f;
        //rotation[4] *= -1.0f;
        //rotation[5] *= -1.0f;
        //rotation[6] *= -1.0f;
        //rotation[7] *= -1.0f;
        //rotation[8] *= -1.0f;

        for (i=0; i < MAGBUFFSIZE; i++) {
            if (magcal.valid[i]) {
                apply_calibration(magcal.BpFast[0][i], magcal.BpFast[1][i],
                    magcal.BpFast[2][i], &point);
                //point.x *= -1.0f;
                //point.y *= -1.0f;
                //point.z *= -1.0f;
                quality_update(&point);
                rotate(&point, &draw, rotation);
                glPushMatrix();
                if (invert_x) draw.x *= -1.0f;
                if (invert_y) draw.y *= -1.0f;
                if (invert_z) draw.z *= -1.0f;
                glTranslatef(
                    draw.x * xscale + xoff,
                    draw.z * yscale + yoff,
                    draw.y * zscale + zoff
                );
                if (draw.y >= 0.0f) {
                    glCallList(spherelist);
                } else {
                    glCallList(spherelowreslist);
                }
                glPopMatrix();
            }
        }
    }

    emit dataUpdate(quality_magnitude_variance_error(),
                    quality_surface_gap_error(),
                    quality_wobble_error(),
                    quality_spherical_fit_error(),
                    magcal.V,
                    magcal.invW);
#if 0
    printf(" quality: %5.1f  %5.1f  %5.1f  %5.1f\n",
        quality_surface_gap_error(),
        quality_magnitude_variance_error(),
        quality_wobble_error(),
        quality_spherical_fit_error());
#endif
}
