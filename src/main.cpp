// VulkanEd Source Code
// Wasim Abbas
// http://www.waZim.com
// Copyright (c) 2021
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the 'Software'),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Version: 1.0.0

#include "vulkan/vulkan_rhi.hpp"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <iostream>

static bool         update_animation = true;
static unsigned int render_cycle     = 1;        // 0=Render everything, 1=Render character only, 2=Render skeleton only
static int          focused_bone     = 0;

void key(GLFWwindow *window, int k, int s, int action, int mods)
{
	(void) s;
	(void) mods;

	switch (k)
	{
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		case GLFW_KEY_SPACE:
			if (action == GLFW_PRESS)
				update_animation = !update_animation;
			break;
		case GLFW_KEY_G:
			// if (action == GLFW_PRESS)
			//	client_renderer->reset_frame_time();
			break;
		case GLFW_KEY_W:
			// glfw_camera_zoom_by(3.5f);
			break;
		case GLFW_KEY_S:
			// glfw_camera_zoom_by(-3.5f);
			break;
		case GLFW_KEY_C:
			if (action == GLFW_PRESS)
			{
				render_cycle += 1;
				render_cycle = render_cycle % 3;
			}
			break;
		case GLFW_KEY_R:
			// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case GLFW_KEY_F:
			// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case GLFW_KEY_I:
			if (action == GLFW_PRESS)
			{
				focused_bone++;
			}
			break;
		case GLFW_KEY_O:
			if (action == GLFW_PRESS)
			{
				focused_bone--;

				if (focused_bone < 0)
					focused_bone = 0;
			}
			break;
		default:
			return;
	}
}

class VulkanApplication
{
  public:
	void run()
	{
		init();
		loop();
		shutdown();
	}

  private:
	static void resize(GLFWwindow *window, int width, int height)
	{
		if (width == 0 || height == 0)
			return;

		auto *app = static_cast<VulkanApplication *>(glfwGetWindowUserPointer(window));

		app->m_context->resize();
	}

	void init()
	{
		if (!glfwInit())
		{
			printf("GLFW Init failed, check if you have a working display set!\n");
			exit(EXIT_FAILURE);
		}

		glfwWindowHint(GLFW_DEPTH_BITS, 16);
		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		this->m_window = glfwCreateWindow(1024, 900, "Vulkan Application", nullptr, nullptr);

		if (!this->m_window)
		{
			glfwTerminate();
			printf("GLFW Windows can't be created\n");
			exit(EXIT_FAILURE);
		}

		glfwMakeContextCurrent(this->m_window);
		glfwSwapInterval(0);

		glfwSetKeyCallback(this->m_window, key);
		glfwSetWindowSizeCallback(this->m_window, resize);

		// Lets use this as a user pointer in glfw
		glfwSetWindowUserPointer(this->m_window, this);

		this->m_context = new vkd::Context(this->m_window);
	}

	void loop()
	{
		while (!glfwWindowShouldClose(this->m_window))
		{
			glfwPollEvents();
			this->m_context->draw_frame(update_animation);
		}
	}

	void shutdown()
	{
		delete this->m_context;
		glfwDestroyWindow(this->m_window);

		glfwTerminate();
	}

	GLFWwindow *  m_window{nullptr};
	vkd::Context *m_context{nullptr};
};

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	VulkanApplication app;

	try
	{
		app.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
