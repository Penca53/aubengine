# Aubengine

Aubengine is an open source C++ game engine. Currently it is being developed on Windows MSVC, but the core libraries are also available for other platforms such as Mac and Linux, with little tweaks.

## Code architecture

Aubengine is built in a hot-swap modular way, enabling users to pick a built-in module implementation, or a custom-built solution, thanks to extra layers of abstraction. For instance, the Input module and the Window module have an OpenGL (Glad2 + GLFW) oriented implementation, but the architecture allows for different implementations, all usable at runtime.

## Code based

Aubengine is a code based game engine, with no GUI or custom tool. Every operation is made in a user-made C++ project which includes the engine's static libraries.

Documentation will be a future addition. 

## Showcase
![showcase2](https://user-images.githubusercontent.com/38438406/202306014-c8dab8a4-8fdc-427e-96cb-ddcc2fafd37f.gif)



## Usage

- Clone the repository 
- Open the .sln file using VisualStudio (make sure you have the C++ development module installed)
- You'll find 2 projects inside the solution
  - Aubengine: the core engine - Application, game loop, rendering, scene management etc...
  - Tester: a sample project which includes the engine's libraries

## Contacts

If you have any question, feel free to contact me.

