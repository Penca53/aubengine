#pragma once

#include <memory>
#include <vector>

#include "Window.h"
#include "RenderAPI.h"

namespace Aubengine
{
	class Application
	{
	public:
		virtual ~Application() = default;

		static Application& GetInstance();


		Application& SetUpdateRate(unsigned int hz);
		long long MillisecondsPerUpdate();
		long long NanosecondsPerUpdate();
		void Run(unsigned int hz);
		void Run();
		std::shared_ptr<Window> CreateWindowOpenGL();

		void PollInput();
		void Update();
		void Render();
	public:
		Application(Application const&) = delete;
		void operator=(Application const&) = delete;
	private:
		Application() = default;

	public:
		unsigned int Hz = 0;
		unsigned int MsPerUpdate = 0;
		std::vector<std::shared_ptr<Window>> Windows;
	};
}