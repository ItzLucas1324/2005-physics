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

Vector2 launchPos;
float launchAngle;
float launchSpeed;

// Displays the world
void draw()
{
        BeginDrawing();
            ClearBackground(SKYBLUE);

            // Ground
            DrawRectangle(0, 700, 1200, 100, GREEN);
            // Text Box
            DrawRectangle(40, 40, 180, 100, BLACK);
            // Text (In the text box)
            DrawText(TextFormat("Launch Position"), 42, 42, 30, WHITE);
            DrawText(TextFormat("(%.0f, %.0f)", launchPos.x, launchPos.y), 42, 82, 30, WHITE);
            // Start Position
            launchPos = { 200.0f, 500.0f };
            DrawCircleV(launchPos, 10, RED);
            
        EndDrawing();
}

int main()
{
    InitWindow(InitialWidth, InitialHeight, "Lucas Adda 101566961 2005 Week 2");
    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose()) // Loops TARGET_FPS per second
    {
        draw();
    }

    CloseWindow();
    return 0;
}
