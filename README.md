# Nawia: Slavic Action-RPG

![Language](https://img.shields.io/badge/language-C%2B%2B20-blue.svg)
![Library](https://img.shields.io/badge/library-SDL3-orange.svg)
![Status](https://img.shields.io/badge/status-In%20Development-yellow)

**Nawia** is an isometric, top-down Action RPG built on a custom game engine. Set in a dark fantasy world deeply rooted in **Slavic mythology**, the game focuses on dynamic combat, epic boss encounters, and atmospheric storytelling.

This project is developed from scratch using **C++** and **SDL3**, emphasizing performance and architectural modularity.

---

## 🎓 Academic Context

This project is currently being developed as a group project at the **Gdańsk University of Technology (Politechnika Gdańska)**.
It is intended to evolve into a full **Engineering Thesis**.

**Core Team:**
* **Michał Sadkowski**
* **Dawid Wesołowski**
* **Michał Matysiak**
* **Ostap Lozovyy**

---

## ⚔️ Key Features

* **Custom Engine:** Built purely in C++ with SDL3 for low-level control over rendering and inputs.
* **Slavic Atmosphere:** Visuals and narrative inspired by the myths, legends, and bestiaries of ancient Slavs.
* **Dynamic Combat:** Fast-paced hack'n'slash gameplay inspired by genre classics like *Diablo 3*.
* **Advanced AI:** Smart enemy behaviors and challenging, multi-stage boss fights.
* **Isometric View:** Classic 2.5D perspective.

---

## 📂 Project Architecture

The project follows a modular architecture, separating the core engine subsystems from the specific gameplay logic.

```text
Nawia/
├── assets/             # Game multimedia resources (Graphics, Audio, Data)
├── docs/               # Project documentation (GDD, Doxygen API docs)
├── external/           # External libraries (SDL3, ImGui, etc.)
├── scripts/            # Helper scripts (build scripts, utilities)
├── src/                # Main C++ source code
│   ├── audio/          # Systems and classes for managing sound and music
│   ├── core/           # Engine foundations and main loops
│   │   ├── game/       # The Game class (main loop, game states)
│   │   ├── system/     # Low-level systems (Renderer, Input, Time)
│   │   └── util/       # Utility classes (Math, data structures)
│   ├── entity/         # Entity Component System (ECS) implementation
│   │   ├── actors/     # Concrete entities (Player, NPC, Enemy)
│   │   ├── components/ # Data components (Position, Health, Inventory)
│   │   └── systems/    # Logic processing components (MovementSystem, CombatSystem)
│   ├── ui/             # User Interface elements
│   │   ├── hud/        # Head-Up Display elements (health bars, minimap)
│   │   └── menu/       # Main menu, inventory screen, options
│   └── world/          # World and map management
│       ├── level/      # Level logic (triggers, enemy spawning)
│       └── map/        # Map loading and rendering, tile handling
└── tests/              # Unit and integration tests
````

-----

## 🛠️ Tech Stack

  * **Language:** C++ (Standard 20/23)
  * **Graphics & Input:** SDL3 (Simple DirectMedia Layer 3)
  * **Build System:** CMake
  * **Scripting/Tooling:** Python

## 🚀 Getting Started

### Prerequisites

  * C++ Compiler supporting C++20 (GCC, Clang, or MSVC)
  * CMake (3.20+)
  * Git

### Build Instructions

1.  Clone the repository:

    ```bash
    git clone https://github.com/m-sadkowski/Nawia_ARPG.git
    cd Nawia_ARPG/Nawia
    ```

2.  Configure the project with CMake:

    ```bash
    mkdir build
    cd build
    cmake ..
    ```

3.  Build and Run:

    ```bash
    cmake --build .
    .\src\Debug\Nawia.exe
    ```

-----

*Copyright © 2025 Nawia Team*