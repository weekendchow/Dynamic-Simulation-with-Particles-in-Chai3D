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

// four mesh objects loaded from file 
cMesh* object[4];

// a light source to illuminate the objects in the virtual scene
cLight *light;

// a little "chai3d" bitmap logo at the bottom of the screen
cBitmap* logo;


// width and height of the current window display
int displayW = 0;
int displayH = 0;

// update camera settings
void updateCameraPosition();

// update object settings
void updateObjectPosition();

// camera position and orientation is spherical coordinates
double cameraAngleH;
double cameraAngleV;
double cameraDistance;
cVector3d cameraPosition;
cVector3d cameraUp;
cVector3d cameraRight;
cVector3d cameraLook;

// camera status
bool flagCameraInMotion;
bool useCameraMode;
int activeObject;

// mouse position and button status
int mouseX, mouseY;
int mouseButton;
int dx;
int dy;

// root resource path
string resourceRoot;

// +++++++object status+++++++++++++++
bool flagObjectInMotion;
int mod;

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



//===========================================================================
/*
Assignment 2:    trackball.cpp

This starter code contains mouse interactions for camera updates. Rotation is currently always around center of the world, zooming is supported but no translation is implemented.

The application first loads a 3D mesh file (in the 3ds format) including a texture.

There is no haptics loop or physics in this example.
*/
//===========================================================================

int main(int argc, char* argv[])
{
	//-----------------------------------------------------------------------
	// INITIALIZATION
	//-----------------------------------------------------------------------

	printf("\n");
	printf("-----------------------------------\n");
	printf("ELG5124/CSI5151 Assignment 2\n");
	printf("Based on Chai3D Demo: 20-map and 21-object\n");
	printf("Copyright 2015\n");
	printf("-----------------------------------\n");
	printf("\n\n");
	printf("Instructions:\n\n");
	printf("- Use left mouse button to rotate camera view. \n");
	printf("- Use right mouse button to control camera zoom. \n");
	printf("\n\n");
	printf("Keyboard Options:\n\n");
	printf("[1] - Texture   (ON/OFF)\n");
	printf("[2] - Wireframe (ON/OFF)\n");
	printf("[c] - Toggle camera/object trackball\n");
	printf("[ ] - Step through objects\n");
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
	string filenames[] = { "camera/camera.3ds", "ducky/duck-full.obj", "gear/gear.3ds", "bunny/bunny.obj" };
	string baseFilename = "resources/models/";


	// Set up and insert the four objects
	for (int i = 0; i<4; ++i) {
		object[i] = new cMesh(world);
		// load an object file
		bool meshload;
		string fn = baseFilename + filenames[i];
		meshload = object[i]->loadFromFile(RESOURCE_PATH(fn.c_str()));
		if (!meshload) {
#if defined(_MSVC)
			string bn = "../../../bin/resources/models/";
			fn = bn + filenames[i];
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
		// resize object to screen
		if (size > 0) {
			object[i]->scale(0.25 / size); // Make opbject fit in a quarter box
		}
		world->addChild(object[i]);
		// set the positions of the objects at the four corners of the world
		object[i]->setPos(-0.5 + (i % 2), -0.5 + (i / 2), 0.0);
		// +++++++Set the tag for each object+++++++++
		object[i]->setTag(i);
	}

	//-----------------------------------------------------------------------
	// PLACE THE CAMERA AND A LIGHTSOURCE
	//-----------------------------------------------------------------------
	// create a light source and attach it to the camera
	light = new cLight(world);
	camera->addChild(light);                   // attach light to camera
	light->setEnabled(true);                   // enable light source
	light->setPos(cVector3d(0.0, 0.3, 0.3));  // position the light source
	light->setDir(cVector3d(-1.0, -0.1, -0.1));  // define the direction of the light beam
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
		for (int i = 0; i<4; ++i) {
			bool useTexture = object[i]->getUseTexture();
			object[i]->setUseTexture(!useTexture);
		}
	}

	// option 2:
	if (key == '2')
	{
		for (int i = 0; i<4; ++i) {
			bool useWireMode = object[i]->getWireMode();
			object[i]->setWireMode(!useWireMode);
		}
	}

	if (key == 'c')
	{
		useCameraMode = !useCameraMode;
		if(useCameraMode)
		{
			std::cout << "CameraMode" << std::endl;
		}
		else 
		{
			std::cout << "ObjectMode" << std::endl;
		}
		
	}

	if (key == ' ')
	{
		activeObject = (++activeObject) % 4;
	}

}

//---------------------------------------------------------------------------

void mouseClick(int button, int state, int x, int y)
{
	// camera version++++++++++
	// mouse button down
	if (state == GLUT_DOWN)
	{
		flagCameraInMotion = true;
		mouseX = x;
		mouseY = y;
		mouseButton = button;

	}

	// mouse button up
	else if (state == GLUT_UP)
	{
		flagCameraInMotion = false;
	}


	// object version++++++++++
	// mouse button down
	if (state == GLUT_DOWN)
	{
		flagObjectInMotion = true;
		mouseX = x;
		mouseY = y;
		mouseButton = button;
		mod = glutGetModifiers();

	}

	// mouse button up
	else if (state == GLUT_UP)
	{
		flagObjectInMotion = false;

	// ++++++++SELECTION++++++++++++++
		if (mouseButton == GLUT_LEFT_BUTTON && mod == GLUT_ACTIVE_SHIFT)
		{
			if (!useCameraMode) 
			{

				cCollisionRecorder *CR = new cCollisionRecorder();
				cCollisionSettings *CS = new cCollisionSettings();
				bool sl = camera->select(x, y, WINDOW_SIZE_W, WINDOW_SIZE_H, *CR, *CS);
				
				if (sl) 
				{
					int k = (*CR).m_nearestCollision.m_object->getSuperParent()->m_tag;
					for (int i = 0; i<4; i++) 
					{
						if (k == object[i]->m_tag) 
						{
							activeObject = i;
						}
					}
					std::cout << "object " << k << " selected" << std::endl;
				}

				else 
				{
					std::cout << "no object selected" << std::endl;
				}
			}
		}
	}
}

//---------------------------------------------------------------------------

void mouseMove(int x, int y)
{
	dx = x - mouseX;
	dy = y - mouseY;

	//++++++++camera move++++++++++++
	if (flagCameraInMotion && useCameraMode)
	{
		if (mouseButton == GLUT_RIGHT_BUTTON)
		{
			cameraDistance = cameraDistance - 0.01 * dy;
		}

		else if (mouseButton == GLUT_LEFT_BUTTON)
		{
			cameraAngleH = cameraAngleH - dx;
			cameraAngleV = cameraAngleV + dy;
		}
		updateCameraPosition();
		mouseX = x;
		mouseY = y;

	}

	//++++++++object move++++++++++++++
	else if (flagObjectInMotion && !useCameraMode) 
	{
		cameraUp = camera->getUpVector();
		cameraRight = camera->getRightVector();
		cameraLook = camera->getLookVector();

		//+++++++++++TRANSLATION+++++++++++++
		if (mouseButton == GLUT_RIGHT_BUTTON && mod == GLUT_ACTIVE_SHIFT)
		{
			cVector3d newOpos = cAdd(
				object[activeObject]->getPos(),
				-0.0004*dy*cameraLook
				);
			object[activeObject]->setPos(newOpos);
		}
		else if (mouseButton == GLUT_RIGHT_BUTTON) 
		{
			cVector3d newOpos = cAdd(
				object[activeObject]->getPos(),
				cAdd(-0.0004*dy*cameraUp, 0.0004*dx*cameraRight)
				);
			object[activeObject]->setPos(newOpos);
		}

		//+++++++++ROTATION++++++++++++
		else if (mouseButton == GLUT_LEFT_BUTTON)
		{
			object[activeObject]->rotate(cameraUp, 0.003*dx);
			object[activeObject]->rotate(cameraRight, 0.003*dy);
		} 
	}
}

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

void updateGraphics(void)
{
	if (useCameraMode) {
		updateCameraPosition();
	}

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
			cameraDistance	* cCosDeg(cameraAngleH) * cCosDeg(cameraAngleV),
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