# Conway's Game of Life

A C++ implementation of Conway's Game of Life using a sparse data structure for an infinite grid. 

Built with GLFW, Glad, ImGui, and OpenGL 3.

## Features

- Infinite sparse grid representation, only live cells are stored, optimising memory.
- Adjustable simulation speed and camera panning.
- Cell toggling with mouse click.
- controls via ImGui.
- Visual grid overlay and smooth rendering using OpenGL points.

![Glider Gun](https://github.com/Loris-Moreau/Game-of-Life/blob/main/Images/GliderGun.png "Glider Gun")

## How It Works

The simulation stores live cells as coordinate pairs in an unordered set. On each update, it counts the neighbours of every live cell, then applies the classic Game of Life rules:

- A dead cell with exactly 3 neighbours becomes alive.
- A live cell survives if it has 2 or 3 neighbours; otherwise, it dies.

<i>ps: these rule are at the top of the file & you can freely change them.</i>

<br>

Cells outside the current viewport are not limited — the grid is conceptually infinite and sparse, so only active cells are tracked.
<br>
User input allows :
- Left mouse button adds a live cell at the cursor.
- Right mouse button removes a live cell.
- Controls for speed, camera offset, and simulation state are exposed via the GUI.

## Requirements

- Windows Operating system
- IDE that can run C++17

## Build & Run

1. Clone the repo.
2. Compile & run
3. Have fun

![](https://github.com/Loris-Moreau/Game-of-Life/blob/main/Images/GliderGun.gif)
