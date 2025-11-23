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
    Vector2 position{};
    Vector2 projectileVelo{}; // Pixels per sec
    Vector2 netForce = {}; // in Newtons
    float mass = 1;
    float coefficientOfFriction = 0.5f;

    virtual void draw() {}

    virtual PhysicsShape Shape() = 0;
};

class PhysicsCircle : public PhysicsBody
{
public:
    Color color = GREEN;
    float radius = 30.0f;
    float timer = 0.0f;
    bool isTimerActive = true;

    void draw() override
    {
        DrawCircleV(position, radius, color);
        DrawText(TextFormat("%.1f", timer), position.x - 14, position.y - 12, 25, BLACK);
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

    int id = 0;

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

        //for (int i = 0; i < 40; i++) 
        //{
        //    float offset = i * 50 + 25;
        //    Vector2 oppositeVec = normal * -offset;
        //    DrawLineEx(position - parallelToSurface * 2000 + oppositeVec, position + parallelToSurface * 2000 + oppositeVec, 50, DARKGREEN);
        //}

        DrawLineEx(position - parallelToSurface * 2000, position + parallelToSurface * 2000, 1, RED);
    }

    PhysicsShape Shape() override
    {
        return HALF_SPACE;
    }
};

std::vector<PhysicsBody*> objects;
PhysicsHalfspace halfspace;
PhysicsHalfspace halfspace2;

bool HalfspaceOverlap(PhysicsCircle* circle, PhysicsHalfspace* halfspace)
{
    Vector2 displacementToCircle = circle->position - halfspace->position;

    float dot = Vector2DotProduct(displacementToCircle, halfspace->getNormal());
    Vector2 vectorProjection = halfspace->getNormal() * dot;

    //DrawLineEx(circle->position, circle->position - vectorProjection, 1, GRAY);
    //Vector2 midpoint = circle->position - vectorProjection * 0.5f;
    //DrawText(TextFormat("D: %3.0f", dot), midpoint.x, midpoint.y, 30, GRAY);

    float overlapHalfspace = circle->radius - dot;

    if (overlapHalfspace > 0)
    {
        if (halfspace->id == 2)
        {
            circle->isTimerActive = false;
        }

        Vector2 mtv = halfspace->getNormal() * overlapHalfspace;
        circle->position += mtv;

        // Get Gravity
        Vector2 FGravity = gravityAcceleration * circle->mass;

        // Perp Magnitude
        float FPerpMagnitude = Vector2DotProduct(FGravity, halfspace->getNormal());
        // Apply Normal Force
        Vector2 FgPerp = halfspace->getNormal() * FPerpMagnitude;
        Vector2 FNormal = FgPerp * -1;
        circle->netForce += FNormal;
        DrawLineEx(circle->position, circle->position + FNormal, 2, GREEN);

        // Friction
        // F = uN where u is coefficient of friction between two surfaces
        float u = circle->coefficientOfFriction;
        float frictionMagnitude = Vector2Length(FNormal) * u;

        Vector2 FPara = Vector2Rotate(halfspace->getNormal(), -PI * 0.5f);

        float vFPara = Vector2DotProduct(circle->projectileVelo, FPara);

        const float veloctiyThreshold = 5.0f;

        if (fabs(vFPara) > veloctiyThreshold)
        {
            Vector2 frictionDirection = (vFPara > 0 ? Vector2Negate(FPara) : FPara);
            Vector2 Ffriction = frictionDirection * frictionMagnitude;
            circle->netForce += Ffriction;
            DrawLineEx(circle->position, circle->position + Ffriction, 2, ORANGE);
        }
        else
        {
            Vector2 velAlongSurface = FPara * vFPara;
            circle->projectileVelo -= velAlongSurface;
        }
        
        return true;
    }
    else
        return false;
}

bool CircleOverlap(PhysicsCircle* circleA, PhysicsCircle* circleB)
{
    Vector2 displacement = circleB->position - circleA->position;
    float distance = Vector2Length(displacement);
    float sumOfRadii = circleA->radius + circleB->radius;

    float overlapCircle = sumOfRadii - distance;

    if (overlapCircle > 0)
    {
        Vector2 normaltAtoB;
        if (abs(distance) < 0.001f)
        {
            normaltAtoB = { 0, 1 };
        }
        else
        {
            normaltAtoB = displacement / distance;
        }

        normaltAtoB = displacement / distance;
        Vector2 mtv = normaltAtoB * overlapCircle; // minimum translation vector. Shortest distance/direction needed to move circles

        circleA->position -= mtv * 0.5;
        circleB->position += mtv * 0.5;
        return true; // Overlapping
    }
    else
        return false; // Not overlapping

    // Overlapping = circleA.radius + circleB.radius - distance
    // normalize displacement vector and multiply it by overlapping distance
}

void checkCollisions()
{
    //for (int i = 0; i < objects.size(); i++) // Resets all circles to green if not touching
    //{
    //    objects[i]->color = GREEN;
    //}

    for (int i = 0; i < objects.size(); i++) // Overlap check
    {
        for (int j = i + 1; j < objects.size(); j++)
        {
            PhysicsBody* objectPointerA = objects[i];
            PhysicsBody* objectPointerB = objects[j];

            PhysicsShape shapeOfA = objectPointerA->Shape();
            PhysicsShape shapeOfB = objectPointerB->Shape();

            bool didOverlap = false;

            if (shapeOfA == CIRCLE && shapeOfB == CIRCLE)
            {
                didOverlap = (CircleOverlap((PhysicsCircle*)objectPointerA, (PhysicsCircle*)objectPointerB));
            }
            else if (shapeOfA == CIRCLE && shapeOfB == HALF_SPACE)
            {
                didOverlap = (HalfspaceOverlap((PhysicsCircle*)objectPointerA, (PhysicsHalfspace*)objectPointerB));
            }
            else if (shapeOfA == HALF_SPACE && shapeOfB == CIRCLE)
            {
                didOverlap = (HalfspaceOverlap((PhysicsCircle*)objectPointerB, (PhysicsHalfspace*)objectPointerA));
            }

            if (didOverlap)
            {
                //objectPointerA->color = RED;
                //objectPointerB->color = RED;
            }
        }
    }
}

void cleanup()
{
    for (int i = 0; i < objects.size(); i++)
    {
        PhysicsBody* obj = objects[i];

        if (obj->isStatic) continue;

        if (obj->Shape() == CIRCLE && obj->position.y > GetScreenHeight() || IsKeyDown(KEY_BACKSPACE))
        {
            auto iterator = (objects.begin() + i);
                      
            delete obj;

            objects.erase(iterator);
            i--;
        }
    }
}

void addGravityForce()
{
    for (int i = 0; i < objects.size(); i++)
    {
        if (objects[i]->isStatic) continue;

        Vector2 FGravity = gravityAcceleration * objects[i]->mass; // F = ma therefore Fg = object mass * accleration due to gravity
        objects[i]->netForce += FGravity;
    }
}

void resetNetForces()
{
    for (int i = 0; i < objects.size(); i++)
    {
        objects[i]->netForce = { 0, 0 };
    }
}

void addKinematics()
{
    // Adds physics to all angry birds created
    for (int i = 0; i < objects.size(); i++)
    {
        if (objects[i]->isStatic) continue;

        PhysicsCircle* circle = dynamic_cast<PhysicsCircle*>(objects[i]);
        if (circle && circle->isTimerActive)
        {
            circle->timer += dt;
        }

        objects[i]->position = objects[i]->position + objects[i]->projectileVelo * dt;

        Vector2 acceleration = objects[i]->netForce / objects[i]->mass;

        objects[i]->projectileVelo = objects[i]->projectileVelo + acceleration * dt;

        // Drawing netforces

        DrawLineEx(objects[i]->position, objects[i]->position + objects[i]->netForce, 3, PINK);

        // Draw gravity force 
        Vector2 FGravity = gravityAcceleration * objects[i]->mass;
        DrawLineEx(objects[i]->position, objects[i]->position + FGravity, 2, PURPLE);
    }
}

void spawnCircle(Vector2 spawnLocation, float mass, float friction, Color color)
{
    // Creates and allocates memory for a new circle
    PhysicsCircle* newCircle = new PhysicsCircle();
    newCircle->position = spawnLocation;
    newCircle->mass = mass;
    newCircle->coefficientOfFriction = friction;
    newCircle->projectileVelo = velocity;
    newCircle->color = color;
    objects.push_back(newCircle);
    // Adds a new circle to the list
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
        spawnCircle(launchPos, 1, 0.5f, GREEN);
    }
    if (IsKeyPressed(KEY_ONE))
    {
        spawnCircle(launchPos, 2, 0.1f, RED);
    }
    if (IsKeyPressed(KEY_TWO))
    {
        spawnCircle(launchPos, 2, 0.8f, GREEN);
    }
    if (IsKeyPressed(KEY_THREE))
    {
        spawnCircle(launchPos, 8, 0.1f, BLUE);
    }
    if (IsKeyPressed(KEY_FOUR))
    {
        spawnCircle(launchPos, 8, 0.8f, YELLOW);
    }

    resetNetForces();

    addGravityForce();

    checkCollisions();

    addKinematics();

    cleanup();
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
    // Halfspace Sliders
    GuiSliderBar(Rectangle{ 80, 270, 700, 20 }, "Halfspace X", TextFormat("%.0f", halfspace.position.x), &halfspace.position.x, 0, GetScreenWidth());
    GuiSliderBar(Rectangle{ 80, 310, 700, 20 }, "Halfspace y", TextFormat("%.0f", halfspace.position.y), &halfspace.position.y, 0, GetScreenHeight());
    float halfspaceRotation = halfspace.getRotation();
    GuiSliderBar(Rectangle{ 110, 350, 500, 20 }, "Halfspace Rotate", TextFormat("%.0f", halfspaceRotation), &halfspaceRotation, -180, 180);
    halfspace.setRotationDegrees(halfspaceRotation);
    //Friction Control (Might use later idk)
    //GuiSliderBar(Rectangle{ 110, 390, 500, 20 }, "Friction Control", TextFormat("%.1f", coefficientOfFriction), &coefficientOfFriction, 0, 1);

    // Text Box
    DrawRectangle(10, 30, 280, 100, BLACK);
    DrawRectangle(310, 30, 280, 100, BLACK);
    DrawRectangle(610, 30, 280, 100, BLACK);
    DrawRectangle(910, 30, 280, 100, BLACK);
    // Velocity Calculation
    velocity = { launchSpeed * cosf(rad), -launchSpeed * sinf(rad) };
    // Creating Line
    DrawLineEx(launchPos, Vector2{ launchPos + velocity }, 7, RED);
    // Number of Circles Spawned Text
    DrawText(TextFormat("Projectiles: %i", objects.size() - 2), 10, 430, 30, WHITE);
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
    // Draws each circle in list
    for (int i = 0; i < objects.size(); i++)
    {
        objects[i]->draw();
    }

    // Draw Free Body Diagram
    //Vector2 location = { (InitialWidth / 2), (InitialHeight / 2)};
    //DrawCircleLines(location.x, location.y, 100, WHITE);
    //float mass = 8;
    //// Draw Gravity
    //Vector2 FGravity = gravityAcceleration * mass;
    //DrawLineEx(location, location + FGravity, 3, PURPLE);
    //// Draw Normal Force
    //Vector2 FgPerp = halfspace.getNormal() * Vector2DotProduct(FGravity, halfspace.getNormal());
    //Vector2 FNormal = FgPerp * -1;
    //DrawLineEx(location, location + FNormal, 3, GREEN);
    //// Draw Friction
    //Vector2 FgPara = FGravity - FgPerp;
    //Vector2 Ffriction = FgPara * -1;
    //DrawLineEx(location, location + Ffriction, 3, ORANGE);

    EndDrawing();
}

int main()
{
    InitWindow(InitialWidth, InitialHeight, "Lucas Adda 101566961 2005 Week 11");
    SetTargetFPS(TARGET_FPS);
    halfspace.isStatic = true;
    halfspace.id = 1;
    halfspace.position = { 240, 500 };
    objects.push_back(&halfspace);
    halfspace.setRotationDegrees(50);

    halfspace2.isStatic = true;
    halfspace2.id = 2;
    halfspace2.position = { 700, 725 };
    objects.push_back(&halfspace2);

    launchPos = { 200.0f, 700.0f };
    launchAngle = 50.0f;
    launchSpeed = 350.0f;

    while (!WindowShouldClose()) // Loops TARGET_FPS per second
    {
        update();
        draw();
    }

    CloseWindow();
    return 0;
}