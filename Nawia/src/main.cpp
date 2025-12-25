#include <Engine.h>

using namespace Nawia;

int main(int argc, char* argv[]) {
    Core::Engine game;

    if (game.isRunning()) {
        game.run();
    }

    return 0;
}
/*
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

int main(void)
{
    const int screenWidth = 1000;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "TPS Player Controller");

    Model model = LoadModel("../assets/models/player_walk.glb");
    int animsCount = 0;
    unsigned int animFrameCounter = 0;
    ModelAnimation* anims = LoadModelAnimations("../assets/models/player_walk.glb", &animsCount);

    Vector3 playerPosition = { 0.0f, 0.0f, 0.0f };
    float playerRotation = 0.0f;
    float moveSpeed = 0.1f;
    float rotationSpeed = 2.0f;

    float visualRotationOffset = 0.0f;
    float modelScale = 0.01f;

    Camera camera = { 0 };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    float cameraDistance = 10.0f;
    float cameraHeight = 5.0f;

    bool playAnim = true;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        // handle input
        if (IsKeyDown(KEY_A)) playerRotation += rotationSpeed;
        if (IsKeyDown(KEY_D)) playerRotation -= rotationSpeed;
        if (IsKeyDown(KEY_W))
        {
            playerPosition.x += sinf(playerRotation * DEG2RAD) * moveSpeed;
            playerPosition.z += cosf(playerRotation * DEG2RAD) * moveSpeed;
        }
        if (IsKeyDown(KEY_S))
        {
            playerPosition.x -= sinf(playerRotation * DEG2RAD) * moveSpeed;
            playerPosition.z -= cosf(playerRotation * DEG2RAD) * moveSpeed;
        }

        // handle zoom scrolling
        if (auto wheel = GetMouseWheelMove()) {
            cameraDistance -= wheel * 1.0f;
            cameraDistance = (cameraDistance < 2.0f ? 20.0f : cameraDistance);
            cameraDistance = (cameraDistance > 50.0f ? 50.0f : cameraDistance);
        }

        // place camera above player
        camera.target = Vector3{ playerPosition.x, playerPosition.y + 1.0f, playerPosition.z };
        // move camera behind the player
        camera.position.x = playerPosition.x - (sinf(playerRotation * DEG2RAD) * cameraDistance);
        camera.position.z = playerPosition.z - (cosf(playerRotation * DEG2RAD) * cameraDistance);
        camera.position.y = playerPosition.y + cameraHeight;

        if (animsCount > 0 && playAnim)
        {
            animFrameCounter++;
            UpdateModelAnimation(model, anims[0], animFrameCounter);
            if (animFrameCounter >= anims[0].frameCount) animFrameCounter = 0;
        }

        // drawing
        BeginDrawing();

        ClearBackground(RAYWHITE);

        // model
        BeginMode3D(camera);
			DrawModelEx(model, playerPosition, Vector3{ 0.0f, 1.0f, 0.0f }, 
            playerRotation + visualRotationOffset, Vector3{ modelScale, modelScale, modelScale }, WHITE);
			DrawGrid(20, 1.0f);
        EndMode3D();

        // UI
        DrawRectangle(10, 10, 350, 120, Fade(SKYBLUE, 0.5f));
        DrawRectangleLines(10, 10, 350, 120, BLUE);
        DrawText("- move: WASD", 20, 45, 15, BLACK);
        DrawText("- scroll: camera zoom", 20, 85, 15, BLACK);
        DrawText(TextFormat("Pos: [%.2f, %.2f]", playerPosition.x, playerPosition.z), 20, 105, 15, DARKGRAY);

        EndDrawing();
    }

    if (animsCount > 0) 
        UnloadModelAnimations(anims, animsCount);

    UnloadModel(model);
    CloseWindow();

    return 0;
}
*/