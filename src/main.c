#include "raylib.h"
#include <math.h>

int main(void)
{
    int screenWidth = 1200;
    int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Micro Mayhem");
    SetTargetFPS(60);

    Texture2D logo = LoadTexture("assets/title.png");

    char *text = "Press ENTER to begin";
    int fontSize = 30;
    int textWidth = MeasureText(text, fontSize);
    
    Vector2 textPosition = {
        (screenWidth - textWidth) / 2.0f,
        (screenHeight / 2.0f) + 150
    };
    
    float logoScale = 0.2f; 
    bool running = true;

    while (running && !WindowShouldClose()) {
        bool showText = fmod(GetTime(), 1.0) < 0.5;

        if (IsKeyPressed(KEY_ENTER)){
            running = false;
        }

        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        BeginDrawing();

        ClearBackground(BLACK);
            if (logo.id > 0)
            {
                Rectangle sourceRec = { 0.0f, 0.0f, (float)logo.width, (float)logo.height };

                float scaledWidth = (float)logo.width * logoScale;
                float scaledHeight = (float)logo.height * logoScale;

                Rectangle destRec = {
                    (screenWidth - scaledWidth) / 2.0f,
                    (screenHeight / 2.0f) - scaledHeight,
                    scaledWidth,
                    scaledHeight
                };
                
                Vector2 origin = { 0.0f, 0.0f };
                
                DrawTexturePro(logo, sourceRec, destRec, origin, 0.0f, WHITE);
            }

            if (showText) {
                DrawText(text, textPosition.x, textPosition.y, fontSize, RAYWHITE);
            }

        EndDrawing();
    }
    UnloadTexture(logo);

    CloseWindow();

    return 0;
}