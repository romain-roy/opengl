#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "utils/Timer.h"
#include "utils/texture.h"
#include "utils/objloader.hpp"

#include <vector>
#include <iostream>
#include <random>
#include <sstream>
#include <fstream>
#include <string>

#define TINYPLY_IMPLEMENTATION
#include <tinyply.h>

int width = 960;
int height = 720;

glm::vec3 position = glm::vec3(0.f, 0.f, 12.f);
float pitch = 0.f, yaw = 0.f;
float speed = 5.f, mouseSpeed = 0.005;

glm::vec2 lmp;

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
	for (const auto s : shaders)
	{
		glAttachShader(prg, s);
	}

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

void debugFramebuffer(GLuint framebuffer);

void drawQuad(GLFWwindow* window, GLuint shader, glm::vec2 position, glm::vec2 size, GLuint texture = 0);

int main(void)
{
	if (!glfwInit())
		exit(EXIT_FAILURE);

	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

	window = glfwCreateWindow(width, height, "OpenGL Gamagora", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// NOTE: OpenGL error checks have been omitted for brevity

	if (!gladLoadGL()) {
		std::cerr << "Something went wrong!" << std::endl;
		exit(-1);
	}

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	loadOBJ("assets/majora.obj", vertices, uvs, normals);

	// Callbacks

	glDebugMessageCallback(opengl_error_callback, nullptr);
	// glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	// Shader

	const auto vertex = MakeShader(GL_VERTEX_SHADER, "shaders/shader.vert");
	const auto fragment = MakeShader(GL_FRAGMENT_SHADER, "shaders/shader.frag");
	const auto program = AttachAndLink({ vertex, fragment });

	const auto vertexQuad = MakeShader(GL_VERTEX_SHADER, "shaders/shaderQuad.vert");
	const auto fragmentQuad = MakeShader(GL_FRAGMENT_SHADER, "shaders/shaderQuad.frag");
	const auto programQuad = AttachAndLink({ vertexQuad, fragmentQuad });

	// Vertex Arrays

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Buffers

	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	if (!vertices.empty())
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	GLuint uvBuffer;
	glGenBuffers(1, &uvBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	if (!uvs.empty())
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

	GLuint normalBuffer;
	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	if (!normals.empty())
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);

	Image myTexture = LoadImage("assets/majora.bmp");

	GLuint textureBuffer;
	glCreateTextures(GL_TEXTURE_2D, 1, &textureBuffer);
	glTextureStorage2D(textureBuffer, 1, GL_RGB8, myTexture.width, myTexture.height);
	glTextureSubImage2D(textureBuffer, 0, 0, 0, myTexture.width, myTexture.height, GL_RGB, GL_UNSIGNED_BYTE, myTexture.data.data());

	unsigned int fboWidth = width * 4;
	unsigned int fboHeight = height * 4;

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	GLuint colorTexture;
	glCreateTextures(GL_TEXTURE_2D, 1, &colorTexture);
	glTextureStorage2D(colorTexture, 1, GL_RGB8, fboWidth, fboHeight);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorTexture, 0);

	GLuint depthTexture;
	glCreateTextures(GL_TEXTURE_2D, 1, &depthTexture);
	glTextureStorage2D(depthTexture, 1, GL_DEPTH_COMPONENT16, fboWidth, fboHeight);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);

	debugFramebuffer(fbo);
 
	// Bindings
	{
		int location = glGetAttribLocation(program, "position");
		glEnableVertexAttribArray(location);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	}

	{
		int location = glGetAttribLocation(program, "uv");
		glEnableVertexAttribArray(location);
		glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
		glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
	}

	{
		int location = glGetAttribLocation(program, "normal");
		glEnableVertexAttribArray(location);
		glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
		glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	}

	glm::mat4 scalingMatrix, translateMatrix, rotateMatrix, model, view, projection;
	GLuint MatrixID;

	float rotateXY = 0.f;
	Timer dt, age;

	// glClearColor(1.0, 1.0, 1.0, 1.0); // Fond blanc

	while (!glfwWindowShouldClose(window))
	{
		float deltaTime = dt.elapsed(); // in seconds
		dt.reset();
		processCameraInput(window, deltaTime);

		int frameWidth, frameHeight;
		glfwGetFramebufferSize(window, &frameWidth, &frameHeight);

		// Draw to custom FBO
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, fboWidth, fboHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// rotateXY += 0.01f;
		if (rotateXY >= 360.f) rotateXY -= 360.f;

		scalingMatrix = glm::scale(glm::vec3(1.f, 1.f, 1.f));
		translateMatrix = glm::translate(glm::vec3(0.f, 1.f, 0.f));
		rotateMatrix = glm::rotate(rotateXY, glm::vec3(0.f, 1.f, 0.f));

		model = translateMatrix * rotateMatrix * scalingMatrix;

		view = glm::rotate(pitch, glm::vec3(1.f, 0.f, 0.f)) * glm::rotate(yaw, glm::vec3(0.f, 1.f, 0.f)) * glm::translate(-position);

		projection = glm::perspective(glm::radians(45.f), (float)frameWidth / (float)frameHeight, 0.1f, 100.f);

		glUseProgram(program);
		glBindTextureUnit(1, textureBuffer);
		glProgramUniform1i(program, glGetUniformLocation(program, "textureSampler"), 1);
		glProgramUniformMatrix4fv(program, glGetUniformLocation(program, "projection"), 1, GL_FALSE, &projection[0][0]);
		glProgramUniformMatrix4fv(program, glGetUniformLocation(program, "view"), 1, GL_FALSE, &view[0][0]);
		glProgramUniformMatrix4fv(program, glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);

		// Light on camera

		glProgramUniform3f(program, glGetUniformLocation(program, "light.position"), 3 * std::cos(age.elapsed() * 3), -15, 30 - 3 * std::sin(age.elapsed() * 3));
		glProgramUniform3f(program, glGetUniformLocation(program, "light.color"), std::abs(std::sin(age.elapsed())) * 3, std::abs(std::sin(age.elapsed())) / 2, 0);

		glBindVertexArray(VertexArrayID);
		glDrawArrays(GL_TRIANGLES, 0, vertices.size() * 3);

		// Draw on Default Frambuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, frameWidth, frameHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawQuad(window, programQuad, glm::vec2(0, 0), glm::vec2(frameWidth, frameHeight), colorTexture);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

void drawQuad(GLFWwindow* window, GLuint shader, glm::vec2 position, glm::vec2 size, GLuint texture)
{
	int frameWidth, frameHeight;
	glfwGetFramebufferSize(window, &frameWidth, &frameHeight);
	glm::mat4 projection = glm::ortho(0.f, (float)frameWidth, (float)frameHeight, 0.f);

	float l = position.x;			// left
	float r = position.x + size.x;	// right
	float t = position.y;			// top
	float b = position.y + size.y;	// bottom

	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> uvs;

	positions.push_back(glm::vec3(l, b, 0)); uvs.push_back(glm::vec2(0, 0)); // Vertex 0
	positions.push_back(glm::vec3(r, b, 0)); uvs.push_back(glm::vec2(1, 0)); // Vertex 1
	positions.push_back(glm::vec3(r, t, 0)); uvs.push_back(glm::vec2(1, 1)); // Vertex 2
	positions.push_back(glm::vec3(l, t, 0)); uvs.push_back(glm::vec2(0, 1)); // Vertex 3

	std::vector<unsigned int> indices;
	// Triangle 1
	indices.push_back(0); indices.push_back(1); indices.push_back(2);
	// Triangle 2
	indices.push_back(2); indices.push_back(3);	indices.push_back(0);

	// Index Buffer
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(glm::vec3), indices.data(), GL_STATIC_DRAW);

	// Vertex Array
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	GLuint uvBuffer;
	glGenBuffers(1, &uvBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec3), uvs.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

	// Configure Shader
	glUseProgram(shader);
	glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, &projection[0][0]);
	glBindTextureUnit(0, texture);
	glUniform1i(glGetUniformLocation(shader, "textureSampler"), 0);

	// Draw Call
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vertexBuffer);
	glDeleteBuffers(1, &uvBuffer);
}

void debugFramebuffer(GLuint framebuffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	switch (glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
	case GL_FRAMEBUFFER_COMPLETE:
		std::cout << "Framebuffer is complete" << std::endl;
		break;

	case GL_FRAMEBUFFER_UNDEFINED:
		std::cout << "Framebuffer doesn't exist" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		std::cout << "Framebuffer contains at least one attachement that is incomplete" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		std::cout << "Framebuffer has no attached Textures" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		std::cout << "Framebuffer draw target is incomplete" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		std::cout << "Framebuffer read target is incomplete" << std::endl;
		break;

	default:
		std::cout << "Framebuffer is unsupported" << std::endl;
		break;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Press any key to Continue..." << std::endl;
		std::cin.get();
	}
}
