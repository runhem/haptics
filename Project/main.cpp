//===========================================================================
/*

	CS277 - Experimental Haptics
	Winter 2010, Stanford University


	You may use this program as a boilerplate for starting your homework
	assignments.  Use CMake (www.cmake.org) on the CMakeLists.txt file to
	generate project files for the development tool of your choice.  The
	CHAI3D library directory (chai3d-2.1.0) should be installed as a sibling
	directory to the one containing this project.


	These files are meant to be helpful should you encounter difficulties
	setting up a working CHAI3D project.  However, you are not required to
	use them for your homework -- you may start from anywhere you'd like.


	\author    Francois Conti & Sonny Chan
	\date      January 2010
	*/
//===========================================================================
// This define is (maybe) needed for some gcc5 reason
// http://stackoverflow.com/questions/33394934/converting-std-cxx11string-to-stdstring
//#define _GLIBCXX_USE_CXX11_ABI 0

//---------------------------------------------------------------------------
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include <iostream>

//---------------------------------------------------------------------------
#include "chai3d.h"
#include <GLFW/glfw3.h>
#include <json/json.h>
#include <sys/types.h>
#include <dirent.h>
#include <fstream>
//--------------------

#include "Assignment.h"

#include "TemplateWorld.h"


using namespace chai3d;
using namespace std;
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// DECLARED CONSTANTS
//---------------------------------------------------------------------------
// Initial size (width/height) in pixels of the display window
const int WINDOW_SIZE_W = 600;
const int WINDOW_SIZE_H = 600;

/*

// Mouse menu options (right button)
const int OPTION_FULLSCREEN = 1;
const int OPTION_WINDOWDISPLAY = 2;


// Maximum number of haptic devices supported in this demo
const int MAX_DEVICES = 8;

*/

//------------------------------------------------------------------------------
// GENERAL SETTINGS
//------------------------------------------------------------------------------

// stereo Mode
/*
    C_STEREO_DISABLED:            Stereo is disabled
    C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
    C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
    C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;

// fullscreen mode
bool fullscreen = false;

// mirrored display
bool mirroredDisplay = false;


//---------------------------------------------------------------------------
// DECLARED VARIABLES
//---------------------------------------------------------------------------
// A list of all available assignments
std::vector<Assignment*> assignments;
volatile size_t currentAssignment = 0;


// A world that contains all objects of the virtual environment
cWorld* world = 0;


// A camera that renders the world in a window display
cCamera* camera;


// A light source to illuminate the objects in the virtual scene
cDirectionalLight *light;


// Width and height of the current window display
int displayW = WINDOW_SIZE_W;
int displayH = WINDOW_SIZE_H;


// A haptic device handler
cHapticDeviceHandler* handler;


// A pointer to the first haptic device detected on this computer
cGenericHapticDevicePtr hapticDevice = 0;


// Labels to show haptic device position, update rate and assignment text
cLabel* positionLabel = 0;
cLabel* rateLabel = 0;
cLabel* assignmentLabel = 0;
double assignmentLabelWidth;
double rateEstimate = 0;


// A clock measuring the total time
cPrecisionClock clockTotal;


// Status of the main simulation haptics loop
bool simulationRunning = false;


// Has exited haptics simulation thread
bool simulationFinished = false;

// haptic thread
cThread* hapticsThread;

// a handle to window display context
GLFWwindow* window = NULL;

// current width of window
int width  = 0;

// current height of window
int height = 0;

// swap interval for the display context (vertical synchronization)
int swapInterval = 1;

// a small sphere (cursor) representing the haptic device
cShapeSphere* cursor;

// a font for rendering text
cFontPtr font;

// a label to display the haptic device model
cLabel* labelHapticDeviceModel;

// a label to display the position [m] of the haptic device
cLabel* labelHapticDevicePosition;

// a global variable to store the position [m] of the haptic device
//cVector3d hapticDevicePosition;

// a label to display the rate [Hz] at which the simulation is running
cLabel* labelRates;

// a frequency counter to measure the simulation graphic rate
cFrequencyCounter freqCounterGraphics;

// a frequency counter to measure the simulation haptic rate
cFrequencyCounter freqCounterHaptics;
//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

// callback when the window display is resized
void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height);

// callback when an error GLFW occurs
void errorCallback(int error, const char* a_description);

// callback when a key is pressed
void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods);

// this function renders the scene
void updateGraphics(void);

// this function contains the main haptics simulation loop
void updateHaptics(void);

// this function closes the application
void close(void);

// Reset the scene with a new scene according to 'assignmentId'
void reset(size_t assignmentId);
//===========================================================================

/*
	This application illustrates the use of the haptic device handler
	"cHapticDevicehandler" to access haptic devices connected to the computer.


	In this example the application opens an OpenGL window and displays a
	3D cursor for the first device found. If the operator presses the device
	user button, the color of the cursor changes accordingly.

	In the main haptics loop function  "updateHaptics()" , the position and
	user switch status of the device are retrieved at each simulation iteration.
	This information is then used to update the position and color of the
	cursor. A force is then commanded to the haptic device to attract the
	end-effector towards the device origin.
*/
//===========================================================================



int main(int argc, char* argv[])
{

	//-----------------------------------------------------------------------
	// INITIALIZATION
	//-----------------------------------------------------------------------
	std::cout << std::endl;
	std::cout << "Based on:" << std::endl;
	std::cout << "-----------------------------------" << std::endl;
	std::cout << "CS277 - Experimental Haptics" << std::endl;
	std::cout << "Homework Boilerplate Application" << std::endl;
	std::cout << "January 2010, Stanford University" << std::endl;
    std::cout << "and 01-mydevice.cpp from Chai 3.2" << std::endl;
    std::cout << "-----------------------------------" << std::endl;
	std::cout << std::endl << std::endl;

	//-----------------------------------------------------------------------
	// ASSIGNMENTS
	//-----------------------------------------------------------------------

    // Load json
    ifstream ifs("./config.json", ifstream::binary);
    Json::Reader reader;
    Json::Value root;
    reader.parse(ifs, root);
    const Json::Value &configs = root["config"];

    cout << "LOL" << endl;

    cout << configs.size() << endl;

    cout << "Creating templates" << endl;
    for(int i = 0; i < configs.size(); i = i + 1) {
        assignments.push_back(new TemplateWorld(configs[i]));
    }
    cout << "Done creating templates" << endl;



    //-----------------------------------------------------------------------
    // OPEN GL - WINDOW DISPLAY
    //-----------------------------------------------------------------------

    cout << "Starting openGL" << endl;
    // initialize GLFW library
    if (!glfwInit())
    {
        cout << "failed initialization" << endl;
        cSleepMs(1000);
        return 1;
    }

    // set error callback
    glfwSetErrorCallback(errorCallback);

    // compute desired size of window
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int w = 0.8 * mode->height;
    int h = 0.5 * mode->height;
    int x = 0.5 * (mode->width - w);
    int y = 0.5 * (mode->height - h);

    // set OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);


    // set active stereo mode
    if (stereoMode == C_STEREO_ACTIVE)
    {
        glfwWindowHint(GLFW_STEREO, GL_TRUE);
    }
    else
    {
        glfwWindowHint(GLFW_STEREO, GL_FALSE);
    }

    // create display context
    window = glfwCreateWindow(w, h, "KTH First 3D Haptics Lab", NULL, NULL);
    if (!window)
    {
        cout << "failed to create window" << endl;
        cSleepMs(1000);
        glfwTerminate();
        return 1;
    }

    // get width and height of window
    glfwGetWindowSize(window, &width, &height);

    // set position of window
    glfwSetWindowPos(window, x, y);

    // set key callback
    glfwSetKeyCallback(window, keyCallback);

    // set resize callback
    glfwSetWindowSizeCallback(window, windowSizeCallback);

    // set current display context
    glfwMakeContextCurrent(window);

    // sets the swap interval for the current display context
    glfwSwapInterval(swapInterval);

#ifdef GLEW_VERSION
    // initialize GLEW library
    if (glewInit() != GLEW_OK)
    {
        cout << "failed to initialize GLEW library" << endl;
        glfwTerminate();
        return 1;
    }
#endif

    //--------------------------------------------------------------------------
    // WORLD - CAMERA - LIGHTING
    //--------------------------------------------------------------------------

    cout << "Creating worlds" << endl;

    // create a new world.
    world = new cWorld();

    // set the background color of the environment
    world->m_backgroundColor.setBlack();

    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);

    // position and orient the camera
    camera->set(cVector3d(0.4, 0.0, 0.2),  // camera position (eye)
                cVector3d(0.0, 0.0, 0.0),  // lookat position (target)
                cVector3d(0.0, 0.0, 1.0)); // direction of the (up) vector

    // set the near and far clipping planes of the camera
    camera->setClippingPlanes(0.01, 100);

    // set stereo mode
    camera->setStereoMode(stereoMode);

    // set stereo eye separation and focal length (applies only if stereo is enabled)
    camera->setStereoEyeSeparation(0.01);
    camera->setStereoFocalLength(0.5);

    // set vertical mirrored display mode
    camera->setMirrorVertical(mirroredDisplay);

    // create a directional light source
    light = new cDirectionalLight(world);

    // insert light source inside world
    world->addChild(light);

    // enable light source
    light->setEnabled(true);

    // define direction of light beam
    light->setDir(-1.0, 0.0, 0.0);

    cout << "Opengl done" << endl;

    //-----------------------------------------------------------------------
    // HAPTIC DEVICES / TOOLS
    //-----------------------------------------------------------------------

    cout << "Configuring haptic device" << endl;
    // Create a haptic device handler
    handler = new cHapticDeviceHandler();

    // get a handle to the first haptic device
    handler->getDevice(hapticDevice, 0);

    // open a connection to haptic device
    hapticDevice->open();

    // calibrate device (if necessary)
    hapticDevice->calibrate();

    // retrieve information about the current haptic device
    cHapticDeviceInfo info = hapticDevice->getSpecifications();

    // display a reference frame if haptic device supports orientations
    if (info.m_sensedRotation == true)
    {
        // display reference frame
        cursor->setShowFrame(true);

        // set the size of the reference frame
        cursor->setFrameSize(0.05);
    }

    // if the device has a gripper, enable the gripper to simulate a user switch
    hapticDevice->setEnableGripperUserSwitch(true);

    cout << "Done Configuring haptic device" << endl;


    //--------------------------------------------------------------------------
    // WIDGETS
    //--------------------------------------------------------------------------

    // create a font
    font = NEW_CFONTCALIBRI20();

    // create a label to display the haptic device model
    labelHapticDeviceModel = new cLabel(font);
    camera->m_frontLayer->addChild(labelHapticDeviceModel);
    labelHapticDeviceModel->setText(info.m_modelName);

    // create a label to display the position of haptic device
    labelHapticDevicePosition = new cLabel(font);
    camera->m_frontLayer->addChild(labelHapticDevicePosition);
    labelHapticDevicePosition->setText("info.m_modelName");

    // create a label to display the haptic and graphic rate of the simulation
    labelRates = new cLabel(font);
    camera->m_frontLayer->addChild(labelRates);

    cout << "Done creating worlds" << endl;

    //-----------------------------------------------------------------------
	// START SIMULATION
	//-----------------------------------------------------------------------
	// Initialize the world with assignment 0
    cout << "Starting first simulation" << endl;
	reset(0);
    cout << "Done starting first simulation" << endl;

	// Simulation in now running
	simulationRunning = true;

    // Create a thread which starts the main haptics rendering loop
    hapticsThread = new cThread();
    hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

    // setup callback when application exits
    atexit(close);


    //--------------------------------------------------------------------------
    // MAIN GRAPHIC LOOP
    //--------------------------------------------------------------------------

    // call window size callback at initialization
    windowSizeCallback(window, width, height);

    cout << glfwWindowShouldClose(window) << endl;

    // main graphic loop
    while (!glfwWindowShouldClose(window))
    {
        cout << "Graphic loop" << endl;

        // get width and height of window
        glfwGetWindowSize(window, &width, &height);

        cout << "1" << endl;

        // render graphics
        updateGraphics();

        cout << "2" << endl;

        // swap buffers
        glfwSwapBuffers(window);

        cout << "3" << endl;

        // process events
        glfwPollEvents();

        cout << "4" << endl;

        // signal frequency counter
        freqCounterGraphics.signal(1);

        cout << "5" << endl;
    }

    cout << glfwWindowShouldClose(window) << endl;


    // close window
    glfwDestroyWindow(window);

    // terminate GLFW library
    glfwTerminate();

    // exit
    return 0;
}
//---------------------------------------------------------------------------


void reset(size_t assignmentId)
{
	// Deactivate the old scene
	assignments[currentAssignment]->setInitialized(false);
	currentAssignment = assignmentId;

	// Delete the old world and create a new one
    delete world;
	world = new cWorld();
	world->setBackgroundColor(0.0, 0.0, 0.0);

	// Create a camera and insert it into the virtual world
	camera = new cCamera(world);
	world->addChild(camera);

	// Position and oriente the camera
	camera->set(cVector3d(0.2, 0.0, 0.0),    // camera position (eye)
                cVector3d(0.0, 0.0, 0.0),    // lookat position (target)
                cVector3d(0.0, 0.0, 1.0));   // direction of the "up" vector


	// Set the near and far clipping planes of the camera
	// anything in front/behind these clipping planes will not be rendered
	camera->setClippingPlanes(0.01, 10.0);

	// Create a light source and attach it to the camera
    light = new cDirectionalLight(world);
	camera->addChild(light);                   // attach light to camera
	light->setEnabled(true);                   // enable light source
    light->setLocalPos(cVector3d(2.0, 0.5, 1.0));  // position the light source
	light->setDir(cVector3d(-2.0, 0.5, 1.0));  // define the direction of the light beam

	// Create a label that shows the haptic loop update rate
    cFontPtr font = NEW_CFONTCALIBRI20();
    rateLabel = new cLabel(font);
    camera->m_frontLayer->addChild(rateLabel);

    // Create a label that shows the current position of the device
    positionLabel = new cLabel(font);
    positionLabel->setLocalPos(8, 8, 0);
    camera->m_frontLayer->addChild(positionLabel);

    // Create a label that shows the current assignment name
    assignmentLabel = new cLabel(font);
    camera->m_frontLayer->addChild(assignmentLabel);

	// Initialize the current assignment
    assignments[currentAssignment]->initialize(world, camera);

	// Update the assignment label
    assignmentLabel->setText(assignments[currentAssignment]->getName());

	// Precalculate width to make it centered
    assignmentLabelWidth = assignmentLabel->m_font->getTextWidth(assignmentLabel->getText(),0.01);
    assignments[currentAssignment]->setInitialized(true);


	// Restart the clock measuring total time elapsed
	clockTotal.start(true);
}
//---------------------------------------------------------------------------


void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height)
{
    // update window size
    width  = a_width;
    height = a_height;

    // update position of label
    //labelHapticDeviceModel->setLocalPos(20, height - 40, 0);

    // update position of label
    //labelHapticDevicePosition->setLocalPos(20, height - 60, 0);
}

//------------------------------------------------------------------------------

void errorCallback(int a_error, const char* a_description)
{
    cout << "Error: " << a_description << endl;
}


void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods)
{
    // filter calls that only include a key press
    if ((a_action != GLFW_PRESS) && (a_action != GLFW_REPEAT))
    {
        return;
    }

    // option - exit
    else if ((a_key == GLFW_KEY_ESCAPE) || (a_key == GLFW_KEY_Q))
    {
        glfwSetWindowShouldClose(a_window, GLFW_TRUE);
    }



    // Key 1 - 6 corresponding to an existing assignment
    else if (a_key == GLFW_KEY_L)
    {
        if((currentAssignment + 1) == assignments.size()) {
            reset(0);
        } else {
            reset(currentAssignment + 1);
        }
    }
  
    // option - toggle fullscreen
    else if (a_key == GLFW_KEY_F)
    {
        // toggle state variable
        fullscreen = !fullscreen;

        // get handle to monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        // get information about monitor
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // set fullscreen or window mode
        if (fullscreen)
        {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
        else
        {
            int w = 0.8 * mode->height;
            int h = 0.5 * mode->height;
            int x = 0.5 * (mode->width - w);
            int y = 0.5 * (mode->height - h);
            glfwSetWindowMonitor(window, NULL, x, y, w, h, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
    }

    // option - toggle vertical mirroring
    else if (a_key == GLFW_KEY_M)
    {
        mirroredDisplay = !mirroredDisplay;
        camera->setMirrorVertical(mirroredDisplay);
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
    hapticDevice->close();

    // delete resources
    delete hapticsThread;
    delete world;
    delete handler;
}


//---------------------------------------------------------------------------

void updateGraphics(void)
{
    /////////////////////////////////////////////////////////////////////
    // UPDATE WIDGETS
    /////////////////////////////////////////////////////////////////////

    cout << "graphics before assignment" << endl;

    //if (assignments.at(currentAssignment)->isInitialized()){
    //    assignments.at(currentAssignment)->updateGraphics();

        // update position display (using global variable set in haptic loop)
        //labelHapticDevicePosition->setText(assignments[currentAssignment]->hapticDevicePosition.str(3));
        //position = position * 1000.0; // Convert to mm
        //sprintf(buffer, "Device position: (%.2lf, %.2lf, %.2lf) mm", position.x(), position.y(), position.z());
    //}

    cout << "graphics after assignments" << endl;



    /////////////////////////////////////////////////////////////////////
    // RENDER SCENE
    /////////////////////////////////////////////////////////////////////

    cout << "Before render" << endl;

    // update shadow maps (if any)
    world->updateShadowMaps(false, mirroredDisplay);

    // render world
    camera->renderView(width, height);

    cout << "After render" << endl;

    // wait until all OpenGL commands are completed
    glFinish();

    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) cout << "Error:  %s\n" << gluErrorString(err);
}

//---------------------------------------------------------------------------

void updateHaptics(void)
{
    // A clock to estimate the haptic simulation loop update rate
    cPrecisionClock pclock;
    pclock.setTimeoutPeriodSeconds(1.0);
    pclock.start(true);
    int counter = 0;
    cPrecisionClock frameClock;
    frameClock.start(true);

    // Main haptic simulation loop
    while (simulationRunning)
    {
        if (!hapticDevice)
            continue;

        // Total time elapsed since the current assignment started
        double totalTime = clockTotal.getCurrentTimeSeconds();

        // Time elapsed since the previous haptic frame
        double timeStep = frameClock.getCurrentTimeSeconds();
        frameClock.start(true);

        // Update assignment
        if (assignments[currentAssignment]->isInitialized())
            assignments[currentAssignment]->updateHaptics(hapticDevice.get(), timeStep, totalTime);

        // Estimate the refresh rate
        ++counter;
        if (pclock.timeoutOccurred()) {
            pclock.stop();
            rateEstimate = counter;
            counter = 0;
            pclock.start(true);
        }
    }

    // Exit haptics thread
    simulationFinished = true;


}

//---------------------------------------------------------------------------
