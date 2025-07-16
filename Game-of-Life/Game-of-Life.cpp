// Conway's Game of Life
// C++ implementation using GLFW, Glad, ImGui, and OpenGL 3

#include <vector>
#include <algorithm>
#include <chrono>
#include <iostream>
#define GLFW_INCLUDE_NONE
#include "glfw/include/GLFW/glfw3.h"
#include "glad/glad.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

// --------------------- Game Rules ---------------------
// Standard Life: B3/S23 (Birth if 3 neighbors, Survive if 2 or 3)
static const std::vector<int> RULE_BIRTH    = { 3 };
static const std::vector<int> RULE_SURVIVAL = { 2, 3 };

// Check if a vector contains a value
static bool contains(const std::vector<int>& v, int x)
{
    return std::find(v.begin(), v.end(), x) != v.end();
}

// --------------------- Global Simulation State ---------------------
int gridWidth  = 100;
int gridHeight = 100;
std::vector<uint8_t> grid, nextGrid;

bool running = false;
bool stepOnce = false;
float speed = 10.0f;              // generations per second

// Selection & clipboard
bool selecting = false;
ImVec2 selStart, selEnd;
std::vector<std::pair<int,int>> clipboard;

// --------------------- Utility Functions ---------------------
// Resize and clear both buffers to new dimensions
void resizeGrid(int w, int h)
{
    gridWidth = w;
    gridHeight = h;
    grid.assign(w * h, 0);
    nextGrid.assign(w * h, 0);
}

// Count live neighbors with wrap-around edges
int countNeighbors(int x, int y)
{
    int count = 0;
    for (int dy=-1; dy<=1; dy++)
    {
        for (int dx=-1; dx<=1; dx++)
        {
            if (dx==0 && dy==0) continue;
            
            int nx = (x + dx + gridWidth) % gridWidth;
            int ny = (y + dy + gridHeight) % gridHeight;
            
            count += grid[ny*gridWidth + nx];
        }
    }
    return count;
}

// Advance one generation
void updateGrid()
{
    for (int y=0; y<gridHeight; y++)
    {
        for (int x=0; x<gridWidth; x++)
        {
            int idx = y*gridWidth + x;
            int n = countNeighbors(x, y);
            if (grid[idx])
            {
                // live cell survives only if in RULE_SURVIVAL
                nextGrid[idx] = contains(RULE_SURVIVAL, n) ? 1 : 0;
            } else
            {
                // dead cell is born only if in RULE_BIRTH
                nextGrid[idx] = contains(RULE_BIRTH, n) ? 1 : 0;
            }
        }
    }
    grid.swap(nextGrid);
}

// Reset to empty grid
void resetGrid()
{
    std::fill(grid.begin(), grid.end(), 0);
}

// Draw grid as points
void drawGrid(float cellSize)
{
    glBegin(GL_POINTS);
    for (int y = 0; y < gridHeight; y++)
    {
        for (int x = 0; x < gridWidth; x++)
        {
            if (grid[y * gridWidth + x])
            {
                glVertex2f(x * cellSize, y * cellSize);
            }
        }
    }
    glEnd();
}

// Convert window mouse position to cell coordinates
bool mouseToCell(const ImVec2& mp, int& outX, int& outY, float cellSize)
{
    outX = int(mp.x / cellSize);
    outY = int(mp.y / cellSize);
    
    return outX >= 0 && outX < gridWidth && outY >= 0 && outY < gridHeight;
}

// Extract pattern from selection rect into clipboard
void copySelection(int x0, int y0, int x1, int y1)
{
    clipboard.clear();
    for (int y = y0; y <= y1; y++)
    {
        for (int x = x0; x <= x1; x++)
        {
            if (grid[y * gridWidth + x])
            {
                clipboard.emplace_back(x - x0, y - y0);
            }
        }
    }
}

// Clear pattern in selection rect
void cutSelection(int x0, int y0, int x1, int y1)
{
    copySelection(x0,y0,x1,y1);
    for (int y = y0; y <= y1; y++)
    {
        for (int x = x0; x <= x1; x++)
        {
            grid[y * gridWidth + x] = 0;
        }
    }
}

// Paste clipboard at target cell
void pasteClipboard(int tx, int ty)
{
    for (auto &p : clipboard)
    {
        int x = tx + p.first;
        int y = ty + p.second;
        
        if (x >= 0 && x < gridWidth && y >= 0 && y < gridHeight)
        {
            grid[y * gridWidth + x] = 1;
        }
    }
}

// --------------------- Main ---------------------
int main()
{
    // GLFW & OpenGL init
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(800, 800, "Game of Life", glfwGetPrimaryMonitor(), nullptr);
    if (!window)
    {
        glfwTerminate(); return -1;
    }
    glfwMakeContextCurrent(window);
    gladLoadGL();
    glPointSize(5.0f);
    
    // ImGui init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Initialize grid buffers
    resizeGrid(gridWidth, gridHeight);

    auto lastTime = std::chrono::high_resolution_clock::now();
    float accumulator = 0.0f;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Time delta
        auto now = std::chrono::high_resolution_clock::now();
        float delta = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        accumulator += delta;

        // Start new ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- Controls Window ---
        ImGui::Begin("Controls");
        if (ImGui::Button(running ? "Pause" : "Start"))
        {
            running = !running;
        }
        ImGui::SameLine();
        if (ImGui::Button("Step"))
        {
            stepOnce = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            resetGrid();
        }
        if (ImGui::Button("Copy") && selStart.x <= selEnd.x)
        {
            int x0=int(selStart.x), y0=int(selStart.y), x1=int(selEnd.x), y1=int(selEnd.y);
            copySelection(x0,y0,x1,y1);
        }
        ImGui::SameLine();
        if (ImGui::Button("Cut") && selStart.x <= selEnd.x)
        {
            int x0=int(selStart.x), y0=int(selStart.y), x1=int(selEnd.x), y1=int(selEnd.y);
            cutSelection(x0,y0,x1,y1);
        }
        ImGui::SameLine();
        if (ImGui::Button("Paste") && !clipboard.empty())
        {
            ImVec2 mp = ImGui::GetIO().MousePos;
            int cx, cy;
            float cellSize = 800.0f / std::max(gridWidth, gridHeight);
            if (mouseToCell(mp, cx, cy, cellSize)) pasteClipboard(cx,cy);
        }
        ImGui::SliderInt("Width", &gridWidth, 10, 1000);
        ImGui::SliderInt("Height", &gridHeight, 10, 1000);
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            resizeGrid(gridWidth, gridHeight);
        }
        ImGui::SliderFloat("Speed (gen/sec)", &speed, 0.1f, 60.0f);
        ImGui::End();

        // --- Handle selection ---
        ImVec2 mp = ImGui::GetIO().MousePos;
        float cellSize = 800.0f / std::max(gridWidth, gridHeight);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
        {
            selecting = true;
            mouseToCell(mp, *(int*)&selStart.x, *(int*)&selStart.y, cellSize);
            selEnd = selStart;
        }
        if (selecting && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            mouseToCell(mp, *(int*)&selEnd.x, *(int*)&selEnd.y, cellSize);
        }
        if (selecting && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            selecting = false;
            
            // normalize
            if (selStart.x > selEnd.x) std::swap(selStart.x, selEnd.x);
            if (selStart.y > selEnd.y) std::swap(selStart.y, selEnd.y);
        }

        // --- Simulation Update ---
        if ((running && accumulator >= 1.0f/speed) || stepOnce)
        {
            updateGrid();
            accumulator = 0.0f;
            stepOnce = false;
        }

        // --- Rendering ---
        glViewport(0,0,800,800);
        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw cells
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        glOrtho(0, gridWidth*cellSize, gridHeight*cellSize, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        glColor3f(0.2f, 1.0f, 0.2f);
        drawGrid(cellSize);

        // Draw selection rectangle
        if (!selecting && selStart.x<=selEnd.x)
        {
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            ImVec2 a{ selStart.x*cellSize, selStart.y*cellSize };
            ImVec2 b{ (selEnd.x+1)*cellSize, (selEnd.y+1)*cellSize };
            dl->AddRect(a, b, IM_COL32(255,255,0,200), 0.0f, 0, 2.0f);
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
