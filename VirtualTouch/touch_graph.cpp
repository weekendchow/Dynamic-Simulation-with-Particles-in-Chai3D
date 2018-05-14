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
#include "virtual_touch.h"




//---------------------------------------------------------------------------
// DECLARED CONSTANTS
//---------------------------------------------------------------------------

// initial size (width/height) in pixels of the display window
const int WINDOW_SIZE_W         = 600;
const int WINDOW_SIZE_H         = 600;

// mouse menu options (right button)
const int OPTION_FULLSCREEN     = 1;
const int OPTION_WINDOWDISPLAY  = 2;

const float PI = 3.1415926f;


//---------------------------------------------------------------------------
// DECLARED VARIABLES
//---------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera that renders the world in a window display
cCamera* camera;

// The end effector or interaction sphere
cShapeSphere* interactSphere;

// load number of meshes for your sololaution from file -
cMesh* object[4]; // Change for A3

// a light source to illuminate the objects in the virtual scene
cLight *light;

// a little "chai3d" bitmap logo at the bottom of the screen
cBitmap* logo;


// width and height of the current window display
int displayW  = 0;
int displayH  = 0;

// update camera settings
void updateCameraPosition();

// camera position and orientation is spherical coordinates
double cameraAngleH;
double cameraAngleV;
double cameraDistance;
cVector3d cameraPosition;

// camera status
bool flagCameraInMotion;

bool useCameraMode;
int activeJoint;

// mouse position and button status
int mouseX, mouseY;
int mouseButton;
int modifier;

// root resource path
string resourceRoot;

// Start communication with Chai3D
VirtualTouch vTouch;

double l1 = 0.129;
double l2 = 0.133;

// the angle of joints

int jointAngle[3];

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

// callback to handle mouse click
void mouseClick(int button, int state, int x, int y);

// callback to handle mouse motion
void mouseMove(int x, int y);

// main graphics callback
void updateGraphics(void);

// method to complete Forward Kinematics
void forwardKinematics();

// method to complete inverse Kinematics
void inverseKinematics();

//Joint angle limit
void jointAngleLimit();

//control up and down
void keyContr(int,int,int);

//===========================================================================
/*
	Assignment 3:    touch_graph.cpp
 
	This starter code contains mouse interactions for camera updates. Rotation is around
 center of the world. The Virtual Touch device is represented by a red sphere. Interactive
 translation of the red sphere is implemented and mapped to the left mouse button. User
 interaction for toggling between the joints: shoulder (2 DOF) and ellbow are implemented.
 
	The application currently only adds the sphere to the scenegraph. But loading a 3D mesh file
 (in obj file format) is still included but #if 0.
 
	There is no haptics loop or physics in this example.
 */
//===========================================================================

int main(int argc, char* argv[])
{
    //-----------------------------------------------------------------------
    // INITIALIZATION
    //-----------------------------------------------------------------------
    
    printf ("\n");
    printf ("-----------------------------------\n");
    printf ("ELG5124/CSI5151 Assignment 3\n");
    printf ("Based on Chai3D Demo: 20-map and 21-object\n");
    printf ("Copyright 2015\n");
    printf ("-----------------------------------\n");
    printf ("\n\n");
    printf ("Instructions:\n\n");
    printf ("- Use left mouse button to rotate camera view or move joints. \n");
    printf ("- Use right mouse button to control camera zoom or move interaction point. \n");
    printf ("\n\n");
    printf ("Keyboard Options:\n\n");
    printf ("[1] - Texture   (ON/OFF)\n");
    printf ("[2] - Wireframe (ON/OFF)\n");
    printf ("[c] - Toggle camera trackball and joints/intercation points\n");
    printf ("[ ] - Step through joints\n");
    printf ("[x] - Exit application\n");
    printf ("\n\n");
    
    
    // parse first arg to try and locate resources
    resourceRoot = string(argv[0]).substr(0,string(argv[0]).find_last_of("/\\")+1);
    
    
    //-----------------------------------------------------------------------
    // 3D - SCENEGRAPH
    //-----------------------------------------------------------------------
    
    // create a new world.
    world = new cWorld();
    
    // set the background color of the environment
    // the color is defined by its (R,G,B) components.
    world->setBackgroundColor(0.1, 0.1, 0.1);
    
    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);
    
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
    
    // here we replace all black pixels (0,0,0) of the logo bitmap
    // with transparent black pixels (0, 0, 0, 0). This allows us to make
    // the background of the logo look transparent.
    logo->m_image.replace(
                          cColorb(0, 0, 0),      // original RGB color
                          cColorb(0, 0, 0, 0)    // new RGBA color
                          );
    
    // enable transparency
    logo->enableTransparency(true);
    
    //-----------------------------------------------------------------------
    // COMPOSE THE VIRTUAL SCENE
    //-----------------------------------------------------------------------
    #if 1
        string filenames[] = { "touch/base.obj", "touch/shoulder.obj", "touch/upperArm.obj", "touch/lowerArm.obj"};
    string baseFilename = "resources/models/";
    
    
    // Set up and insert the four objects
    for (int i=0; i<4; ++i) {
        object[i] = new cMesh(world);
        // load an object file
        bool meshload;
        string fn = baseFilename+filenames[i];
        meshload = object[i]->loadFromFile(RESOURCE_PATH(fn.c_str()));
        if (!meshload) {
#if defined(_MSVC)
            string bn = "../../../bin/resources/models/";
            fn = bn+filenames[i];
            meshload = object[i]->loadFromFile(fn.c_str());
#endif
        }
        if (!meshload)
        {
            printf("Error - 3D Model %s failed to load correctly.\n", filenames[i].c_str());
            return (-1);
        }
        
        // compute a boundary box
        object[i]->computeBoundaryBox(true);
        // get dimensions of object
        double size = cSub(object[i]->getBoundaryMax(), object[i]->getBoundaryMin()).length();
        std::cout << "object:" << i << "size:" << size << std::endl;
        // resize object to screen
        if (size > 0) {  // Change for A3
            object[i]->scale(0.001); // Make object fit in a quarter box
        }
        world->addChild(object[i]); // Change for A3
        // set the positions of the objects at the four corners of the world
        
        //object[i]->setPos(-0.5+(i%2), -0.5+(i/2), 0.0); // Change for A3
    }
#endif
    
    for (int i = 0; i < 3; i++) {
        jointAngle[i] = 0;
    }
    
    
    // Add end-effector interaction sphere
    // create a sphere and define its radius
    interactSphere = new cShapeSphere(0.02);
    // add object to world
    world->addChild(interactSphere);
    
    
    //COLOR-------------------------
    object[1]->setVertexColor(cColorf(0.98f, 0.5f, 0.558f, 0.1));
    object[0]->setVertexColor(cColorf(0, 0.496f, 0.496f, 1));
    object[2]->setVertexColor(cColorf(0.702f, 0.933f, 0.22745f, 1));
    object[3]->setVertexColor(cColorf(0.98f, 0.54f, 0, 1));
    
    
    //POSITION SETTING--(ZERO CONFIGERATION)----------------------
    
    object[0]->setPos(cAdd(object[1]->getPos(), cVector3d(-0.0125, 0, -0.089)));
    
    cVector3d interactPos = object[3]->getPos();
    interactSphere->setPos(interactPos);
    
    
    // send position to Virtual Environment
    vTouch.setPosition(interactPos);
    
    interactSphere->m_material.m_ambient.set(0.1, 0.0, 0.0);
    interactSphere->m_material.m_diffuse.set(0.3, 0.0, 0.0);
    interactSphere->m_material.m_specular.set(0.6, 0.0, 0.0);
    interactSphere->m_material.setShininess(15);
    
    forwardKinematics();
    std::cout << "interactSpherePos:" << interactSphere->getPos() << std::endl;
    //-----------------------------------------------------------------------
    // PLACE THE CAMERA AND A LIGHTSOURCE
    //-----------------------------------------------------------------------
    // create a light source and attach it to the camera
    light = new cLight(world);
    camera->addChild(light);                   // attach light to camera
    light->setEnabled(true);                   // enable light source
    light->setPos(cVector3d( 0.0, 0.3, 0.3));  // position the light source
    light->setDir(cVector3d(-1.0,-0.1, -0.1));  // define the direction of the light beam
    light->m_ambient.set(0.5, 0.5, 0.5);
    light->m_diffuse.set(0.8, 0.8, 0.8);
    light->m_specular.set(1.0, 1.0, 1.0);
    
    // define a default position of the camera (described in spherical coordinates)
    cameraAngleH = 0;
    cameraAngleV = 45;
    cameraDistance = 1.8; // object is unit box put the camera at 1.8 * box
    updateCameraPosition();
    
    // set the near and far clipping planes of the camera
    // anything in front/behind these clipping planes will not be rendered
    camera->setClippingPlanes(0.01, 10.0);
    
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
    glutMouseFunc(mouseClick);
    glutMotionFunc(mouseMove);
    glutKeyboardFunc(keySelect);
    glutSpecialFunc(keyContr);
    glutReshapeFunc(resizeWindow);
    glutSetWindowTitle("CHAI 3D");
    
    //-----------------------------------------------------------------------
    // START RENDERING LOOP
    //-----------------------------------------------------------------------
    // start the main graphics rendering loop
    glutMainLoop();
    
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
        // exit application
        exit(0);
    }
    
    // option 1:
    if (key == '1')
    {
#if 0
        // Can reactivate for texture on/off rendering
        // Assumes your device scenegraph is below a node called virtual touch
        bool useTexture = virtualTouch->getUseTexture();
        virtualTouch->setUseTexture(!useTexture);
#endif
    }
    
    // option 2:
    if (key == '2')
    {
#if 0
        // Can reactivate for wire frame rendering
        bool useWireMode = virtualTouch->getWireMode();
        virtualTouch->setWireMode(!useWireMode);
#endif
    }
    
    if (key == 'c')
    {
        useCameraMode = !useCameraMode;
    }
    
    if (key == ' ')
    {
        activeJoint = (++activeJoint)%3;
    }
    
    if (key == 'd' )
    {
        // d for debugging
        std::cout << "Active joint: " << activeJoint << std::endl;
        cMatrix3d gRot = interactSphere->getParent()->getGlobalRot();
        std::cout << "InteractionSphere: "; gRot.print();
        cMatrix3d gCam = camera->getGlobalRot();
        std::cout << "Camera: "; gCam.print();
        // Take a vector from the camera frame into the object's parent via the world
        cMatrix3d prod = gRot.inv() * gCam;
        std::cout << "Object to camera: "; prod.print();
        std::cout << "Vertical axis: "; prod.getCol2().print();
        std::cout << "Horizontal axis: "; prod.getCol1().print();
    }
    
    if (key == 'f')
    {
        interactSphere->setShowFrame(!interactSphere->getShowFrame());
    }
}

//---------------------------------------------------------------------------
void selectObject( int x, int y ) {
    // This will hold a vector of collision as well as the nearest collision
    cCollisionRecorder rec;
    cCollisionSettings how;
    how.m_checkForNearestCollisionOnly = true;
    how.m_returnMinimalCollisionData = false;
    how.m_checkVisibleObjectsOnly = true;
    how.m_checkHapticObjectsOnly = false;
    how.m_checkBothSidesOfTriangles = false;
    how.m_adjustObjectMotion = false;
    how.m_collisionRadius = 0;
    bool objectHit = camera->select(x, y, displayW, displayH, rec, how );
    if (objectHit) {
        std::cout << "Intersection!" << std::endl;
        for (int i=0; i<4; ++i ) {
            if ( rec.m_nearestCollision.m_object->getSuperParent() == interactSphere ) {
                // pointer comparison uiih
                std::cout << "Interactionsphere intersected!" << std::endl;
                break;
            }
        }
        
    } else {
        std::cout << "No intersection!" << std::endl;
    }
}

void mouseClick(int button, int state, int x, int y)
{
    // mouse button down
    if (state == GLUT_DOWN)
    {
        flagCameraInMotion = true;
        mouseX = x;
        mouseY = y;
        mouseButton = button;
        int modifier = glutGetModifiers();
        std::cout << "Modifier: " << modifier << std::endl;
        if (modifier == GLUT_ACTIVE_SHIFT && mouseButton == GLUT_LEFT_BUTTON)
        {
            std::cout << "Selection" << std::endl;
            selectObject(x,y);
        }
    }
    
    // mouse button up
    else if (state == GLUT_UP)
    {
        flagCameraInMotion = false;
    }
}

//---------------------------------------------------------------------------
void cameraMotion(int x, int y) {
    if (mouseButton == GLUT_RIGHT_BUTTON)
    {
        cameraDistance = cameraDistance - 0.01 * (y - mouseY);
    }
    
    else if (mouseButton == GLUT_LEFT_BUTTON)
    {
        cameraAngleH = cameraAngleH - (x - mouseX);
        cameraAngleV = cameraAngleV + (y - mouseY);
    }
    
    updateCameraPosition();
}



void objectMotion( int x, int y) {
    if (mouseButton == GLUT_RIGHT_BUTTON)
    {
        double dx = 0.05* (x - mouseX)/cameraDistance;
        double dy = 0.05* (y - mouseY)/cameraDistance;
        
        // Calculate screen motion in object coordinate frame
        cMatrix3d gRot = interactSphere->getParent()->getGlobalRot();
        cMatrix3d gCam = camera->getGlobalRot();
        // Take a vector from the camera frame into the object's parent via the world
        cMatrix3d prod = gRot.inv() * gCam;
        // For mouse interactions we need translation on z (vertical) or y (horizontal)
        // Add a vertical rotation
        cVector3d translate = dx*prod.getCol1() - dy*prod.getCol2();
        interactSphere->translate( translate);
        
        inverseKinematics();
        jointAngleLimit();
        forwardKinematics();
        
        
        // send position across to Virtual environment
        cVector3d interactPos = interactSphere->getPos();
        vTouch.setPosition(interactPos);
    }
    
    else if (mouseButton == GLUT_LEFT_BUTTON)
    {
        double dx = 0.01 * (x - mouseX)/cameraDistance;
        double dy = 0.01 * (y - mouseY)/cameraDistance;
        // Calculate the joint motion based on dy and update
        
    }
    
};

void mouseMove(int x, int y)
{
    if (flagCameraInMotion)
    {
        if ( useCameraMode ) {
            cameraMotion(x,y);
        } else {
            if ( !(modifier == GLUT_ACTIVE_SHIFT && mouseButton == GLUT_LEFT_BUTTON)) {
                objectMotion(x,y);
            }
        }
    }
    
    mouseX = x;
    mouseY = y;
}

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

void updateGraphics(void)
{
    updateCameraPosition();
    
    // update object normals
    // object->computeAllNormals(true);
    
    // render world
    camera->renderView(displayW, displayH);
    
    // Swap buffers
    glutSwapBuffers();
    
    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));
    
    // inform the GLUT window to call updateGraphics again (next frame)
    glutPostRedisplay();
}

//---------------------------------------------------------------------------

void updateCameraPosition()
{
    // check values
    if (cameraDistance < 0.1) { cameraDistance = 0.1; }
    if (cameraAngleV > 89) { cameraAngleV = 89; }
    if (cameraAngleV < -89) { cameraAngleV = -89; }
    
    // compute position of camera in space
    cVector3d pos = cAdd(
                         cameraPosition,
                         cVector3d(
                                   cameraDistance * cCosDeg(cameraAngleH) * cCosDeg(cameraAngleV),
                                   cameraDistance * cSinDeg(cameraAngleH) * cCosDeg(cameraAngleV),
                                   cameraDistance * cSinDeg(cameraAngleV)
                                   )
                         );
    
    // compute lookat position
    cVector3d lookat = cameraPosition;
    
    // define role orientation of camera
    cVector3d up(0.0, 0.0, 1.0);
    
    // set new position to camera
    camera->set(pos, lookat, up);
    
    // recompute global positions
    world->computeGlobalPositions(true);
}

//---------------------------------------------------------------------------

void forwardKinematics()
{
    // Change of the angles
    cMatrix3d rotateZ;
    rotateZ.identity();
    rotateZ.rotate(cVector3d(0, 0, 1), cDegToRad(jointAngle[0]));
    cMatrix3d rotateX_upper;
    rotateX_upper.identity();
    rotateX_upper.rotate(cVector3d(1, 0, 0), cDegToRad(jointAngle[1]));
    cMatrix3d rotateX_lower;
    rotateX_lower.identity();
    rotateX_lower.rotate(cVector3d(1, 0, 0), cDegToRad(jointAngle[2]));
    
    // calculate position and rotation of the base
    object[1]->setRot(rotateZ);
    // rotation of the upperArm
    object[2]->setRot(rotateZ * rotateX_upper);
    // rotation of the lowerArm
    cMatrix3d rotateX_min180;
    rotateX_min180.identity();
    rotateX_min180.rotate(cVector3d(1, 0, 0), cDegToRad(-180));
    object[3]->setRot(object[2]->getRot() * rotateX_min180 * rotateX_lower);
    // position of the lower Arm
    object[3]->setPos(object[2]->getRot() * cVector3d(0, 0.129, 0));
    // new position of the interactSphere
    interactSphere->setPos(object[3]->getRot() * cVector3d(0, 0 , 0.133) + object[3]->getPos());
    cVector3d interactPos = interactSphere->getPos();
    vTouch.setPosition(interactPos);
    std::cout << "interactSpherePos:" << interactSphere->getPos() << std::endl;
    std::cout << "shoulderPos:" << object[1]->getPos() << std::endl;
    std::cout << "upperArmPos:" << object[2]->getPos() << std::endl;
    std::cout << "lowerArmPos:" << object[3]->getPos() << std::endl;
}

//---------------------------------------------------------------------------

void inverseKinematics()
{
    //std::cout << "interactSpherePos:" << interactSphere->getPos() << std::endl;
    // translate the coordinate
    cVector3d newPos = cVector3d(interactSphere->getPos().x,
                                 interactSphere->getPos().z, -interactSphere->getPos().y);
    //std::cout << "new_position:" << new_position << std::endl;
    
    // calculate the joint angles
    double x = newPos.x;
    double y = newPos.y;
    double z = newPos.z;
    jointAngle[0] = (atan2(x, z +l1)) * 180 / PI;
    double d = sqrt(pow(x, 2) + pow(z + l1, 2));
    double r = sqrt(pow(x, 2) + pow(y - l2, 2) + pow(z + l1, 2));
    double theta2 = acos((l1*l1 + r*r - l2*l2) / (2 * l1*r)) + atan2(y - l2, d);
    jointAngle[1] =  90-theta2 * 180 / PI;
    double theta3 = theta2 + acos((l1*l1 + l2*l2- r*r) / (2 * l1*l2)) - PI / 2;
    jointAngle[2] =  90- jointAngle[1]-theta3 * 180 / PI  ;
}

//---------------------------------------------------------------------------

void jointAngleLimit()
{
    if (jointAngle[0] < -45)
        jointAngle[0] = -45;
    if (jointAngle[0] > 45)
        jointAngle[0] = 45;
    if (jointAngle[1] < -10)
        jointAngle[1] = -10;
    if (jointAngle[1] > 90)
        jointAngle[1] = 90;
    if (jointAngle[2] < -10)
        jointAngle[2] = -10;
    if (jointAngle[2] > 50)
        jointAngle[2] = 50;
}

//---------------------------------------------------------------------------

void keyContr(int key, int x, int y)
{
    switch (key)
    {
        case 100:
            //left key
            break;
        case 101:
            //up key
            jointAngle[activeJoint] += 5;
            break;
        case 102:
            //right key
            break;
        case 103:
            // down key
            jointAngle[activeJoint] -= 5;
            break;
    }
    
    // the forward kinematic
    jointAngleLimit();
    forwardKinematics();
    
}
