#include "raylib.h"
#include <math.h>

typedef enum {
    STATE_SPLASH_FADE_IN,
    STATE_SPLASH_CESAR,
    STATE_FADE_OUT,
    STATE_FADE_IN,
    STATE_TITLE_MM
} GameState;

int main(void) {
    int screenWidth = 1200;
    int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Micro Mayhem");
    SetTargetFPS(60);

    GameState currentState = STATE_SPLASH_FADE_IN;

    Texture2D cesarLogo = LoadTexture("assets/cesar_logo.png");
    Texture2D mmLogo = LoadTexture("assets/title.png");

    char *text = "Press ENTER to begin";
    int fontSize = 30;
    int textWidth = MeasureText(text, fontSize);
    
    Vector2 textPosition = {
        (screenWidth - textWidth) / 2.0f,
        (screenHeight / 2.0f) + 150
    };
    
    float CESARlogoscale = 2.0f;
    float MMlogoScale = 0.2f; 
    bool running = true;
    int frameCounter = 0;
    float fadeAlpha = 255.0f;

    while (running && !WindowShouldClose()) {
        
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        switch (currentState) {
            case STATE_SPLASH_FADE_IN:
                fadeAlpha -= 8.0f;
                if (fadeAlpha <= 0) {
                    fadeAlpha = 0;
                    currentState = STATE_SPLASH_CESAR;
                }
                break;
            case STATE_SPLASH_CESAR:
                frameCounter++;
                if (frameCounter > 120) {
                    currentState = STATE_FADE_OUT;
                    frameCounter = 0;
                }
                break;

            case STATE_FADE_OUT:
                fadeAlpha += 8.0f;
                if (fadeAlpha >= 255.0f) {
                    fadeAlpha = 255.0f;
                    currentState = STATE_FADE_IN;
                }
                break;

            case STATE_FADE_IN:
                fadeAlpha -= 4.0f;
                if (fadeAlpha <= 0) {
                    fadeAlpha = 0;
                    currentState = STATE_TITLE_MM;
                }
                break;

            case STATE_TITLE_MM:
                if (IsKeyPressed(KEY_ENTER)){
                    running = false;        // por enquanto, o jogo fecha após apertar a tecla
                }
                break;
        }

        BeginDrawing();
            ClearBackground(BLACK);

            switch (currentState) {
                case STATE_SPLASH_FADE_IN:
                case STATE_SPLASH_CESAR:
                case STATE_FADE_OUT:
                {
                    Rectangle sourceRec = { 0.0f, 0.0f, (float)cesarLogo.width, (float)cesarLogo.height };

                    float scaledWidth = (float)cesarLogo.width * CESARlogoscale;
                    float scaledHeight = (float)cesarLogo.height * CESARlogoscale;

                    Rectangle destRec = {
                        (screenWidth - scaledWidth) / 2.0f,
                        (screenHeight - scaledHeight) / 2.0f,
                        scaledWidth,
                        scaledHeight
                    };
                
                    Vector2 origin = { 0.0f, 0.0f };

                    DrawTexturePro(cesarLogo, sourceRec, destRec, origin, 0.0f, WHITE);
                }   break;

                case STATE_FADE_IN:
                case STATE_TITLE_MM:
                    {
                    Rectangle sourceRec = { 0.0f, 0.0f, (float)mmLogo.width, (float)mmLogo.height };

                    float scaledWidth = (float)mmLogo.width * MMlogoScale;
                    float scaledHeight = (float)mmLogo.height * MMlogoScale;

                    Rectangle destRec = {
                        (screenWidth - scaledWidth) / 2.0f,
                        (screenHeight / 2.0f) - scaledHeight,
                        scaledWidth,
                        scaledHeight
                    };

                    Vector2 origin = { 0.0f, 0.0f };
                    DrawTexturePro(mmLogo, sourceRec, destRec, origin, 0.0f, WHITE);

                    if (currentState == STATE_TITLE_MM) {
                        bool showText = fmod(GetTime(), 1.0) < 0.5;
                        if (showText) {
                            DrawText(text, textPosition.x, textPosition.y, fontSize, RAYWHITE);
                        }
                    }
                }   break;
            }

            if (currentState == STATE_SPLASH_FADE_IN || currentState == STATE_FADE_OUT || currentState == STATE_FADE_IN) {
                DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 0, 0, 0, (unsigned char)fadeAlpha });
            }

        EndDrawing();
    }
    
    UnloadTexture(mmLogo);
    UnloadTexture(cesarLogo);

    CloseWindow();

    return 0;
}