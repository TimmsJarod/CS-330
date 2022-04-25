#include <iostream>             // cout, cerr
#include <cstdlib>              // EXIT_FAILURE
#include <GL/glew.h>            // GLEW library
#include <GLFW/glfw3.h>         // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions
#include "cylinder.h"


// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnOpengl/camera.h> // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Dice Set - by Jarod Timms"; // Macro for window title
    
    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo;         // Handle for the vertex buffer object
        GLuint nVertices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh D4;
    GLMesh D6;
    GLMesh D8;
    GLMesh Can;
    GLMesh Table;
    GLMesh Tag;
    GLMesh Lamp;
    // Texture
    GLuint imgD6Id;
    GLuint imgD8Id;
    GLuint imgTableId;
    GLuint imgTagId;
    GLuint imgMetalId;

    glm::vec2 gUVScale(1.0f, 1.0f);
    GLint gTexWrapMode = GL_REPEAT;

    // Shader program
    GLuint gProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 1.0f, 6.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    bool persp = true;
    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);


    // Cube and light color
    //m::vec3 gObjectColor(0.6f, 0.5f, 0.75f);
    glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
    glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

    // Light position and scale
    glm::vec3 gLightPosition (-1.0f, 5.0f, -2.0f);
    glm::vec3 gLightScale(0.3f);
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void UCreateD4(GLMesh& mesh);
void UCreateD6(GLMesh& mesh);
void UCreateD8(GLMesh& mesh);
void UCreateCylinder(GLMesh& mesh);
void UCreatePlane(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 normal; // VAP position 1 for normals
    layout(location = 2) in vec2 textureCoordinate;

    out vec3 vertexNormal; // For outgoing normals to fragment shader
    out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
    out vec2 vertexTextureCoordinate;

    //Global variables for the transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);

/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
    in vec3 vertexFragmentPos; // For incoming fragment position
    in vec2 vertexTextureCoordinate;

    out vec4 fragmentColor;

    // Uniform / Global variables for object color, light color, light position, and camera/view position
    uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPosition;
    uniform sampler2D uTexture; // Useful when working with multiple textures
    uniform vec2 uvScale;

    void main()
    {
        /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

        //Calculate Ambient lighting*/
        float ambientStrength = 0.5f; // Set ambient or global lighting strength
        vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

        //Calculate Diffuse lighting*/
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse w by generating dot product of normal and light
        vec3 diffuse = impact * lightColor; // Generate diffuse light color

        //Calculate Specular lighting*/
        float specularIntensity = 0.8f; // Set specular light strength
        float highlightSize = 16.0f; // Set specular highlight size
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
        //Calculate specular component
        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = specularIntensity * specularComponent * lightColor;

        // Texture holds the color to be used for all three components
        vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

        if (textureColor.a < 0.1)
            discard;

        // Calculate phong result
        vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

        fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
    }
    );


/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

        //Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);



// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;


    // Create the mesh
    UCreateD4(D4);
    UCreateD6(D6);
    UCreateD8(D8); // Calls the function to create the Vertex Buffer Object
    UCreateCylinder(Can);
    UCreatePlane(Table);
    UCreatePlane(Tag);
    UCreateD6(Lamp);

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;


    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    

    // Load texture
    const char* texFilename = "../resources/textures/D6.png";
    if (!UCreateTexture(texFilename, imgD6Id))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    texFilename = "../resources/textures/D8.jpg";
    if (!UCreateTexture(texFilename, imgD8Id))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    texFilename = "../resources/textures/table.jpg";
    if (!UCreateTexture(texFilename, imgTableId))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    texFilename = "../resources/textures/tag.png";
    if (!UCreateTexture(texFilename, imgTagId))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    texFilename = "../resources/textures/metal.jpg";
    if (!UCreateTexture(texFilename, imgMetalId))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;
        bool isProJectionPerspective = true;
        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(D4);
    UDestroyMesh(D6);
    UDestroyMesh(D8);
    UDestroyMesh(Can);
    UDestroyMesh(Table);
    UDestroyMesh(Tag);

    // Release texture
    UDestroyTexture(imgD6Id);
    UDestroyTexture(imgD8Id);
    UDestroyTexture(imgTableId);
    UDestroyTexture(imgTagId);

    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);
    glfwSetKeyCallback(*window, UKeyCallback); // Added callback for key presses with GLFW compatible function

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;
    

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)//Added Q Key to allow camera to move up
        gCamera.ProcessKeyboard(UP, gDeltaTime);//Added Enum state for UP that modifies Y axis positioning
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)//Added E Key to allow camera to move down
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);//Added Enum state for DOWN that modifies Y axis positioning
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset); // Edited the camera.h to have this callback modify the camera speed instead of axis offset
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}

void UKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch (key)
    {
    case GLFW_KEY_P:
    {
        if (action == GLFW_PRESS)
        {
            persp = !persp;
            cout << "P key pressed" << endl;
        }
        else
            cout << "P key released" << endl;
    }
    break;

    default:
        break;
    }
}
// Function called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    //glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    if (persp) {
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else {
        float scale = 120;
        projection = glm::ortho((800.0f / scale), -(800.0f / scale), -(600.0f / scale), (600.0f / scale), -2.5f, 6.5f);
    }

    // Set the shader to be used
    glUseProgram(gProgramId);

    // 1. Scales the object by 1
    glm::mat4 scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    // 2. Rotates shape by n degrees
    glm::mat4 rotation = glm::rotate(glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    /* Draw D8 */
    scale = glm::scale(glm::vec3(0.6f, 0.6f, 0.6f));
    rotation = glm::rotate(glm::radians(56.0f), glm::vec3(0.0f, -0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-1.0f, -0.25f, -0.5f));
    model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(D8.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, imgD8Id);
    glDrawArrays(GL_TRIANGLES, 0, D8.nVertices);

    /* Draw D6 */
    scale = glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
    rotation = glm::rotate(glm::radians(45.0f), glm::vec3(0.0, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(0.8f, 1.3f, 1.0f));
    model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindTexture(GL_TEXTURE_2D, imgD6Id);
    glBindVertexArray(D6.vao);
    glDrawArrays(GL_TRIANGLES, 0, D6.nVertices);

    /* Draw D4 */
    scale = glm::scale(glm::vec3(0.6f, 0.6f, 0.6f));
    rotation = glm::rotate(glm::radians(15.0f), glm::vec3(0.0, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(-1.0f, -0.5f, 2.0f));
    model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindTexture(GL_TEXTURE_2D, imgD8Id);
    glBindVertexArray(D4.vao);
    glDrawArrays(GL_TRIANGLES, 0, D4.nVertices);

    /* Draw Can */
    glBindTexture(GL_TEXTURE_2D, imgMetalId);
    glBindVertexArray(Can.vao);
    scale = glm::scale(glm::vec3(1.0f, 0.5f, 1.0f));
    rotation = glm::rotate(0.0f, glm::vec3(1.0, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(1.0f, 0.0f, 1.0f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    static_meshes_3D::Cylinder C1(1, 20, 3, true, true, true);
    C1.render();

    /* Draw Can Lid*/
    glBindTexture(GL_TEXTURE_2D, imgMetalId);
    glBindVertexArray(Can.vao);
    scale = glm::scale(glm::vec3(1.025f, 0.1f, 1.025f));
    rotation = glm::rotate(0.0f, glm::vec3(1.0, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(1.0f, 0.9f, 1.0f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    static_meshes_3D::Cylinder C2(1, 20, 3, true, true, true);
    C2.render();

    /* Draw Table */
    scale = glm::scale(glm::vec3(10.0f, 10.0f, 10.0f));
    rotation = glm::rotate(glm::radians(0.0f), glm::vec3(0.0, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(0.0f, -0.5f, 0.0f));
    model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindTexture(GL_TEXTURE_2D, imgTableId);
    glBindVertexArray(Table.vao);
    glDrawArrays(GL_TRIANGLES, 0, Table.nVertices);

    /* Draw Tag on Table */
    scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    rotation = glm::rotate(glm::radians(30.0f), glm::vec3(0.0, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(-2.0f, -0.49f, 1.0f));
    model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindTexture(GL_TEXTURE_2D, imgTagId);
    glBindVertexArray(Tag.vao);
    glDrawArrays(GL_TRIANGLES, 0, Tag.nVertices);

    /* Draw Lamp */
    glUseProgram(gLampProgramId);
    glBindTexture(GL_TEXTURE_2D, imgTableId);
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");
    gLightColor.r = 1.0f;
    gLightColor.g = 0.9f;
    gLightColor.b = 0.9f;
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(Lamp.vao);
    glDrawArrays(GL_TRIANGLES, 0, Lamp.nVertices);
    
    /* Draws second Lamp */
    translation = glm::translate(glm::vec3(0.0f, 5.0f, 4.0f));
    model = translation * glm::scale(gLightScale);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, Lamp.nVertices);
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateD4(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts[] = {
        //Positions       //Normals          //Texture Coordinates
        0.0f, 0.86f,0.0f, 0.0f, 0.5f, 1.0f,  0.25f, 0.5f,//  0 //Top
       -0.5f, 0.0f, 0.36f, 0.0f, 0.5f, 1.0f,  0.0f, 1.0f, //  1 //Front
        0.5f, 0.0f, 0.36f, 0.0f, 0.5f, 1.0f,  0.0f, 0.0f, //  2

        0.0f, 0.86f,0.0f, 0.5f, 0.5f,-0.5f,  0.25f, 0.5f,//  0
        0.0f, 0.0f,-0.5f, 0.5f, 0.5f,-0.5f,  0.5f, 0.0f, //  4 //Right
        0.5f, 0.0f, 0.36f, 0.5f, 0.5f,-0.5f,  0.0f, 0.0f, //  3
                            
        0.0f, 0.86f,0.0f,-0.5f, 0.5f,-0.5f,  0.25f, 0.5f,//  0
       -0.5f, 0.0f, 0.36f,-0.5f, 0.5f,-0.5f,  0.5f, 1.0f, //  2 //Left
        0.0f, 0.0f,-0.5f,-0.5f, 0.5f,-0.5f,  0.5f, 0.0f, //  4

       -0.5f, 0.0f, 0.36f, 0.0f,-0.5f, 0.0f,  0.25f, 0.5f,//  1
        0.5f, 0.0f, 0.36f, 0.0f,-0.5f, 0.0f,  0.0f, 1.0f, //  2 //Bottom
        0.0f, 0.0f,-0.5f, 0.0f,-0.5f, 0.0f,  0.5f, 1.0f, //  3

    };



    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// Implements the UCreateMesh function
void UCreateD6(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts[] = {
        //Positions          //Normals
        // ------------------------------------------------------
        //Back Face          //Negative Z Normal  Texture Coords.
       -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.5f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.66f, 0.5f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.66f, 1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.66f, 1.0f,
       -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
       -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.5f,

       //Front Face         //Positive Z Normal
      -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.5f,
       0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.33f, 0.5f,
       0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.33f, 1.0f,
       0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.33f, 1.0f,
      -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
      -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.5f,

      //Left Face          //Negative X Normal
     -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.33f, 0.5f,
     -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.5f,
     -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.33f, 0.0f,
     -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.33f, 0.5f,

     //Right Face         //Positive X Normal
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.33f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.66f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.66f, 0.5f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.66f, 0.5f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.33f, 0.5f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.33f, 1.0f,

     //Bottom Face        //Negative Y Normal
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.66f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.5f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.5f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.66f, 0.5f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.66f, 0.0f,

    //Top Face           //Positive Y Normal
   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.66f, 0.0f,
    0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.33f, 0.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.33f, 0.5f,
    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.33f, 0.5f,
   -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.66f, 0.5f,
   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.66f, 0.0f
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// Implements the UCreateMesh function
void UCreateD8(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts[] = {
        //Positions       //Normals          //Texture Coordinates
        0.0f, 0.75f,0.0f, 0.0f, 0.5f, 1.0f,  0.25f, 0.5f,//  0 //Top
       -0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 1.0f,  0.0f, 1.0f, //  1 //Front
        0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 1.0f,  0.0f, 0.0f, //  3

        0.0f, 0.75f,0.0f, 1.0f, 0.5f, 0.0f,  0.25f, 0.5f,//  0
        0.5f, 0.0f,-0.5f, 1.0f, 0.5f, 0.0f,  0.5f, 0.0f, //  4 //Right
        0.5f, 0.0f, 0.5f, 1.0f, 0.5f, 0.0f,  0.0f, 0.0f, //  3

        0.0f, 0.75f,0.0f, 0.0f, 0.5f,-1.0f,  0.25f, 0.5f,//  0
       -0.5f, 0.0f,-0.5f, 0.0f, 0.5f,-1.0f,  0.5f, 1.0f, //  2 //Back
        0.5f, 0.0f,-0.5f, 0.0f, 0.5f,-1.0f,  0.5f, 0.0f, //  4

        0.0f, 0.75f,0.0f,-1.0f, 0.5f, 0.0f,  0.25f, 0.5f,//  0
       -0.5f, 0.0f, 0.5f,-1.0f, 0.5f, 0.0f,  0.0f, 1.0f, //  1 //Left
       -0.5f, 0.0f,-0.5f,-1.0f, 0.5f, 0.0f,  0.5f, 1.0f, //  2

       -0.5f, 0.0f, 0.5f, 0.0f,-0.5f, 1.0f,  0.5f, 1.0f, //  5 //Bottom
        0.5f, 0.0f, 0.5f, 0.0f,-0.5f, 1.0f,  0.5f, 0.0f, //  7 //Front
        0.0f,-0.75f,0.0f, 0.0f,-0.5f, 1.0f,  0.75f, 0.5f,// 9

        0.5f, 0.0f, 0.5f, 1.0f,-0.5f, 0.0f,  0.5f, 0.0f, //  7
        0.5f, 0.0f,-0.5f, 1.0f,-0.5f, 0.0f,  1.0f, 0.0f, //  8 //Right
        0.0f,-0.75f,0.0f, 1.0f,-0.5f, 0.0f,  0.75f, 0.5f,// 9

       -0.5f, 0.0f,-0.5f, 0.0f,-0.5f,-1.0f,  1.0f, 1.0f, //  6
        0.5f, 0.0f,-0.5f, 0.0f,-0.5f,-1.0f,  1.0f, 0.0f, //  8 //Back
        0.0f,-0.75f,0.0f, 0.0f,-0.5f,-1.0f,  0.75f, 0.5f,// 9

       -0.5f, 0.0f, 0.5f,-1.0f,-0.5f, 0.0f,  0.5f, 1.0f, //  5
       -0.5f, 0.0f,-0.5f,-1.0f,-0.5f, 0.0f,  1.0f, 1.0f, //  6 //Left
        0.0f,-0.75f,0.0f,-1.0f,-0.5f, 0.0f,  0.75f, 0.5f //  9
    };



    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// Implements the UCreateMesh function
void UCreatePlane(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts[] = {
     
    //Top Face           //Positive Y Normal //Texture
   -0.5f,  0.0f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
    0.5f,  0.0f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
    0.5f,  0.0f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    0.5f,  0.0f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
   -0.5f,  0.0f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
   -0.5f,  0.0f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void UCreateCylinder(GLMesh& mesh)
{
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
