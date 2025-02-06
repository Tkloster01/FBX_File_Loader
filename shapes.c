#include "func.h"

void Vertex(double th, double ph, int color) {
    // color based on position or solid
    if (color) {
        glColor3f(Cos(th)*Cos(th) , Sin(ph)*Sin(ph) , Sin(th)*Sin(th));
    }
    else {
        glColor3f(1,1,1);
    }

    double x = Sin(th)*Cos(ph);
    double y = Cos(th)*Cos(ph);
    double z = Sin(ph);

    glNormal3d(x,y,z);
    glVertex3d(x,y,z);
}

void sphere(double x, double y, double z, double r, double color, int tex) {
    const int d = 5;

    glPushMatrix();
    glTranslated(x,y,z);
    glScaled(r,r,r);

    glColor3f(1,1,1);

    for (int ph = -90; ph < 90; ph += d) {
        glBegin(GL_QUAD_STRIP);
        for (int th = 0; th <= 360; th += d) {
            // Vertex converts polar coords to euclidean
            Vertex(th, ph, color);
            Vertex(th, ph + d, color);
        }
        glEnd();
    }
    glPopMatrix();
}