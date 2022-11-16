#include <iostream>
#include <functional>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shader.hpp"
#include "objects.hpp"
// kuemmert sich um die Pfade zu den Shadern und Texturen
#include "asset.hpp"
#include "objloader.hpp"
#include "texture.hpp"

glm::mat4 Projection{glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f)};
glm::mat4 View {glm::lookAt(glm::vec3(0, 0, -5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0))};
glm::mat4 Model{};
glm::vec3 angle{};
glm::vec4 robot_modules{};
glm::vec3 pos{};
uint module{3};

void error_callback(int error, const char *description)
{
	std::cerr << error << '\n';
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, GL_TRUE);
		break;
	case GLFW_KEY_UP:
		pos.y += 0.1f;
		break;
	case GLFW_KEY_DOWN:
		pos.y -= 0.1f;
		break;
	case GLFW_KEY_LEFT:
		pos.x += 0.1f;
		break;
	case GLFW_KEY_RIGHT:
		pos.x -= 0.1f;
		break;
	case GLFW_KEY_0:
		pos.z += 0.1f;
		break;
	case GLFW_KEY_9:
		pos.z -= 0.1f;
		break;
	case GLFW_KEY_SPACE:
		robot_modules.w += 0.05f;
		break;
	case GLFW_KEY_1:
		angle.x += 0.1f;
		break;
	case GLFW_KEY_2:
		angle.y += 0.1f;
		break;
	case GLFW_KEY_3:
		angle.z += 0.1f;
		break;
	case GLFW_KEY_A:
		module = 1;
		std::cout << "Modul: " << module << '\n';
		break;
	case GLFW_KEY_S:
		module = 2;
		std::cout << "Modul: " << module << '\n';
		break;
	case GLFW_KEY_D:
		module = 3;
		std::cout << "Modul: " << module << '\n';
		break;
	case GLFW_KEY_J:
		if (module == 1)
			robot_modules.x += 0.05f;
		else if (module == 2)
			robot_modules.y += 0.05f;
		else if (module == 3)
			robot_modules.z += 0.05f;
		break;
	default:
		break;
	}
}

void sendMVP(GLuint programID)
{
	glm::mat4 MVP{Projection * View * Model};
	glUniformMatrix4fv(glGetUniformLocation(programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(programID, "M"), 1, GL_FALSE, &Model[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(programID, "V"), 1, GL_FALSE, &View[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(programID, "P"), 1, GL_FALSE, &Projection[0][0]);
}

void save_and_restore(const std::function<void()>& callback)
{
	glm::mat4 Save{Model};
	callback();
	Model = Save;
}

void draw_coordinate_system(GLuint programID)
{
	auto draw_axis{[programID](const glm::vec3& scale_vector) -> void {
		save_and_restore([&scale_vector, programID]() -> void {
			Model = glm::scale(Model, scale_vector);
			sendMVP(programID);
			drawCube();
		});
	}};
	float axis_length{10.0f};
	float axis_width{0.005f};
	draw_axis(glm::vec3(axis_length, axis_width, axis_width)); // x
	draw_axis(glm::vec3(axis_width, axis_length, axis_width)); // y
	draw_axis(glm::vec3(axis_width, axis_width, axis_length)); // z
}

void draw_robot(float height, GLuint programID)
{
	auto draw_module{[height, programID]() -> void {
		save_and_restore([height, programID]() -> void {
			Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, height));
			Model = glm::scale(Model, glm::vec3(0.2f, 0.2f, height));
			sendMVP(programID);
			drawSphere(10, 10);
		});
	}};
	save_and_restore([&draw_module, height, programID]() -> void {
		Model = glm::rotate(Model, robot_modules.z, glm::vec3(1.0f, 0.0f, 0.0f));
		draw_module();
		Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 2 * height));
		Model = glm::rotate(Model, robot_modules.y, glm::vec3(1.0f, 0.0f, 0.0f));
		draw_module();
		Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 2 * height));
		Model = glm::rotate(Model, robot_modules.x, glm::vec3(1.0f, 0.0f, 0.0f));
		draw_module();
		glm::vec4 lightPos{Model * glm::vec4(0.0f, 0.0f, 2 * height, 1.0f)};
		glUniform3f(glGetUniformLocation(programID, "LightPosition_worldspace"), lightPos.x, lightPos.y, lightPos.z);
	});
}

void write_data(GLuint* buffer_ptr, GLsizeiptr buffer_size, const void* buffer_data, GLuint attrib_array_index)
{
	glGenBuffers(1, buffer_ptr);
	glBindBuffer(GL_ARRAY_BUFFER, *buffer_ptr);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, buffer_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(attrib_array_index);

}

int main(void)
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW\n";
		exit(EXIT_FAILURE);
	}
	GLFWwindow *window = glfwCreateWindow(1024, 768, "CGTutorial", NULL, NULL);
	if (!window)
	{

		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

	glfwSetErrorCallback(error_callback);
	glewExperimental = true;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to initialize GLEW\n";
		return -1;
	}
	
	GLuint programID{LoadShaders(SHADER_DIR "/StandardShading.vertexshader", SHADER_DIR "/StandardShading.fragmentshader")};
	glUseProgram(programID);
	GLuint VertexArrayIDTeapot{};
	glGenVertexArrays(1, &VertexArrayIDTeapot);
	glBindVertexArray(VertexArrayIDTeapot);

	std::vector<glm::vec3> normals{};
	GLuint normalbuffer{};
	std::vector<glm::vec3> vertices{};
	GLuint vertexbuffer{};
	std::vector<glm::vec2> uvs{};
	GLuint uvbuffer{};
	
	loadOBJ(RESOURCES_DIR "/teapot.obj", vertices, uvs, normals);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, loadBMP_custom(RESOURCES_DIR "/mandrill.bmp"));

	write_data(&normalbuffer, normals.size() * sizeof(glm::vec3), (const void*)&normals[0], 2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	write_data(&vertexbuffer, vertices.size() * sizeof(glm::vec3), (const void*)&vertices[0], 0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	write_data(&uvbuffer, uvs.size() * sizeof(glm::vec2), (const void*)&uvs[0], 1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
	glUniform1i(glGetUniformLocation(programID, "myTextureSampler"), 0);

	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		Model = glm::mat4(1.0f);
		Model = glm::translate(Model, glm::vec3(pos.x, 0.0f, 0.0f));
		Model = glm::translate(Model, glm::vec3(0.0f, pos.y, 0.0f));
		Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, pos.z));
		Model = glm::rotate(Model, angle.x, glm::vec3(1.0f, 0.0f, 0.0f));
		Model = glm::rotate(Model, angle.y, glm::vec3(0.0f, 1.0f, 0.0f));
		Model = glm::rotate(Model, angle.z, glm::vec3(0.0f, 0.0f, 1.0f));
		save_and_restore([VertexArrayIDTeapot, programID, &vertices]() -> void {
			Model = glm::translate(Model, glm::vec3(1.5, 0.0, 0.0));
			Model = glm::scale(Model, glm::vec3(1.0 / 1000.0, 1.0 / 1000.0, 1.0 / 1000.0));
			sendMVP(programID);
			glBindVertexArray(VertexArrayIDTeapot);
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());
			Model = glm::scale(Model, glm::vec3(0.5, 0.5, 0.5));
			sendMVP(programID);
		});
		draw_coordinate_system(programID);
		Model = glm::rotate(Model, robot_modules.w, glm::vec3(0.0f, 0.0f, 1.0f));
		draw_robot(0.5f, programID);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glDeleteProgram(programID);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glfwTerminate();
	return 0;
}