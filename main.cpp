#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <vector>
#include <iostream>
#include <random>
#include <sstream>
#include <fstream>
#include <string>

#include "utils/Timer.h"
#include "utils/Shader.h"
#include "utils/texture.h"
#include "utils/objloader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "utils/stb_image.h"

#define TINYPLY_IMPLEMENTATION
#include <tinyply.h>

// Settings

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

static void error_callback(int /*error*/, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

GLuint MakeShader(GLuint t, std::string path)
{
	std::cout << path << std::endl;
	std::ifstream file(path.c_str(), std::ios::in);
	std::ostringstream contents;
	contents << file.rdbuf();
	file.close();
	const auto content = contents.str();
	std::cout << content << std::endl;
	const auto s = glCreateShader(t);
	GLint sizes[] = { (GLint)content.size() };
	const auto data = content.data();
	glShaderSource(s, 1, &data, sizes);
	glCompileShader(s);
	GLint success;
	glGetShaderiv(s, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		GLsizei l;
		glGetShaderInfoLog(s, 512, &l, infoLog);
		std::cout << infoLog << std::endl;
	}
	return s;
}

GLuint AttachAndLink(std::vector<GLuint> shaders)
{
	const auto prg = glCreateProgram();
	for (const auto s : shaders) glAttachShader(prg, s);
	glLinkProgram(prg);
	GLint success;
	glGetProgramiv(prg, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		GLsizei l;
		glGetProgramInfoLog(prg, 512, &l, infoLog);
		std::cout << infoLog << std::endl;
	}
	return prg;
}

void APIENTRY opengl_error_callback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	std::cout << message << std::endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

static void processCameraInput(GLFWwindow* window, float deltaTime);
unsigned int loadTexture(const char* path);

void renderScene(const Shader& shader, float rotate);
void renderCube();
void renderMajoraMask();
void renderQuad();

glm::vec3 position = glm::vec3(0.f, 2.f, 10.f);
glm::vec2 lmp;
float pitch = 0.f, yaw = 0.f;
float speed = 5.f, mouseSpeed = 0.005;

// Timing

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Meshes

GLuint planeVAO;

unsigned int majoraTexture;

std::vector<glm::vec3> majoraVertices;
std::vector<glm::vec2> majoraUvs;
std::vector<glm::vec3> majoraNormals;

int main(void)
{
	if (!glfwInit())
		exit(EXIT_FAILURE);

	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Gamagora", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);
	// glfwSwapInterval(1);

	// NOTE: OpenGL error checks have been omitted for brevity

	if (!gladLoadGL()) {
		std::cerr << "Something went wrong!" << std::endl;
		exit(-1);
	}

	glEnable(GL_DEPTH_TEST);

	// Shader

	const auto vertex = MakeShader(GL_VERTEX_SHADER, "shaders/shader.vert");
	const auto fragment = MakeShader(GL_FRAGMENT_SHADER, "shaders/shader.frag");
	const auto program = AttachAndLink({ vertex, fragment });

	const auto vertexQuad = MakeShader(GL_VERTEX_SHADER, "shaders/shaderQuad.vert");
	const auto fragmentQuad = MakeShader(GL_FRAGMENT_SHADER, "shaders/shaderQuad.frag");
	const auto programQuad = AttachAndLink({ vertexQuad, fragmentQuad });

	// Plane

	float planeVertices[] = {
		 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
		-25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
		-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
		 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
		-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
		 25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
	};

	GLuint planeVBO;
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

	unsigned int woodTexture = loadTexture("assets/grass.png");

	// Configure depth map FBO

	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	// Create depth texture

	unsigned int depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Attach depth texture as FBO's depth buffer

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Build and compile shaders

	Shader shader("shaders/shadow_mapping.vert", "shaders/shadow_mapping.frag");
	Shader simpleDepthShader("shaders/shadow_mapping_depth.vert", "shaders/shadow_mapping_depth.frag");
	Shader debugDepthQuad("shaders/debug_quad.vert", "shaders/debug_quad_depth.frag");

	// Shader configuration

	shader.use();
	shader.setInt("diffuseTexture", 0);
	shader.setInt("shadowMap", 1);
	debugDepthQuad.use();
	debugDepthQuad.setInt("depthMap", 0);

	// Lighting info

	glm::vec3 lightPos(-2.0f, 4.0f, 1.0f);

	// Callbacks

	glDebugMessageCallback(opengl_error_callback, nullptr);
	// glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	glm::mat4 view, projection;

	majoraTexture = loadTexture("assets/majora.png");

	loadOBJ("assets/majora.obj", majoraVertices, majoraUvs, majoraNormals);

	float rotate = 0.f;

	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processCameraInput(window, deltaTime);

		glClearColor(124.f / 255.f, 173.f / 255.f, 206.f / 255.f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;
		float near_plane = 1.0f, far_plane = 7.5f;
		lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		lightSpaceMatrix = lightProjection * lightView;
		simpleDepthShader.use();
		simpleDepthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, woodTexture);
		renderScene(simpleDepthShader, rotate);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();

		int frameWidth, frameHeight;
		glfwGetFramebufferSize(window, &frameWidth, &frameHeight);

		view = glm::rotate(pitch, glm::vec3(1.f, 0.f, 0.f)) * glm::rotate(yaw, glm::vec3(0.f, 1.f, 0.f)) * glm::translate(-position);
		projection = glm::perspective(glm::radians(45.f), (float)frameWidth / (float)frameHeight, 0.1f, 100.f);

		shader.setMat4("projection", projection);
		shader.setMat4("view", view);

		// set light uniforms
		shader.setVec3("viewPos", position);
		shader.setVec3("lightPos", lightPos);
		shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		rotate += 0.01f;
		if (rotate >= 360.f) rotate -= 360.f;

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, woodTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		renderScene(shader, rotate);

		debugDepthQuad.use();
		debugDepthQuad.setFloat("near_plane", near_plane);
		debugDepthQuad.setFloat("far_plane", far_plane);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMap);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &planeVBO);
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
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

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
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

static void processCameraInput(GLFWwindow* window, float deltaTime)
{
	glm::mat4 pitchRotation = glm::rotate(-pitch, glm::vec3(1.f, 0.f, 0.f));
	glm::mat4 yawRotation = glm::rotate(-yaw, glm::vec3(0.f, 1.f, 0.f));

	glm::vec3 forward = yawRotation * pitchRotation * glm::vec4(0.f, 0.f, -1.f, 0.f);
	glm::vec3 right = yawRotation * glm::vec4(1.f, 0.f, 0.f, 0.f);
	glm::vec3 up = glm::vec3(0, 1, 0);

	glm::vec3 velocity = glm::vec3();

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) velocity += forward;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) velocity -= forward;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) velocity -= right;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) velocity += right;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) velocity -= up;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) velocity += up;

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // lock cursor

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // unlock cursor

	double x, y;
	glfwGetCursorPos(window, &x, &y);
	glm::vec2 mp = glm::vec2(x, y);

	if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
		glm::vec2 dmp = mp - lmp;
		pitch += dmp.y * mouseSpeed;
		yaw += dmp.x * mouseSpeed;
	}

	lmp = mp;

	if (velocity.x || velocity.y || velocity.z)
		position += glm::normalize(velocity) * speed * deltaTime;
}

void renderScene(const Shader& shader, float rotate)
{
	// Floor

	glm::mat4 model = glm::mat4(1.0f);
	shader.setMat4("model", model);
	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, majoraTexture);

	// Model

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
	model = glm::scale(model, glm::vec3(0.5f));
	model = glm::rotate(model, rotate, glm::vec3(0.f, 1.f, 0.f));
	shader.setMat4("model", model);
	renderMajoraMask();
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;

void renderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float* vertices = new float[1]();
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

GLuint majoraVAO = 0;
GLuint majoraVBO = 0;
GLuint majoraUVsBuffer = 0;
GLuint majoraNormalsBuffer = 0;

void renderMajoraMask()
{
	// initialize (if necessary)
	if (majoraVAO == 0)
	{
		glGenVertexArrays(1, &majoraVAO);

		glGenBuffers(1, &majoraVBO);
		glBindBuffer(GL_ARRAY_BUFFER, majoraVBO);
		if (!majoraVertices.empty())
			glBufferData(GL_ARRAY_BUFFER, majoraVertices.size() * sizeof(glm::vec3), &majoraVertices[0], GL_STATIC_DRAW);

		glGenBuffers(1, &majoraNormalsBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, majoraNormalsBuffer);
		if (!majoraNormals.empty())
			glBufferData(GL_ARRAY_BUFFER, majoraNormals.size() * sizeof(glm::vec3), &majoraNormals[0], GL_STATIC_DRAW);

		glGenBuffers(1, &majoraUVsBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, majoraUVsBuffer);
		if (!majoraUvs.empty())
			glBufferData(GL_ARRAY_BUFFER, majoraUvs.size() * sizeof(glm::vec2), &majoraUvs[0], GL_STATIC_DRAW);

		// link vertex attributes
		glBindVertexArray(majoraVAO);

		glBindBuffer(GL_ARRAY_BUFFER, majoraVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, majoraNormalsBuffer);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, majoraUVsBuffer);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	glBindVertexArray(majoraVAO);
	glDrawArrays(GL_TRIANGLES, 0, majoraVertices.size() * 3);
	glBindVertexArray(0);
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
