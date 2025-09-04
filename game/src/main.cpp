/*
This project uses the Raylib framework to provide us functionality for math, graphics, GUI, input etc.
See documentation here: https://www.raylib.com/, and examples here: https://www.raylib.com/examples.html
*/

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h"

const unsigned int TARGET_FPS = 50; // frames per second
float dt = 1; // seconds per frame
float time = 0;
float x = 500;
float y = 500;
float frequency = 1;
float amplitude = 100;

// Changes world state
void update()
{
        dt = 1.0f / TARGET_FPS;
        time += dt;

        x = x + (-sin(time * frequency)) * frequency * amplitude * dt;
        y = y + (cos(time * frequency)) * frequency * amplitude * dt;
}

// Displays the world
void draw()
{
        BeginDrawing();
            ClearBackground(BLACK);
            DrawText("Lucas Adda 101566961", 10, float(GetScreenHeight() - 25), 20, LIGHTGRAY);
           
            GuiSliderBar(Rectangle{ 60, 15, 1000, 10}, "Time", TextFormat("%.2f", time), &time, 0, 240);
            DrawText(TextFormat("T: %.2f", time), GetScreenWidth() - 150, 10, 30, LIGHTGRAY);

            DrawCircle(x, y, 70, RED);
            DrawCircle(500 + cos(time * frequency) * amplitude, 500 + sin(time * frequency), 70, GREEN);

        EndDrawing();
}

int main()
{
    InitWindow(InitialWidth, InitialHeight, "Lucas Adda 101566961 2005-Physics");
    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose()) // Loops TARGET_FPS per second
    {
        update();
        draw();
    }

    CloseWindow();
    return 0;
}
