// Conway's Game of Life (Sparse Infinite Grid)
// C++ implementation using GLFW, Glad, ImGui, and OpenGL 3

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#define GLFW_INCLUDE_NONE
#include "glad/glad.h"
#include "glfw/include/GLFW/glfw3.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

// --------------------- Game Rules ---------------------
static const std::vector<int> RULE_BIRTH    = { 3 };
static const std::vector<int> RULE_SURVIVAL = { 2, 3 };

// Utility function to check if a number is in a vector
static bool contains(const std::vector<int>& v, int x)
{
    return std::find(v.begin(), v.end(), x) != v.end();
}

// --------------------- Hashing for cell coordinates ---------------------
struct PairHash
{
    size_t operator()(const std::pair<int,int>& p) const
    {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
    }
};

// --------------------- Global Simulation State ---------------------
static std::unordered_set<std::pair<int, int>, PairHash> liveCells;
static int generationCount = 0;
// Simulation control variables
static bool running = false, stepOnce = false;
static float speed = 10.0f;
static float offsetX = 50.0f, offsetY = 50.0f; // Camera offset
static float cellSize = 20.0f; // Cell size in pixels

// Mouse drag state for panning
static bool leftWasDown = false;
static bool leftDragging = false;
static double dragStartX = 0.0, dragStartY = 0.0;

// --------------------- Simulation Update ---------------------
void updateLiveCells()
{
    std::unordered_map<std::pair<int, int>, int, PairHash> neighborCounts;

    // Count neighbors for each live cell
    for (const auto& cell : liveCells)
    {
        int x = cell.first, y = cell.second;
        for (int dy = -1; dy <= 1; dy++)
        {
            for (int dx = -1; dx <= 1; dx++)
            {
                if (!(dx == 0 && dy == 0))
                {
                    neighborCounts[{x + dx, y + dy}]++;
                }
            }
        }
    }

    std::unordered_set<std::pair<int, int>, PairHash> newLiveCells;
    // Determine which cells survive or are born
    for (const auto& kv : neighborCounts)
    {
        const auto& cell = kv.first;
        int count = kv.second;
        bool alive = liveCells.count(cell) > 0;
        if ((alive && contains(RULE_SURVIVAL, count)) || (!alive && contains(RULE_BIRTH, count)))
        {
            newLiveCells.insert(cell);
        }
    }

    // Replace old set with new generation
    liveCells = std::move(newLiveCells);
    generationCount++;
}

// --------------------- Drawing ---------------------
void drawLiveCells(float cs, float winW, float winH)
{
    glBegin(GL_POINTS);
    for (const auto& cell : liveCells)
    {
        float x = (cell.first - offsetX) * cs + cs / 2.0f;
        float y = (cell.second - offsetY) * cs + cs / 2.0f;
        // Frustum culling: draw only visible cells
        if (x < 0 || x > winW || y < 0 || y > winH)
        {
            continue;
        }
        glVertex2f(x, y);
    }
    glEnd();
}

void drawGridLines(float cs, float winW, float winH)
{
    glColor4f(0.5f, 0.5f, 0.5f, 0.5f); // Grid Lines Color
    glBegin(GL_LINES);
    // Calculate offset in pixels
    float offX_px = fmod(offsetX * cs, cs);
    if (offX_px < 0) offX_px += cs;
    float offY_px = fmod(offsetY * cs, cs);
    if (offY_px < 0) offY_px += cs;
    // Vertical lines
    for (float x = -offX_px; x <= winW; x += cs)
    {
        glVertex2f(x, 0);
        glVertex2f(x, winH);
    }
    // Horizontal lines
    for (float y = -offY_px; y <= winH; y += cs)
    {
        glVertex2f(0, y);
        glVertex2f(winW, y);
    }
    glEnd();
}

// --------------------- Main ---------------------
int main()
{
    // Initialize GLFW and create a window
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    GLFWwindow* window = glfwCreateWindow(900,900, "Game of Life", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    // Enable VSync for frame rate limiting
    glfwSwapInterval(1);

    // Load OpenGL functions via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to init GLAD" << '\n';
        return -1;
    }
    
    glPointSize(5.0f); // Set default point size for cells
    
    // Initialize ImGui
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    auto lastTime = std::chrono::high_resolution_clock::now();
    float accumulator = 0.0f; // Time accumulator for simulation timing

    // --------------------- Main Loop ---------------------
    while (!glfwWindowShouldClose(window))
    {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now; accumulator += dt;

        // New ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --------------------- ImGui Controls ---------------------
        ImGui::Begin("Controls");
        if (ImGui::Button(running ? "Pause" : "Start")) running = !running;
        ImGui::SameLine(); if (ImGui::Button("Step")) stepOnce = true;
        ImGui::SameLine(); if (ImGui::Button("Clear")) { liveCells.clear(); generationCount = 0; }
        ImGui::SliderFloat("Speed", &speed, 0.1f, 100.0f);
        ImGui::Text("Generation: %d", generationCount);
        ImGui::End();

        // --------------------- Mouse Interaction ---------------------
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        int fbw, fbh;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        
        int cx = int(mx / cellSize + offsetX);
        int cy = int(my / cellSize + offsetY);

        // Handle mouse input for toggling cells
        bool leftDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool rightDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        // Zoom control with scroll wheel
        float scrollY = ImGui::GetIO().MouseWheel;
        if (!ImGui::GetIO().WantCaptureMouse && scrollY != 0.0f)
        {
            cellSize *= (scrollY > 0 ? 1.1f : 0.9f);
            cellSize = std::clamp(cellSize, 2.0f, 100.0f);
        }

        // Pan with left click drag, add cell on click
        if (leftDown && !leftWasDown) // press
        {
            dragStartX = mx; dragStartY = my;
            leftDragging = false;
        }
        else if (leftDown && leftWasDown) // held
        {
            double dx = mx - dragStartX;
            double dy = my - dragStartY;
            if (!leftDragging && std::sqrt(dx*dx + dy*dy) > 5.0)
                leftDragging = true;
            if (leftDragging && !ImGui::GetIO().WantCaptureMouse)
        {
                offsetX -= dx / cellSize;
                offsetY -= dy / cellSize;
                dragStartX = mx; dragStartY = my;
            }
            }
        else if (!leftDown && leftWasDown) // release
            {
            if (!leftDragging && !ImGui::GetIO().WantCaptureMouse)
                liveCells.insert({cx, cy});
            }
        leftWasDown = leftDown;

        // Erase with right click
        if (rightDown && !ImGui::GetIO().WantCaptureMouse)
        {
            liveCells.erase({cx, cy});
        }

        // --------------------- Simulation Step ---------------------
        if ((running && accumulator >= 1.0f/speed) || stepOnce)
        {
            updateLiveCells(); accumulator = 0; stepOnce = false;
        }

        // --------------------- Rendering ---------------------
        glViewport(0,0,fbw,fbh);
        glClearColor(0.1f,0.1f,0.1f,1.0f); // bg
        glClear(GL_COLOR_BUFFER_BIT);

        // Orthographic Projection
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        glOrtho(0, fbw, fbh, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        
        drawGridLines(cellSize, fbw, fbh);

        glColor3f(0.2f, 1.0f, 0.2f); // Cell Color
        glPointSize(cellSize);
        drawLiveCells(cellSize, fbw, fbh);

        // Render ImGui UI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers and poll input
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --------------------- Cleanup ---------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
