#include "GL/glu.h"
#include "imuread.h"
#include "visualize.h"

MagCalibration_t magcal;
Quaternion_t current_orientation;


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




static GLuint spherelist;
static GLuint spherelowreslist;

int invert_q0=0;
int invert_q1=0;
int invert_q2=0;
int invert_q3=1;
int invert_x=0;
int invert_y=0;
int invert_z=0;

void display_callback(void)
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
#if 0
    gluLookAt(0.0, 0.0, 0.8, // eye location
          0.0, 0.0, 0.0, // center
          0.0, 1.0, 0.0); // up direction
#endif
    xscale = 0.05;
    yscale = 0.05;
    zscale = 0.05;
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
#if 0
    printf(" quality: %5.1f  %5.1f  %5.1f  %5.1f\n",
        quality_surface_gap_error(),
        quality_magnitude_variance_error(),
        quality_wobble_error(),
        quality_spherical_fit_error());
#endif
}

void resize_callback(int width, int height)
{
    const float ar = (float) width / (float) height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity() ;
}


static const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
static const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

static const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
static const GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
static const GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat high_shininess[] = { 100.0f };

void visualize_init(void)
{
    GLUquadric *sphere;

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    //glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_LIGHT0);
    //glEnable(GL_NORMALIZE);
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

    sphere = gluNewQuadric();
    gluQuadricDrawStyle(sphere, GLU_FILL);
    gluQuadricNormals(sphere, GLU_SMOOTH);
    spherelist = glGenLists(1);
    glNewList(spherelist, GL_COMPILE);
    gluSphere(sphere, 0.08, 16, 14);
    glEndList();
    spherelowreslist = glGenLists(1);
    glNewList(spherelowreslist, GL_COMPILE);
    gluSphere(sphere, 0.08, 12, 10);
    glEndList();
    gluDeleteQuadric(sphere);
}
