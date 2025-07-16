// Conway's Game of Life (Sparse Infinite Grid)
// C++ implementation using GLFW, Glad, ImGui, and OpenGL 3

#include <algorithm>
#include <chrono>
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
std::unordered_set<std::pair<int, int>, PairHash> liveCells;
bool running = false, stepOnce = false;
float speed = 10.0f;
float offsetX = 50.0f, offsetY = 50.0f; // Camera offset

// --------------------- Simulation Update ---------------------
void updateLiveCells()
{
    std::unordered_map<std::pair<int, int>, int, PairHash> neighborCounts;

    for (const auto& cell : liveCells)
    {
        int x = cell.first, y = cell.second;
        for (int dy = -1; dy <= 1; dy++)
        {
            for (int dx = -1; dx <= 1; dx++)
            {
                if (dx == 0 && dy == 0) continue;
                std::pair<int, int> neighbor = {x + dx, y + dy};
                neighborCounts[neighbor]++;
            }
        }
    }

    std::unordered_set<std::pair<int, int>, PairHash> newLiveCells;
    for (const auto& [cell, count] : neighborCounts)
    {
        bool alive = liveCells.count(cell) > 0;
        if ((alive && contains(RULE_SURVIVAL, count)) || (!alive && contains(RULE_BIRTH, count)))
            newLiveCells.insert(cell);
    }

    liveCells = std::move(newLiveCells);
}

// --------------------- Drawing ---------------------
void drawLiveCells(float cs)
{
    glBegin(GL_POINTS);
    for (const auto& cell : liveCells)
    {
        float x = (cell.first - offsetX) * cs + cs / 2.0f;
        float y = (cell.second - offsetY) * cs + cs / 2.0f;
        glVertex2f(x, y);
    }
    glEnd();
}

void drawGridLines(float cs, float winW, float winH)
{
    glColor4f(0.5f, 0.5f, 0.5f, 0.5f); // light gray
    glBegin(GL_LINES);
    for (float x = 0; x <= winW; x += cs)
    {
        glVertex2f(x, 0);
        glVertex2f(x, winH);
    }
    for (float y = 0; y <= winH; y += cs)
    {
        glVertex2f(0, y);
        glVertex2f(winW, y);
    }
    glEnd();
}

// --------------------- Main ---------------------
int main()
{
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    GLFWwindow* window = glfwCreateWindow(900,900, "Game of Life", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr<<"Failed to init GLAD"; return -1;
    }

    glPointSize(5.0f);
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    auto lastTime = std::chrono::high_resolution_clock::now();
    float accumulator = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now; accumulator += dt;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Controls");
        if (ImGui::Button(running ? "Pause" : "Start")) running = !running;
        ImGui::SameLine(); if (ImGui::Button("Step")) stepOnce = true;
        ImGui::SameLine(); if (ImGui::Button("Clear")) liveCells.clear();
        ImGui::SliderFloat("Speed", &speed, 0.1f, 100.0f);
        ImGui::SliderFloat("Offset X", &offsetX, -500.0f, 500.0f);
        ImGui::SliderFloat("Offset Y", &offsetY, -500.0f, 500.0f);
        ImGui::End();

        double mx, my; glfwGetCursorPos(window, &mx, &my);
        int fbw, fbh; glfwGetFramebufferSize(window, &fbw, &fbh);
        float cs = 10.0f; // Cell Size
        int cx = int(mx / cs + offsetX);
        int cy = int(my / cs + offsetY);

        bool leftDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool rightDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        if (!ImGui::GetIO().WantCaptureMouse)
        {
            if (rightDown)
            {
                liveCells.erase({cx, cy});
            }
            else if (leftDown)
            {
                liveCells.insert({cx, cy});
            }
        }

        if ((running && accumulator >= 1.0f/speed) || stepOnce)
        {
            updateLiveCells(); accumulator = 0; stepOnce = false;
        }

        glViewport(0,0,fbw,fbh);
        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        glOrtho(0, fbw, fbh, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        
        drawGridLines(cs, fbw, fbh);

        glColor3f(0.2f, 1.0f, 0.2f);
        glPointSize(cs);
        drawLiveCells(cs);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
