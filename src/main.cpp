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

void renderQuad();

unsigned int loadTexture(char const * path);

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 800;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool hdr = true;
bool hdrKeyPressed = false;
bool bloom = true;
bool bloomKeyPressed = false;
float exposure = 1.0f;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


    Camera camera(glm::vec3(0.0f, 0.0f, -0.3f));

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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "The Day after", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(false);


    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs",
                     "resources/shaders/2.model_lighting.fs");
    Shader blendingShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader hdrShader("resources/shaders/hdr.vs","resources/shaders/hdr.fs");
    Shader bloomShader("resources/shaders/bloom.vs","resources/shaders/bloom.fs");
    //skybox
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/posx.jpg"),
                    FileSystem::getPath("resources/textures/skybox/negx.jpg"),
                    FileSystem::getPath("resources/textures/skybox/posy.jpg"),
                    FileSystem::getPath("resources/textures/skybox/negy.jpg"),
                    FileSystem::getPath("resources/textures/skybox/posz.jpg"),
                    FileSystem::getPath("resources/textures/skybox/negz.jpg")
            };
    unsigned int cubemapTexture = loadCubemap(faces);

    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f,-0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

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

    int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/2k32.png").c_str());

    vector<glm::vec3> graffiti{
        glm::vec3(-7.5f, -1.5f, -26.27f),
        glm::vec3(-2.9f, -1.75f,  11.85f),
        glm::vec3(-11.78f, -2.25f,  -2.5f),
        glm::vec3(0.0f, -2.5f, 30.0f)
    };
    //z=5.0f prva granica
    //z=-12.0f druga

    blendingShader.use();
    blendingShader.setInt("texture1", 0);
//**********************************************************************************************
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // color attachments we'll use for rendering
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }
//***********************************************************************************************

    bloomShader.use();
    bloomShader.setInt("image", 0);

    hdrShader.use();
    hdrShader.setInt("hdrBuffer", 0);
    hdrShader.setInt("bloomBlur", 1);
    // load models
    // -----------
    Model ourModel("resources/objects/scene/scene.obj");
    ourModel.SetShaderTextureNamePrefix("material.");
    Model billboardModel("resources/objects/billboard/billboard.obj");
    billboardModel.SetShaderTextureNamePrefix("material.");
    Model streetLight("resources/objects/streetlight/streetlight.obj");
    streetLight.SetShaderTextureNamePrefix("material.");
    Model streetLight2("resources/objects/streetlight/streetlight.obj");
    streetLight2.SetShaderTextureNamePrefix("material.");
    Model shrub("resources/objects/cliff_shurb/cliff_shurb.obj");
    shrub.SetShaderTextureNamePrefix("material.");

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //pointLight.position = glm::vec3(-4.0f , 8.0f, 2.0f);
        float linear=0.35f;
        float quadratic=0.44f;
        glm::vec3 ambient=glm::vec3(0.2f, 0.2f, 0.2f);

        ourShader.setVec3("pointLight[0].position", glm::vec3(33.8f, 1.15f, -2.8f));
        ourShader.setVec3("pointLight[0].ambient", ambient);
        ourShader.setVec3("pointLight[0].diffuse", glm::vec3(1.7f, 1.7, 1.0));
        ourShader.setVec3("pointLight[0].specular", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setFloat("pointLight[0].constant", 1.0f);
        ourShader.setFloat("pointLight[0].linear", linear);
        ourShader.setFloat("pointLight[0].quadratic",  quadratic);

        ourShader.setVec3("pointLight[1].position", glm::vec3(-21.91f, 3.62f,  2.65f));
        ourShader.setVec3("pointLight[1].ambient", ambient);
        ourShader.setVec3("pointLight[1].diffuse", glm::vec3(0.4, 0.4, 0.4));
        ourShader.setVec3("pointLight[1].specular", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setFloat("pointLight[1].constant", 1.0f);
        ourShader.setFloat("pointLight[1].linear", linear);
        ourShader.setFloat("pointLight[1].quadratic",  quadratic);

        ourShader.setVec3("pointLight[2].position", glm::vec3(19.25f, 3.62f,  3.2f));
        ourShader.setVec3("pointLight[2].ambient",ambient);
        ourShader.setVec3("pointLight[2].diffuse", glm::vec3(0.4, 0.4, 0.4));
        ourShader.setVec3("pointLight[2].specular", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setFloat("pointLight[2].constant", 1.0f);
        ourShader.setFloat("pointLight[2].linear", linear);
        ourShader.setFloat("pointLight[2].quadratic",  quadratic);

        ourShader.setVec3("pointLight[3].position", glm::vec3(35.15f, 3.62f,  3.0f));
        ourShader.setVec3("pointLight[3].ambient", ambient);
        ourShader.setVec3("pointLight[3].diffuse", glm::vec3(0.4, 0.4, 0.4));
        ourShader.setVec3("pointLight[3].specular", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setFloat("pointLight[3].constant", 1.0f);
        ourShader.setFloat("pointLight[3].linear", linear);
        ourShader.setFloat("pointLight[3].quadratic",  quadratic);

        ourShader.setVec3("pointLight[4].position", glm::vec3(-4.6f, 1.15f, -2.6f));
        ourShader.setVec3("pointLight[4].ambient", ambient);
        ourShader.setVec3("pointLight[4].diffuse", glm::vec3(1.7, 1.7, 0.4));
        ourShader.setVec3("pointLight[4].specular", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setFloat("pointLight[4].constant", 1.0f);
        ourShader.setFloat("pointLight[4].linear", linear);
        ourShader.setFloat("pointLight[4].quadratic",  quadratic);

        ourShader.setVec3("pointLight[5].position", glm::vec3(-5.9f, 1.32f,  -11.75f));
        ourShader.setVec3("pointLight[5].ambient", ambient);
        ourShader.setVec3("pointLight[5].diffuse", glm::vec3(0.4, 0.4, 0.4));
        ourShader.setVec3("pointLight[5].specular", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setFloat("pointLight[5].constant", 1.0f);
        ourShader.setFloat("pointLight[5].linear", linear);
        ourShader.setFloat("pointLight[5].quadratic",  quadratic);

        ourShader.setVec3("pointLight[6].position", glm::vec3(-19.7f, 3.62f,  -9.35f));
        ourShader.setVec3("pointLight[6].ambient", ambient);
        ourShader.setVec3("pointLight[6].diffuse", glm::vec3(0.4, 0.4, 0.4));
        ourShader.setVec3("pointLight[6].specular", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setFloat("pointLight[6].constant", 1.0f);
        ourShader.setFloat("pointLight[6].linear", linear);
        ourShader.setFloat("pointLight[6].quadratic",  quadratic);

        ourShader.setVec3("pointLight[7].position", glm::vec3(20.95f, 3.62f,  -9.44f));
        ourShader.setVec3("pointLight[7].ambient", ambient);
        ourShader.setVec3("pointLight[7].diffuse", glm::vec3(0.4, 0.4, 0.4));
        ourShader.setVec3("pointLight[7].specular", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setFloat("pointLight[7].constant", 1.0f);
        ourShader.setFloat("pointLight[7].linear", linear);
        ourShader.setFloat("pointLight[7].quadratic",  quadratic);

        ourShader.setVec3("viewPosition", camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        //glEnable(GL_CULL_FACE);
        //glCullFace(GL_BACK);
        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(0.0f, -3.0f, 0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.05f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);

        float z=5.0f;
        bool forward=true;
        if(forward){
            z-=glfwGetTime();
            if(z<-12.0f)
                forward=false;
        }
        else{
            z+=glfwGetTime();
            if(z>5.0f)
                forward=true;
        }

        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(-2.5f, -3.0f+abs(sin(glfwGetTime())), z));
        model = glm::scale(model, glm::vec3(0.75f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        shrub.Draw(ourShader);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(11.0f, 1.5f, -11.75f));
        model = glm::scale(model, glm::vec3(0.2f, 0.2f,0.2f));
        ourShader.setMat4("model", model);
        billboardModel.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(35.0f, -3.0f, -3.0f));
        model = glm::scale(model, glm::vec3(1.45f,1.45f,1.45f));
        ourShader.setMat4("model", model);
        streetLight.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(-5.5f, -3.0f, -2.8f));
        model = glm::scale(model, glm::vec3(1.45f,1.45f,1.45f));
        ourShader.setMat4("model", model);
        streetLight2.Draw(ourShader);
        glDisable(GL_CULL_FACE);

        blendingShader.use();
        projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        view = camera.GetViewMatrix();
        model = glm::mat4(1.0f);
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        for (unsigned int i = 0; i < graffiti.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, graffiti[i]);
            model = glm::scale(model, glm::vec3(2.5f));
            if(i==2) {
                model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, 1.0, 0.0));
                model = glm::scale(model, glm::vec3(0.5f));
            }
            if(i==1) {
                model = glm::scale(model, glm::vec3(0.45f));
            }
            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
//*****************************************************************
// draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        //---------------------------------------------------------------------------
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        bloomShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            bloomShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);

            renderQuad();

            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        //----------------------------------------------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //--------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        hdrShader.setBool("hdr", hdr);
        hdrShader.setBool("bloom", bloom);
        hdrShader.setFloat("exposure", exposure);
        renderQuad();
        //--------------------------------------------------------------------------------
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


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && !hdrKeyPressed)
    {
        hdr = !hdr;
        hdrKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE)
    {
        hdrKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (exposure > 0.0f)
            exposure -= 0.005f;
        else
            exposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        exposure += 0.005f;
    }
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

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);

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

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}