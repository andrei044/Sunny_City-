#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"

#include <iostream>
#include "SkyBox.hpp"

#define MAX_PARTICLES 3000
float velocity = 0.0f;
float slowdown = 0.4f;
typedef struct {
    // Life
    bool alive;	// is the particle alive?
    float life;	// particle lifespan
    float fade; // decay
    // color
    float red;
    float green;
    float blue;
    // Position/direction
    float xpos;
    float ypos;
    float zpos;
    // Velocity/Direction, only goes down in y dir
    float vel;
    // Gravity
    float gravity;
}particles;
// Paticle System
particles par_sys[MAX_PARTICLES];

// window
gps::Window myWindow;
int retina_width, retina_height;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;
glm::mat4 lightRotation;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

//cube map texture
GLuint textureID;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));


GLboolean pressedKeys[1024];

// models
gps::Model3D teapot;
gps::Model3D hoonicorn;
gps::Model3D lightCube;

GLfloat angle;

// shaders
gps::Shader myBasicShader;
gps::Shader lightShader;
gps::Shader depthMapShader;

float lastX = 512, lastY = 384;

float yaw = -90.0f;

float pitch = 0.0f;

bool firstMouse = true;

float fov = 45.0f;

float deltaTime = 0.0f;
float lastFrameTime = 0.0f;

GLfloat globalCameraSpeed = 1.4f;

GLfloat lightAngle;

//skybox
gps::SkyBox mySkyBox;
gps::SkyBox myNightSkyBox;
gps::Shader skyboxShader;




//shadows
GLuint shadowMapFBO;
GLuint depthMapTexture;

bool night;

bool wireframe;

struct PointLight {
    glm::vec3 position;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

glm::vec3 pointLightPositions[] = {
    glm::vec3(0.7f,  0.2f,  2.0f),
    glm::vec3(2.3f, -3.3f, -4.0f),
    glm::vec3(-4.0f,  2.0f, -12.0f),
    glm::vec3(0.0f,  0.0f, -3.0f)
};


GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
    //TODO
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (key == GLFW_KEY_MINUS && action == GLFW_PRESS) {
        night = !night;
    }
    if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS) {
        wireframe = !wireframe;
    }
    

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
    if (yaw > 180.0f)
        yaw -= 360.0f;
    if (yaw < -180.0f)
        yaw += 360.0f;

    myCamera.rotate(pitch, yaw);
    //view = myCamera.getViewMatrix();
    //myBasicShader.useShaderProgram();
    //glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    // compute normal matrix for teapot
    //normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
}

void processMovement() {
    if (pressedKeys[GLFW_KEY_LEFT_SHIFT]) {
        globalCameraSpeed = 15.0f;
    }
    else {
        globalCameraSpeed = 1.4f;
    }

    GLfloat cameraSpeed = globalCameraSpeed * deltaTime;

    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        /*// update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));*/
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        /*// update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));*/
    }

    if (pressedKeys[GLFW_KEY_J]) {
        lightAngle -= 1.0f;
    }

    if (pressedKeys[GLFW_KEY_L]) {
        lightAngle += 1.0f;
    }

    
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 179.0f)
        fov = 179.0f;
    projection = glm::perspective(glm::radians(fov),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 10000000.0f);
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    // send projection matrix to shader
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

bool initOpenGLWindow() {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    //window scaling for HiDPI displays
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    //for sRBG framebuffer
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    //for antialising
    glfwWindowHint(GLFW_SAMPLES, 4);


    myWindow.Create(1024, 768, "OpenGL Project Core");
    //myWindow.Create(1920, 1080, "OpenGL Project Core");


    GLFWwindow* glWindow = myWindow.getWindow();

    if (!glWindow) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return false;
    }

    glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
    glfwSetKeyCallback(glWindow, keyboardCallback);
    glfwSetCursorPosCallback(glWindow, mouseCallback);
    //glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwMakeContextCurrent(glWindow);

    glfwSwapInterval(1);

#if not defined (__APPLE__)
    // start GLEW extension handler
    glewExperimental = GL_TRUE;
    glewInit();
#endif

    // get version info
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    //for RETINA display
    glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

    return true;
}

void setWindowCallbacks() {
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetScrollCallback(myWindow.getWindow(), scrollCallback);
}

void initOpenGLState() {
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK); // cull back face
    glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);//
}



void initModels() {
    teapot.LoadModel("models/teapot/teapot20segUT.obj");
    hoonicorn.LoadModel("models/city/city2.obj");
    lightCube.LoadModel("models/cube/cube.obj");
}

/*void initShaders() {
    myBasicShader.loadShader("shaders/basic.vert","shaders/basic.frag");
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    skyboxShader.useShaderProgram();
}*/

void initShaders() {
    myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    myBasicShader.useShaderProgram();
    lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
    lightShader.useShaderProgram();
    depthMapShader.loadShader("shaders/shadowShader.vert", "shaders/shadowShader.frag");
    depthMapShader.useShaderProgram();
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    skyboxShader.useShaderProgram();
}

void initUniforms() {
    myBasicShader.useShaderProgram();

    // create model matrix for teapot
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

    // get view matrix for current camera
    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    // send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

    // create projection matrix
    projection = glm::perspective(glm::radians(fov),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 10000000.0f);
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    // send projection matrix to shader
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //set the light direction (direction towards the light)
    lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    // send light dir to shader
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    //TODO REMOVE THIS BECAUSE IUN RENDERSCENE ALREADY
    //set light color
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    // send light color to shader
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    struct PointLight pointLights[4] = {{glm::vec3(0.0f, 0.5f, 1.5f),1.0f,1.0f,1.0f,glm::vec3(0.7f, 0.2f, 2.0f),glm::vec3(0.7f, 0.2f, 2.0f)},
    {glm::vec3(-4.0f, 2.0f, -12.0f),1.0f,1.0f,1.0f,glm::vec3(0.7f, 0.2f, 2.0f),glm::vec3(0.7f, 0.2f, 2.0f)},
    {glm::vec3(2.3f, -3.3f, -4.0f),1.0f,1.0f,1.0f,glm::vec3(0.7f, 0.2f, 2.0f),glm::vec3(0.7f, 0.2f, 2.0f)},
    {glm::vec3(0.0f, 0.0f, -3.0f),1.0f,1.0f,1.0f,glm::vec3(0.7f, 0.2f, 2.0f),glm::vec3(0.7f, 0.2f, 2.0f)}
    };
    glm::vec3 spotLightPosition = glm::vec3(0.0f, 0.5f, 1.5f);
    float constant = 1.0f;
    float linear = 0.7f;
    float quadratic = 1.8f;
    float cutOff = glm::cos(glm::radians(5.5f));
    float outerCutOff = glm::cos(glm::radians(10.5f));
    glm::vec3 ambient = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 specular = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 direction= glm::vec3(0.0f, -0.5f, -1.0f);
    
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLights[0].position"), 1, glm::value_ptr(spotLightPosition));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLights[0].direction"), 1, glm::value_ptr(direction));
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLights[0].constant"), constant);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLights[0].linear"), linear);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLights[0].quadratic"), quadratic);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLights[0].cutOff"), cutOff);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLights[0].outerCutOff"), outerCutOff);
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLights[0].ambient"), 1, glm::value_ptr(ambient));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLights[0].diffuse"), 1, glm::value_ptr(diffuse));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLights[0].specular"), 1, glm::value_ptr(specular));

    /*glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLights[0].position"), 1, glm::value_ptr(pointLights[0].position));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLights[0].direction"), 1, glm::value_ptr(direction));
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "pointLights[0].constant"), constant);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "pointLights[0].linear"), linear);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "pointLights[0].quadratic"), quadratic);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "pointLights[0].cutOff"), theta);
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLights[0].ambient"), 1, glm::value_ptr(ambient));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLights[0].diffuse"), 1, glm::value_ptr(diffuse));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLights[0].specular"), 1, glm::value_ptr(specular));*/

    //glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLights[0]"), 1,
    //    glm::value_ptr(pointLightPositions[0]));
    //
    //glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLights[1]"), 1,
    //    glm::value_ptr(pointLightPositions[1]));
    //
    //glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLights[2]"), 1,
    //    glm::value_ptr(pointLightPositions[2]));
    //
    //glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLights[3]"), 1,
    //    glm::value_ptr(pointLightPositions[3]));
}

void initFBO() {
    //generate FBO ID
    glGenFramebuffers(1, &shadowMapFBO);
    //create depth texture for FBO
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    //attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initSkyBox() {
    std::vector<const GLchar*> faces;
    faces.push_back("skybox/right.tga");
    faces.push_back("skybox/left.tga");
    faces.push_back("skybox/top.tga");
    faces.push_back("skybox/bottom.tga");
    faces.push_back("skybox/back.tga");
    faces.push_back("skybox/front.tga");
    mySkyBox.Load(faces);
}

void initNightSkyBox() {
    std::vector<const GLchar*> faces;
    faces.push_back("night_skybox/pz.tga");
    faces.push_back("night_skybox/nz.tga");
    faces.push_back("night_skybox/py.tga");
    faces.push_back("night_skybox/ny.tga");
    faces.push_back("night_skybox/nx.tga");
    faces.push_back("night_skybox/px.tga");
    myNightSkyBox.Load(faces);
}

glm::mat4 computeLightSpaceTrMatrix() {
    glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const GLfloat near_plane = 0.1f, far_plane = 6.0f;
    glm::mat4 lightProjection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, near_plane, far_plane);
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
    return lightSpaceTrMatrix;
}

void renderTeapot(gps::Shader shader) {
    // select active shader program
    shader.useShaderProgram();

    //send teapot model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send teapot normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw teapot
    teapot.Draw(shader);
}

void renderHoonicorn(gps::Shader shader) {
    // select active shader program
    shader.useShaderProgram();

    //send teapot model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send teapot normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw teapot
    hoonicorn.Draw(shader);
}

// Initialize/Reset Particles - give them their attributes
void initParticles(int i) {
    par_sys[i].alive = true;
    par_sys[i].life = 1.0;
    par_sys[i].fade = float(rand() % 100) / 1000.0f + 0.003f;

    par_sys[i].xpos = (float)(rand() % 21) - 10;
    par_sys[i].ypos = 10.0;
    par_sys[i].zpos = (float)(rand() % 21) - 10;

    /*par_sys[i].xpos = myPosition.x+(float)(rand()%2);
    par_sys[i].ypos =  10.0;
    par_sys[i].zpos = myPosition.z+(float)(rand()%2);*/

    par_sys[i].red = 0.5;
    par_sys[i].green = 0.5;
    par_sys[i].blue = 1.0;

    par_sys[i].vel = velocity;
    par_sys[i].gravity = -0.8;

}

void drawRain() {
    float x, y, z;

    GLfloat particlePositions[MAX_PARTICLES * 6]; // Each particle is represented by two vertices (start and end points)
    int vertexIndex = 0;
    glm::vec3 myPosition = myCamera.getPosition();
    //std::cout << myPosition.x << " " << myPosition.y << " " << myPosition.z<<"\n";

    for (int loop = 0; loop < MAX_PARTICLES; loop = loop + 2) {
        if (par_sys[loop].alive == true) {
            x = myPosition.x + par_sys[loop].xpos;
            y = myPosition.y + par_sys[loop].ypos;
            z = myPosition.z + par_sys[loop].zpos;

            // Update particle positions
            particlePositions[vertexIndex++] = x;
            particlePositions[vertexIndex++] = y;
            particlePositions[vertexIndex++] = z;

            particlePositions[vertexIndex++] = x;
            particlePositions[vertexIndex++] = y + 0.1;
            particlePositions[vertexIndex++] = z;

            // Update values
            //Move
            // Adjust slowdown for speed!
            par_sys[loop].ypos += par_sys[loop].vel / (slowdown * 1000);
            par_sys[loop].vel += par_sys[loop].gravity;
            // Decay
            par_sys[loop].life -= par_sys[loop].fade;

            if (par_sys[loop].ypos <= -10) {
                par_sys[loop].life = -1.0;
            }
            //Revive
            if (par_sys[loop].life < 0.0) {
                initParticles(loop);
            }
        }
    }

    // Create/update VBO and VAO
    GLuint vbo, vao;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);

    // Bind VAO
    glBindVertexArray(vao);

    // Bind VBO and update data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(particlePositions), particlePositions, GL_DYNAMIC_DRAW);

    // Specify vertex attribute pointers
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // Draw particles
    glDrawArrays(GL_LINES, 0, vertexIndex / 3);

    // Clean up
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void initParticles() {
    for (int loop = 0; loop < MAX_PARTICLES; loop++) {
        initParticles(loop);
    }
}

void drawObjects(gps::Shader shader, bool depthPass) {

    //shader.useShaderProgram();

    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    if (!depthPass) {
        if (night) {
            myNightSkyBox.Draw(skyboxShader, view, projection);
        }else {
            mySkyBox.Draw(skyboxShader, view, projection);
        }
    }
    teapot.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    //model = glm::scale(model, glm::vec3(0.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    hoonicorn.Draw(shader);
}

/*void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrameTime;
    lastFrameTime = currentFrame;
    //first draw skybox
    mySkyBox.Draw(skyboxShader, view, projection);

    //render the scene
    renderHoonicorn(myBasicShader);

    renderTeapot(myBasicShader);

}*/

void renderScene() {
    if (night) {
        lightColor = glm::vec3(0.0f, 0.0f, 0.0f); //black light
    }
    else {
        lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
    }
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));

    // depth maps creation pass
    //TODO - Send the light-space transformation matrix to the depth map creation shader and
    //		 render the scene in the depth map
    depthMapShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix()));
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    drawObjects(depthMapShader, true);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // final scene rendering pass (with shadows)

    //TO CHANGE-window width height
    glViewport(0, 0, retina_width, retina_height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrameTime;
    lastFrameTime = currentFrame;

    myBasicShader.useShaderProgram();

    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));


    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    lightDir = lightRotation * glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);

    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

    //bind the shadow map
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix()));

    drawObjects(myBasicShader, false);

    drawRain();

    //draw a white cube around the light

    /*lightShader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

    model = lightRotation;
    model = glm::translate(model, 1.0f * lightDir);
    model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    lightCube.Draw(lightShader);*/
}

void updateOpenGLState() {
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char* argv[]) {

    try {
        initOpenGLWindow();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    initModels();
    initShaders();
    initUniforms();
    initFBO();
    initSkyBox();
    initNightSkyBox();
    setWindowCallbacks();
    initParticles();

    glCheckError();
    // application loop
    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
        updateOpenGLState();
        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

        glCheckError();
    }

    cleanup();

    return EXIT_SUCCESS;
}
