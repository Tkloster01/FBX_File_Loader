#include "func.h"

double dim = 15;
double oldDim = 2;
int th = 15; // theta
int ele = 7; // elevation
int mode = 0; // switches objects

int axisSize = 5;

// light values
int distance = 10; // distance to light

// texture values
unsigned int texture[24];
int shader[4];

// This structure is used to store all the vertex data from each 
// of the objects in a file so they can be displayed later
struct vertexDataDouble{
   double* vertecies;
   int vertLength;
   int* vertexIndex;
   int indexLength;
   int* edges;
   int edgesLength;
   double* UV;
   int uvLength;
   int* UVIndex;
   int uvIndexLen;
   double* normals;
   int normalsLength;
};

// holds translations for each object in the file
struct translations{
   double* translation;
   double* rotations;
   int count;
   int translationSize;
};

// helper returned from reading an entire file, the data and translations
// line up where if there are no translations the values will be (0,0,0)
struct dataDoubleList{
    struct vertexDataDouble** data;
    struct translations* trans;
    int objectNum;
    int dataSize;
};

int zh = 0;
float yLight = 0;

const int width = 750;
const int height = 750;
struct dataDoubleList* object[5]; // stores the values of read files
int objectSize = 2;

float Emission[]  = {0.0,0.0,0.0,1.0};
float Ambient[]   = {0.3,0.3,0.3,1.0};
float Diffuse[]   = {1.0,1.0,1.0,1.0};
float Specular[]  = {1.0,1.0,1.0,1.0};
float Shinyness[] = {16};

// This takes a vertexDataDouble and draws the object 
//      passes 4 texture maps to the shader, base color,
//      Diffuse, Glossiness, and Specular
void drawGear(struct vertexDataDouble* object) {
    int i = 0;
    int normalCount = 0;
    while (i < object -> indexLength) {
        int* storedInd = malloc(sizeof(int) * 32);
        int* storedUV = malloc(sizeof(int) * 32);
        int storedCount = 0;
        

        int vertexIndex = 0;
        int uvIndex = 0;
        while (vertexIndex >= 0) {
            vertexIndex = object -> vertexIndex[i];
            uvIndex = object -> UVIndex[i] * 2;
            storedUV[storedCount] = uvIndex;

            if (storedCount == 31) {
                printf("make stored Indicies buffer bigger \n");
            }
            if (vertexIndex < 0) {
                storedInd[storedCount] = ~vertexIndex * 3;
            } else {
                storedInd[storedCount] = vertexIndex * 3;
            }
            storedCount++;
            i++;

        } 

        float color[] = {1,1,1,1.0};
        glColor3f(1,1,1);
        glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,color);

        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, texture[11]);

        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, texture[12]);

        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, texture[17]);

        glActiveTexture(GL_TEXTURE0 + 3);
        glBindTexture(GL_TEXTURE_2D, texture[13]);

        glEnable(GL_TEXTURE_2D);
        glBegin(GL_POLYGON);
        for (int j = 0; j < storedCount; j++) {
            glTexCoord2f(object -> UV[storedUV[j]], object -> UV[storedUV[j] + 1]);
            glNormal3f(object -> normals[(normalCount * 3)], object -> normals[(normalCount * 3) + 1], object -> normals[(normalCount * 3)+ 2]);
            glVertex3d(object -> vertecies[storedInd[j]], object -> vertecies[storedInd[j] + 1], object -> vertecies[storedInd[j] + 2]);
            normalCount++;
        }
        glEnd();

    }
}

// does the same but only passes a single texture
void drawSingleTex(struct vertexDataDouble* object) {
    int i = 0;
    int normalCount = 0;
    while (i < object -> indexLength) {
        int* storedInd = malloc(sizeof(int) * 32);
        int* storedUV = malloc(sizeof(int) * 32);
        int storedCount = 0;
        

        int vertexIndex = 0;
        int uvIndex = 0;
        while (vertexIndex >= 0) {
            vertexIndex = object -> vertexIndex[i];
            uvIndex = object -> UVIndex[i] * 2;
            storedUV[storedCount] = uvIndex;

            if (storedCount - 1 >= 32) {
                printf("make stored Indicies buffer bigger \n");
            }
            if (vertexIndex < 0) {
                storedInd[storedCount] = ~vertexIndex * 3;
            } else {
                storedInd[storedCount] = vertexIndex * 3;
            }
            storedCount++;
            i++;

        } 
        
        
        float color[] = {1,1,1,1.0};
        glColor3f(1,1,1);
        glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,color);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_POLYGON);
        for (int j = 0; j < storedCount; j++) {
            glTexCoord2f(object -> UV[storedUV[j]], object -> UV[storedUV[j] + 1]);
            glNormal3f(object -> normals[(normalCount * 3)], object -> normals[(normalCount * 3) + 1], object -> normals[(normalCount * 3)+ 2]);
            glVertex3d(object -> vertecies[storedInd[j]], object -> vertecies[storedInd[j] + 1], object -> vertecies[storedInd[j] + 2]);
            normalCount++;
        }
        
        glEnd();

    }
    
}
// one of the files had normals stored in a different byte order, byVertexIndex vs byIndices
void drawByVertice(struct vertexDataDouble* object) {
    int i = 0;
    int normalCount = 0;
    while (i < object -> indexLength) {
        int* storedInd = malloc(sizeof(int) * 32);
        int* storedUV = malloc(sizeof(int) * 32);
        int storedCount = 0;
        

        int vertexIndex = 0;
        int uvIndex = 0;
        while (vertexIndex >= 0) {
            vertexIndex = object -> vertexIndex[i];
            uvIndex = object -> UVIndex[i] * 2;
            storedUV[storedCount] = uvIndex;

            if (storedCount - 1 >= 32) {
                printf("make stored Indicies buffer bigger \n");
            }
            if (vertexIndex < 0) {
                storedInd[storedCount] = ~vertexIndex * 3;
            } else {
                storedInd[storedCount] = vertexIndex * 3;
            }
            storedCount++;
            i++;

        } 
        
        
        float color[] = {1,1,1,1.0};
        glColor3f(1,1,1);
        glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,color);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_POLYGON);
        for (int j = 0; j < storedCount; j++) {
            glTexCoord2f(object -> UV[storedUV[j]], object -> UV[storedUV[j] + 1]);
            glNormal3f(object -> normals[storedInd[j]], object -> normals[storedInd[j] + 1], object -> normals[storedInd[j]+ 2]);
            glVertex3d(object -> vertecies[storedInd[j]], object -> vertecies[storedInd[j] + 1], object -> vertecies[storedInd[j] + 2]);
            normalCount++;
        }
        
        glEnd();

    }
    
}

void changeDim() {
    if (dim != oldDim) {
        reshape(width, height);
        oldDim = dim;
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glLoadIdentity();

    // set view angle with elevation and theta
    glRotated(ele, 1, 0, 0);
    glRotated(th, 0, 1, 0);

    if (mode == 2) {
        glTranslated(0, -10, 0);
    }

    glShadeModel(GL_SMOOTH);

    // draw ball at rotating position - position is used for the ball and the light

    float position[] = {distance*Cos(zh), yLight, distance*Sin(zh), 1.0};
    sphere(position[0], position[1], position[2], .5, 0, 0);

    // nothing happens in open gl without you saying so - vilkas
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT0, GL_AMBIENT, Ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, Diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, Specular);
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,Shinyness);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,Specular);
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);

    int tex0 = glGetUniformLocation(shader[0], "tex0");
    int tex1 = glGetUniformLocation(shader[0], "tex1");
    int tex2 = glGetUniformLocation(shader[0], "tex2");
    int tex3 = glGetUniformLocation(shader[0], "tex3");
    
    
    if (mode == 0) {
        dim = 10;
        changeDim();
        glPushMatrix();

        glUseProgram(shader[0]);

        glUniform1i(tex0, 0);
        glUniform1i(tex1, 1);
        glUniform1i(tex2, 2);
        glUniform1i(tex3, 3);

        glRotated(-90, 1,0,0);
        glScaled(.75,.75,.75);
        drawGear(object[0] -> data[0]);
        
        glPopMatrix();
    } else if (mode == 1) {
        dim = 20;
        changeDim();

        glPushMatrix();

        glUseProgram(shader[1]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[23]);
        glRotated(90, 1,0,0);
        glScaled(.65, .65, .65);

        for (int i = 0; i < object[1] -> objectNum; i++) {
            glPushMatrix();
            glTranslated(object[1] -> trans -> translation[i * 3], object[1] -> trans -> translation[i * 3 + 1], object[1] -> trans -> translation[i * 3 + 2]);
            glRotated(object[1] -> trans -> rotations[i * 3], 1,0,0);
            glRotated(object[1] -> trans -> rotations[i * 3 + 1], 0,1,0);
            glRotated(object[1] -> trans -> rotations[i * 3 + 2], 0,0,1);
            drawSingleTex(object[1] -> data[i]);
            glPopMatrix();
        }
        

        glDisable(GL_TEXTURE_2D);
        glPopMatrix();
    } else if (mode == 2) {
        dim = 20;
        changeDim();

        glPushMatrix();

        glUseProgram(shader[1]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[0]);

        glScaled(.075,.075,.075);
        
        for (int i = 0; i < object[2] -> objectNum; i++) {
            drawByVertice(object[2] -> data[i]);
        }

        glDisable(GL_TEXTURE_2D);
        glPopMatrix();
    } 

    glDisable(GL_LIGHTING);

    glUseProgram(0);

    glColor3f(1, 1, 1);
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    glVertex3d(axisSize, 0, 0);
    glVertex3d(0, 0, 0);
    glVertex3d(0, axisSize, 0);
    glVertex3d(0, 0, 0);
    glVertex3d(0, 0, axisSize);
    glEnd();

    ErrCheck("display");
    glFlush();
    glutSwapBuffers();
}

int main(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(width, height);
    glutCreateWindow("Tyler Kloster, Lighting");
    #ifdef USEGLEW
        if (glewInit() != GLEW_OK) Fatal("Error initializing GLEW \n");
    #endif 
        glutDisplayFunc(display);
        glutReshapeFunc(reshape);
        glutSpecialFunc(special);
        glutKeyboardFunc(key);
        glutIdleFunc(idle);

        texture[0] = LoadTexBMP("./aliengreenbase.bmp");

        texture[11] = LoadTexBMP("./gear_2_basecolor.bmp");
        texture[12] = LoadTexBMP("./gear_2_diffuse.bmp");
        texture[13] = LoadTexBMP("./gear_2_glossiness.bmp");
        texture[17] = LoadTexBMP("./gear_2_specular.bmp");

        texture[23] = LoadTexBMP("./sting_base_color.bmp");

        shader[0] = CreateShaderProg("gear.vert","gear.frag");
        shader[1] = CreateShaderProg("lighting.vert","lighting.frag");

        object[0] = loadFBX("./gear2binaryblender.fbx");
        object[1] = loadFBX("./stingsword.fbx");
        object[2] = loadFBX("./reptilealien.fbx");

        glutMainLoop();
        return 0;
}