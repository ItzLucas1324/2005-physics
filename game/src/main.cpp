/*
This project uses the Raylib framework to provide us functionality for math, graphics, GUI, input etc.
See documentation here: https://www.raylib.com/, and examples here: https://www.raylib.com/examples.html
*/

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h"
#include <vector>

const unsigned int TARGET_FPS = 60; // frames per second

float lpmSpeed = 100;
float dt = 1; // seconds per frame
float time = 0;

Vector2 launchPos;

Vector2 velocity; 

Vector2 gravityAcceleration = { 0, 100 };
float launchAngle;
float launchSpeed;
float rad;

class PhysicsBody
{
public:
    Vector2 launchStart {};
    Vector2 projectileVelo {};

    // Projectile Specific Physics
    void update(float dt, Vector2 gravity)
    {
        launchStart += projectileVelo * dt;
        projectileVelo += gravity * dt;
    }

    virtual void draw() {}
};

class PhysicsCircle : public PhysicsBody
{
public:
    float radius = 30.0f;
    Color circleColor = GREEN;
    void draw() override
    {
        DrawCircleV(launchStart, radius, circleColor);
    }
};

std::vector<PhysicsBody*> bird;

bool CircleOverlap(PhysicsCircle circleA, PhysicsCircle circleB)
{
    Vector2 displacement = circleB.launchStart - circleA.launchStart;
    float distance = Vector2Length(displacement);
    float sumOfRadii = circleA.radius + circleB.radius;
    if (sumOfRadii > distance)
    {
        return true; // Overlapping
    }
    else
        return false; // Not overlapping
}

void checkCollisions()
{
    for (int i = 0; i < bird.size(); i++) // Resets all circles to green if not touching
    {
        ((PhysicsCircle*)bird[i])->circleColor = GREEN;
    }

    for (int i = 0; i < bird.size(); i++) // Overlap check
    {
        for (int j = i + 1; j < bird.size(); j++)
        {
            PhysicsCircle* circlePointerA = (PhysicsCircle*)bird[i];
            PhysicsCircle* circlePointerB = (PhysicsCircle*)bird[j];

            if (CircleOverlap(*circlePointerA, *circlePointerB))
            {
                circlePointerA->circleColor = RED;
                circlePointerB->circleColor = RED;
            }
        }
    }
}

void cleanup()
{
    for (int i = 0; i < bird.size(); i++)
    {
        if (bird[i]->launchStart.y > GetScreenHeight())
        {
            auto iterator = (bird.begin() + i);
            PhysicsBody* pointer = *iterator;
            delete pointer;
            bird.erase(iterator);
            i--;
        }
    }
}

void update()
{
    dt = 1.0f / TARGET_FPS;
    time += dt;
    rad = launchAngle * DEG2RAD;

    // Start Position Movement
    if (IsKeyDown(KEY_W))
        launchPos.y -= lpmSpeed * dt;
    if (IsKeyDown(KEY_S))
        launchPos.y += lpmSpeed * dt;
    if (IsKeyDown(KEY_A))
        launchPos.x -= lpmSpeed * dt;
    if (IsKeyDown(KEY_D))
        launchPos.x += lpmSpeed * dt;

    // Spawn launch bird
    if (IsKeyPressed(KEY_SPACE))
    {
        // Creates and allocates memory for a new bird
        PhysicsCircle* newBird = new PhysicsCircle();
        newBird->launchStart = launchPos;
        newBird->projectileVelo = velocity;
        bird.push_back(newBird); 
        // Adds a new bird to the list
    }

    cleanup();

    // Adds physics to all angry birds created
    for (int i = 0; i < bird.size(); i++)
    {
        bird[i]->update(dt, gravityAcceleration);
    }

    checkCollisions();
}

// Displays the world
void draw()
{
        BeginDrawing();
            ClearBackground(SKYBLUE);

            // Variable Adjustment Sliders
            GuiSliderBar(Rectangle{ 10, 150, 700, 20 }, "", TextFormat("Angle: %.2f", launchAngle), &launchAngle, 0, 180);
            GuiSliderBar(Rectangle{ 10, 190, 700, 20 }, "", TextFormat("Speed: %.2f", launchSpeed), &launchSpeed, 0, 500);
            GuiSliderBar(Rectangle{ 10, 230, 700, 20 }, "", TextFormat("Gravity: %.2f", gravityAcceleration.y), &gravityAcceleration.y, -350, 700);
            // Ground
            DrawRectangle(0, 700, 1200, 100, DARKGREEN);
            // Text Box
            DrawRectangle(10, 30, 280, 100, BLACK);
            DrawRectangle(310, 30, 280, 100, BLACK);
            DrawRectangle(610, 30, 280, 100, BLACK);
            DrawRectangle(910, 30, 280, 100, BLACK);
            // Velocity Calculation
            velocity = { launchSpeed * cosf(rad), -launchSpeed * sinf(rad) };
            // Creating Line
            DrawLineEx(launchPos, Vector2{ launchPos + velocity }, 7, RED);
            
            DrawText(TextFormat("Objects: %i", bird.size()), 10, 270, 30, WHITE);
            // Text (In the text box)
            DrawText("Launch Position", 32, 42, 30, WHITE);
            DrawText(TextFormat("(%.0f, %.0f)", launchPos.x, launchPos.y), 32, 82, 30, WHITE);
            DrawText("Launch Angle", 342, 42, 30, WHITE);
            DrawText(TextFormat("(%.1f Degrees)", launchAngle), 342, 82, 30, WHITE);
            DrawText("Launch Speed", 642, 42, 30, WHITE);
            DrawText(TextFormat("(%.1f)", launchSpeed), 642, 82, 30, WHITE);
            DrawText("Gravitational Pull", 919, 42, 30, WHITE);
            DrawText(TextFormat("(%.1f)", gravityAcceleration.y), 925, 82, 30, WHITE);
            // Start Position
            DrawCircleV(launchPos, 10, RED);
            // Draws each bird in list
            for (int i = 0; i < bird.size(); i++)
            {
                bird[i]->draw();
            }

        EndDrawing();
}

int main()
{
    InitWindow(InitialWidth, InitialHeight, "Lucas Adda 101566961 2005 Week 3");
    SetTargetFPS(TARGET_FPS);

    launchPos = { 200.0f, 700.0f };
    launchAngle = 50.0f;
    launchSpeed = 350.0f;

    while (!WindowShouldClose()) // Loops TARGET_FPS per second
    {
        draw();
        update();
    }

    CloseWindow();
    return 0;
}
