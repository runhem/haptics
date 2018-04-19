#ifndef HAPTICWALL_H
#define HAPTICWALL_H

#include "Assignment.h"

#include "chai3d.h"

class HapticWall : public Assignment
{
private:
    // A 3D cursor for the haptic device
    cShapeSphere* m_cursor;

    // Material properties used to render the color of the cursors
    cMaterialPtr m_matCursorButtonON;
    cMaterialPtr m_matCursorButtonOFF;

public:
    virtual std::string getName() const { return "4: Haptic Wall"; }

	virtual void initialize(cWorld* world, cCamera* camera);
	virtual void updateGraphics();
	virtual void updateHaptics(cGenericHapticDevice* hapticDevice, double timeStep, double totalTime);
};

void HapticWall::initialize(cWorld* world, cCamera* camera)
{
	//Change the background
	world->setBackgroundColor(0, 0, 0.3f);

	// Create a cursor with its radius set
	m_cursor = new cShapeSphere(0.01);
	// Add cursor to the world
	world->addChild(m_cursor);


    cShapeLine *myLine = new
            cShapeLine(cVector3d(0,0,1),cVector3d(0,0,-1));
    world->addChild(myLine);
	// Here we define the material properties of the cursor when the
	// user button of the device end-effector is engaged (ON) or released (OFF)
    // A light orange material color

    m_matCursorButtonOFF = cMaterialPtr(new cMaterial());
    m_matCursorButtonOFF->m_ambient.set(0.5, 0.2, 0.0);
    m_matCursorButtonOFF->m_diffuse.set(1.0, 0.5, 0.0);
    m_matCursorButtonOFF->m_specular.set(1.0, 1.0, 1.0);

    // A blue material color
    // A light orange material color
    m_matCursorButtonON = cMaterialPtr(new cMaterial());
    m_matCursorButtonON->m_ambient.set(0.1, 0.1, 0.4);
    m_matCursorButtonON->m_diffuse.set(0.3, 0.3, 0.8);
    m_matCursorButtonON->m_specular.set(1.0, 1.0, 1.0);

    // Apply the 'off' material to the cursor
    m_cursor->m_material = m_matCursorButtonOFF;
}

void HapticWall::updateGraphics()
{

}

void HapticWall::updateHaptics(cGenericHapticDevice* hapticDevice, double timeStep, double totalTime)
{
	//Read the current position of the haptic device
	cVector3d newPosition;
	hapticDevice->getPosition(newPosition);

    // update global variable for graphic display update
    hapticDevicePosition = newPosition;

	// Update position and orientation of cursor
    m_cursor->setLocalPos(newPosition);

	//Tip: Copy your material changing code from 2_ReadDevicePosition here, and line
        //     in to the initialization above. Do not create new objects right here.

	cVector3d force(0, 0, 0);
	//TODO: Add forces which simulates a wall.

    force = cVector3d(0,0,0);
    if(newPosition.y() > 0.0){
        cVector3d proxyPos;
        proxyPos.x(newPosition.x());
        proxyPos.y(0.0);
        proxyPos.z(newPosition.z());
        m_cursor->setLocalPos(proxyPos);

        //force.y(-3.0);
        int k = -800;
        double newForce = k*newPosition.y();
        force.y(newForce);
    }

	//Set a force to the haptic device
	hapticDevice->setForce(force);
}
#endif

