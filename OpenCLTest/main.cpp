#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>
#include <vector>
#include <pthread.h>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#include <GLUT/glut.h>
#else
#include <CL/cl.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "resourceManager.hpp"
#include "MyCamera.h";
#include "Vector.hpp"
#include "Matrix.hpp"


using namespace std;
ResourceManager *r;
MyCamera *c;
cl_device_id device=nullptr;
cl_context context=nullptr;
cl_command_queue queue=nullptr;
cl_program program=nullptr;
cl_kernel kernel = nullptr;
cl_int outcome=0;

#define WINDOW_W 500
#define WINDOW_H 500

int NUM_POINTS =1024;
cl_mem d_mem=nullptr;
cl_float3 *h_mem=nullptr;

float a_time=0.0f;
float a_dt=0.01f;

float m_width;
float m_height;

size_t global_work_size[1];
size_t local_work_size[1];

int frameCount=0;
float fps =0.0f;
int currentTime=0, previousTime=0;

int mode=0;

bool keyboard_status = false;


extern void cpu_d_rungekutta(int num_particles, int index, cl_float3 *position, float a_time, float a_dt);

void runGPU();
void runCPU();
void runCPUThreads();

void (* const operations[])() ={
  runGPU,runCPUThreads,runCPU,
    
};
const string operation_type[]={"GPU","CPU Thread","CPU", };


struct CpuParam{
    int num_particles;
    int index;
    int block_s;
    cl_float3 *position;
    float a_time;
    float a_dt;
};

struct MousePointer{
    int x=0;
    int y=0;
}oldMousePosition,newMousePosition ;

void* tfunction(void * x){
    CpuParam *p = (CpuParam *)x;
    int i=p->index*p->block_s;
    int l=i+p->block_s;
    while (i<l) {
        cpu_d_rungekutta(p->num_particles, i, p->position, p->a_time, p->a_dt);
        i++;
    }
    
    return 0;
}
void init(int n)
{
    if (h_mem){
        delete [] h_mem;
    }
    if (d_mem){
        clReleaseMemObject(d_mem);
    }
    
    
    a_time=0.0f;
    
    d_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float3)*n, nullptr, &outcome);
    h_mem = new cl_float3[n];
    
    srand(12345);
    for (int i =0; i<n; i++){
        h_mem[i].x=(float)rand()/RAND_MAX*0.5f-0.25f;
        h_mem[i].y=(float)rand()/RAND_MAX*0.5f-0.25f;
        h_mem[i].z=(float)rand()/RAND_MAX*0.5f-0.25f;
    }
    
    outcome = clEnqueueWriteBuffer(queue, d_mem, CL_TRUE, 0, sizeof(cl_float3)*n, h_mem, 0, nullptr, nullptr);
    
    
    //creating 1-dim-array at once with assignment of array.
    //size_t global_work_size[1]={NUM_POINTS};
    //size_t local_work_size[1]={512};
    
    
    global_work_size[0]=n;
    local_work_size[0]=512;
    
}

void DrawString(const string &str,void *font,float x,float y){
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, m_width, m_height, 0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glRasterPos2f(x, y);
    const char *ite=str.c_str();
    while(*ite){
        glutBitmapCharacter(font, *ite);
        ++ite;
    }
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void runCPU(){
    int n=NUM_POINTS;
    for (int i=0; i<n; i++){
        cpu_d_rungekutta(n, i, h_mem, a_time, a_dt);
    }
    a_time+=a_dt;
}

void runCPUThreads(){
    int n=NUM_POINTS;
    int block_s=512;
    int l=n/block_s;
    vector<CpuParam> params(l);
    vector<pthread_t> threads(l,0);
    for (int i=0; i<l; i++){
        params[i].num_particles=n;
        params[i].index=i;
        params[i].block_s=block_s;
        params[i].position=h_mem;
        params[i].a_time=a_time;
        params[i].a_dt=a_dt;
    }
    for (int i=0; i<l; i++){
        int s= pthread_create(&(threads[i]), 0, &tfunction,&(params[i]));
        if (s!=0) {
            threads[i]=0;
        }
    }
    for (int i=0; i<l; i++){
        if (threads[i]==0) continue;
        pthread_join(threads[i], 0);
    }
    a_time+=a_dt;
}

void runGPU(){
    int n=NUM_POINTS;
    clSetKernelArg(kernel, 0, sizeof(cl_int), (void *)&n);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&d_mem);
    clSetKernelArg(kernel, 2, sizeof(cl_float), (void *)&a_time);
    clSetKernelArg(kernel, 3, sizeof(cl_float), (void *)&a_dt);
    clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, global_work_size,local_work_size, 0, nullptr, nullptr);
    outcome = clEnqueueReadBuffer(queue, d_mem, CL_TRUE, 0, sizeof(cl_float3)*n, h_mem, 0, nullptr, nullptr);
    a_time+=a_dt;
    
}

void display(void)
{
    if (keyboard_status) {
        return;
    }
    
    int n = NUM_POINTS;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, m_width/m_height, 1.0, 20.0);
    glMatrixMode(GL_MODELVIEW);
    glClearColor(1.0f,1.0f,1.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glLoadIdentity();
    
    
    gluLookAt(c->x, c->y, c->z, c->targetX, c->targetY, c->targetZ, 0.0, 1.0, 0.0);
    
    glColor3f(0.0f, 0.0f, 0.0f);
    
    ostringstream stream;
    stream<<"FrameRate (fps): "<<fps;
    DrawString(stream.str(),GLUT_BITMAP_HELVETICA_12,30.0, 30.0);
    
    stream.str("");
    
    stream<<"Operation type: "<<operation_type[mode];
    DrawString(stream.str(),GLUT_BITMAP_HELVETICA_12,30.0, 50.0);
    
    stream.str("");
    
    stream<<"Particles: "<<NUM_POINTS;
    DrawString(stream.str(),GLUT_BITMAP_HELVETICA_12,30.0, 70.0);
    
    
    
    glBegin(GL_LINES);
    glColor3d(0.8,0.8,0.8);
    glVertex3d(-100,0,0);
    glVertex3d(100, 0,0);
    glVertex3d(0,0,0);
    glVertex3d(0,100,0);
    glVertex3d(0,0,-100);
    glVertex3d(0,0, 100);
    glEnd();
    
    switch (mode) {
        case 0:
            glColor3f(0.8f, 0.5f, 0.5f);
            break;
        case 1:
            glColor3f(0.5f, 0.8f, 0.5f);
            break;
        case 2:
            glColor3f(0.5f, 0.5f, 0.8f);
            break;
            
        default:
            break;
    }
    
    glBegin(GL_POINTS);
    for(int i=0; i<n; i++){
        glVertex3f(h_mem[i].x, h_mem[i].y,h_mem[i].z);
    }
    glEnd();
    
    

    
    
    
    glutSwapBuffers();

}


void resize(int width, int height)
{
    m_width=width;
    m_height=height;
    glViewport(0, 0, width, height);
}


void cleanUp(void)
{
    clFlush(queue);
    clFinish(queue);
    clReleaseCommandQueue(queue);
    clReleaseMemObject(d_mem);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseContext(context);
    delete r;
    delete c;
    delete [] h_mem;
}


void keyboard(int key, int x, int y){
    const int size = sizeof operations / sizeof operations[0];
    keyboard_status=true;
    int n=NUM_POINTS;
    
    
    switch (key) {
        case GLUT_KEY_LEFT:
            mode=(mode-1)<0?size-1:mode-1;
            break;
        case GLUT_KEY_RIGHT:
            mode=(mode+1)%size;
            break;
        case GLUT_KEY_UP:
            NUM_POINTS=n*2;
            break;
        case GLUT_KEY_DOWN:
            NUM_POINTS=(n/2)<1024?1024:n/2;
            break;
        default:
            break;
    }
    
    init(NUM_POINTS);
    keyboard_status=false;
    

}
void calculateFPS(){
    frameCount++;
    currentTime=glutGet(GLUT_ELAPSED_TIME);
    
    int timeInterval=currentTime-previousTime;
    
    if (timeInterval>1000){
        fps=frameCount / (timeInterval/1000.0f);
        
        previousTime=currentTime;
        frameCount=0;
    }
    
}
void idle(void){
    if (keyboard_status) {
        return;
    }
    
    operations[mode]();
    
    c->update();
    calculateFPS();
    glutPostRedisplay();
    
}

/*
void timer(int value){
    runGPU();
    //runCPU();
    //runCPUThreads();
    glutPostRedisplay();
    glutTimerFunc(50,timer, 0);
}
*/

void dragMotion(int x,int y){
    
    oldMousePosition.x=newMousePosition.x;
    oldMousePosition.y=newMousePosition.y;
    
    newMousePosition.x=x;
    newMousePosition.y=y;
    
    if (newMousePosition.x-oldMousePosition.x>0){
        c->rotateY+=1.0f;
    }
    else if(newMousePosition.x-oldMousePosition.x<0){
        c->rotateY-=1.0f;
    }
    
    if (newMousePosition.y-oldMousePosition.y>0){
        c->rotateX-=1.0f;
    }
    else if(newMousePosition.y-oldMousePosition.y<0){
        c->rotateX+=1.0f;
    }
    
    
    

}
void passiveMotion(int x,int y){
    
    oldMousePosition.x=x;
    oldMousePosition.y=y;
    newMousePosition.x=x;
    newMousePosition.y=y;
}


int main(int argc, char * argv[]) {
    
    r = new ResourceManager();
    c= new MyCamera(3.0f);
    string absPath=r->GetResourcePath();
    
    cout<<oldMousePosition.x<<endl;
    


    cl_platform_id platform;
    cl_uint platformCount;
    outcome = clGetPlatformIDs(1, &platform, &platformCount);
    if (platformCount == 0) {
        cerr << "No platform.\n";
        return EXIT_FAILURE;
    }
    
    char vendor[100]={0};
    char version[100]={0};
    clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, sizeof(vendor), vendor, nullptr);
    clGetPlatformInfo(platform, CL_PLATFORM_VERSION, sizeof(version), version, nullptr);
    cout << "Platform id: " << platform << ", Vendor: " << vendor << ", Version: " << version << "\n";

    cl_uint deviceCount;
    outcome = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &deviceCount);
    if (deviceCount == 0) {
        cerr << "No device.\n";
        return EXIT_FAILURE;
    }
    
    char dname[100]={0};
    size_t d_len;
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(dname), dname, &d_len);
    cout << "Device name: " << dname << "\n";
    
    ifstream sourcepath("kernel.cl");
    istreambuf_iterator<char> vdataBegin(sourcepath);
    istreambuf_iterator<char> vdataEnd;
    string source(vdataBegin,vdataEnd);
    const char *c_source = source.c_str();
    const size_t source_l = source.length();
    

    
    context = clCreateContext(0 ,1 ,&device ,nullptr ,nullptr ,&outcome);
    queue = clCreateCommandQueue(context, device, 0, &outcome);
    program = clCreateProgramWithSource(context, 1, &c_source, &source_l, &outcome);

    clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    
    
    kernel = clCreateKernel(program, "d_rungekutta", &outcome);
    

    
    init(NUM_POINTS);

    
    glutInit( &argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(WINDOW_W, WINDOW_H);
    glutCreateWindow("OpenCLSimulation");
    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    //glutTimerFunc(100,timer, 0);
    glutIdleFunc(idle);
    glutMotionFunc(dragMotion);
    glutPassiveMotionFunc(passiveMotion);
    glutSpecialFunc(keyboard);
    
    atexit(cleanUp);
    glutMainLoop();

    return EXIT_SUCCESS;

    
}
