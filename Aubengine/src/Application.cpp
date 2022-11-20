#include "Application.h"

#include <iostream>
#include <chrono>

#include "WindowOpenGL.h"
#include "ResourceManager.h"
#include "Input.h"

namespace Aubengine
{
	Application& Application::GetInstance()
	{
		static Application s;
		return s;
	}

	Application& Application::SetUpdateRate(unsigned int hz)
	{
		Hz = hz;
		return *this;
	}

	long long Application::MillisecondsPerUpdate()
	{
		return 1000LL / Hz;
	}
	long long Application::NanosecondsPerUpdate()
	{
		return 1000000000LL / Hz;
	}

	void Application::Run(unsigned int hz)
	{
		SetUpdateRate(hz);
		Run();
	}
	void Application::Run()
	{
		auto previous = std::chrono::steady_clock::now();
		long long lag = 0;
		while (true)
		{
			bool shouldClose = false;
			for (auto& window : Windows)
			{
				shouldClose |= window->WindowShouldClose();
			}

			if (shouldClose)
			{
				break;
			}

			auto current = std::chrono::steady_clock::now();

			long long elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(current - previous).count();

			previous = current;
			lag += elapsed;

			while (lag >= NanosecondsPerUpdate())
			{
				PollInput();
				Update();
				lag -= NanosecondsPerUpdate();
			}

			Render();
		}

		for (auto& window : Windows)
		{
			ResourceManager::Clear((GladGLContext*)window->GetContext());
		}
	}

	void Application::PollInput()
	{
		for (auto& window : Windows)
		{
			window->Begin();
		}
		Input::PollInput();
	}


	void Application::Update()
	{
		for (auto& window : Windows)
		{
			window->Update();
		}
	}

	void Application::Render()
	{
		for (auto& window : Windows)
		{
			window->Render();
			window->End();
		}
	}

	std::shared_ptr<Window> Application::CreateWindowOpenGL()
	{
		std::shared_ptr<Window> window = std::make_shared<WindowOpenGL>();
		Windows.push_back(window);
		return window;
	}
}