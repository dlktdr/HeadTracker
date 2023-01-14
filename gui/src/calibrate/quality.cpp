#include "imuread.h"

// Discussion of what these 4 quality metrics really do
// https://forum.pjrc.com/threads/59277-Motion-Sensor-Calibration-Tool-Parameter-Understanding

//static int countdown=1000;
//static int pr=0;

// return 0 to 99 - which region on the sphere (100 of equal surface area)
static int sphere_region(float x, float y, float z)
{
    float latitude, longitude;
    int region;

    //if (pr) printf("  region %.1f,%.1f,%.1f  ", x, y, z);

    // longitude = 0 to 2pi  (meaning 0 to 360 degrees)
    longitude = atan2f(y, x) + (float)M_PI;
    // latitude = -pi/2 to +pi/2  (meaning -90 to +90 degrees)
    latitude = (float)(M_PI / 2.0) - atan2f(sqrtf(x * x + y * y), z);

    //if (pr) printf("   lat=%.1f", latitude * (float)(180.0 / M_PI));
    //if (pr) printf(",lon=%.1f  ", longitude * (float)(180.0 / M_PI));

    // https://etna.mcs.kent.edu/vol.25.2006/pp309-327.dir/pp309-327.html
    // sphere equations....
    //  area of unit sphere = 4*pi
    //  area of unit sphere cap = 2*pi*h  h = cap height
    //  lattitude of unit sphere cap = arcsin(1 - h)
    if (latitude > 1.37046f /* 78.52 deg */) {
        // arctic cap, 1 region
        region = 0;
    } else if (latitude < -1.37046f /* -78.52 deg */) {
        // antarctic cap, 1 region
        region = 99;
    } else if (latitude > 0.74776f /* 42.84 deg */ || latitude < -0.74776f ) {
        // temperate zones, 15 regions each
        region = floorf(longitude * (float)(15.0 / (M_PI * 2.0)));
        if (region < 0) region = 0;
        else if (region > 14) region = 14;
        if (latitude > 0.0) {
            region += 1; // 1 to 15
        } else {
            region += 84; // 84 to 98
        }
    } else {
        // tropic zones, 34 regions each
        region = floorf(longitude * (float)(34.0 / (M_PI * 2.0)));
        if (region < 0) region = 0;
        else if (region > 33) region = 33;
        if (latitude >= 0.0) {
            region += 16; // 16 to 49
        } else {
            region += 50; // 50 to 83
        }
    }
    //if (pr) printf("  %d\n", region);
    return region;
}


static int count=0;
static int spheredist[100];
static Point_t spheredata[100];
static Point_t sphereideal[100];
static int sphereideal_initialized=0;
static float magnitude[MAGBUFFSIZE];
static float quality_gaps_buffer;
static float quality_variance_buffer;
static float quality_wobble_buffer;
static int quality_gaps_computed=0;
static int quality_variance_computed=0;
static int quality_wobble_computed=0;

void quality_reset(void)
{
    float longitude;
    int i;
/*
    countdown--;
    if (countdown == 0) {
        countdown = 1000;
        pr = 1;
    } else {
        pr = 0;
    }
*/
    count=0;
    memset(spheredist, 0, sizeof(spheredist));
    memset(spheredata, 0, sizeof(spheredata));
    if (!sphereideal_initialized) {
        sphereideal[0].x = 0.0f;
        sphereideal[0].y = 0.0f;
        sphereideal[0].z = 1.0f;
        for (i=1; i <= 15; i++) {
            longitude = ((float)(i - 1) + 0.5f) * (M_PI * 2.0 / 15.0);
            sphereideal[i].x = cosf(longitude) * cosf(1.05911f) * -1.0f;
            sphereideal[i].y = sinf(longitude) * cosf(1.05911f) * -1.0f;
            sphereideal[i].z = sinf(1.05911f);
        }
        for (i=16; i <= 49; i++) {
            longitude = ((float)(i - 16) + 0.5f) * (M_PI * 2.0 / 34.0);
            sphereideal[i].x = cosf(longitude) * cos(0.37388f) * -1.0f;
            sphereideal[i].y = sinf(longitude) * cos(0.37388f) * -1.0f;
            sphereideal[i].z = sinf(0.37388f);
        }
        for (i=50; i <= 83; i++) {
            longitude = ((float)(i - 50) + 0.5f) * (M_PI * 2.0 / 34.0);
            sphereideal[i].x = cosf(longitude) * cos(0.37388f) * -1.0f;
            sphereideal[i].y = sinf(longitude) * cos(0.37388f) * -1.0f;
            sphereideal[i].z = sinf(-0.37388f);
        }
        for (i=84; i <= 98; i++) {
            longitude = ((float)(i - 1) + 0.5f) * (M_PI * 2.0 / 15.0);
            sphereideal[i].x = cosf(longitude) * cosf(1.05911f) * -1.0f;
            sphereideal[i].y = sinf(longitude) * cosf(1.05911f) * -1.0f;
            sphereideal[i].z = sinf(-1.05911f);
        }
        sphereideal[99].x = 0.0f;
        sphereideal[99].y = 0.0f;
        sphereideal[99].z = -1.0f;
        sphereideal_initialized = 1;
    }
    quality_gaps_computed = 0;
    quality_variance_computed = 0;
    quality_wobble_computed = 0;
}

void quality_update(const Point_t *point)
{
    float x, y, z;
    int region;

    x = point->x;
    y = point->y;
    z = point->z;
    magnitude[count] = sqrtf(x * x + y * y + z * z);
    region = sphere_region(x, y, z);
    spheredist[region]++;
    spheredata[region].x += x;
    spheredata[region].y += y;
    spheredata[region].z += z;
    count++;
    quality_gaps_computed = 0;
    quality_variance_computed = 0;
    quality_wobble_computed = 0;
}

// How many surface gaps
float quality_surface_gap_error(void)
{
    float error=0.0f;
    int i, num;

    if (quality_gaps_computed) return quality_gaps_buffer;
    for (i=0; i < 100; i++) {
        num = spheredist[i];
        if (num == 0) {
            error += 1.0f;
        } else if (num == 1) {
            error += 0.2f;
        } else if (num == 2) {
            error += 0.01f;
        }
    }
    quality_gaps_buffer = error;
    quality_gaps_computed = 1;
    return quality_gaps_buffer;
}

// Variance in magnitude
float quality_magnitude_variance_error(void)
{
    float sum, mean, diff, variance;
    int i;

    if (quality_variance_computed) return quality_variance_buffer;
    sum = 0.0f;
    for (i=0; i < count; i++) {
        sum += magnitude[i];
    }
    mean = sum / (float)count;
    variance = 0.0f;
    for (i=0; i < count; i++) {
        diff = magnitude[i] - mean;
        variance += diff * diff;
    }
    variance /= (float)count;
    quality_variance_buffer = sqrtf(variance) / mean * 100.0f;
    quality_variance_computed = 1;
    return quality_variance_buffer;
}

// Offset of piecewise average data from ideal sphere surface
float quality_wobble_error(void)
{
    float sum, radius, x, y, z, xi, yi, zi;
    float xoff=0.0f, yoff=0.0f, zoff=0.0f;
    int i, n=0;

    if (quality_wobble_computed) return quality_wobble_buffer;
    sum = 0.0f;
    for (i=0; i < count; i++) {
        sum += magnitude[i];
    }
    radius = sum / (float)count;
    //if (pr) printf("  radius = %.2f\n", radius);
    for (i=0; i < 100; i++) {
        if (spheredist[i] > 0) {
            //if (pr) printf("  i=%3d", i);
            x = spheredata[i].x / (float)spheredist[i];
            y = spheredata[i].y / (float)spheredist[i];
            z = spheredata[i].z / (float)spheredist[i];
            //if (pr) printf("  at: %5.1f %5.1f %5.1f :", x, y, z);
            xi = sphereideal[i].x * radius;
            yi = sphereideal[i].y * radius;
            zi = sphereideal[i].z * radius;
            //if (pr) printf("   ideal: %5.1f %5.1f %5.1f :", xi, yi, zi);
            xoff += x - xi;
            yoff += y - yi;
            zoff += z - zi;
            //if (pr) printf("\n");
            n++;
        }
    }
    if (n == 0) return 100.0f;
    //if (pr) printf("  off = %.2f, %.2f, %.2f\n", xoff, yoff, zoff);
    xoff /= (float)n;
    yoff /= (float)n;
    zoff /= (float)n;
    //if (pr) printf("  off = %.2f, %.2f, %.2f\n", xoff, yoff, zoff);
    quality_wobble_buffer = sqrtf(xoff * xoff + yoff * yoff + zoff * zoff) / radius * 100.0f;
    quality_wobble_computed = 1;
    return quality_wobble_buffer;
}

// Freescale's algorithm fit error
float quality_spherical_fit_error(void)
{
    return magcal.FitError;
}

