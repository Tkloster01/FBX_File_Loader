#ifndef func
#define func

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#ifdef USEGLEW
#include <GL/glew.h>
#endif
//  OpenGL with prototypes for glext
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <GLUT/glut.h>
// Tell Xcode IDE to not gripe about OpenGL deprecation
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#else
#include <GL/glut.h>
#endif
//  Default resolution
//  For Retina displays compile with -DRES=2
#ifndef RES
#define RES 1
#endif

extern double dim;
extern double oldDim;
extern int th;
extern int ele;
extern int mode;

extern int distance;
extern int zh;
extern float yLight;
extern double t;

extern unsigned int texture[24];

typedef struct {
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
} vertexDataDouble;

typedef struct {
   double* translation;
   double* rotations;
   int count;
   int translationSize;
} translations;

typedef struct {
    struct vertexDataDouble** data;
    struct translations* trans;
    int objectNum;
    int dataSize;
} dataDoubleList;

typedef struct {
    unsigned long startOffset;
    unsigned long EndOffset;
    unsigned long NumProperties;
    unsigned long PropertyListLen;
    unsigned char propType;
    unsigned char NameLen;
    int nameSet;
    int propSet;
    int nullNode;
    char* Name;
    char* Property;
    int totalBytes;
    int depth;
} node;

typedef struct {
   struct node** nodeListInter;
   int nodeListCount;
   int nodeListSize;
} nodeList;

typedef struct {
    struct node* FBXHeaderExtension;
    nodeList _FBXHeaderExtension;
    struct node* FileID;
    nodeList _FileID;
    struct node* CreationTime;
    nodeList _CreationTime;
    struct node* Creator;
    nodeList _Creator;
    struct node* GlobalSettings;
    nodeList _GlobalSettings;
    struct node* Documents;
    nodeList _Documents;
    struct node* References;
    nodeList _References;
    struct node* Definitions;
    nodeList _Definitions;
    struct node* Objects;
    nodeList _Objects;
    struct node* Connections;
    nodeList _Connections;
    struct node* Takes;
    nodeList _Takes;
} fbxFile;

extern struct dataDoubleList* object[5];


extern const int width;
extern const int height;

#define Cos(x) (cos((x) * 3.14159265 / 180))
#define Sin(x) (sin((x) * 3.14159265 / 180))

#ifdef __cplusplus
extern "C" {
#endif

#define LEN 8192

void Print(const char* format , ...);
void reshape(int width, int height);
void Fatal(const char* format , ...);
void ErrCheck(const char* where);
void idle();
void changeDim();
unsigned int LoadTexBMP(const char* file);
struct dataDoubleList* loadFBX(const char* file);

void projectFunc(double fov, double asp, double dim);

void special(int key, int x, int y);
void key(unsigned char ch, int x, int y);

int CreateShaderProg(char* VertFile,char* FragFile);

void Vertex(double th, double ph, int color);
void sphere(double x, double y, double z, double r, double color, int tex);

#ifdef __cplusplus
}
#endif

#endif