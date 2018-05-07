#ifndef TEMPLATEWORLD_H
#define TEMPLATEWORLD_H

#include "Assignment.h"
#include "chai3d.h"
#include <json/json.h>

using namespace chai3d;
using namespace std;



class TemplateWorld : public Assignment
{
private:
    // A 3D cursor for the haptic device
    cToolCursor* tool;

    Json::Value json;
    cMultiMesh *object;
    cCamera *camera;
    cWorld *world;
    bool simulationRunning = false;
    bool simulationFinished = true;

    // A line to display velocity of the haptic interface
    cShapeLine *m_velocityVector;

    // Material properties used to render the color of the cursors
    cMaterialPtr m_matCursorButtonON;
    cMaterialPtr m_matCursorButtonOFF;

    cFrequencyCounter freqCounterHaptics;

	// A label used to demonstrate debug output
    cLabel* m_debugLabel;

    // stifness properties
    double maxStiffness;

public:
	TemplateWorld(Json::Value modelJson) {
		json = modelJson;
	}
    virtual string getName() const { return json.get("name", "ASCII").asString(); }

	virtual void initialize(cWorld* world, cCamera* camera);

	virtual void updateGraphics();
        virtual void updateHaptics(cGenericHapticDevice* hapticDevice, double timeStep, double totalTime);
};

void TemplateWorld::initialize(cWorld* world, cCamera* camera)
{
    world = world;
    camera = camera;

    //Change the background
    world->setBackgroundColor(0.0f, 1.0f, 0.0f);

    // Create a cursor with its radius set
    tool = new cToolCursor(world);

    double toolRadius = 0.008;
    // Add cursor to the world
    world->addChild(tool);

    // Create a small line to illustrate velocity
    m_velocityVector = new cShapeLine(cVector3d(0, 0, 0), cVector3d(0, 0, 0));

    // Add line to the world
    world->addChild(m_velocityVector);


    // Create a font
    cFontPtr font2 = NEW_CFONTCALIBRI20();

	// Create a label used to show how debug output can be handled
    m_debugLabel = new cLabel(font2);

	// Labels need to be added to the camera instead of the world
    camera->m_frontLayer->addChild(m_debugLabel);

	/////////////////////////////////////////////////////////////////////
	// CREATE OBJECT
	/////////////////////////////////////////////////////////////////////

	cMultiMesh *object = new cMultiMesh();
	world->addChild(object);
	
	bool fileload;
        string fileString = json.get("model", "ASCII").asString();
	fileload = object->loadFromFile(fileString);
	if (!fileload)
	{
	#if defined(_MSVC)
		fileload = object->loadFromFile(fileString);
	#endif
	}
	if (!fileload)
	{
		cout << "Error - 3D Model failed to load correctly" << endl;
                return;
	}
	//ANCHOR

	object->setWireMode(json.get("wireMode", "ASCII").asInt(), true);

	cMaterial m;
	m.setBlueCadet();
	object->setMaterial(m);

	// disable culling so that faces are rendered on both sides
	object->setUseCulling(false);

	// compute a boundary box
	object->computeBoundaryBox(true);

	// show/hide boundary box
	object->setShowBoundaryBox(false);

	// compute collision detection algorithm
	object->createAABBCollisionDetector(toolRadius);

	// define a default stiffness for the object
        object->setStiffness(0.2 * maxStiffness, true);

	// define some haptic friction properties
	object->setFriction(json.get("staticFriction", "ASCII").asDouble(),
	 json.get("dynamicFriction", "ASCII").asDouble(), true);

	object->setShowEdges(json.get("wireMode", "ASCII").asInt());

	// enable display list for faster graphic rendering
	object->setUseDisplayList(true);

	cVector3d position = object->getBoundaryCenter();
	position.x(0);
	position.y(-0.10);
	position.z(-0.10);

	// center object in scene
	object->setLocalPos(position);

	// rotate object in scene
	//object->rotateExtrinsicEulerAnglesDeg(0, 0, 90, C_EULER_ORDER_XYZ);

	// compute all edges of object for which adjacent triangles have more than 40 degree angle
	object->computeAllEdges(0);

	// set line width of edges and color
	cColorf colorEdges;
	colorEdges.setBlack();
	object->setEdgeProperties(1, colorEdges);

	// set normal properties for display
	cColorf colorNormals;
	colorNormals.setOrangeTomato();
	object->setNormalsProperties(0.01, colorNormals);

	// display options
	object->setShowTriangles(json.get("showTriangles", "ASCII").asInt());
	object->setShowNormals(json.get("showNormals", "ASCII").asInt());

        cout << "Assignment initialized" << endl;

}

void TemplateWorld::updateGraphics()
{
    cout << "Template ug" << endl;

    stringstream ss;

    ss << "You can add debug output like this: " << tool->getLocalPos().length() * 1000.0
		<< " mm (Distance from center)";

    m_debugLabel->setText(ss.str());

	// Position the label
    m_debugLabel->setLocalPos(30, 150, 0);
    cout << "Template ug done" << endl;
}


enum cMode
{
    IDLE,
    SELECTION
};

void TemplateWorld::updateHaptics(cGenericHapticDevice* hapticDevice, double timeStep, double totalTime)
{
	//Read the current position of the haptic device
	cVector3d newPosition;
	hapticDevice->getPosition(newPosition);

        // update global variable for graphic display update
        hapticDevicePosition = newPosition;

        //tool->setHapticDevice(hapticDevice);


        double workspaceScaleFactor = tool->getWorkspaceScaleFactor();
        cHapticDeviceInfo info = hapticDevice->getSpecifications();
        maxStiffness = info.m_maxLinearStiffness / workspaceScaleFactor;


	// if the haptic device has a gripper, enable it as a user switch
	hapticDevice->setEnableGripperUserSwitch(true);

	// define the radius of the tool (sphere)
	double toolRadius = 0.008;

	// define a radius for the tool
	tool->setRadius(toolRadius);

	// hide the device sphere. only show proxy.
	tool->setShowContactPoints(true, false);

	// create a white cursor
	tool->m_hapticPoint->m_sphereProxy->m_material->setWhite();

	// map the physical workspace of the haptic device to a larger virtual workspace.
	tool->setWorkspaceRadius(0.1);

	// oriente tool with camera
	tool->setLocalRot(camera->getLocalRot());

	// haptic forces are enabled only if small forces are first sent to the device;
	// this mode avoids the force spike that occurs when the application starts when
	// the tool is located inside an object for instance.
	tool->setWaitForSmallForce(true);

	// start the haptic tool
        tool->start();        // Total time elapsed since the current assignment started


        cMode state = IDLE;
        cGenericObject *selectedObject = NULL;
        cTransform tool_T_object;

        // main haptic simulation loop
            /////////////////////////////////////////////////////////////////////////
            // HAPTIC RENDERING
            /////////////////////////////////////////////////////////////////////////

            // signal frequency counter
            freqCounterHaptics.signal(1);

            // compute global reference frames for each object
            world->computeGlobalPositions(true);

            // update position and orientation of tool
            tool->updateFromDevice();

            // compute interaction forces
            tool->computeInteractionForces();

            /////////////////////////////////////////////////////////////////////////
            // MANIPULATION
            /////////////////////////////////////////////////////////////////////////

            // compute transformation from world to tool (haptic device)
            cTransform world_T_tool = tool->getDeviceGlobalTransform();

            // get status of user switch
            bool button = tool->getUserSwitch(0);

            //
            // STATE 1:
            // Idle mode - user presses the user switch
            //
            if ((state == IDLE) && (button == true))
            {
                // check if at least one contact has occurred
                if (tool->m_hapticPoint->getNumCollisionEvents() > 0)
                {
                    // get contact event
                    cCollisionEvent *collisionEvent = tool->m_hapticPoint->getCollisionEvent(0);

                    // get object from contact event
                    selectedObject = collisionEvent->m_object;
                }
                else
                {
                    //selectedObject = currentObject;
                }

                // get transformation from object
                cTransform world_T_object = selectedObject->getGlobalTransform();

                // compute inverse transformation from contact point to object
                cTransform tool_T_world = world_T_tool;
                tool_T_world.invert();

                // store current transformation tool
                tool_T_object = tool_T_world * world_T_object;

                // update state
                state = SELECTION;
            }

            //
            // STATE 2:
            // Selection mode - operator maintains user switch enabled and moves object
            //
            else if ((state == SELECTION) && (button == true))
            {
                // compute new transformation of object in global coordinates
                cTransform world_T_object = world_T_tool * tool_T_object;

                // compute new transformation of object in local coordinates
                cTransform parent_T_world = selectedObject->getParent()->getLocalTransform();
                parent_T_world.invert();
                cTransform parent_T_object = parent_T_world * world_T_object;

                // assign new local transformation to object
                selectedObject->setLocalTransform(parent_T_object);

                // set zero forces when manipulating objects
                tool->setDeviceGlobalForce(0.0, 0.0, 0.0);

                tool->initialize();
            }

            //
            // STATE 3:
            // Finalize Selection mode - operator releases user switch.
            //
            else
            {
                state = IDLE;
            }

            /////////////////////////////////////////////////////////////////////////
            // FINALIZE
            /////////////////////////////////////////////////////////////////////////

            // send forces to haptic device
            tool->applyToDevice();

}

#endif

