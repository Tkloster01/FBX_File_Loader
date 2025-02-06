#include "func.h"
#include <ctype.h>
#include <string.h>
#include <stdint.h>

// implemented from https://www.zlib.net/zlib_how.html
// this is required for decoding compressed binary data
#include <assert.h>
#include "zlib.h"
#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384

// Helper structure to return both the char* pointer and size
struct infHelp {
   char* list;
   int length;
   int error;
};

// takes in a character pointer of encrypted data and returns 
// an infHelp variable with the new decrypted data
struct infHelp inf(char* sourceChar, int iSize, int oSize) {

   // the documentation for this inf function was reading out of files
   // but since i had already put the data into my own data structures
   // i rewrite them to files to use fread
   FILE* fptrSource;
   FILE* fptrDest;
   fptrSource = fopen("inflateSource.txt", "w+");
   fptrDest = fopen("inflateDest.txt", "w+");
   for (int i = 0; i < iSize; i++) {
      fputc(sourceChar[i], fptrSource);
   }
   fseek(fptrSource, 0, SEEK_SET);
   

   // directly implemented from documentation above
   int ret;
   unsigned have;
   z_stream strm;
   unsigned char in[iSize];
   unsigned char out[oSize];
   

   strm.zalloc = Z_NULL;
   strm.zfree = Z_NULL;
   strm.opaque = Z_NULL;
   strm.avail_in = 0;
   strm.next_in = 0;
   ret = inflateInit(&strm);
   struct infHelp retu;
   if (ret != Z_OK) {
      retu.error = ret;
      return retu;
   }
   
   
   do {
      strm.avail_in = fread(in, 1, iSize, fptrSource);
      if (ferror(fptrSource)) {
         (void)inflateEnd(&strm);
         retu.error = Z_ERRNO;
         return retu;
      }
      if (strm.avail_in == 0) {
         
         break;
      }
      strm.next_in = in;
      
      
      do {
         strm.avail_out = oSize;
         strm.next_out = out;
         ret = inflate(&strm, Z_NO_FLUSH);
         assert(ret != Z_STREAM_ERROR);
         switch(ret) {
            case Z_NEED_DICT:
               ret = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
               (void)inflateEnd(&strm);
               retu.error = ret;
               return retu;
         }

         have = oSize - strm.avail_out;
         if (fwrite(out, 1, have, fptrDest) != have || ferror(fptrDest)) {
            (void)inflateEnd(&strm);
            retu.error = Z_ERRNO;
            return retu;
         }
         
      } while (strm.avail_out == 0);
   } while (ret != Z_STREAM_END);
   


   
   // read from the new output file into infHelp data type to be returned
   char* finalList = malloc(strm.total_out);
   fseek(fptrDest, 0, SEEK_SET);

   for (int i = 0; i < strm.total_out; i++) {
      finalList[i] = (unsigned char)fgetc(fptrDest);
   }

   (void)inflateEnd(&strm);

   remove("inflateDest.txt");
   remove("inflateSource.txt");

   struct infHelp final;
   final.list = finalList;
   final.length = strm.total_out;
   return final;
}

static void Reverse(void* x,const int n)
{
   char* ch = (char*)x;
   for (int k=0;k<n/2;k++)
   {
      char tmp = ch[k];
      ch[k] = ch[n-1-k];
      ch[n-1-k] = tmp;
   }
}

unsigned int LoadTexBMP(const char* file) {
   //  Open file
   FILE* f = fopen(file,"rb");
   if (!f) Fatal("Cannot open file %s\n",file);
   //  Check image magic
   unsigned short magic;
   if (fread(&magic,2,1,f)!=1) Fatal("Cannot read magic from %s\n",file);
   if (magic!=0x4D42 && magic!=0x424D) Fatal("Image magic not BMP in %s\n",file);
   //  Read header
   unsigned int dx,dy,off,k; // Image dimensions, offset and compression
   unsigned short nbp,bpp;   // Planes and bits per pixel
   if (fseek(f,8,SEEK_CUR) || fread(&off,4,1,f)!=1 ||
       fseek(f,4,SEEK_CUR) || fread(&dx,4,1,f)!=1 || fread(&dy,4,1,f)!=1 ||
       fread(&nbp,2,1,f)!=1 || fread(&bpp,2,1,f)!=1 || fread(&k,4,1,f)!=1)
     Fatal("Cannot read header from %s\n",file);
   //  Reverse bytes on big endian hardware (detected by backwards magic)
   if (magic==0x424D)
   {
      Reverse(&off,4);
      Reverse(&dx,4);
      Reverse(&dy,4);
      Reverse(&nbp,2);
      Reverse(&bpp,2);
      Reverse(&k,4);
   }
   //  Check image parameters
   unsigned int max;
   glGetIntegerv(GL_MAX_TEXTURE_SIZE,(int*)&max);
   if (dx<1 || dx>max) Fatal("%s image width %d out of range 1-%d\n",file,dx,max);
   if (dy<1 || dy>max) Fatal("%s image height %d out of range 1-%d\n",file,dy,max);
   if (nbp!=1)  Fatal("%s bit planes is not 1: %d\n",file,nbp);
   if (bpp!=24) Fatal("%s bits per pixel is not 24: %d\n",file,bpp);
   if (k!=0)    Fatal("%s compressed files not supported\n",file);
#ifndef GL_VERSION_2_0
   //  OpenGL 2.0 lifts the restriction that texture size must be a power of two
   for (k=1;k<dx;k*=2);
   if (k!=dx) Fatal("%s image width not a power of two: %d\n",file,dx);
   for (k=1;k<dy;k*=2);
   if (k!=dy) Fatal("%s image height not a power of two: %d\n",file,dy);
#endif

   //  Allocate image memory
   unsigned int size = 3*dx*dy;
   unsigned char* image = (unsigned char*) malloc(size);
   if (!image) Fatal("Cannot allocate %d bytes of memory for image %s\n",size,file);
   //  Seek to and read image
   if (fseek(f,off,SEEK_SET) || fread(image,size,1,f)!=1) Fatal("Error reading data from image %s\n",file);
   fclose(f);
   //  Reverse colors (BGR -> RGB)
   for (k=0;k<size;k+=3)
   {
      unsigned char temp = image[k];
      image[k]   = image[k+2];
      image[k+2] = temp;
   }

   //  Sanity check
   ErrCheck("LoadTexBMP");
   //  Generate 2D texture
   unsigned int texture;
   glGenTextures(1,&texture);
   glBindTexture(GL_TEXTURE_2D,texture);
   //  Copy image
   glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,dx,dy,0,GL_RGB,GL_UNSIGNED_BYTE,image);
   if (glGetError()) Fatal("Error in glTexImage2D %s %dx%d\n",file,dx,dy);
   //  Scale linearly when image size doesn't match
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

   //  Free image memory
   free(image);
   //  Return texture name
   return texture;
}


// load object starts here

typedef struct
{
   char* name;                 //  Material name
   float Ka[4],Kd[4],Ks[4],Ns; //  Colors and shininess
   float d;                    //  Transparency
   int map;                    //  Texture
} mtl_t;

//  Material count and array
static int Nmtl=0;
static mtl_t* mtl=NULL;

//
//  Return true if CR or LF
//
static int CRLF(char ch)
{
   return ch == '\r' || ch == '\n';
}

//
//  Read line from file
//    Returns pointer to line or NULL on EOF
//
static int linelen=0;    //  Length of line
static char* line=NULL;  //  Internal storage for line
static char* readlineOBJ(FILE* f)
{
   char ch;  //  Character read
   int k=0;  //  Character count
   while ((ch = fgetc(f)) != EOF)
   {
      //  Allocate more memory for long strings
      if (k>=linelen)
      {
         linelen += 8192;
         line = (char*)realloc(line,linelen);
         if (!line) Fatal("Out of memory in readline\n");
      }
      //  End of Line
      if (CRLF(ch))
      {
         // Eat extra CR or LF characters (if any)
         while ((ch = fgetc(f)) != EOF)
           if (!CRLF(ch)) break;
         //  Stick back the overrun
         if (ch != EOF) ungetc(ch,f);
         //  Bail
         break;
      }
      //  Pad character to line
      else
         line[k++] = ch;
   }
   //  Terminate line if anything was read
   if (k>0) line[k] = 0;
   //  Return pointer to line or NULL on EOF
   return k>0 ? line : NULL;
}

//
//  Read to next non-whitespace word
//  Note that this destroys line in the process
//
static char* getword(char** line)
{
   //  Skip leading whitespace
   while (**line && isspace(**line))
      (*line)++;
   if (!**line) return NULL;
   //  Start of word
   char* word = *line;
   //  Read until next whitespace
   while (**line && !isspace(**line))
      (*line)++;
   //  Mark end of word if not NULL
   if (**line)
   {
      **line = 0;
      (*line)++;
   }
   return word;
}

//
//  Read n floats
//
static void readfloat(char* line,int n,float x[])
{
   for (int i=0;i<n;i++)
   {
      char* str = getword(&line);
      if (!str)  Fatal("Premature EOL reading %d floats\n",n);
      if (sscanf(str,"%f",x+i)!=1) Fatal("Error reading float %d\n",i);
   }
}

//
//  Read coordinates
//    n is how many coordiantes to read
//    N is the coordinate index
//    M is the number of coordinates
//    x is the array
//    This function adds more memory as needed in 8192 work chunks
//
static void readcoord(char* line,int n,float* x[],int* N,int* M)
{
   //  Allocate memory if necessary
   if (*N+n > *M)
   {
      *M += 8192;
      *x = (float*)realloc(*x,(*M)*sizeof(float));
      if (!*x) Fatal("Cannot allocate memory\n");
   }
   //  Read n coordinates
   readfloat(line,n,(*x)+*N);
   (*N)+=n;
}

//
//  Read string conditionally
//     Line must start with skip string
//     After skip sting return first word
//     getword terminates the line
//
static char* readstr(char* line,const char* skip)
{
   //  Check for a match on the skip string
   while (*skip && *line && *skip==*line)
   {
      skip++;
      line++;
   }
   //  Skip must be NULL for a match
   if (*skip || !isspace(*line)) return NULL;
   //  Read string
   return getword(&line);
}

//
//  Load materials from file
//
static void LoadMaterial(const char* file)
{
   int k=-1;
   char* line;
   char* str;

   //  Open file or return with warning on error
   FILE* f = fopen(file,"r");
   if (!f)
   {
      fprintf(stderr,"Cannot open material file %s\n",file);
      return;
   }

   //  Read lines
   while ((line = readlineOBJ(f)))
   {
      //  New material
      if ((str = readstr(line,"newmtl")))
      {
         int l = strlen(str);
         //  Allocate memory for structure
         k = Nmtl++;
         mtl = (mtl_t*)realloc(mtl,Nmtl*sizeof(mtl_t));
         //  Store name
         mtl[k].name = (char*)malloc(l+1);
         if (!mtl[k].name) Fatal("Cannot allocate %d for name\n",l+1);
         strcpy(mtl[k].name,str);
         //  Initialize materials
         mtl[k].Ka[0] = mtl[k].Ka[1] = mtl[k].Ka[2] = 0;   mtl[k].Ka[3] = 1;
         mtl[k].Kd[0] = mtl[k].Kd[1] = mtl[k].Kd[2] = 0;   mtl[k].Kd[3] = 1;
         mtl[k].Ks[0] = mtl[k].Ks[1] = mtl[k].Ks[2] = 0;   mtl[k].Ks[3] = 1;
         mtl[k].Ns  = 0;
         mtl[k].d   = 0;
         mtl[k].map = 0;
      }
      //  If no material short circuit here
      else if (k<0)
      {}
      //  Ambient color
      else if (line[0]=='K' && line[1]=='a')
         readfloat(line+2,3,mtl[k].Ka);
      //  Diffuse color
      else if (line[0]=='K' && line[1] == 'd')
         readfloat(line+2,3,mtl[k].Kd);
      //  Specular color
      else if (line[0]=='K' && line[1] == 's')
         readfloat(line+2,3,mtl[k].Ks);
      //  Material Shininess
      else if (line[0]=='N' && line[1]=='s')
      {
         readfloat(line+2,1,&mtl[k].Ns);
         //  Limit to 128 for OpenGL
         if (mtl[k].Ns>128) mtl[k].Ns = 128;
      }
      //  Textures (must be BMP - will fail if not)
      else if ((str = readstr(line,"map_Kd")))
         mtl[k].map = LoadTexBMP(str);
      //  Ignore line if we get here
   }
   fclose(f);
}

//
//  Set material
//
static void SetMaterial(const char* name)
{
   //  Search materials for a matching name
   for (int k=0;k<Nmtl;k++)
      if (!strcmp(mtl[k].name,name))
      {
         //  Set material colors
         glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT  ,mtl[k].Ka);
         glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE  ,mtl[k].Kd);
         glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR ,mtl[k].Ks);
         glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,&mtl[k].Ns);
         //  Bind texture if specified
         if (mtl[k].map)
         {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D,mtl[k].map);
         }
         else
            glDisable(GL_TEXTURE_2D);
         return;
      }
   //  No matches
   fprintf(stderr,"Unknown material %s\n",name);
}

//
//  Load OBJ file
//
int LoadOBJ(const char* file)
{
   int  Nv,Nn,Nt;  //  Number of vertex, normal and textures
   int  Mv,Mn,Mt;  //  Maximum vertex, normal and textures
   float* V;       //  Array of vertexes
   float* N;       //  Array of normals
   float* T;       //  Array of textures coordinates
   char*  line;    //  Line pointer
   char*  str;     //  String pointer

   //  Open file
   FILE* f = fopen(file,"r");
   if (!f) Fatal("Cannot open file %s\n",file);

   // Reset materials
   mtl = NULL;
   Nmtl = 0;

   //  Start new displaylist
   int list = glGenLists(1);
   glNewList(list,GL_COMPILE);
   //  Push attributes for textures
   glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT);

   //  Read vertexes and facets
   V  = N  = T  = NULL;
   Nv = Nn = Nt = 0;
   Mv = Mn = Mt = 0;
   while ((line = readlineOBJ(f)))
   {
      //  Vertex coordinates (always 3)
      if (line[0]=='v' && line[1]==' ')
         readcoord(line+2,3,&V,&Nv,&Mv);
      //  Normal coordinates (always 3)
      else if (line[0]=='v' && line[1] == 'n')
         readcoord(line+2,3,&N,&Nn,&Mn);
      //  Texture coordinates (always 2)
      else if (line[0]=='v' && line[1] == 't')
         readcoord(line+2,2,&T,&Nt,&Mt);
      //  Read and draw facets
      else if (line[0]=='f')
      {
         line++;
         //  Read Vertex/Texture/Normal triplets
         glBegin(GL_POLYGON);
         while ((str = getword(&line)))
         {
            int Kv,Kt,Kn;
            //  Try Vertex/Texture/Normal triplet
            if (sscanf(str,"%d/%d/%d",&Kv,&Kt,&Kn)==3)
            {
               if (Kv<0 || Kv>Nv/3) Fatal("Vertex %d out of range 1-%d\n",Kv,Nv/3);
               if (Kn<0 || Kn>Nn/3) Fatal("Normal %d out of range 1-%d\n",Kn,Nn/3);
               if (Kt<0 || Kt>Nt/2) Fatal("Texture %d out of range 1-%d\n",Kt,Nt/2);
            }
            //  Try Vertex//Normal pairs
            else if (sscanf(str,"%d//%d",&Kv,&Kn)==2)
            {
               if (Kv<0 || Kv>Nv/3) Fatal("Vertex %d out of range 1-%d\n",Kv,Nv/3);
               if (Kn<0 || Kn>Nn/3) Fatal("Normal %d out of range 1-%d\n",Kn,Nn/3);
               Kt = 0;
            }
            //  Try Vertex index
            else if (sscanf(str,"%d",&Kv)==1)
            {
               if (Kv<0 || Kv>Nv/3) Fatal("Vertex %d out of range 1-%d\n",Kv,Nv/3);
               Kn = 0;
               Kt = 0;
            }
            //  This is an error
            else
               Fatal("Invalid facet %s\n",str);
            //  Draw vectors
            if (Kt) glTexCoord2fv(T+2*(Kt-1));
            if (Kn) glNormal3fv(N+3*(Kn-1));
            if (Kv) glVertex3fv(V+3*(Kv-1));
         }
         glEnd();
      }
      //  Use material
      else if ((str = readstr(line,"usemtl")))
         SetMaterial(str);
      //  Load materials
      else if ((str = readstr(line,"mtllib")))
         LoadMaterial(str);
      //  Skip this line
   }
   fclose(f);
   //  Pop attributes (textures)
   glPopAttrib();
   glEndList();

   //  Free materials
   for (int k=0;k<Nmtl;k++)
      free(mtl[k].name);
   free(mtl);

   //  Free arrays
   free(V);
   free(T);
   free(N);

   return list;
}

// load fbx starts here

// the fbx file format is a long list of nested nodes
// because fgetc will not terminate at the end of the file, it is important
// to have the endoffset set
struct node {
   unsigned long startOffset; // byte where the data starts
   unsigned long EndOffset; // amount of characters from beginning of file until the start of the next node
   unsigned long NumProperties; // number of properties
   unsigned long PropertyListLen; // length of property list in bytes
   unsigned char propType; // unused
   unsigned char NameLen; // length of the name string
   int nameSet; // check to make sure the name has been set
   int propSet; // check to make sure the property has been set
   int nullNode; // is this a null terminating node
   char* Name; // pointer to name buffer
   char* Property; // pointer to property buffer
   int totalBytes; // total bytes taken by node, was using to track memory leaks
   int depth; // depth in nested file structure
   int order; // order the nodes appear, used to sort back into file order
};

struct nodeList {
   struct node** nodeListInter;
   int nodeListCount;
   int nodeListSize;
};

// an fbx file has these standard 10 header nodes
// (I thought it would be easier to have a name to reference all of them
// but it would have been much simpler to have a double pointer and keep them
// all in one array)
struct fbxFile {
   struct node* FBXHeaderExtension;
   struct nodeList _FBXHeaderExtension;
   struct node* FileID;
   struct nodeList _FileID;
   struct node* CreationTime;
   struct nodeList _CreationTime;
   struct node* Creator;
   struct nodeList _Creator;
   struct node* GlobalSettings;
   struct nodeList _GlobalSettings;
   struct node* Documents;
   struct nodeList _Documents;
   struct node* References;
   struct nodeList _References;
   struct node* Definitions;
   struct nodeList _Definitions;
   struct node* Objects;
   struct nodeList _Objects;
   struct node* Connections;
   struct nodeList _Connections;
   struct node* Takes;
   struct nodeList _Takes;
};

// 0 is not an FBX file, 1 is an FBX file
// tests to see if the loaded file is actualy an fbx file
char test[23] = {0x4b, 0x61, 0x79, 0x64, 0x61, 0x72, 0x61, 0x20, 0x46, 0x42, 0x58, 0x20, 0x42, 0x69, 0x6e, 0x61, 0x72, 0x79, 0x20, 0x20, 0x0, 0x1a, 0x0};
char firstLine[27];
int testFirstLine() {
   for (int i = 0; i < 23; i += 1) {
      if (test[i] != firstLine[i]) {
         return 0;
      }
   }
   return 1;
}

// The first line will always be 26 bytes but it is not a node
// so it requires its own function
int lc = 0; // line count
void getFirstLine(FILE* f) {
   for (int i = 0; i < 27; i++) {
      firstLine[i] = fgetc(f);
   }
   lc++;
}

// toString used in debugging, prints the values of a node structure
void nodeToString(struct node* Node) {
   printf("Start Offset = ");
   printf("%x ", (int)Node -> startOffset);
   printf("\n");

   printf("End Offset = ");
   printf("%x ", (int)Node -> EndOffset);
   printf("\n");

   printf("Number of Properties = ");
   printf("%i", (int)Node -> NumProperties);
   printf("\n");

   printf("Property List Length = ");
   printf("%i", (int)Node -> PropertyListLen);
   printf("\n");

   printf("Name length = ");
   printf("%i", Node -> NameLen);
   printf("\n");

   printf("propSet = ");
   printf("%i", Node -> propSet);
   printf("\n");

   printf("depth = ");
   printf("%i", Node -> depth);
   printf("\n");

   printf("order = ");
   printf("%i", Node -> order);
   printf("\n");

   printf("Name = ");
   for (int i = 0; i < Node -> NameLen; i++) {
      printf("%c", Node -> Name[i]);
   }
   printf("\n");

   if (Node -> NumProperties > 0) {
      printf("Property Type = ");
      printf("%c", Node -> propType);
      printf("\n");

      printf("Property = ");
      if (Node -> Property[0] == 'S') {
         for (int i = 0; i < Node -> PropertyListLen; i++) {
            if (Node -> Property[i] != 0x08 && Node -> Property[i] != 0x1b) {
               printf("%c%x ", Node -> Property[i], Node -> Property[i]);
            }
         }
      } else if (Node -> propType == 'R') {
         for (int i = 0; i < Node -> PropertyListLen; i++) {
            printf("%x ", (unsigned char)Node -> Property[i]);
         }
      } else if (Node -> propType == 'I') {
         for (int i = 1; i < Node -> PropertyListLen; i += 4) {
            int h0 = (unsigned char)Node -> Property[i];
            int h1 = (unsigned char)Node -> Property[i + 1] << 8;
            int h2 = (unsigned char)Node -> Property[i + 2] << 16;
            int h3 = (unsigned char)Node -> Property[i + 3] << 24;

            int h4 = h0 | h1 | h2 | h3;
            printf("%i ", h4);
         }
      } else {
         for (int i = 0; i < 13; i++) {
            printf("%x ", (unsigned char)Node -> Property[i]);
         }
      }
      
      printf("\n");
   }


   printf("Total Bytes = ");
   printf("%i", Node -> totalBytes);
   printf("\n");
   
   printf("\n");
}

// global variables for readNodeFBX
int bLength = 0;     // buffer length
char* buffer = NULL; // stores all characters after they are read
int size = 0; // size of the char buffer
int nullTerminatedReached = 0; // bool to see if we have reached a null terminating node and if so exit one more depth
int offsetCount = 26; // track of how far we are into the file, starts at 26 because the first line has already been read
struct node** nodeListGlobal = NULL; // running list of nodes as they are read
int nodeListSizeGlobal = 0;
int nodeListCountGlobal = 0;
int orderGlobal = 0; // This gets incremented for each node completed and reset after one of the header nodes finishes
int globalDataClaimed = 0; // used in tracking memory loss

void resetGlobal() {
   bLength = 0;
   buffer = NULL;
   size = 0;
   nullTerminatedReached = 0;
   offsetCount = 26;
   nodeListGlobal = NULL;
   nodeListSizeGlobal = 0;
   nodeListCountGlobal = 0;
   orderGlobal = 0;
   globalDataClaimed = 0;
}

// allocate global node list
void setNodeList() {
   nodeListGlobal = malloc(sizeof(struct node*));
   globalDataClaimed += sizeof(*nodeListGlobal);
}

// reset node list after header node returns
void resetNodeList() {
   globalDataClaimed -= sizeof(*nodeListGlobal);
   
   nodeListSizeGlobal = 0;
   nodeListCountGlobal = 0;
   orderGlobal = 0;
}

struct node** resizeList(struct node** nodeList) {
   nodeListSizeGlobal += 3200;
   globalDataClaimed += 3200;
   nodeList = (struct node**)realloc(nodeList, nodeListSizeGlobal);
   if (!nodeList) Fatal("Out of memory in nodeList\n");
   return nodeList;
}


// FBX nodes have a standard 13 bytes of standard data at the beginning of the node then a char list for the name,
//       after these bytes have been read, if there are more bytes until the end offset, there is
//       another node and you need to recusively call the function again
struct node* readNodeFBX(FILE* f, int depth) {
   char ch; // last character read
   int byteCount = -1; // char count of node
   int propCount = 0;
   struct node* Node = malloc(sizeof(struct node));
   globalDataClaimed += sizeof(struct node);
   Node -> order = orderGlobal;
   orderGlobal++;
   Node -> startOffset = offsetCount + 1;
   Node -> nullNode = 0;
   Node -> NameLen = 0;
   Node -> PropertyListLen = 0;
   Node -> nameSet = 0;
   Node -> propSet = 0;
   Node -> depth = depth;

   lc++;
   while (1) {
      byteCount++;
      offsetCount++;

      // reallocate the size of the buffer
      if (byteCount >= bLength) { 
         bLength += 8192;
         globalDataClaimed += 8192;
         buffer = (char*)realloc(buffer, bLength);
         if (!buffer) Fatal("Out of memory in buffer\n");
      }
      
      // stop once the offsetCount reaches the endoffset
      if ((byteCount > 3) && (offsetCount == Node -> EndOffset)) { 
         break;
      }
      
      // if a node has a nested portion, it will always be terminated with 13 bytes of 0s
      // after the nested portion has been read, you must return from both the null node and 
      // the node that called, this is done with the nullTerminatedReached global variable
      if ((byteCount == 13) && (Node -> EndOffset == 0x00 && Node -> NumProperties == 0x00 && Node -> PropertyListLen == 0x00 && Node -> NameLen == 0x00)) {
         nullTerminatedReached++;
         Node -> nullNode = 1;
         offsetCount--;
         byteCount--;
         break;
      }

      // If there are more bytes to read after the name, call the function again at a depth one lower 
      // (This could have been written better as a do while loop but it works for the time being)
      if ((byteCount >= 13 + Node -> NameLen + Node -> PropertyListLen) && (Node -> EndOffset - offsetCount > 1) && (Node -> EndOffset > 0)) { 
         offsetCount--;
         byteCount--;
         struct node* hold = readNodeFBX(f, depth + 1);
         if (nodeListCountGlobal + 1 > nodeListSizeGlobal) {
            nodeListGlobal = resizeList(nodeListGlobal);
         }
         nodeListGlobal[nodeListCountGlobal++] = hold; 
         while (hold -> nullNode == 0) {
            offsetCount--;
            hold = readNodeFBX(f, depth + 1);
            if (nodeListCountGlobal + 1 > nodeListSizeGlobal) {
               nodeListGlobal = resizeList(nodeListGlobal);
            }
            nodeListGlobal[nodeListCountGlobal++] = hold;
         }
      }

      // only get another ch if the node just exited was not a null terminating node
      if (nullTerminatedReached > 0) {
         nullTerminatedReached--;
      } else if (nullTerminatedReached == 0) {
         ch = fgetc(f);
         // printf("%x,%x ", ch,offsetCount);
         buffer[byteCount] = ch;
      }
      
      
      // the first 4 bytes contain the end offset, adjusted for little endian byte ordering 
      if (byteCount == 3) {
         unsigned int h0 = (unsigned char)buffer[0];
         unsigned int h1 = (unsigned char)buffer[1] << 8;
         unsigned int h2 = (unsigned char)buffer[2] << 16;
         unsigned int h3 = (unsigned char)buffer[3] << 24;
         
         Node -> EndOffset = h0 | h1 | h2 | h3;
      }
      // the next 4 bytes contain the number of properties
      if (byteCount == 7) {
         unsigned int h0 = (unsigned char)buffer[4];
         unsigned int h1 = (unsigned char)buffer[5] << 8;
         unsigned int h2 = (unsigned char)buffer[6] << 16;
         unsigned int h3 = (unsigned char)buffer[7] << 24;

         Node -> NumProperties = h0 | h1 | h2 | h3;
      }
      // the next 4 bytes contain the length required to store all bytes of the property
      if (byteCount == 11) {
         unsigned int h0 = (unsigned char)buffer[8];
         unsigned int h1 = (unsigned char)buffer[9] << 8;
         unsigned int h2 = (unsigned char)buffer[10] << 16;
         unsigned int h3 = (unsigned char)buffer[11] << 24;
         Node -> PropertyListLen = h0 | h1 | h2 | h3;
         Node -> Property = (char*)malloc(Node -> PropertyListLen);
         globalDataClaimed += Node -> PropertyListLen;
      }
      // the next byte contains the length of the name
      if (byteCount == 12) { // get the length of the name string
         Node -> NameLen = (unsigned char)buffer[12];
         Node -> Name = NULL;
         Node -> Name = (char*)realloc(Node -> Name, Node -> NameLen);
         globalDataClaimed += Node -> NameLen;
      }
      // the next nameLength bytes contain the name
      if ((byteCount >= 13) && (byteCount < 13 + Node -> NameLen) && (Node -> nameSet == 0)) { // fill the name buffer
         Node -> Name[byteCount - 13] = ch;
         if (byteCount == 12 + Node -> NameLen) {
            Node ->  nameSet = 1;
         }
      }

      // if the number of properties is greater than one, record the property for propLen bytes
      if ((byteCount >= 13 + Node -> NameLen) && (byteCount < 13 + Node -> NameLen + Node -> PropertyListLen) && (Node -> propSet == 0)) {
         Node -> Property[propCount++] = ch;
         if (byteCount == 12 + Node -> NameLen + Node -> PropertyListLen) {
            Node -> propSet = 1;
         }
      }
   }

   lc++;
   Node -> totalBytes = Node -> EndOffset - Node -> startOffset;
   return Node;
}

// This function fill the fbx file structure with pointers to each of the lists of nodes
// it also sorts by the nodes order value, so they appear in the same order as in the file
void fill(struct nodeList* n1, struct node** n2, int nodeListCount) {
   struct node** hold1 = malloc(sizeof(struct node*) * nodeListCount);
   globalDataClaimed += sizeof(*hold1);
   for (int i = 1; i <= nodeListCount; i++) {
      for (int j = 0; j < nodeListCount; j++) {
         if (n2[j] -> order == i) {
            hold1[i - 1] = n2[j];
         }
      }
   }

   n1 -> nodeListInter = malloc(sizeof(struct node*) * nodeListCount);
   globalDataClaimed += sizeof(*n1 -> nodeListInter);
   for (int i = 0; i < nodeListCount; i++) {
      struct node* hold2 = malloc(sizeof(struct node));
      globalDataClaimed += sizeof(*hold2);
      *hold2 = *hold1[i];
      n1 -> nodeListInter[i] = hold2;
      globalDataClaimed -= sizeof(*hold1[i]);
      free(hold1[i]);
   }
   globalDataClaimed -= sizeof(*hold1);
   free(hold1);
}

// test if two strings are equal
int stringEquals(char* s1, char* s2, int l1, int l2) {
   if (l1 != l2) {
      return 0;
   } else {
      for (int i = 0; i < l1; i++) {
         if (s1[i] != s2[i]) {
            return 0;
         }
      }
      return 1;
   }
}

// attempt to free the data allocated when making an fbxfile, helps some but dosen't get everything
void freeFBXFile(struct fbxFile* file) {
   for (int i = 0; i < file -> _FBXHeaderExtension.nodeListCount; i++) {
      globalDataClaimed -= sizeof(*file -> _FBXHeaderExtension.nodeListInter[i]);
      free(file -> _FBXHeaderExtension.nodeListInter[i]);
   }
   if (file -> _FBXHeaderExtension.nodeListCount > 0) {
      globalDataClaimed -= file -> _FBXHeaderExtension.nodeListSize;
      free(file -> _FBXHeaderExtension.nodeListInter);
   }
   
   for (int i = 0; i < file -> _FileID.nodeListCount; i++) {
      globalDataClaimed -= sizeof(*file -> _FileID.nodeListInter[i]);
      free(file -> _FileID.nodeListInter[i]);
   }
   if (file -> _FileID.nodeListCount > 0) {
      globalDataClaimed -= file -> _FileID.nodeListSize;
      free(file -> _FileID.nodeListInter);
   }
   
   for (int i = 0; i < file -> _CreationTime.nodeListCount; i++) {
      globalDataClaimed -= sizeof(*file -> _CreationTime.nodeListInter[i]);
      free(file -> _CreationTime.nodeListInter[i]);
   }
   if (file -> _CreationTime.nodeListCount > 0) {
      globalDataClaimed -= file -> _CreationTime.nodeListSize;
      free(file -> _CreationTime.nodeListInter);
   }

   for (int i = 0; i < file -> _Creator.nodeListCount; i++) {
      globalDataClaimed -= sizeof(*file -> _Creator.nodeListInter[i]);
      free(file -> _Creator.nodeListInter[i]);
   }
   if (file -> _Creator.nodeListCount > 0) {
      globalDataClaimed -= file -> _Creator.nodeListSize;
      free(file -> _Creator.nodeListInter);
   }

   for (int i = 0; i < file -> _GlobalSettings.nodeListCount; i++) {
      globalDataClaimed -= sizeof(*file -> _GlobalSettings.nodeListInter[i]);
      free(file -> _GlobalSettings.nodeListInter[i]);
   }
   if (file -> _GlobalSettings.nodeListCount > 0) {
      globalDataClaimed -= file -> _GlobalSettings.nodeListSize;
      free(file -> _GlobalSettings.nodeListInter);
   }

   for (int i = 0; i < file -> _Documents.nodeListCount; i++) {
      globalDataClaimed -= sizeof(*file -> _Documents.nodeListInter[i]);
      free(file -> _Documents.nodeListInter[i]);
   }
   if (file -> _Documents.nodeListCount > 0) {
      globalDataClaimed -= file -> _Documents.nodeListSize;
      free(file -> _Documents.nodeListInter);
   }

   for (int i = 0; i < file -> _References.nodeListCount; i++) {
      globalDataClaimed -= sizeof(*file -> _References.nodeListInter[i]);
      free(file -> _References.nodeListInter[i]);
   }
   if (file -> _References.nodeListCount > 0) {
      globalDataClaimed -= file -> _References.nodeListSize;
      free(file -> _References.nodeListInter);
   }

   for (int i = 0; i < file -> _Definitions.nodeListCount; i++) {
      globalDataClaimed -= sizeof(*file -> _Definitions.nodeListInter[i]);
      free(file -> _Definitions.nodeListInter[i]);
   }
   if (file -> _Definitions.nodeListCount > 0) {
      globalDataClaimed -= file -> _Definitions.nodeListSize;
      free(file -> _Definitions.nodeListInter);
   }

   for (int i = 0; i < file -> _Objects.nodeListCount; i++) {
      globalDataClaimed -= sizeof(*file -> _Objects.nodeListInter[i]);
      free(file -> _Objects.nodeListInter[i]);
   }
   if (file -> _Objects.nodeListCount > 0) {
      globalDataClaimed -= file -> _Objects.nodeListSize;
      free(file -> _Objects.nodeListInter);
   }

   for (int i = 0; i < file -> _Connections.nodeListCount; i++) {
      globalDataClaimed -= sizeof(*file -> _Connections.nodeListInter[i]);
      free(file -> _Connections.nodeListInter[i]);
   }
   if (file -> _Connections.nodeListCount > 0) {
      globalDataClaimed -= file -> _Connections.nodeListSize;
      free(file -> _Connections.nodeListInter);
   }

   for (int i = 0; i < file -> _Takes.nodeListCount; i++) {
      globalDataClaimed -= sizeof(*file -> _Takes.nodeListInter[i]);
      free(file -> _Takes.nodeListInter[i]);
   }
   if (file -> _Takes.nodeListCount > 0) {
      globalDataClaimed -= file -> _Takes.nodeListSize;
      free(file -> _Takes.nodeListInter);
   }
   globalDataClaimed -= sizeof(*file);
   free(file);
}

// These functions take the property value of a node and turn it into an array of readable data
// there are 13 different formats data can take and they are all adjusted for little-endian byte ordering
int intReadProp4Byte(char* prop) {
   int h0 = (unsigned char)prop[0];
   int h1 = (unsigned char)prop[1] << 8;
   int h2 = (unsigned char)prop[2] << 16;
   int h3 = (unsigned char)prop[3] << 24;
   
   int final =  h0 | h1 | h2 | h3;
   return final;
}
int boolReadProp(char* prop) {
   return *prop;
}
float floatReadProp(char* prop) {
   int h0 = (unsigned char)prop[0];
   int h1 = (unsigned char)prop[1] << 8;
   int h2 = (unsigned char)prop[2] << 16;
   int h3 = (unsigned char)prop[3] << 24;

   int h4 =  h0 | h1 | h2 | h3;
   float* final1 = &h4;
   float final2 = *final1;
   return final2;
}
// no matter what data type i used, it wouldn't let me bit shift more than 24 bits
// so i have to make two ints and memcpy over the space for a double
double doubleReadProp(char* prop) {
   int h0 = (unsigned char)prop[0];
   int h1 = (unsigned char)prop[1] << 8;
   int h2 = (unsigned char)prop[2] << 16;
   int h3 = (unsigned char)prop[3] << 24;
   int h4 = (unsigned char)prop[4];
   int h5 = (unsigned char)prop[5] << 8;
   int h6 = (unsigned char)prop[6] << 16;
   int h7 = (unsigned char)prop[7] << 24;
   int h8 = h0 | h1 | h2 | h3;
   int h9 = h4 | h5 | h6 | h7;
   
   double* finalPtr = malloc(sizeof(double));
   int* finalPtr1 = finalPtr;
   int* finalPtr2 = finalPtr1 + 1;
   memcpy(finalPtr1, &h8, 4);
   memcpy(finalPtr2, &h9, 4);
   double final = *finalPtr;
   return final;
}

// the ones with underscores indicate that they are for arrays (_floatReadProp reads an array of floats)
float* _floatReadProp(char* prop, int size) { 
   float* final = malloc(sizeof(float) * (size / sizeof(float)));
   char hold[4];
   char* holdPtr = hold;
   for (int i = 0; i < size; i += 4) {
      holdPtr[0] = (unsigned char)prop[i];
      holdPtr[1] = (unsigned char)prop[i+1];
      holdPtr[2] = (unsigned char)prop[i+2];
      holdPtr[3] = (unsigned char)prop[i+3];
      final[i / 4] = floatReadProp(holdPtr);
   }
   return final;
}

uint32_t* _doubleMeta(char* prop) {
   uint32_t* meta = malloc(sizeof(uint32_t) * 3);
   
   for (int i = 0; i < 12; i += 4) {
      int h0 = (unsigned char)prop[i];
      int h1 = (unsigned char)prop[i + 1] << 8;
      int h2 = (unsigned char)prop[i + 2] << 16;
      int h3 = (unsigned char)prop[i + 3] << 24;
      
      uint32_t h4 = h0 | h1 | h2 | h3;
      meta[i / 4] = h4;
   }
   return meta;
}
double* _doubleReadProp(char* prop, int size) {
   double* final = malloc(sizeof(double) * (size / sizeof(double)));
   char hold[8];
   char* holdPtr = hold;
   for (int i = 0; i < size; i += 8) {
      holdPtr[0] = (unsigned char)prop[i];
      holdPtr[1] = (unsigned char)prop[i + 1];
      holdPtr[2] = (unsigned char)prop[i + 2];
      holdPtr[3] = (unsigned char)prop[i + 3];
      holdPtr[4] = (unsigned char)prop[i + 4];
      holdPtr[5] = (unsigned char)prop[i + 5];
      holdPtr[6] = (unsigned char)prop[i + 6];
      holdPtr[7] = (unsigned char)prop[i + 7];
      final[i / 8] = doubleReadProp(holdPtr);
   }
   return final;
}
int* _intReadProp(char* prop, int size) {
   int* final = malloc(sizeof(int) * (size / sizeof(int)));
   char hold[4];
   char* holdPtr = hold;
   for (int i = 0; i < size; i += 4) {
      holdPtr[0] = (unsigned char)prop[i];
      holdPtr[1] = (unsigned char)prop[i+1];
      holdPtr[2] = (unsigned char)prop[i+2];
      holdPtr[3] = (unsigned char)prop[i+3];
      final[i / 4] = intReadProp4Byte(holdPtr);
   }
   return final;
}


// This was made just to move vertex buffer data over to the display function
struct vertexDataDouble {
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

struct translations{
   double* translation;
   double* rotations;
   int count;
   int translationSize;
};

// stores both vertexDataDouble and translations to store in the object buffer
struct dataDoubleList{
   struct vertexDataDouble** data;
   struct translations* trans;
   int objectNum;
   int dataSize;
};



// This function loads and fbx file and returns the vertex data and vertex indexes for one object
// in the file
struct dataDoubleList* loadFBX(const char* file) {
   FILE* f = fopen(file, "r");
   struct node* Node1 = NULL;
   struct fbxFile* file1 = malloc(sizeof(struct fbxFile));
   globalDataClaimed += sizeof(*file1);
   int count = 0;

   struct dataDoubleList* fin = malloc(sizeof(struct dataDoubleList));
   fin -> dataSize = 8;
   fin -> objectNum = 0;
   fin -> data = malloc(sizeof(int) * fin -> dataSize);

   getFirstLine(f);
   if (!testFirstLine()) Fatal("Not an FBX File");
   while (count < 11) {
      setNodeList();
      Node1 = readNodeFBX(f, 1);
      // This portion initializes the values of the fbxFile data type
      switch (count) {
         case 0: 
            file1 -> FBXHeaderExtension = Node1;
            fill(&file1 -> _FBXHeaderExtension, nodeListGlobal, nodeListCountGlobal);
            file1 -> _FBXHeaderExtension.nodeListCount = nodeListCountGlobal;
            file1 -> _FBXHeaderExtension.nodeListSize = nodeListSizeGlobal;
            break;
         case 1: 
            file1 -> FileID = Node1;
            fill(&file1 -> _FileID, nodeListGlobal, nodeListCountGlobal);
            file1 -> _FileID.nodeListCount = nodeListCountGlobal;
            file1 -> _FileID.nodeListSize = nodeListSizeGlobal;
            break;
         case 2:
            file1 -> CreationTime = Node1;
            fill(&file1 -> _CreationTime, nodeListGlobal, nodeListCountGlobal);
            file1 -> _CreationTime.nodeListCount = nodeListCountGlobal;
            file1 -> _CreationTime.nodeListSize = nodeListSizeGlobal;
            break;
         case 3:
            file1 -> Creator = Node1;
            fill(&file1 -> _Creator, nodeListGlobal, nodeListCountGlobal);
            file1 -> _Creator.nodeListCount = nodeListCountGlobal;
            file1 -> _Creator.nodeListSize = nodeListSizeGlobal;
            break;
         case 4:
            file1 -> GlobalSettings = Node1;
            fill(&file1 -> _GlobalSettings, nodeListGlobal, nodeListCountGlobal);
            file1 -> _GlobalSettings.nodeListCount = nodeListCountGlobal;
            file1 -> _GlobalSettings.nodeListSize = nodeListSizeGlobal;
            break;
         case 5:
            file1 -> Documents = Node1;
            fill(&file1 -> _Documents, nodeListGlobal, nodeListCountGlobal);
            file1 -> _Documents.nodeListCount = nodeListCountGlobal;
            file1 -> _Documents.nodeListSize = nodeListSizeGlobal;
            break;
         case 6:
            file1 -> References = Node1;
            fill(&file1 -> _References, nodeListGlobal, nodeListCountGlobal);
            file1 -> _References.nodeListCount = nodeListCountGlobal;
            file1 -> _References.nodeListSize = nodeListSizeGlobal;
            break;
         case 7:
            file1 -> Definitions = Node1;
            fill(&file1 -> _Definitions, nodeListGlobal, nodeListCountGlobal);
            file1 -> _Definitions.nodeListCount = nodeListCountGlobal;
            file1 -> _Definitions.nodeListSize = nodeListSizeGlobal;
            break;
         case 8:
            file1 -> Objects = Node1;
            fill(&file1 -> _Objects, nodeListGlobal, nodeListCountGlobal);
            file1 -> _Objects.nodeListCount = nodeListCountGlobal;
            file1 -> _Objects.nodeListSize = nodeListSizeGlobal;
            break;
         case 9:
            file1 -> Connections = Node1;
            fill(&file1 -> _Connections, nodeListGlobal, nodeListCountGlobal);
            file1 -> _Connections.nodeListCount = nodeListCountGlobal;
            file1 -> _Connections.nodeListSize = nodeListSizeGlobal;
            break;
         case 10:
            file1 -> Takes = Node1;
            fill(&file1 -> _Takes, nodeListGlobal, nodeListCountGlobal);
            file1 -> _Takes.nodeListCount = nodeListCountGlobal;
            file1 -> _Takes.nodeListSize = nodeListSizeGlobal;
            break;
      }
      offsetCount--;
      count++;
      resetNodeList();
      globalDataClaimed -= sizeof(*Node1);
      free(Node1);
   }
   fclose(f);
   struct vertexDataDouble* vert;
   struct translations* trans;
   
   
   fflush(stdout);
   trans = malloc(sizeof(struct translations));
   
   
   trans -> translationSize = 8;
   trans -> translation = malloc(sizeof(double) * 3 * trans -> translationSize);
   trans -> rotations = malloc(sizeof(double) * 3 * trans -> translationSize);
   trans -> count = -1;
   int allocVert = 1;
   int reachedModel = 0;
   
   
   // this portion looks through the object header of the fbxfile type and finds the vertex data
   for (int i = 0; i < file1 -> _Objects.nodeListCount; i++) {
      if (allocVert) {
         vert = malloc(sizeof(struct vertexDataDouble));
         vert -> vertLength = 0;
         fin -> data[fin -> objectNum] = vert;
         allocVert = 0;
      }

      struct node** propList = file1 -> _Objects.nodeListInter;

      // All of these stringEquals statements find a node based on the name and propegates the vertexDataDouble
      // object accordingly, Everytime it reads a vertices node when the vertLength is not 0, it knows there is more
      // than one object so it allocates a new vertexDataDouble object and adds the address to the final list
      if (stringEquals(propList[i] -> Name, "Vertices", propList[i] -> NameLen, 8)) {
         if (vert -> vertLength > 0) {
            vert = malloc(sizeof(struct vertexDataDouble));
            vert -> vertLength = 0;
            fin -> objectNum++;
            if (fin -> objectNum - 1 >= fin -> dataSize) {
               fin -> dataSize += 8;
               fin -> data = realloc(fin -> data, fin -> dataSize);
            }
            fin -> data[fin -> objectNum] = vert;
         }
         // if the property type is a list of doubles
         if (propList[i] -> Property[0] == 'd') {
            
            // get the list meta data
            uint32_t* meta = _doubleMeta(propList[i] -> Property + 1);

            // if the data is not encoded (stored in meta[1]) read doubles normaly
            // otherwise read them as a byte string and decrypt them and bring them back
            if (meta[1] == 0) {
               vert -> vertecies = _doubleReadProp(propList[i] -> Property + 13, meta[0] * 8);
               vert -> vertLength = meta[0];
            } else if (meta[1] == 1) {
               
               char* in = malloc(sizeof(char) * meta[2]);
               for (int k = 0; k < meta[2]; k++) {
                  in[k] = (unsigned char)propList[i] -> Property[k + 13];
               }
               struct infHelp decoded = inf(in, meta[2], meta[0] * 8);
               
               vert -> vertecies = _doubleReadProp(decoded.list, meta[0] * 8);
               vert -> vertLength = meta[0];
            }
         } 
      }
      if (stringEquals(propList[i] -> Name, "PolygonVertexIndex", propList[i] -> NameLen, 18)) {
         if (propList[i] -> Property[0] == 'i') {
            
            uint32_t* meta = _doubleMeta(propList[i] -> Property + 1);
            if (meta[1] == 0) {
               vert -> vertexIndex = _intReadProp(propList[i] -> Property + 13, meta[0] * 4);
               vert -> indexLength = meta[0];
            } else if (meta[1] == 1) {

               char* in = malloc(sizeof(char) * meta[2]);
               for (int k = 0; k < meta[2]; k++) {
                  in[k] = (unsigned char)propList[i] -> Property[k + 13];
               }
               struct infHelp decoded = inf(in, meta[2], meta[0] * 4);

               vert -> vertexIndex = _intReadProp(decoded.list, meta[0] * 4);
               vert -> indexLength = meta[0];
            }
         }
      }
      if (stringEquals(propList[i] -> Name, "Edges", propList[i] -> NameLen, 5)) {
         if (propList[i] -> Property[0] == 'i') {

            uint32_t* meta = _doubleMeta(propList[i] -> Property + 1);
            if (meta[1] == 0) {
               vert -> edges = _intReadProp(propList[i] -> Property + 13, meta[0] * 4);
               vert -> edgesLength = meta[0];
            } else if (meta[1] == 1) {

               char* in = malloc(sizeof(char) * meta[2]);
               for (int k = 0; k < meta[2]; k++) {
                  in[k] = (unsigned char)propList[i] -> Property[k + 13];
               }
               struct infHelp decoded = inf(in, meta[2], meta[0] * 4);

               vert -> edges = _intReadProp(decoded.list, meta[0] * 4);
               vert -> edgesLength = meta[0];
            }
         }
      }
      if (stringEquals(propList[i] -> Name, "UV", propList[i] -> NameLen, 2)) {
         if (propList[i] -> Property[0] == 'd') {
            
            uint32_t* meta = _doubleMeta(propList[i] -> Property + 1);
            if (meta[1] == 0) {
               vert -> UV = _doubleReadProp(propList[i] -> Property + 13, meta[0] * 8);
               vert -> uvLength = meta[0];
            } else if (meta[1] == 1) {

               char* in = malloc(sizeof(char) * meta[2]);
               for (int k = 0; k < meta[2]; k++) {
                  in[k] = (unsigned char)propList[i] -> Property[k + 13];
               }
               struct infHelp decoded = inf(in, meta[2], meta[0] * 8);

               vert -> UV = _doubleReadProp(decoded.list, meta[0] * 8);
               vert -> uvLength = meta[0];
            }
         }
      }
      if (stringEquals(propList[i] -> Name, "UVIndex", propList[i] -> NameLen, 7)) {
         
         if (propList[i] -> Property[0] == 'i') {
            
            uint32_t* meta = _doubleMeta(propList[i] -> Property + 1);
            if (meta[1] == 0) {
               vert -> UVIndex = _intReadProp(propList[i] -> Property + 13, meta[0] * 4);
               vert -> uvIndexLen = meta[0];
            } else if (meta[1] == 1) {

               char* in = malloc(sizeof(char) * meta[2]);
               for (int k = 0; k < meta[2]; k++) {
                  in[k] = (unsigned char)propList[i] -> Property[k + 13];
               }
               struct infHelp decoded = inf(in, meta[2], meta[0] * 4);

               vert -> UVIndex = _intReadProp(decoded.list, meta[0] * 4);
               vert -> uvIndexLen = meta[0];
            }
         }
      }
      if (stringEquals(propList[i] -> Name, "Normals", propList[i] -> NameLen, 7)) {
         if (propList[i] -> Property[0] == 'd') {

            uint32_t* meta = _doubleMeta(propList[i] -> Property + 1);
            if (meta[1] == 0) {
               vert -> normals = _doubleReadProp(propList[i] -> Property + 13, meta[0] * 8);
               vert -> normalsLength = meta[0];
            } else if (meta[1] == 1) {
               
               char* in = malloc(sizeof(char) * meta[2]);
               for (int k = 0; k < meta[2]; k++) {
                  in[k] = (unsigned char)propList[i] -> Property[k + 13];
               }

               struct infHelp decoded = inf(in, meta[2], meta[0] * 8);

               vert -> normals = _doubleReadProp(decoded.list, meta[0] * 8);
               vert -> normalsLength = meta[0];
            }
         }
         
      }

      // The translations are slightly different in how they are stored because the node name
      // will be P but there are many other values P can contain so I check for the first couple
      // characters of the property buffer to see if they are a rotation or translation
      if (stringEquals(propList[i] -> Name, "Model", propList[i] -> NameLen, 5)) {
         reachedModel = 1;
         trans -> count++;
      }
      if (reachedModel && stringEquals(propList[i] -> Name, "P", propList[i] -> NameLen, 1) ) {
         char hold1[16] = {0x53, 0xf, 0x0, 0x0, 0x0, 0x4c, 0x63, 0x6c, 0x20, 0x54, 0x72, 0x61, 0x6e, 0x73, 0x6c, 0x61}; // "S     Lcl Transla"
         char hold2[17] = {0x53, 0xc, 0x0, 0x0, 0x0, 0x4c, 0x63, 0x6c, 0x20, 0x52, 0x6f, 0x74, 0x61, 0x74, 0x69, 0x6f, 0x6e};
         
         if (stringEquals(propList[i] -> Property, hold1, 16, 16)) {
            if (trans -> count - 1 >= trans -> translationSize) {
               trans -> translationSize += 8;
               trans -> translation = realloc(trans -> translation, trans -> translationSize);
               trans -> rotations = realloc(trans -> rotations, trans -> translationSize);
            }

            double x = 0;
            double y = 0;
            double z = 0;
            if (propList[i] -> Property[51] == 0x44) {
               x = doubleReadProp(&propList[i] -> Property[52]);
            }
            if (propList[i] -> Property[60] == 0x44) {
               y = doubleReadProp(&propList[i] -> Property[61]);
            }
            if (propList[i] -> Property[69] == 0x44) {
               z = doubleReadProp(&propList[i] -> Property[70]);
            }
            trans -> translation[trans -> count * 3] = x;
            trans -> translation[trans -> count * 3 + 1] = y;
            trans -> translation[trans -> count * 3 + 2] = z;
         }
         if (stringEquals(propList[i] -> Property, hold2, 17, 17)) {

            if (trans -> count - 1 >= trans -> translationSize) {
               trans -> translationSize += 8;
               trans -> translation = realloc(trans -> translation, trans -> translationSize);
               trans -> rotations = realloc(trans -> rotations, trans -> translationSize);
            }

            double x = 0;
            double y = 0;
            double z = 0;
            if (propList[i] -> Property[45] == 0x44) {
               x = doubleReadProp(&propList[i] -> Property[46]);
            }
            if (propList[i] -> Property[54] == 0x44) {
               y = doubleReadProp(&propList[i] -> Property[55]);
            }
            if (propList[i] -> Property[63] == 0x44) {
               z = doubleReadProp(&propList[i] -> Property[64]);
            }
            trans -> rotations[trans -> count * 3] = x;
            trans -> rotations[trans -> count * 3 + 1] = y;
            trans -> rotations[trans -> count * 3 + 2] = z;
         }
      }
      
   }
   

   if (buffer != NULL) {
      globalDataClaimed -= sizeof(*buffer);
      free(buffer);
   }
   globalDataClaimed -= sizeof(*file1);
   freeFBXFile(file1);

   resetGlobal();

   fin -> trans = trans;
   fin -> objectNum++;
   return fin;
}