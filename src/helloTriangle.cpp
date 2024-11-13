// template based on material from learnopengl.com
#define GLEW_STATIC
#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"
#include "mesh.hpp"

#include <string>
#include <fstream>
#include <streambuf>

#include <iostream>

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// light source
glm::vec3 lightPos(-1.8f, -1.5f, -3.0f);
// view position
glm::vec3 viewPos(0.0f, 0.0f, -3.0f);

// transform
glm::mat4 model;
glm::mat4 rotation;
glm::mat4 translation;
glm::mat4 scale;
glm::vec3 scaleSize;

// dummy transform to act as placeholder uniform for cpu calculations
glm::mat4 dummyTransform;

// transform control setting
double rotatStrength;
double transStrength;
double scaleStrength;

// calculation method toggle boolean
bool gpuCalc;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // get user input for file to load
    // -------------------------------
    std::string objFile;
    std::string lightingModel;

    std::cout << "\nPlease type in .obj filename or type 'DEFAULT' (loads shark.obj).\nFile name: ";
    std::cin >> objFile;

    while (true)
    {
        std::cout << "\nPlease choose lighting model ('1' for Flat. '2' for Gouraud, '3' for Phong, '4' for Depth Test): ";
        std::cin >> lightingModel;
        if (stoi(lightingModel) == 1)
        {
            lightingModel = "flat";
            break;
        }
        else if (stoi(lightingModel) == 2)
        {
            lightingModel = "gouraud";
            break;
        }
        else if (stoi(lightingModel) == 3)
        {
            lightingModel = "phong";
            break;
        }
        else if (stoi(lightingModel) == 4)
        {
            lightingModel = "depth";
            break;
        }
        else
            std::cout << "Invalid selection. Please select one of the three lighting models." << std::endl;
    }

    if (objFile == "DEFAULT")
        objFile = "shark.obj";
    
    std::size_t extension = objFile.find(".obj");
    if (extension == std::string::npos)
        objFile.append(".obj");

    std::string objPath = "../data/" + objFile;

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "viewGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse (for use with scaling transform)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // glew: load all OpenGL function pointers
    glewInit();

    // build our shader program
    std::string vs = "../src/" + lightingModel + ".vs";
    std::string fs = "../src/" + lightingModel + ".fs";
    Shader ourShader(vs.c_str(), fs.c_str());
    // build our mesh object
    Mesh ourMesh(objPath.c_str());
    ourMesh.load();

    // build the light shader
    Shader lightShader("../src/light.vs", "../src/light.fs");
    // build and render light source cube
    Mesh lightCube("../data/cube.obj");
    lightCube.load();

    // print out control instructions to the console
    // ---------------------------------------------
    std::cout << "\n'" + objFile + "'" + " loaded successfully." << std::endl;
    std::cout << "------------------------\nCONTROLS:" << std::endl;
    std::cout << "Rotation:" << std::endl;
    std::cout << "roll -> A and D" << std::endl;
    std::cout << "pitch -> W and S" << std::endl;
    std::cout << "yaw -> Q and E" << std::endl;
    std::cout << "Translation: arrow keys" << std::endl;
    std::cout << "Scale: mouse scroll wheel / I and O" << std::endl;
    std::cout << "Toggle between CPU and GPU calc: T" << std::endl;

    // uncomment this call to draw in wireframe polygons.
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Enable z-buffer for depth checking
    glEnable(GL_DEPTH_TEST);

    // set value of dummy transform to identity matrix
    dummyTransform = glm::mat4(1.0f);
    // initialize the transform memory matrix to the identity matrix
    glm::mat4 oldTransform(1.0f);

    // intialize transformation matrices as identity matrices
    model = glm::mat4(1.0f);
    rotation = glm::mat4(1.0f);
    translation = glm::mat4(1.0f);
    scale = glm::mat4(1.0f);
    // initialize scale matrix by using mesh data
    double normalScale = 1.0 / glm::length(ourMesh.largestVertex);
    scale = glm::scale(scale, glm::vec3(normalScale));
    // initialize transformation control setting
    rotatStrength = 0.02;
    transStrength = 0.0001;
    scaleStrength = 0.00025;

    // set some new values for the depth test if selected
    if (lightingModel == "depth")
    {
        scale = glm::scale(scale, glm::vec3(20.0));
        translation = glm::translate(translation, glm::vec3(0.0, 0.0, -50.0));
        transStrength = 0.005;
    }

    // define our initial calculation method to be by using the GPU
    gpuCalc = true;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // enable shader
        ourShader.use();
        ourShader.setVec3("objectColor", 1.0f, 0.5f, 0.5f);
        ourShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("lightPos", lightPos);
        ourShader.setVec3("viewPos", viewPos);

        // calculate our model transformation
        // ----------------------------------
        model = translation * rotation * scale;

        glm::mat4 view = glm::mat4(1.0f);
        // note that we're translating the scene in the reverse direction of where we want to move
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f)); 

        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    

        // apply our transformation
        // ------------------------
        int modelLoc = glGetUniformLocation(ourShader.ID, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        int viewLoc = glGetUniformLocation(ourShader.ID, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        int projectionLoc = glGetUniformLocation(ourShader.ID, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // render object
        ourMesh.render();

        lightShader.use();

        model = glm::mat4(1.0f);
        model = glm::translate(model, -lightPos);
        normalScale = 1.0 / glm::length(lightCube.largestVertex);
        model = glm::scale(model, glm::vec3(normalScale));

        modelLoc = glGetUniformLocation(lightShader.ID, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        viewLoc = glGetUniformLocation(lightShader.ID, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        projectionLoc = glGetUniformLocation(lightShader.ID, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        lightCube.render();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Control keys for rotation
    // -------------------------

    // Roll
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        rotation = glm::rotate(rotation, glm::radians((float)rotatStrength), glm::vec3(0.0, 0.0, 1.0));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        rotation = glm::rotate(rotation, glm::radians((float)rotatStrength), glm::vec3(0.0, 0.0, -1.0));
    
    // Yaw
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        rotation = glm::rotate(rotation, glm::radians((float)rotatStrength), glm::vec3(0.0, -1.0, 0.0));
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        rotation = glm::rotate(rotation, glm::radians((float)rotatStrength), glm::vec3(0.0, 1.0, 0.0));
    
    // Pitch
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        rotation = glm::rotate(rotation, glm::radians((float)rotatStrength), glm::vec3(1.0, 0.0, 0.0));
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        rotation = glm::rotate(rotation, glm::radians((float)rotatStrength), glm::vec3(-1.0, 0.0, 0.0));

    // Control keys for translation
    // ----------------------------
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        translation = glm::translate(translation, glm::vec3(-transStrength, 0.0, 0.0));
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        translation = glm::translate(translation, glm::vec3(transStrength, 0.0, 0.0));
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        translation = glm::translate(translation, glm::vec3(0.0, transStrength, 0.0));
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        translation = glm::translate(translation, glm::vec3(0.0, -transStrength, 0.0));

    // Control keys for scaling
    // ------------------------
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        scale = glm::scale(scale, glm::vec3(1.0 + scaleStrength));
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        scale = glm::scale(scale, glm::vec3(1.0 - scaleStrength));

    // Change control setting speed
    // ----------------------------
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
    {
        rotatStrength *= 1.5;
        transStrength *= 1.5;
        scaleStrength *= 1.5;
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
    {
        rotatStrength /= 1.5;
        transStrength /= 1.5;
        scaleStrength /= 1.5;
    }

    // Calculation method toggle setting
    // ---------------------------
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
        gpuCalc = !gpuCalc;
}

// Detect mouse wheel scroll for scaling transformation
// ----------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (yoffset > 0.0)
        scale = glm::scale(scale, glm::vec3(1.05));
    else if (yoffset < 0.0)
        scale = glm::scale(scale, glm::vec3(0.95));    
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}