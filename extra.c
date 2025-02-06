#include "func.h"

void Print(const char* format , ...) {
    char buf[LEN];
    char* ch = buf;
    va_list args;

    // load paramaters into the va_list object
    va_start(args, format);
    vsnprintf(buf, LEN, format, args);
    va_end(args);

    // display characters one by one 
    while (*ch) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *ch++);
    }
}

void reshape(int width, int height) {
    // set the viewport
    glViewport(0,0, RES*width,RES*height);
    // tell opengl we want to mess with the projection matrix
    glMatrixMode(GL_PROJECTION);
    // undo any previous transformations
    glLoadIdentity();

    // Orthogonal projection box adjusted for the aspect ration of the window
    double aspect = (height>0) ? (double)width/height : 1;
    glOrtho(-aspect * dim,+aspect * dim, -dim,+dim, -dim,+dim);
    // switch from telling gl we want to change the projection matrix to the model matrix
    glMatrixMode(GL_MODELVIEW);
    // load identitiy matrix
    glLoadIdentity();
}

void Fatal(const char* format , ...) {
    va_list args;
    va_start(args,format);
    vfprintf(stderr,format,args);
    va_end(args);
    printf("Exit at fatal \n");
    exit(1); 
}

void ErrCheck(const char* where) {
    int err = glGetError();
    if (err) {
        fprintf(stderr,"ERROR: %s [%s]\n",gluErrorString(err),where);
    }
}

void idle () {
    // t is the time since last called
    double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    // zh is the angle that the ball is at
    zh = fmod(90*t, 360.0);

    glutPostRedisplay();
}

char* ReadText(char *file)
{
   char* buffer;
   //  Open file
   FILE* f = fopen(file,"rt");
   if (!f) Fatal("Cannot open text file %s\n",file);
   //  Seek to end to determine size, then rewind
   fseek(f,0,SEEK_END);
   int n = ftell(f);
   rewind(f);
   //  Allocate memory for the whole file
   buffer = (char*)malloc(n+1);
   if (!buffer) Fatal("Cannot allocate %d bytes for text file %s\n",n+1,file);
   //  Snarf the file
   if (fread(buffer,n,1,f)!=1) Fatal("Cannot read %d bytes for text file %s\n",n,file);
   buffer[n] = 0;
   //  Close and return
   fclose(f);
   return buffer;
}

void PrintShaderLog(int obj,char* file)
{
   int len=0;
   glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&len);
   if (len>1)
   {
      int n=0;
      char* buffer = (char *)malloc(len);
      if (!buffer) Fatal("Cannot allocate %d bytes of text for shader log\n",len);
      glGetShaderInfoLog(obj,len,&n,buffer);
      fprintf(stderr,"%s:\n%s\n",file,buffer);
      free(buffer);
   }
   glGetShaderiv(obj,GL_COMPILE_STATUS,&len);
   if (!len) Fatal("Error compiling %s\n",file);
}

void PrintProgramLog(int obj)
{
   int len=0;
   glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&len);
   if (len>1)
   {
      int n=0;
      char* buffer = (char *)malloc(len);
      if (!buffer) Fatal("Cannot allocate %d bytes of text for program log\n",len);
      glGetProgramInfoLog(obj,len,&n,buffer);
      fprintf(stderr,"%s\n",buffer);
   }
   glGetProgramiv(obj,GL_LINK_STATUS,&len);
   if (!len) Fatal("Error linking program\n");
}

int CreateShader(GLenum type,char* file)
{
   //  Create the shader
   int shader = glCreateShader(type);
   //  Load source code from file
   char* source = ReadText(file);
   glShaderSource(shader,1,(const char**)&source,NULL);
   free(source);
   //  Compile the shader
   fprintf(stderr,"Compile %s\n",file);
   glCompileShader(shader);
   //  Check for errors
   PrintShaderLog(shader,file);
   //  Return name
   return shader;
}

int CreateShaderProg(char* VertFile,char* FragFile)
{
   //  Create program
   int prog = glCreateProgram();
   //  Create and compile vertex shader
   int vert = CreateShader(GL_VERTEX_SHADER,VertFile);
   //  Create and compile fragment shader
   int frag = CreateShader(GL_FRAGMENT_SHADER,FragFile);
   //  Attach vertex shader
   glAttachShader(prog,vert);
   //  Attach fragment shader
   glAttachShader(prog,frag);
   //  Link program
   glLinkProgram(prog);
   //  Check for errors
   PrintProgramLog(prog);
   //  Return name
   return prog;
}