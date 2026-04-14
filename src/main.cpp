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
glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
glm::vec3 camFocus(0.0f, 0.0f, 0.0f), camUp(0.0f, 1.0f, 0.0f);
double lastCursorX, lastCursorY;
bool clicking = false;
float fov = 60.0f;
float radius = 5.0f;

// Lighting variables
bool pointLightOn = true;
bool dirLightOn = true;

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Project mouse coords onto a virtual sphere
glm::vec3 projectToSphere(double x, double y, GLuint w, GLuint h, float r = 2.0f) {
    float nx = (2.0f * x / w - 1.0f), ny = (1.0f - 2.0f * y / h);
    float d = nx * nx + ny * ny;
    float nz = (d <= (r * r) / 2) ? sqrt(r * r - d) : (r * r) / (2.0f * sqrt(d));
    glm::vec3 v(nx, ny, nz);
    return glm::normalize(v);
}

// Compute quaternion rotating v1 to v2
glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest) {
    start = glm::normalize(start);
    dest = glm::normalize(dest);
    float cosTheta = dot(start, dest);
    glm::vec3 rotationAxis;
    if (cosTheta < -1 + 0.001f) {
        // special case when vectors in opposite directions:
        // there is no "ideal" rotation axis
        // So guess one; any will do as long as it's perpendicular to start
        rotationAxis = cross(glm::vec3(0.0f, 0.0f, 1.0f), start);
        if (glm::length2(rotationAxis) <
            0.01) // they were parallel, try again.
            rotationAxis = cross(glm::vec3(1.0f, 0.0f, 0.0f), start);
        rotationAxis = normalize(rotationAxis);
        return glm::angleAxis(glm::radians(180.0f), rotationAxis);
    }
    rotationAxis = cross(start, dest);
    float s = sqrt((1 + cosTheta) * 2);
    float invs = 1 / s;
    return glm::quat(s * 0.5f,
        rotationAxis.x * invs,
        rotationAxis.y * invs,
        rotationAxis.z * invs);
}

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
        glm::vec3 tempCamPos = rotation * glm::vec3(0.0f, 0.0f, radius);
        glm::vec3 tempCamUp = rotation * camUp;

        // Matrices setting
        glm::mat4 model = glm::identity<glm::mat4>();
        glm::mat4 view = glm::lookAt(tempCamPos , camFocus, tempCamUp);
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

void cursor_position_callback(GLFWwindow* win, double mx, double my) {
    if (!clicking) return;

    float sensitivity = 2.0;

    glm::vec3 v0 = projectToSphere(lastCursorX, lastCursorY, WIDTH, HEIGHT, sensitivity);
    glm::vec3 v1 = projectToSphere(mx, my, WIDTH, HEIGHT, sensitivity);

    rotation *= RotationBetweenVectors(v1, v0);
    glm::normalize(rotation);
    
    lastCursorX = mx;
    lastCursorY = my;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    float dollySpeed = 0.5f;
    radius -= float(yoffset) * dollySpeed;
    if (radius < 3)
        radius = 3;
}
