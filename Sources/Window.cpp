#include "Window.h"

using namespace std;


	Window::Window(unsigned int width, unsigned int height, const char* title)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	}

	Window::~Window()
	{
		m_Callbacks.erase(m_Window);
		glfwDestroyWindow(m_Window);
		glfwTerminate();
		m_Window = nullptr;
	}

	bool Window::shouldClose()
	{
		return glfwWindowShouldClose(m_Window);
	}

	void Window::pollEvents()
	{
		glfwPollEvents();
	}

	GLFWwindow& Window::getWindow()
	{
		assert(m_Window != nullptr);
		return *m_Window;
	}

	void Window::setKeyCallback( function<void(Window&, int, int, int, int)> callback)
	{
		m_Callbacks[m_Window].m_Instance = this;

		m_Callbacks[m_Window].m_KeyCallback = callback;
		glfwSetKeyCallback(m_Window, onKeyCallback);
	}

	void Window::setMouseButtonCallback(function<void(Window&, int, int, int)> callback)
	{
		m_Callbacks[m_Window].m_Instance = this;

		m_Callbacks[m_Window].m_MouseButtonCallback = callback;
		glfwSetMouseButtonCallback(m_Window, onMouseButtonCallback);
	}

	void Window::setMouseMoveCallback(function<void(Window&, double, double)> callback)
	{
		m_Callbacks[m_Window].m_Instance = this;

		m_Callbacks[m_Window].m_MouseMoveCallback = callback;
		glfwSetCursorPosCallback(m_Window, onMouseMoveCallback);
	}

	void Window::onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		auto it = m_Callbacks.find(window);
		if (it != end(m_Callbacks) && it->second.m_KeyCallback!=nullptr)
		{
			it->second.m_KeyCallback(*it->second.m_Instance, key, scancode, action, mods);
		}
	}
	
	void Window::onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		auto it = m_Callbacks.find(window);
		if (it != end(m_Callbacks) && it->second.m_MouseButtonCallback != nullptr)
		{
			it->second.m_MouseButtonCallback(*it->second.m_Instance, button, action, mods);
		}
	}

	void Window::onMouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto it = m_Callbacks.find(window);
		if (it != end(m_Callbacks) && it->second.m_MouseMoveCallback != nullptr)
		{
			it->second.m_MouseMoveCallback(*it->second.m_Instance,  xpos, ypos);
		}
	}

unordered_map< GLFWwindow*, Window::GLFWCallbacks> Window::m_Callbacks;

