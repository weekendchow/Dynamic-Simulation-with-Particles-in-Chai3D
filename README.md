# Virtual-Environments
This repository includes several projects in Virtual Environments class. These projects were simulated in Visual Studio environment by using C++, OpenGL and C++ simulation framework Chai 3D.

### Haptic Point Interactions
This project simply familiarize with Chai3D and to implement haptic point interactions. We implement a simple spring interaction and haptic force shading.

### Track Ball
This project is to create a ball and track it. The application first loads a 3D mesh file (in the 3ds format) including a texture. What we need to do is compose the virtual scene, place the camera and a light source, display the window and render.

### Virtual Touch
This project create a virtual haptics device `Virtual Touch` in Chai3D including its forward and inverse kinematics., a crude visual approximation of the Geomagic TouchTM, the former Sensable OmniTM, as the virtual device in Chai3D. The Touch is 3-DOF haptics device for point interaction. In real-life it also has a tool which is connected to the end-effector with three additional joints. The tool is often refered to as stylus and the three stylus joints are assembled in a gimbal configuration. These joints are however not actuated but only sensed. 

![VirtualTouch](https://github.com/weekendchow/Dynamic-Simulation-with-Particles-in-Chai3D/blob/master/images/VirtualTouch.png)

### Dynamic Simulation with Particles
This project is mainly aims at developing a particle system includes 3 particles connected with springs, falling from certain height and collide with a plane then bounce back. The plane was draw by cMesh(AABB collision detector). Set fixed or random initial position, then update the position and velocity using Forward Euler Integration. Also complete functions in keyselect(adjust parameters/switch mode/restart falling). 

![DynamicSimulationwithParticles](https://github.com/weekendchow/Dynamic-Simulation-with-Particles-in-Chai3D/blob/master/images/DynamicSimulationwithParticles.png)



## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details
