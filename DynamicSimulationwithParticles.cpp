//===========================================================================
/*
 This file is part of the CHAI 3D visualization and haptics libraries.
 Copyright (C) 2003-2009 by CHAI 3D. All rights reserved.
 
 This library is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License("GPL") version 2
 as published by the Free Software Foundation.
 
 For using the CHAI 3D libraries with software that can not be combined
 with the GNU GPL, and for taking advantage of the additional benefits
 of our support services, please contact CHAI 3D about acquiring a
 Professional Edition License.
 
 \author    <http://www.chai3d.org>
 \author    Francois Conti
 \version   2.0.0 $Rev: 269 $
 */
//===========================================================================

//---------------------------------------------------------------------------
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//---------------------------------------------------------------------------
#include "chai3d.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// DECLARED CONSTANTS
//---------------------------------------------------------------------------

// initial size (width/height) in pixels of the display window
const int WINDOW_SIZE_W = 600;
const int WINDOW_SIZE_H = 600;

// mouse menu options (right button)
const int OPTION_FULLSCREEN = 1;
const int OPTION_WINDOWDISPLAY = 2;

//---------------------------------------------------------------------------
// DECLARED VARIABLES
//---------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera that renders the world in a window display
cCamera* camera;

// a light source to illuminate the objects in the virtual scene
cLight *light;

// a little "chai3d" bitmap logo at the bottom of the screen
cBitmap* logo;

// width and height of the current window display
int displayW = 0;
int displayH = 0;

// a haptic device handler
cHapticDeviceHandler* handler;

// a virtual tool representing the haptic device in the scene
cGeneric3dofPointer* tool;

// radius of the tool proxy
double proxyRadius;

//collision plane with sphere
cMesh* plane;

// 3 spheres and
cShapeSphere * s[3];

cVector3d s_pos[3];
cVector3d v[3];
cVector3d g(0,0,-9.8);

//3 springs
cShapeLine *l[3];

//default parameters
double para[4];
double m = 10;
double restLength = 0.5;
double SPRING_C = 100;
double DAMPING_C_z = 0.9;
double DAMPING_G = 0.6;
int i;

// status of the main simulation haptics loop
bool simulationRunning = false;

// simulation clock
cPrecisionClock simClock;

// root resource path
string resourceRoot;

// has exited haptics simulation thread
bool simulationFinished = false;

// set a random Inital Position
bool randomInitPos = false;

//---------------------------------------------------------------------------
// DECLARED MACROS
//---------------------------------------------------------------------------
// convert to resource path
#define RESOURCE_PATH(p)    (char*)((resourceRoot+string(p)).c_str())


//---------------------------------------------------------------------------
// DECLARED FUNCTIONS
//---------------------------------------------------------------------------

// callback when the window display is resized
void resizeWindow(int w, int h);

// callback when a keyboard key is pressed
void keySelect(unsigned char key, int x, int y);

// callback when the right mouse button is pressed to select a menu item
void menuSelect(int value);

// function called before exiting the application
void close(void);

// main graphics callback
void updateGraphics(void);

// main haptics loop
void updateHaptics(void);

//detect collision and update velocity
void updateVafterCollision(int);

//constrains of parameters
void pararestrict(void);
//===========================================================================
/*
 DEMO:    polygons.cpp
 
 This example illustrates how to build an object composed of triangle
 with individual colors. A finger-proxy algorithm is used to compute
 the interaction force between the tool and the object.
 */
//===========================================================================

int main(int argc, char* argv[])
{
    //-----------------------------------------------------------------------
    // INITIALIZATION
    //-----------------------------------------------------------------------
    
    printf("\n");
    printf("-----------------------------------\n");
    printf("CHAI 3D\n");
    printf("Demo: 12-polygons\n");
    printf("Copyright 2003-2009\n");
    printf("-----------------------------------\n");
    printf("\n\n");
    printf("Keyboard Options:\n\n");
    printf("[1] - restart\n");
    printf("[2] - select start mode\n");
    printf("[9] - increase parameters\n");
    printf("[0] - decrease parameters\n");
    printf("[x] - Exit application\n");
    printf("\n\n");
    
    // parse first arg to try and locate resources
    resourceRoot = string(argv[0]).substr(0, string(argv[0]).find_last_of("/\\") + 1);
    
    
    //-----------------------------------------------------------------------
    // 3D - SCENEGRAPH
    //-----------------------------------------------------------------------
    
    // create a new world.
    world = new cWorld();
    
    // set the background color of the environment
    // the color is defined by its (R,G,B) components.
    world->setBackgroundColor(0.15, 0.15, 0.15);
    
    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);
    
    // position and oriente the camera
    camera->set(cVector3d(3.0, 0.0, 0.0),    // camera position (eye)
                cVector3d(0.0, 0.0, 0.0),    // lookat position (target)
                cVector3d(0.0, 0.0, 1.0));   // direction of the "up" vector
    
    // set the near and far clipping planes of the camera
    // anything in front/behind these clipping planes will not be rendered
    camera->setClippingPlanes(0.01, 10.0);
    
    // create a light source and attach it to the camera
    light = new cLight(world);
    camera->addChild(light);                   // attach light to camera
    light->setEnabled(true);                   // enable light source
    light->setPos(cVector3d(2.0, 0.5, 1.0));  // position the light source
    light->setDir(cVector3d(-2.0, 0.5, 1.0));  // define the direction of the light beam
    
    
    //-----------------------------------------------------------------------
    // 2D - WIDGETS
    //-----------------------------------------------------------------------
    
    // create a 2D bitmap logo
    logo = new cBitmap();
    
    // add logo to the front plane
    camera->m_front_2Dscene.addChild(logo);
    
    // load a "chai3d" bitmap image file
    bool fileload;
    fileload = logo->m_image.loadFromFile(RESOURCE_PATH("resources/images/chai3d.bmp"));
    if (!fileload)
    {
#if defined(_MSVC)
        fileload = logo->m_image.loadFromFile("../../../bin/resources/images/chai3d.bmp");
#endif
    }
    
    // position the logo at the bottom left of the screen (pixel coordinates)
    logo->setPos(10, 10, 0);
    
    // scale the logo along its horizontal and vertical axis
    logo->setZoomHV(0.4, 0.4);
    
    // here we replace all wite pixels (1,1,1) of the logo bitmap
    // with transparent black pixels (1, 1, 1, 0). This allows us to make
    // the background of the logo look transparent.
    logo->m_image.replace(
                          cColorb(0x00, 0x00, 0x00),      // original RGB color
                          cColorb(0x00, 0x00, 0x00, 0x00) // new RGBA color
                          );
    
    // enable transparency
    logo->enableTransparency(true);
    
    
    //-----------------------------------------------------------------------
    // HAPTIC DEVICES / TOOLS
    //-----------------------------------------------------------------------
    
    // create a haptic device handler
    handler = new cHapticDeviceHandler();
    
    // get access to the first available haptic device
    cGenericHapticDevice* hapticDevice;
    handler->getDevice(hapticDevice, 0);
    
    // retrieve information about the current haptic device
    cHapticDeviceInfo info;
    if (hapticDevice)
    {
        info = hapticDevice->getSpecifications();
    }
    
    // create a 3D tool and add it to the world
    tool = new cGeneric3dofPointer(world);
    world->addChild(tool);
    
    // connect the haptic device to the tool
    tool->setHapticDevice(hapticDevice);
    
    // initialize tool by connecting to haptic device
    tool->start();
    
    // map the physical workspace of the haptic device to a larger virtual workspace.
    tool->setWorkspaceRadius(1.2);
    
    // define a radius for the tool (graphical display)
    tool->setRadius(0.05);
    
    // hide the device sphere. only show proxy.
    tool->m_deviceSphere->setShowEnabled(false);
    
    // set the physical readius of the proxy.
    proxyRadius = 0.1;
    tool->m_proxyPointForceModel->setProxyRadius(proxyRadius);
    
    // enable if objects in the scene are going to rotate of translate
    // or possibly collide against the tool. If the environment
    // is entirely static, you can set this parameter to "false"
    tool->m_proxyPointForceModel->m_useDynamicProxy = true;
    
    // read the scale factor between the physical workspace of the haptic
    // device and the virtual workspace defined for the tool
    double workspaceScaleFactor = tool->getWorkspaceScaleFactor();
    
    // define a maximum stiffness that can be handled by the current
    // haptic device. The value is scaled to take into account the
    // workspace scale factor
    double stiffnessMax = info.m_maxForceStiffness / workspaceScaleFactor;
    
    
    //-----------------------------------------------------------------------
    // COMPOSE THE VIRTUAL SCENE
    //-----------------------------------------------------------------------
    
    // create a plane as mesh
    plane = new cMesh(world);
    world->addChild(plane);
    
    // create two Triangles in plane
    plane->newTriangle(cVector3d(1,-1,-0.5), cVector3d(1, 1, -0.5), cVector3d(-1, -1, -0.5));
    plane->newTriangle(cVector3d(1, 1, -0.5), cVector3d(-1, 1, -0.5), cVector3d(-1, -1, -0.5));
    
    // setup collision detector
    plane->createAABBCollisionDetector(0.05, true, false);
    
    s[0] = new cShapeSphere(0.05);
    world->addChild(s[0]);
    s[1] = new cShapeSphere(0.05);
    world->addChild(s[1]);
    s[2] = new cShapeSphere(0.05);
    world->addChild(s[2]);
    
    if (randomInitPos) {
        for (int i = 0;i < 3;i++) {
            s[i]->setPos((140.0 * rand() / RAND_MAX) / 100.0 - 0.7,
                         (140.0 * rand() / RAND_MAX) / 100.0 - 0.7, 0.5);
            s_pos[i] = s[i]->getPos();
        }
    }
    else {
        s[0]->setPos(-0.5, 0, 0.5);
        s[1]->setPos(0, 0.4, 0.5);
        s[2]->setPos(0, -0.3, 0.5);
        
        s_pos[0] = s[0]->getPos();
        s_pos[1] = s[1]->getPos();
        s_pos[2] = s[2]->getPos();
    }
    
    l[0] = new cShapeLine(s_pos[0], s_pos[1]);
    world->addChild(l[0]);
    l[1] = new cShapeLine(s_pos[0], s_pos[2]);
    world->addChild(l[1]);
    l[2] = new cShapeLine(s_pos[1], s_pos[2]);
    world->addChild(l[2]);
    
    std::cout << "s_pos[0]: " << s_pos[0] << std::endl;
    std::cout << "s_pos[1]: " << s_pos[1] << std::endl;
    std::cout << "s_pos[2]: " << s_pos[2] << std::endl;
    
    para[0] = m;
    para[1] = restLength;
    para[2] = SPRING_C;
    para[3] = DAMPING_C_z;
    para[4] = DAMPING_G;
    
    //-----------------------------------------------------------------------
    // OPEN GL - WINDOW DISPLAY
    //-----------------------------------------------------------------------
    
    // initialize GLUT
    glutInit(&argc, argv);
    
    // retrieve the resolution of the computer display and estimate the position
    // of the GLUT window so that it is located at the center of the screen
    int screenW = glutGet(GLUT_SCREEN_WIDTH);
    int screenH = glutGet(GLUT_SCREEN_HEIGHT);
    int windowPosX = (screenW - WINDOW_SIZE_W) / 2;
    int windowPosY = (screenH - WINDOW_SIZE_H) / 2;
    
    // initialize the OpenGL GLUT window
    glutInitWindowPosition(windowPosX, windowPosY);
    glutInitWindowSize(WINDOW_SIZE_W, WINDOW_SIZE_H);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(updateGraphics);
    glutKeyboardFunc(keySelect);
    glutReshapeFunc(resizeWindow);
    glutSetWindowTitle("CHAI 3D");
    
    // create a mouse menu (right button)
    glutCreateMenu(menuSelect);
    glutAddMenuEntry("full screen", OPTION_FULLSCREEN);
    glutAddMenuEntry("window display", OPTION_WINDOWDISPLAY);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    
    //-----------------------------------------------------------------------
    // START SIMULATION
    //-----------------------------------------------------------------------
    
    // simulation in now running
    simulationRunning = true;
    
    // create a thread which starts the main haptics rendering loop
    cThread* hapticsThread = new cThread();
    hapticsThread->set(updateHaptics, CHAI_THREAD_PRIORITY_HAPTICS);
    
    // start the main graphics rendering loop
    glutMainLoop();
    
    // close everything
    close();
    
    // exit
    return (0);
}

//---------------------------------------------------------------------------

void resizeWindow(int w, int h)
{
    // update the size of the viewport
    displayW = w;
    displayH = h;
    glViewport(0, 0, displayW, displayH);
}

//---------------------------------------------------------------------------

void keySelect(unsigned char key, int x, int y)
{
    // escape key
    if ((key == 27) || (key == 'x'))
    {
        // close everything
        close();
        
        // exit application
        exit(0);
    }
    
    if (key == '2')
    {
        randomInitPos = !randomInitPos;
        if(randomInitPos){
            std::cout << "random start mode on " << std::endl;
        }
        else {
            std::cout << "random start mode off " << std::endl;
        }
        
    }
    
    if (key == '1')
    {
        close();
        for (int i = 0;i < 3;i++){
            v[i].zero();
            s_pos[i].zero();
        }
        
        if (randomInitPos) {
            for (int i = 0;i < 3;i++) {
                s[i]->setPos((140.0 * rand() / RAND_MAX) / 100.0 - 0.7,
                             (140.0 * rand() / RAND_MAX) / 100.0 - 0.7, 0.5);
                s_pos[i] = s[i]->getPos();
            }
        }
        else {
            s[0]->setPos(-0.5, 0, 0.5);
            s[1]->setPos(0, 0.4, 0.5);
            s[2]->setPos(0, -0.3, 0.5);
            
            s_pos[0] = s[0]->getPos();
            s_pos[1] = s[1]->getPos();
            s_pos[2] = s[2]->getPos();
        }
        std::cout << "pos[0]: " << s_pos[0] << std::endl;
        std::cout << "pos[1]: " << s_pos[1] << std::endl;
        std::cout << "pos[2]: " << s_pos[2] << std::endl;
        
        simulationRunning = true;
        
        // create a thread which starts the main haptics rendering loop
        cThread* hapticsThread = new cThread();
        hapticsThread->set(updateHaptics, CHAI_THREAD_PRIORITY_HAPTICS);
        
        // start the main graphics rendering loop
        glutMainLoop();
        
        // close everything
        close();
        
    }
    if (key == '9')
    {
        switch (i + 1)
        {
            case 1:
                std::cout << "m: " << m << std::endl;
                para[i] = para[i] + 20;
                m = para[i];
                break;
            case 2:
                std::cout << "restLength: " << restLength << std::endl;
                para[i] = para[i] + 0.1;
                restLength = para[i];
                break;
            case 3:
                std::cout << "SPRING_C: " << SPRING_C << std::endl;
                para[i] = para[i] + 50;
                SPRING_C = para[i];
                break;
            case 4:
                std::cout << "DAMPING_C_z: " << DAMPING_C_z << std::endl;
                para[i] = para[i] + 0.05;
                DAMPING_C_z = para[i];
                break;
            case 5:
                std::cout << "DAMPING_C: " << DAMPING_G << std::endl;
                para[i] = para[i] + 0.05;
                DAMPING_G = para[i];
                break;
                
                
        }
        
        
    }
    if (key == '0')
    {
        switch(i+1)
        {
            case 1:
                std::cout << "m: " << m << std::endl;
                para[i] = para[i] -20;
                m = para[i];
                break;
            case 2:
                std::cout << "restLength: " << restLength << std::endl;
                para[i] = para[i] - 0.1;
                restLength = para[i];
                break;
            case 3:
                std::cout << "SPRING_C: " << SPRING_C << std::endl;
                para[i] = para[i] - 50;
                SPRING_C = para[i];
                break;
            case 4:
                std::cout << "DAMPING_C_z: " << DAMPING_C_z << std::endl;
                para[i] = para[i] - 0.05;
                DAMPING_C_z = para[i];
                break;
            case 5:
                std::cout << "DAMPING_C: " << DAMPING_G << std::endl;
                para[i] = para[i] - 0.05;
                DAMPING_G = para[i];
                break;
                
                
        }
        
        
    }
    
    if (key == ' ')
    {
        i = (++i ) % 5;
        std::cout << "i: " << i << std::endl;
        switch (i+1)
        {
                
            case 1:
                std::cout << "m: " << m << std::endl;
                break;
            case 2:
                std::cout << "restLength: " << restLength << std::endl;
                break;
            case 3:
                std::cout << "SPRING_C: " << SPRING_C << std::endl;
                break;
            case 4:
                std::cout << "DAMPING_C_z: " << DAMPING_C_z << std::endl;
                break;
            case 5:
                std::cout << "DAMPING_C: " << DAMPING_G << std::endl;
                break;
        }
        
    }
}

//---------------------------------------------------------------------------

void menuSelect(int value)
{
    switch (value)
    {
            // enable full screen display
        case OPTION_FULLSCREEN:
            glutFullScreen();
            break;
            
            // reshape window to original size
        case OPTION_WINDOWDISPLAY:
            glutReshapeWindow(WINDOW_SIZE_W, WINDOW_SIZE_H);
            break;
    }
}

//---------------------------------------------------------------------------

void close(void)
{
    // stop the simulation
    simulationRunning = false;
    
    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }
    
    // close haptic device
    tool->stop();
}

//---------------------------------------------------------------------------

void updateGraphics(void)
{
    // render world
    camera->renderView(displayW, displayH);
    
    // Swap buffers
    glutSwapBuffers();
    
    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));
    
    // inform the GLUT window to call updateGraphics again (next frame)
    if (simulationRunning)
    {
        glutPostRedisplay();
    }
}

//---------------------------------------------------------------------------

void updateHaptics(void)
{
    //pararestrict();
    // reset clock
    simClock.reset();
    
    // main haptic simulation loop
    while (simulationRunning)
    {
        // stop the simulation clock
        simClock.stop();
        
        // read the time increment in seconds
        double timeInterval = simClock.getCurrentTimeSeconds();
        
        // restart the simulation clock
        simClock.reset();
        simClock.start();
        
        //calculate distance between 3 balls
        cVector3d dis0 = s_pos[1] - s_pos[0];
        cVector3d dis1 = s_pos[2] - s_pos[0];
        cVector3d dis2 = s_pos[2] - s_pos[1];
        
        //compare the distance with restlength
        double sub0 = dis0.length() - restLength;
        double sub1 = dis1.length() - restLength;
        double sub2 = dis2.length() - restLength;
        
        //normalize the distance vecter to get the direction
        dis0.normalize();
        dis1.normalize();
        dis2.normalize();
        
        //update velocity of s0-------------------------------------------------------
        
        v[0].add(timeInterval*(g + dis0*SPRING_C*sub0 + dis1*SPRING_C*sub1)/m);
        s_pos[0].z = s_pos[0].z + v[0].z*timeInterval;
        s_pos[0].y = s_pos[0].y + v[0].y*timeInterval;
        s_pos[0].x = s_pos[0].x + v[0].x*timeInterval;
        
        s[0]->setPos(s_pos[0]);
        
        updateVafterCollision(0);
        
        // update velocity of s1--------------------------------------------------------
        
        
        v[1].add(timeInterval*(g - dis0*SPRING_C*sub0 + dis2*SPRING_C*sub2)/m);
        s_pos[1].z = s_pos[1].z + v[1].z*timeInterval;
        s_pos[1].y = s_pos[1].y + v[1].y*timeInterval;
        s_pos[1].x = s_pos[1].x + v[1].x*timeInterval;
        
        s[1]->setPos(s_pos[1]);
        
        updateVafterCollision(1);
        
        //update velocity of s2----------------------------------------------------
        
        
        v[2].add(timeInterval*(g - dis1*SPRING_C*sub1 - dis2*SPRING_C*sub2)/m);
        s_pos[2].z = s_pos[2].z + v[2].z*timeInterval;
        s_pos[2].y = s_pos[2].y + v[2].y*timeInterval;
        s_pos[2].x = s_pos[2].x + v[2].x*timeInterval;
        
        s[2]->setPos(s_pos[2]);
        
        updateVafterCollision(2);
        
        //update spring position
        l[0]->m_pointA = s_pos[0];
        l[0]->m_pointB = s_pos[1];
        l[1]->m_pointA = s_pos[0];
        l[1]->m_pointB = s_pos[2];
        l[2]->m_pointA = s_pos[1];
        l[2]->m_pointB = s_pos[2];
        
        //apply damping
        v[0].mul(1.0 - DAMPING_G * timeInterval);
        v[1].mul(1.0 - DAMPING_G * timeInterval);
        v[2].mul(1.0 - DAMPING_G * timeInterval);
        
        
    }
    
    // exit haptics thread
    simulationFinished = true;
}

//---------------------------------------------------------------------------

void updateVafterCollision(int i)
{
    if(s[i]->getPos().z > -0.475)
    {
        cCollisionRecorder *recorder = new cCollisionRecorder();
        cCollisionSettings *setting = new cCollisionSettings();
        bool collision = plane->getCollisionDetector()->
        computeCollision(cVector3d(0, 0, 0), cVector3d(s[i]->getPos().x,
                                                       s[i]->getPos().y, s[i]->getPos().z - 0.05), *recorder, *setting);
        if (collision)
        {
            // change the z axis velocity to positive 
            double v_z = v[i].z * (-DAMPING_C_z);
            v[i] = cVector3d(v[i].x, v[i].y, v_z);
        }
    }
    else {
        s[i]->setPos(s[i]->getPos().x,
                     s[i]->getPos().y, -0.475);
        
    }
    
    
}

//---------------------------------------------------------------------------

/*
 void pararestrict(void) 
 {
	//for m
	if (para[0] < 0) 
	{
 para[0] = 0;
	};
	//for restLength
	if (para[1] < 0.1)
	{
 para[1] = 0.1;
	};
	//for SPRING_C
	if (para[2] < 0)
	{
 para[2] = 0;
	};
	// for DAMPING_C_z
	if (para[3] < 0)
	{
 para[3] = 0;
	};
	// for DAMPING_C_z
	if (para[3] > 1)
	{
 para[3] = 1;
	};
	// for DAMPING_C
	if (para[4] < 0)
	{
 para[4] = 0;
	};
	// for DAMPING_C
	if (para[4] > 1)
	{
 para[4] = 1;
	};
	
 }*/