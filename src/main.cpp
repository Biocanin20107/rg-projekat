#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(char const * path, bool gammaCorrection);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool blinn = true;
bool screen = false;
bool hdrKeyPressed = false;
bool hdr = true;
float exposure = 1.0f;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

unsigned int loadTexture(char const * path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(false);
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

struct ProgramState {

    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 bubnjeviPosition = glm::vec3(9.0, 8.2, -1.0);
    float bubnjeviScale = 0.1f;

    glm::vec3 zvucnikPosition = glm::vec3(-6.0, 0.260, -5.0);
    float zvucnikScale = 0.2f;

    glm::vec3 stoPosition = glm::vec3(-1.0, 0.28, 5.0);
    float stoScale = 0.0089f;

    glm::vec3 gramofonPosition = glm::vec3(-1.170, 2.870, 5.0);
    float gramofonScale = 0.080f;

    glm::vec3 bubanjRotation = glm::vec3(-296.0, 729.0, 726.0);
    float bubanjAngle = 148.45f;

    glm::vec3 stoRotation = glm::vec3(-272.0, 55.0, 54.0);
    float stoAngle = 92.2f;

    glm::vec3 prozorPosition = glm::vec3(-5.0, 5.0, -27.0);
    float prozorScale = 8.1f;

    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);

    glm::vec3 probaRotation = glm::vec3(0.0f);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

void renderQuad();

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded textureColorbuffer's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/lighting.vs", "resources/shaders/lighting.fs");
    Shader shader("resources/shaders/blending.vs", "resources/shaders/blending.fs");
    Shader hdrShader("resources/shaders/hdr.vs", "resources/shaders/hdr.fs");

    // load models
    // -----------
    Model bubanjModel("resources/objects/bubnjevi/cadnav.com_model/ModelsBubanj/KettleDrum.obj");
    bubanjModel.SetShaderTextureNamePrefix("material.");

    Model zvucnikModel("resources/objects/muzika/zvucnjak/cadnav.com_model/Models_G0701A001/speaker.obj");
    zvucnikModel.SetShaderTextureNamePrefix("material.");

    Model stoModel("resources/objects/muzika/sto/cadnav.com_model/Models_G0402A298/table.obj");
    stoModel.SetShaderTextureNamePrefix("material.");

    Model gramofonModel("resources/objects/muzika/gramofon/cadnav.com_model/Models_G0402A410/gramophone.obj");
    gramofonModel.SetShaderTextureNamePrefix("material.");

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(-12.0f, 8.0, -18.0);
    pointLight.ambient = glm::vec3(1.0, 1.0, 1.0);
    pointLight.diffuse = glm::vec3(1.0, 1.0, 1.0);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.6f;
    pointLight.quadratic = 0.0f;



    float planeVertices[] = {
            // positions            // normals         // texcoords
            400.0f, -32.5f,  400.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
            -400.0f, -32.5f,  400.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
            -400.0f, -32.5f, -400.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,

            400.0f, -32.5f,  400.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
            -400.0f, -32.5f, -400.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,
            400.0f, -32.5f, -400.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f
    };
    float transparentVertices[] = {
            // positions
            0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
            0.0f, 0.0f,  0.0f,  0.0f,  1.0f,
            1.0f, 0.0f,  0.0f,  1.0f,  1.0f,

            0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
            1.0f, 0.0f,  0.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  0.0f,  1.0f,  0.0f
    };

    // plane VAO
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    // transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    unsigned int floorTexture = loadTexture(FileSystem::getPath("resources/textures/CD141.JPG").c_str(), true);
    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/prozorce.png").c_str(), true);

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    // create floating point color buffer
    unsigned int colorBuffer;
    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // create depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    // attach buffers
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shader.use();
    shader.setInt("diffuseTexture", 0);

    hdrShader.use();
    hdrShader.setInt("hdrBuffer", 0);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = (currentFrame - lastFrame) * 4;
        lastFrame = currentFrame;

        // input
        processInput(window);

        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // don't forget to enable shader before setting uniforms
            ourShader.use();
            pointLight.position = glm::vec3(-7.7, 10.0f, 16.8);
            ourShader.setVec3("pointLight.position", pointLight.position);
            ourShader.setVec3("pointLight.ambient", pointLight.ambient);
            ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
            ourShader.setVec3("pointLight.specular", pointLight.specular);
            ourShader.setFloat("pointLight.constant", pointLight.constant);
            ourShader.setFloat("pointLight.linear", pointLight.linear);
            ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
            ourShader.setVec3("viewPosition", programState->camera.Position);
            ourShader.setFloat("material.shininess", 32.0f);
            // view/projection transformations
            glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                    (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
            glm::mat4 view = programState->camera.GetViewMatrix();
            ourShader.setMat4("projection", projection);
            ourShader.setMat4("view", view);
            ourShader.setInt("blinn", blinn);

            // BUBANJ
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model,
                                   programState->bubnjeviPosition); // translate it down so it's at the center of the scene
            model = glm::scale(model, glm::vec3(programState->bubnjeviScale));    // it's a bit too big for our scene, so scale it down
            model = glm::rotate(model, glm::radians(programState->bubanjAngle), programState->bubanjRotation);
            ourShader.setMat4("model", model);
            bubanjModel.Draw(ourShader);

            // ZVUCNIK
            model = glm::mat4(1.0f);
            model = glm::translate(model,
                                   programState->zvucnikPosition); // translate it down so it's at the center of the scene
            model = glm::scale(model, glm::vec3(programState->zvucnikScale));    // it's a bit too big for our scene, so scale it down
            ourShader.setMat4("model", model);
            zvucnikModel.Draw(ourShader);

            // STO
            model = glm::mat4(1.0f);
            model = glm::translate(model,
                                   programState->stoPosition); // translate it down so it's at the center of the scene
            model = glm::scale(model, glm::vec3(programState->stoScale));    // it's a bit too big for our scene, so scale it down
            model = glm::rotate(model, glm::radians(programState->stoAngle), programState->stoRotation);
            ourShader.setMat4("model", model);
            stoModel.Draw(ourShader);

            // GRAMOFON

            model = glm::mat4(1.0f);
            model = glm::translate(model,
                                   programState->gramofonPosition); // translate it down so it's at the center of the scene
            model = glm::scale(model, glm::vec3(programState->gramofonScale));    // it's a bit too big for our scene, so scale it down
            ourShader.setMat4("model", model);
            gramofonModel.Draw(ourShader);

            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);

            glBindVertexArray(planeVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, floorTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glCullFace(GL_BACK);

            shader.use();
            shader.setMat4("projection", projection);
            shader.setMat4("view", view);
            shader.setInt("screen", screen);
            glBindVertexArray(transparentVAO);
            glBindTexture(GL_TEXTURE_2D, transparentTexture);
            model = glm::mat4(1.0f);
            model = glm::translate(model, programState->prozorPosition);
            model = glm::scale(model, glm::vec3(programState->prozorScale));
            shader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glDisable(GL_CULL_FACE);

            if (programState->ImGuiEnabled)
                DrawImGui(programState);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);

        hdrShader.setInt("hdr", hdr);
        hdrShader.setFloat("exposure", exposure);
        renderQuad();

        std::cout << "hdr: " << (hdr ? "on" : "off") << "| exposure: " << exposure << std::endl;
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------

    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);

    glDeleteVertexArrays(1, &transparentVAO);
    glDeleteBuffers(1, &transparentVBO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(DOWN, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(CAMERA_UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(CAMERA_DOWN, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);

        ImGui::DragFloat3("Prozor position", (float*)&programState->prozorPosition);
        ImGui::DragFloat("Prozor scale", (float*)&programState->prozorScale);

        ImGui::DragFloat3("Bubnjevi position", (float*)&programState->bubnjeviPosition);
        ImGui::DragFloat("Bubnjevi scale", &programState->bubnjeviScale, 0.05, 0.1, 4.0);
        ImGui::DragFloat3("Bubnjevi rotation", (float*)&programState->bubanjRotation);
        ImGui::DragFloat("Bubnjevi angle", &programState->bubanjAngle, 0.05, 0.1, 200.0);

        ImGui::DragFloat3("Zvucnik position", (float*)&programState->zvucnikPosition);
        ImGui::DragFloat("Zvucnik scale", &programState->zvucnikScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat3("Sto position", (float*)&programState->stoPosition);
        ImGui::DragFloat("Sto scale", &programState->stoScale, 0.05, 0.1, 4.0);
        ImGui::DragFloat3("Sto rotation", (float*)&programState->stoRotation);
        ImGui::DragFloat("Sto angle", &programState->stoAngle, 0.05, 0.1, 200.0);

        ImGui::DragFloat3("Gramofon position", (float*)&programState->gramofonPosition);
        ImGui::DragFloat("Gramofon scale", &programState->gramofonScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS){
        programState->CameraMouseMovementUpdateEnabled = !programState->CameraMouseMovementUpdateEnabled;
    }
    if (key == GLFW_KEY_B && action == GLFW_PRESS){
        blinn = !blinn;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !hdrKeyPressed)
    {
        hdr = !hdr;
        hdrKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        hdrKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (exposure > 0.0f)
            exposure -= 0.05f;
        else
            exposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        exposure += 0.05f;
    }
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}