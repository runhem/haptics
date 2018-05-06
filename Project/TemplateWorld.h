#ifndef TEMPLATEWORLD_H
#define TEMPLATEWORLD_H

#include "Assignment.h"
#include "chai3d.h"
#include <json/json.h>

class TemplateWorld : public Assignment
{
private:
    // A 3D cursor for the haptic device
    cShapeSphere* tool;

	Json::Value json;
	cMultiMesh *object;

	// A line to display velocity of the haptic interface
	cShapeLine *m_velocityVector;

	// Material properties used to render the color of the cursors
    cMaterialPtr m_matCursorButtonON;
    cMaterialPtr m_matCursorButtonOFF;

	// A label used to demonstrate debug output
    cLabel* m_debugLabel;

public:
	TemplateWorld(Json::Value modelJson) {
		json = modelJson;
	}
    virtual std::string getName() const { return json.get("name", "ASCII"); }

	virtual void initialize(cWorld* world, cCamera* camera);

	virtual void updateGraphics();
	virtual void updateHaptics(cGenericHapticDevice* hapticDevice, double timeStep, double totalTime);
};

void TemplateWorld::initialize(cWorld* world, cCamera* camera)
{
	//Change the background
	world->setBackgroundColor(0.0f, 1.0f, 0.0f);

	// Create a cursor with its radius set
	tool = new cToolCursor(world);
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
	std::string fileString = json.get("model", "ASCII").asString();
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
		close();
		return (-1);
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

}

void TemplateWorld::updateGraphics()
{
	std::stringstream ss;

    ss << "You can add debug output like this: " << tool->getLocalPos().length() * 1000.0
		<< " mm (Distance from center)";

    m_debugLabel->setText(ss.str());

	// Position the label
    m_debugLabel->setLocalPos(30, 150, 0);
}

void TemplateWorld::updateHaptics(cGenericHapticDevice* hapticDevice, double timeStep, double totalTime)
{
	//Read the current position of the haptic device
	cVector3d newPosition;
	hapticDevice->getPosition(newPosition);

    // update global variable for graphic display update
    hapticDevicePosition = newPosition;

	tool->setHapticDevice(hapticDevice);

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
	tool->start();
}

#endif

