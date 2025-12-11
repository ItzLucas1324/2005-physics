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

float circleMass = 1.0f;
int currentBirdType = 1;

enum PhysicsShape
{
    CIRCLE,
    HALF_SPACE,
    BLOCK
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
    float bounciness = 0.9f; // for determing coefficient of restitution

    virtual void draw() {}

    virtual PhysicsShape Shape() = 0;
};

class PhysicsCircle : public PhysicsBody
{
public:
    Color color = GREEN;
    float radius = 30.0f;
    //float timer = 0.0f;
    bool isTimerActive = true;

    void draw() override
    {
        DrawCircleV(position, radius, color);
        DrawText(TextFormat("%.1f", mass), position.x - 14, position.y - 12, 25, BLACK);
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

class PhysicsBlock : public PhysicsBody
{
public:
    Vector2 halfExtents = { 20, 20 };
    Color color = BROWN;
    bool isBird = false;

    void draw() override
    {
        float left = position.x - halfExtents.x;
        float top = position.y - halfExtents.y;

        DrawRectangle(left, top, halfExtents.x * 2, halfExtents.y * 2, color);

        DrawRectangleLines(left, top, halfExtents.x * 2, halfExtents.y * 2, BLACK);

        DrawText(TextFormat("%.1f", mass), position.x - 14, position.y - 12, 25, WHITE);
    }

    PhysicsShape Shape() override 
    { 
        return BLOCK; 
    }
};

std::vector<PhysicsBody*> objects;
PhysicsHalfspace halfspace;
//PhysicsHalfspace halfspace2;

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
        //if (halfspace->id == 2)
        //{
        //    circle->isTimerActive = false;
        //}

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

        // Bouncing!
        // From perspective of A
        //Vector2 velocityBRelativeToA = circleB->projectileVelo - circleA->projectileVelo;
        float closingVelocity = Vector2DotProduct(circle->projectileVelo, halfspace->getNormal());

        // If is negative then we are colliding. If positive not colliding
        if (closingVelocity >= 0) return true;

        float restitution = circle->bounciness * halfspace->bounciness;
        circle->projectileVelo += halfspace->getNormal() * closingVelocity * -(1.0f + restitution);
        
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

    if (overlapCircle >= 0)
    {
        Vector2 normalAtoB;
        if (abs(distance) < 0.001f)
        {
            normalAtoB = { 0, 1 };
        }
        else
        {
            normalAtoB = displacement / distance;
        }

        normalAtoB = displacement / distance;
        Vector2 mtv = normalAtoB * overlapCircle; // minimum translation vector. Shortest distance/direction needed to move circles

        circleA->position -= mtv * 0.5;
        circleB->position += mtv * 0.5;

        // From perspective of A
        Vector2 velocityBRelativeToA = circleB->projectileVelo - circleA->projectileVelo;
        float closingVelocity = Vector2DotProduct(velocityBRelativeToA, normalAtoB);

        // If is negative then we are colliding. If positive not colliding
        if (closingVelocity >= 0) return true;
        
        float restitution = circleA->bounciness * circleB->bounciness;

        float totalMass = circleA->mass + circleB->mass;
        float impulseMagnitude = ((1.0f + restitution) * closingVelocity * circleA->mass * circleB->mass) / totalMass;
        // A -->  <-- B
        Vector2 impulseForB = normalAtoB * -impulseMagnitude;
        Vector2 impulseForA = normalAtoB * impulseMagnitude;

        // Apply impulse
        circleA->projectileVelo += impulseForA / circleA->mass;
        circleB->projectileVelo += impulseForB / circleB->mass;

        return true; // Overlapping
    }
    else
        return false; // Not overlapping

    // Overlapping = circleA.radius + circleB.radius - distance
    // normalize displacement vector and multiply it by overlapping distance
}

bool BlockOverlap(PhysicsBlock* A, PhysicsBlock* B)
{
    Vector2 delta = B->position - A->position;

    float overlapX = (A->halfExtents.x + B->halfExtents.x) - fabsf(delta.x);
    if (overlapX <= 0) return false;

    float overlapY = (A->halfExtents.y + B->halfExtents.y) - fabsf(delta.y);
    if (overlapY <= 0) return false;

    Vector2 mtv = { 0, 0 };
    Vector2 normal = { 0, 0 };

    if (overlapX < overlapY)
    {
        mtv.x = (delta.x > 0 ? overlapX : -overlapX);
        normal.x = (delta.x > 0 ? 1.0f : -1.0f);
    }
    else
    {
        mtv.y = (delta.y > 0 ? overlapY : -overlapY);
        normal.y = (delta.y > 0 ? 1.0f : -1.0f);
    }

    float invA = (A->isStatic || A->mass <= 0.0f) ? 0.0f : 1.0f / A->mass;
    float invB = (B->isStatic || B->mass <= 0.0f) ? 0.0f : 1.0f / B->mass;
    float invSum = invA + invB;

    if (invSum > 0.0f)
    {
        float shareA = invA / invSum;
        float shareB = invB / invSum;

        A->position -= mtv * shareA;
        B->position += mtv * shareB;
    }

    Vector2 relVel = B->projectileVelo - A->projectileVelo;
    float closingVel = Vector2DotProduct(relVel, normal);

    if (closingVel >= 0.0f || invSum <= 0.0f)
        return true;

    float restitution = A->bounciness * B->bounciness;

    float j = -(1.0f + restitution) * closingVel / invSum;
    Vector2 impulse = normal * j;

    if (invA > 0.0f)
        A->projectileVelo -= impulse * invA;
    if (invB > 0.0f)
        B->projectileVelo += impulse * invB;

    return true;
}


bool CircleBlockOverlap(PhysicsCircle* C, PhysicsBlock* B)
{
    float minX = B->position.x - B->halfExtents.x;
    float maxX = B->position.x + B->halfExtents.x;
    float minY = B->position.y - B->halfExtents.y;
    float maxY = B->position.y + B->halfExtents.y;

    Vector2 closestPoint = {
        fmaxf(minX, fminf(C->position.x, maxX)),
        fmaxf(minY, fminf(C->position.y, maxY))
    };

    Vector2 diff = C->position - closestPoint;
    float dist = Vector2Length(diff);

    Vector2 normal;
    float overlap;

    if (dist < 0.001f)
    {
        float dx = fminf(C->position.x - minX, maxX - C->position.x);
        float dy = fminf(C->position.y - minY, maxY - C->position.y);

        if (dx < dy) normal = { (C->position.x < B->position.x) ? -1.f : 1.f, 0.f };
        else         normal = { 0.f, (C->position.y < B->position.y) ? -1.f : 1.f };

        overlap = C->radius + fminf(dx, dy);
    }
    else
    {
        normal = diff / dist;
        overlap = C->radius - dist;
        if (overlap <= 0) return false;
    }

    Vector2 mtv = normal * overlap;

    float invC = (!C->isStatic && C->mass > 0) ? 1.f / C->mass : 0.f;
    float invB = (!B->isStatic && B->mass > 0) ? 1.f / B->mass : 0.f;
    float invSum = invC + invB;

    if (invSum > 0)
    {
        C->position += mtv * (invC / invSum);
        B->position -= mtv * (invB / invSum);
    }

    Vector2 relVel = C->projectileVelo - B->projectileVelo;
    float closingVel = Vector2DotProduct(relVel, normal);
    if (closingVel >= 0) return true;

    float restitution = C->bounciness * B->bounciness;
    float j = -(1.f + restitution) * closingVel / invSum;
    Vector2 impulse = normal * j;

    if (invC > 0) C->projectileVelo += impulse * invC;
    if (invB > 0) B->projectileVelo -= impulse * invB;

    return true;
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
            else if (shapeOfA == BLOCK && shapeOfB == BLOCK)
            {
                didOverlap = (BlockOverlap((PhysicsBlock*)objectPointerA, (PhysicsBlock*)objectPointerB));
            }
            else if (shapeOfA == BLOCK && shapeOfB == CIRCLE)
            {
                didOverlap = (CircleBlockOverlap((PhysicsCircle*)objectPointerB, (PhysicsBlock*)objectPointerA));
            }
            else if (shapeOfA == CIRCLE && shapeOfB == BLOCK)
            {
                didOverlap = (CircleBlockOverlap((PhysicsCircle*)objectPointerA, (PhysicsBlock*)objectPointerB));
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

        if (obj->isStatic && obj->Shape() == HALF_SPACE) continue;

        if (obj->Shape() == CIRCLE && (obj->position.y > GetScreenHeight() || IsKeyDown(KEY_BACKSPACE)))
        {
            delete obj;
            objects.erase(objects.begin() + i);
            i--;
            continue;
        }

        if (obj->Shape() == BLOCK && obj->position.y > GetScreenHeight())
        {
            delete obj;
            objects.erase(objects.begin() + i);
            i--;
            continue;
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
        //if (circle && circle->isTimerActive)
        //{
        //    circle->timer += dt;
        //}

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

void spawnBlock(Vector2 pos)
{
    PhysicsBlock* newBlock = new PhysicsBlock();
    newBlock->position = pos;
    newBlock->halfExtents = { 30, 30 };
    newBlock->color = BLACK;
    newBlock->mass = circleMass;
    newBlock->projectileVelo = velocity;
    objects.push_back(newBlock);
}

void update()
{
    dt = 1.0f / TARGET_FPS;
    time += dt;
    rad = launchAngle * DEG2RAD;

    if (IsKeyPressed(KEY_ONE))
        currentBirdType = 1;

    if (IsKeyPressed(KEY_TWO))
        currentBirdType = 2;

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
        if (currentBirdType == 1)
            spawnCircle(launchPos, circleMass, 0.5f, GREEN);
        if (currentBirdType == 2)
            spawnBlock(launchPos);
    }

    resetNetForces();

    addGravityForce();

    checkCollisions();

    addKinematics();

    cleanup();
}

void spawnAABBTower()
{
    float x = 800;          // tower horizontal position
    float baseY = 650;      // height of the ground
    float blockSize = 30;   // half extents of block
    int numBlocks = 5;      // height of tower

    for (int i = 0; i < numBlocks; i++)
    {
        PhysicsBlock* block = new PhysicsBlock();

        block->halfExtents = { blockSize, blockSize };

        if (i == 0)
        {
            block->halfExtents = { blockSize * 10, blockSize };
            block->isStatic = true;
        }

        block->position = { x, baseY - i * (block->halfExtents.y * 2 + 2) };
        block->color = BROWN;
        block->mass = 5;
        block->projectileVelo = { 0,0 };

        objects.push_back(block);
    }
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
    GuiSliderBar(Rectangle{ 900, 150, 250, 20 }, "Circle Mass", TextFormat("%.1f", circleMass), &circleMass, 1, 10);

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
    DrawText(TextFormat("Projectiles: %i", objects.size() - 1), 10, 400, 30, WHITE);
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
    InitWindow(InitialWidth, InitialHeight, "Lucas Adda 101566961 2005 Week 15");
    SetTargetFPS(TARGET_FPS);
    halfspace.isStatic = true;
    halfspace.id = 1;
    halfspace.position = { 600, 700 };
    objects.push_back(&halfspace);

    spawnAABBTower();

    launchPos = { 200.0f, 700.0f };
    launchAngle = 50.0f;
    launchSpeed = 0.0f;

    while (!WindowShouldClose()) // Loops TARGET_FPS per second
    {
        update();
        draw();
    }

    CloseWindow();
    return 0;
}