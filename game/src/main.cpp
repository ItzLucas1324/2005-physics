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

enum PhysicsShape
{
    CIRCLE,
    HALF_SPACE
};

class PhysicsBody
{
public:
    bool isStatic = false; // If this is set to true, don't move the object according to velocity or gravity
    Vector2 position {};
    Vector2 projectileVelo {};
    Color color = GREEN;

    // Projectile Specific Physics
    void update(float dt, Vector2 gravity)
    {
        if (isStatic) return;

        position += projectileVelo * dt;
        projectileVelo += gravity * dt;
    }

    virtual void draw() {}

    virtual PhysicsShape Shape() = 0;
};

class PhysicsCircle : public PhysicsBody
{
public:
    float radius = 30.0f;
    void draw() override
    {
        DrawCircleV(position, radius, color);
    }

    PhysicsShape Shape() override
    {
        return CIRCLE;
    }
};

class PhysicsHalfspace : public PhysicsBody
{
    // distance to the normal hints: a dot b * (b/||b||) ||b|| referring to magnitude
    // dot product: a.x * b.x + a.y * b.y
private:
    float rotation = 0;
    Vector2 normal = { 0, -1 };

public:

    void setRotationDegrees(float rotationInDeg)
    {
        rotation = rotationInDeg;
        normal = Vector2Rotate({ 0, -1 }, rotation * DEG2RAD);
    }

    float getRotation()
    {
        return rotation;
    }

    Vector2 getNormal()
    {
        return normal;
    }

    void draw() override
    {
        DrawCircle(position.x, position.y, 8, RED);

        DrawLineEx(position, position + normal * 30, 1, RED);

        Vector2 parallelToSurface = Vector2Rotate(normal, PI * 0.5f);
        DrawLineEx(position - parallelToSurface * 2000, position + parallelToSurface * 2000, 1, RED);
    }

    PhysicsShape Shape() override
    {
        return HALF_SPACE;
    }
};

std::vector<PhysicsBody*> objects;
PhysicsHalfspace halfspace;

bool HalfspaceOverlap(PhysicsCircle* circle, PhysicsHalfspace* halfspace) 
{
    Vector2 displacementToCircle = circle->position - halfspace->position;
    // Let d be the Dot Product of this displacement and the normal vector
    // If D < 0, cricle is behind it and overlapping, if D > 0, circle is in front
    // In other words... return (D < radius)
    // return (Dot (displacement, normal) < radius)

    float dot = Vector2DotProduct(displacementToCircle, halfspace->getNormal());
    Vector2 vectorProjection = halfspace->getNormal() * dot;

    DrawLineEx(circle->position, halfspace->position, 1, GRAY);

    return false;
}

bool CircleOverlap(PhysicsCircle circleA, PhysicsCircle circleB)
{
    Vector2 displacement = circleB.position - circleA.position;
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
    for (int i = 0; i < objects.size(); i++) // Resets all circles to green if not touching
    {
        ((PhysicsCircle*)objects[i])->color = GREEN;
    }

    for (int i = 0; i < objects.size(); i++) // Overlap check
    {
        for (int j = i + 1; j < objects.size(); j++)
        {
            PhysicsCircle* circlePointerA = (PhysicsCircle*)objects[i];
            PhysicsCircle* circlePointerB = (PhysicsCircle*)objects[j];

            if (CircleOverlap(*circlePointerA, *circlePointerB))
            {
                circlePointerA->color = RED;
                circlePointerB->color = RED;
            }
        }
    }
}

void cleanup()
{
    for (int i = 0; i < objects.size(); i++)
    {
        if (objects[i]->position.y > GetScreenHeight())
        {
            auto iterator = (objects.begin() + i);
            PhysicsBody* pointer = *iterator;
            delete pointer;
            objects.erase(iterator);
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
        newBird->position = launchPos;
        newBird->projectileVelo = velocity;
        objects.push_back(newBird); 
        // Adds a new bird to the list
    }

    cleanup();
    // Adds physics to all angry birds created
    for (int i = 0; i < objects.size(); i++)
    {
        objects[i]->update(dt, gravityAcceleration);
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
            GuiSliderBar(Rectangle{ 80, 270, 700, 20 }, "Halfspace X", TextFormat("%.0f", halfspace.position.x), &halfspace.position.x, 0, GetScreenWidth());
            GuiSliderBar(Rectangle{ 80, 310, 700, 20 }, "Halfspace y", TextFormat("%.0f", halfspace.position.y), &halfspace.position.y, 0, GetScreenHeight());
            float halfspaceRotation = halfspace.getRotation();
            GuiSliderBar(Rectangle{ 110, 350, 500, 20 }, "Halfspace Rotate", TextFormat("%.0f", halfspaceRotation), &halfspaceRotation, -360, 360);
            halfspace.setRotationDegrees(halfspaceRotation);
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
            // Number of Birds Spawned Text
            DrawText(TextFormat("Objects: %i", objects.size()), 10, 500, 30, WHITE);
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
            for (int i = 0; i < objects.size(); i++)
            {
                objects[i]->draw();
            }

        EndDrawing();
}

int main()
{
    InitWindow(InitialWidth, InitialHeight, "Lucas Adda 101566961 2005 Week 5");
    SetTargetFPS(TARGET_FPS);
    halfspace.isStatic = true;
    halfspace.position = { 500, 700 };
    objects.push_back(&halfspace);

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
