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

static bool contains(const std::vector<int>& v, int x)
{
    return std::find(v.begin(), v.end(), x) != v.end();
}

// --------------------- Global Simulation State ---------------------
int gridWidth = 100, gridHeight = 100;
std::vector<uint8_t> grid, nextGrid;

bool running = false, stepOnce = false;
float speed = 10.0f;  // generations per second

// Edit modes for mouse input
enum class EditMode { None, Copy, Cut, Paste };
EditMode editMode = EditMode::None;
// Selection coords
int selX0 = -1, selY0 = -1, selX1 = -1, selY1 = -1;
// Clipboard
std::vector<std::pair<int,int>> clipboard;

// --------------------- Utility Functions ---------------------
void resizeGrid(int w, int h)
{
    gridWidth = w; gridHeight = h;
    grid.assign(w*h, 0);
    nextGrid.assign(w*h, 0);
}

/*
// wraps around screen
int countNeighbors(int x, int y)
{
    int c = 0;
    for (int dy=-1; dy<=1; dy++)
    {
        for (int dx=-1; dx<=1; dx++)
        {
            if (dx==0 && dy==0) continue;
            int nx = (x + dx + gridWidth) % gridWidth;
            int ny = (y + dy + gridHeight) % gridHeight;
            c += grid[ny*gridWidth + nx];
        }
    }
    return c;
}
*/
// doesn't wrap around
int countNeighbors(int x, int y)
{
    int c = 0;
    for (int dy = -1; dy <= 1; dy++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < gridWidth && ny >= 0 && ny < gridHeight)
            {
                c += grid[ny * gridWidth + nx];
            }
        }
    }
    return c;
}

void updateGrid()
{
    for (int y=0; y<gridHeight; y++)
    {
        for (int x=0; x<gridWidth; x++)
        {
            int idx = y*gridWidth + x;
            int n = countNeighbors(x, y);
            nextGrid[idx] = grid[idx] ? (contains(RULE_SURVIVAL, n) ? 1 : 0) : (contains(RULE_BIRTH, n)    ? 1 : 0);
        }
    }
    grid.swap(nextGrid);
}

void resetGrid()
{
    std::fill(grid.begin(), grid.end(), 0);
}

void drawGrid(float cs)
{
    glBegin(GL_POINTS);
    for (int y=0; y<gridHeight; y++)
    {
        for (int x=0; x<gridWidth; x++)
        {
            if (grid[y * gridWidth + x])
            {
                glVertex2f(x * cs + cs / 2.0f, y * cs + cs / 2.0f);
            }
        }
    }
    glEnd();
}

void drawGridLines(float cs)
{
    glColor4f(0.5f, 0.5f, 0.5f, 0.5f); // line color
    glBegin(GL_LINES);
    for (int x = 0; x <= gridWidth; ++x)
    {
        glVertex2f(x * cs, 0);
        glVertex2f(x * cs, gridHeight * cs);
    }
    for (int y = 0; y <= gridHeight; ++y)
    {
        glVertex2f(0, y * cs);
        glVertex2f(gridWidth * cs, y * cs);
    }
    glEnd();
}

bool mouseToCell(double mx, double my, int& cx, int& cy, float cs)
{
    cx = int(mx/cs); cy = int(my/cs);
    return cx>=0 && cx<gridWidth && cy>=0 && cy<gridHeight;
}

void copySelection()
{
    clipboard.clear();
    for (int y=selY0; y<=selY1; y++)
    {
        for (int x=selX0; x<=selX1; x++)
        {
            if (grid[y*gridWidth + x])
                clipboard.emplace_back(x - selX0, y - selY0);
        }
    }
}

void cutSelection()
{
    copySelection();
    for (int y=selY0; y<=selY1; y++)
    {
        for (int x=selX0; x<=selX1; x++)
        {
            grid[y*gridWidth + x] = 0;
        }
    }
}

void pasteClipboard(int tx, int ty) 
{
    for (auto &p : clipboard) 
    {
        int x = tx + p.first;
        int y = ty + p.second;
        if (x>=0 && x<gridWidth && y>=0 && y<gridHeight)
        {
            grid[y*gridWidth + x] = 1;
        }
    }
    editMode = EditMode::None;
}

// --------------------- Main ---------------------
int main()
{
    // GLFW & Glad init
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    GLFWwindow* window = glfwCreateWindow(900,900, "Game of Life", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate(); return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr<<"Failed to init GLAD"; return -1;
    }
    glPointSize(5.0f);

    // ImGui init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Setup grid
    resizeGrid(gridWidth, gridHeight);
    auto lastTime = std::chrono::high_resolution_clock::now();
    float accumulator = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        // Time
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now; accumulator += dt;

        // ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Controls
        ImGui::Begin("Controls");
        if (ImGui::Button(running?"Pause":"Start"))
        {
            running = !running;
        }
        ImGui::SameLine(); if (ImGui::Button("Step"))
        {
            stepOnce = true;
        }
        ImGui::SameLine(); if (ImGui::Button("Reset"))
        {
            resetGrid();
        }
        ImGui::Separator();
        if (ImGui::Button("Copy"))
        {
            editMode = EditMode::Copy;
        }
        ImGui::SameLine(); if (ImGui::Button("Cut"))
        {
            editMode = EditMode::Cut;
        }
        if (!clipboard.empty())
        {
            ImGui::SameLine(); if (ImGui::Button("Paste")) editMode = EditMode::Paste;
        }
        /*ImGui::SliderInt("Width", &gridWidth, 10, 1000);
        ImGui::SliderInt("Height",&gridHeight,10,1000);
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            resizeGrid(gridWidth, gridHeight);
        }*/
        ImGui::SliderFloat("Speed (gen/sec)", &speed, 0.1f, 100.0f);
        ImGui::End();

        // Mouse & cell mapping
        double mx,my;
        glfwGetCursorPos(window, &mx, &my);
        int fbw, fbh; glfwGetFramebufferSize(window, &fbw, &fbh);
        float cellSize = std::min((float)fbw,(float)fbh) / std::max(gridWidth, gridHeight);

        int cx, cy;
        bool inGrid = mouseToCell(mx, my, cx, cy, cellSize);
        
        bool leftDown  = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS;
        bool rightDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS;

        // Editing
        if (!ImGui::GetIO().WantCaptureMouse)
        {
            switch (editMode)
            {
            case EditMode::Copy:
            case EditMode::Cut:
                {
                    if (leftDown && selX0<0 && inGrid)
                    {
                        selX0=cx; selY0=cy; selX1=cx; selY1=cy;
                    } else if (leftDown && selX0>=0)
                    {
                        selX1=cx; selY1=cy;
                    } else if (!leftDown && selX0>=0)
                    {
                        // finalize
                        if (selX1<selX0) std::swap(selX0, selX1);
                        if (selY1<selY0) std::swap(selY0, selY1);
                        if (editMode==EditMode::Copy) copySelection();
                        else cutSelection();
                        selX0=selY0=selX1=selY1=-1;
                        editMode=EditMode::None;
                    }
                } break;
            case EditMode::Paste:
                if (leftDown && inGrid)
                {
                    pasteClipboard(cx, cy);
                }
                break;
                /*
                case EditMode::None:
                if (leftDown && inGrid)
                {
                    grid[cy*gridWidth+cx] ^= 1;  // TOGGLE CELL STATE
                }
                break;
                */
            case EditMode::None:
                if (leftDown && inGrid)
                {
                    grid[cy * gridWidth + cx] = 1; 
                }
                else if (rightDown && inGrid)
                {
                    grid[cy * gridWidth + cx] = 0; 
                }
                break;
            }
        }

        // Simulation update
        if ((running && accumulator>=1.0f/speed) || stepOnce)
        {
            updateGrid(); accumulator=0; stepOnce=false;
        }

        // Render
        glViewport(0,0,fbw,fbh);
        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        glOrtho(0, gridWidth*cellSize, gridHeight*cellSize, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        
        glLineWidth(1.0f); 
        glDisable(GL_DEPTH_TEST);     // Ensure depth doesn't obscure lines
        glColor3f(1.0f, 1.0f, 1.0f);  // Use bright color
        glLineWidth(1.0f);            // Explicitly set line width (even if limited)
        drawGridLines(cellSize);      // Draw the grid lines   
        glColor3f(0.2f, 1.0f, 0.2f);       // live cells color
        glPointSize(cellSize);            
        drawGrid(cellSize);               // draw cells
        glPointSize(cellSize);
        
        // Draw selection rectangle
        if (selX0>=0)
        {
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            ImVec2 a{(float)selX0*cellSize, (float)selY0*cellSize};
            ImVec2 b{(float)(selX1+1)*cellSize, (float)(selY1+1)*cellSize};
            dl->AddRect(a,b,IM_COL32(255,255,0,200),0.0f,0,2.0f);
        }
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
