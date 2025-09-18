/*
This project uses the Raylib framework to provide us functionality for math, graphics, GUI, input etc.
See documentation here: https://www.raylib.com/, and examples here: https://www.raylib.com/examples.html
*/

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h"

const unsigned int TARGET_FPS = 60; // frames per second

Vector2 launchPos;
float launchAngle;
float launchSpeed;
float dt = 1; // seconds per frame
float time = 0;

void update()
{
    dt = 1.0f / TARGET_FPS;
    time += dt;
}

// Displays the world
void draw()
{
        BeginDrawing();
            ClearBackground(SKYBLUE);

            // Variable Adjustment Sliders
            GuiSliderBar(Rectangle{ 10, 150, 700, 20 }, "", TextFormat("Angle: %.2f", launchAngle), &launchAngle, 0, 180);
            GuiSliderBar(Rectangle{ 10, 190, 700, 20 }, "", TextFormat("Speed: %.2f", launchSpeed), &launchSpeed, 0, 500);
            // Ground
            DrawRectangle(0, 700, 1200, 100, GREEN);
            // Text Box
            DrawRectangle(20, 30, 280, 100, BLACK);
            DrawRectangle(320, 30, 280, 100, BLACK);
            DrawRectangle(620, 30, 280, 100, BLACK);
            // Velocity Calculation
            float rad = launchAngle * DEG2RAD;
            Vector2 velocity = { launchSpeed * cosf(rad), -launchSpeed * sinf(rad) };
            // Creating Line
            DrawLineEx(launchPos, Vector2{ launchPos + velocity }, 7, RED);
            // Text (In the text box)
            DrawText("Launch Position", 42, 42, 30, WHITE);
            DrawText(TextFormat("(%.0f, %.0f)", launchPos.x, launchPos.y), 42, 82, 30, WHITE);
            DrawText("Launch Angle", 342, 42, 30, WHITE);
            DrawText(TextFormat("(%.1f Degrees)", launchAngle), 342, 82, 30, WHITE);
            DrawText("Launch Speed", 642, 42, 30, WHITE);
            DrawText(TextFormat("(%.1f)", launchSpeed), 642, 82, 30, WHITE);
            // Start Position
            DrawCircleV(launchPos, 10, RED);

        EndDrawing();
}

int main()
{
    InitWindow(InitialWidth, InitialHeight, "Lucas Adda 101566961 2005 Week 2");
    SetTargetFPS(TARGET_FPS);

    launchPos = { 200.0f, 500.0f };
    launchAngle = 50.0f;
    launchSpeed = 400.0f;

    while (!WindowShouldClose()) // Loops TARGET_FPS per second
    {
        draw();
        update();
    }

    CloseWindow();
    return 0;
}
