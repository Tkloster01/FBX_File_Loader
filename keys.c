#include "func.h"

void key(unsigned char ch, int x, int y) {
    if (ch == 27) {
        exit(0);
    } else if (ch == '0') {
        th = ele = 0;
    } 
    else if (ch == '+' && mode < 2) {
        mode++;
    } else if (ch == '-' && mode > 0) {
        mode--;
    } 
    else if (ch == '}') {
        yLight += 1;
    } else if (ch == '{') {
        yLight -= 1;
    } 
    else if (ch == '[') {
        distance++;
    } else if (ch == ']') {
        distance--;
    } 

    glutIdleFunc(idle);
    glutPostRedisplay();
}

void special(int key, int x, int y) {

    if (key == GLUT_KEY_RIGHT) {
        th += 5;
    } else if (key == GLUT_KEY_LEFT) {
        th -= 5;
    } else if (key == GLUT_KEY_UP) {
        ele -= 5;
    } else if (key == GLUT_KEY_DOWN) {
        ele += 5;
    }
    
    // if over 360 degrees reset
    th %= 360;
    ele %= 360;
    // tell glut they have to redraw the scene
    glutPostRedisplay();
}