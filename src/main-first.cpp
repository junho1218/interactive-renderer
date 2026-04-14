#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tinyobjloader/tiny_obj_loader.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

// Window dimensions
const GLuint WIDTH = 1280, HEIGHT = 720;

// Camera variables
glm::vec3 camFocus(0.0f, 0.0f, 0.0f), camUp(0.0f, 1.0f, 0.0f);
glm::vec3 offset, up;
double lastCursorX, lastCursorY;
bool clicking = false;
float fov = 60.0f;
float radius = 5.0f;
float yaw = 0;
float pitch = 0;

// Lighting variables
bool pointLightOn = true;
bool dirLightOn = true;

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// The MAIN function, from here we start the application and run the game loop
int main()
{
    std::cout << "Starting GLFW context, OpenGL 3.1" << std::endl;
    // Init GLFW
    glfwInit();
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Create a GLFWwindow object that we can use for GLFW's functions
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "glskeleton", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Set the required callback functions
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // Define the viewport dimensions
    glViewport(0, 0, WIDTH, HEIGHT);

    // Load .obj mesh
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "./bunny.obj")) {
        std::cerr << "Obj load error: " << warn << err << std::endl;
        return -1;
    }

    // Lighting setting
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);  // Gouraud Shading

    GLfloat ambientLight[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientLight);

    // Point light setup
    GLfloat pointLightPos[] = { 5.0f, 5.0f, 5.0f, 1.0f };
    GLfloat pointLightColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, pointLightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, pointLightColor);
    glLightfv(GL_LIGHT0, GL_SPECULAR, pointLightColor);

    // Directional light setup
    GLfloat dirLightDir[] = { -1.0f, -1.0f, -1.0f, 0.0f };
    GLfloat dirLightColor[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, dirLightDir);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, dirLightColor);
    glLightfv(GL_LIGHT1, GL_SPECULAR, dirLightColor);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glfwPollEvents();
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Lighting On/Off
        if (pointLightOn)
            glEnable(GL_LIGHT0);
        else
            glDisable(GL_LIGHT0);

        if (dirLightOn)
            glEnable(GL_LIGHT1);
        else
            glDisable(GL_LIGHT1);

        // Cam setting
        float radYaw = glm::radians(yaw);
        float radPitch = glm::radians(pitch);

        offset.x = radius * cos(radPitch) * sin(radYaw); 
        offset.y = radius * sin(radPitch);
        offset.z = radius * cos(radPitch) * cos(radYaw);

        up.x = -sin(radPitch) * sin(radYaw);
        up.y = cos(radPitch);
        up.z = -sin(radPitch) * cos(radYaw);
 
        // Matrices setting
        glm::mat4 model = glm::identity<glm::mat4>();
        glm::mat4 view = glm::lookAt(offset, camFocus, up);
        glm::mat4 proj = glm::perspective(glm::radians(fov), float(WIDTH) / float(HEIGHT), 0.1f, 100.0f);

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(glm::value_ptr(proj));
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glm::value_ptr(view * model));

        // Draw mesh
        glBegin(GL_TRIANGLES);
        glColor3f(0.7f, 0.6f, 0.6f);
        for (const auto& shape : shapes) {
            for (const auto& idx : shape.mesh.indices) {
                int vi = 3 * idx.vertex_index;
                if (idx.normal_index >= 0) {
                    int ni = 3 * idx.normal_index;
                    glNormal3f(attrib.normals[ni], attrib.normals[ni + 1], attrib.normals[ni + 2]);
                }
                glVertex3f(attrib.vertices[vi], attrib.vertices[vi + 1], attrib.vertices[vi + 2]);
            }
        }
        glEnd();

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

// Callbacks
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(window, GL_TRUE);
        if (key == GLFW_KEY_1) pointLightOn = true;
        if (key == GLFW_KEY_2) pointLightOn = false;
        if (key == GLFW_KEY_3) dirLightOn = true;
        if (key == GLFW_KEY_4) dirLightOn = false;
    }
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        if (fov != 1.0f) fov -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        if (fov != 120.0f) fov += 1.0f;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            clicking = true;
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);
            lastCursorX = mx;
            lastCursorY = my;
        }
        else {
            clicking = false;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) 
{ 
    if (clicking) { 
        float sensitivity = 0.1f; 
        float dx = float(xpos - lastCursorX) * sensitivity; 
        float dy = float(ypos - lastCursorY) * sensitivity;

        yaw -= dx;
        pitch += dy;

        lastCursorX = xpos; 
        lastCursorY = ypos; 
    } 
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    float dollySpeed = 0.5f;
    radius -= float(yoffset) * dollySpeed;
    if (radius < 3)
        radius = 3;
}
