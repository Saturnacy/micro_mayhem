#include "raylib.h"
#include <math.h>
#include "game_scene.h"

#define MENU_OPTIONS 5
#define QP_OPTIONS 3

typedef enum {
    STATE_SPLASH_FADE_IN,
    STATE_SPLASH_CESAR,
    STATE_FADE_OUT,
    STATE_REVEAL_MM,
    STATE_TITLE_MM,
    STATE_MENU,
    STATE_QUICKPLAY_MENU,
    STATE_GAMEPLAY
} GameState;

typedef struct {
    Texture2D texture;
    Vector2 positionRatio;
    float scale;
} TitleBG;

void DrawRadialBackground(int screenWidth, int screenHeight);
void DrawInitialBackground(int screenWidth, int screenHeight, TitleBG *titleBGs, Texture2D mmLogo, float MMlogoScale);

int main(void) {
    int screenWidth = 1200;
    int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Micro Mayhem");
    InitAudioDevice();
    SetTargetFPS(60);

    GameState currentState = STATE_SPLASH_FADE_IN;

    Image icon = LoadImage("assets/exe_icon.png");
    SetWindowIcon(icon);

    Texture2D cesarLogo = LoadTexture("assets/cesar_logo.png");
    Texture2D mmLogo = LoadTexture("assets/title.png");
    Texture2D titlebg1 = LoadTexture("assets/title_bg1.png");
    Texture2D titlebg2 = LoadTexture("assets/title_bg2.png");
    Texture2D titlebg3 = LoadTexture("assets/title_bg3.png");
    Texture2D titlebg4 = LoadTexture("assets/title_bg4.png");
    Texture2D titlebg5 = LoadTexture("assets/title_bg5.png");
    Texture2D titlebg6 = LoadTexture("assets/title_bg6.png");
    Texture2D titlebg7 = LoadTexture("assets/title_bg7.png");
    Texture2D titlebg8 = LoadTexture("assets/title_bg8.png");
    Texture2D titlebg9 = LoadTexture("assets/title_bg9.png");
    Texture2D titlebg10 = LoadTexture("assets/title_bg10.png");
    Texture2D titlebg11 = LoadTexture("assets/title_bg11.png");
    Texture2D titlebg12 = LoadTexture("assets/title_bg12.png");
    Texture2D titlebg13 = LoadTexture("assets/title_bg13.png");
    Texture2D titlebg14 = LoadTexture("assets/title_bg14.png");
    Texture2D titlebg15 = LoadTexture("assets/title_bg15.png");
    Texture2D titlebg16 = LoadTexture("assets/title_bg16.png");
    Texture2D titlebg17 = LoadTexture("assets/title_bg17.png");
    Texture2D titlebg18 = LoadTexture("assets/title_bg18.png");

    Music menuMusic = LoadMusicStream("../assets/audio/menu_principal.ogg");
    menuMusic.looping = true;
    SetMasterVolume(1.0f);
    bool isMenuMusicPlaying = false;


    TitleBG titleBGs[20] = {
        { titlebg1, {0.34f, 0.195f}, 2.6f },
        { titlebg2, {0.35f, 0.79f}, 3.0f },
        { titlebg3, {0.8f, 0.36f}, 2.7f },
        { titlebg4, {0.56f, 0.17f}, 2.7f },
        { titlebg5, {0.73f, 0.18f}, 2.5f },
        { titlebg6, {0.39f, 0.79f}, 3.0f },
        { titlebg7, {0.88f, 0.83f}, 2.5f },
        { titlebg8, {0.88f, 0.26f}, 2.9f },
        { titlebg9, {0.125f, 0.72f}, 2.7f },
        { titlebg10, {0.16f, 0.17f}, 2.7f },
        { titlebg11, {0.12f, 0.42f}, 2.8f },
        { titlebg12, {0.28f, 0.77f}, 2.7f },
        { titlebg13, {0.67f, 0.81f}, 2.8f },
        { titlebg14, {0.895f, 0.47f}, 2.8f },
        { titlebg15, {0.21f, 0.82f}, 2.5f },
        { titlebg16, {0.86f, 0.47f}, 2.8f },
        { titlebg16, {0.825f, 0.47f}, 2.8f },
        { titlebg17, {0.615f, 0.82f}, 2.2f },
        { titlebg18, {0.22f, 0.35f}, 1.6f },
        { titlebg18, {0.78f, 0.72f}, 1.5f },
    };
    
    RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);

    Shader pixelShader = LoadShader(0, "assets/shaders/pixelizer.fs");

    Shader gradientShader = LoadShader(0, "assets/shaders/radial_gradient.fs");
    int resLoc = GetShaderLocation(gradientShader, "resolution");
    int centerLoc = GetShaderLocation(gradientShader, "colorCenter");
    int edgeLoc = GetShaderLocation(gradientShader, "colorEdge");

    Vector2 resolution = { (float)screenWidth, (float)screenHeight };

    float centerColor[3] = { 11/255.0f, 22/255.0f, 79/255.0f };
    float edgeColor[3] = { 9/255.0f, 15/255.0f, 29/255.0f };

    int pixelSizeLoc = GetShaderLocation(pixelShader, "pixelSize");
    int renderSizeLoc = GetShaderLocation(pixelShader, "renderSize");

    float renderSize[2] = { (float)screenWidth, (float)screenHeight };
    SetShaderValue(pixelShader, renderSizeLoc, renderSize, SHADER_UNIFORM_VEC2);

    char *text = "Press ENTER to begin";
    int fontSize = 30;
    int textWidth = MeasureText(text, fontSize);
    
    Vector2 textPosition = {
        (screenWidth - textWidth) / 2.0f,
        (screenHeight / 2.0f) + 150
    };
    
    float CESARlogoscale = 2.0f;
    float MMlogoScale = 2.0f; 
    bool running = true;
    int frameCounter = 0;
    float currentPixelSize = 20.0f;
    float fadeAlpha = 255.0f;

    char* menuOptions[MENU_OPTIONS] = {
        "Quick Play",
        "Arcade",
        "Extras",
        "Settings",
        "Quit Game"
    };

    char *QP_options[3] = {
        "Singleplayer",
        "Multiplayer",
        "Return"
    };

    int selectedOption = 0;
    int optionFontSize = 40;

    while (running && !WindowShouldClose()) {
        
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        if (isMenuMusicPlaying) UpdateMusicStream(menuMusic);

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
                    currentState = STATE_REVEAL_MM;
                    
                    SetShaderValue(pixelShader, pixelSizeLoc, &currentPixelSize, SHADER_UNIFORM_FLOAT);
                }
                break;

            case STATE_REVEAL_MM:
            {
                bool pixelDone = false;
                bool fadeDone = false;

                if (currentPixelSize > 1.0f) {
                    currentPixelSize -= 0.3f;
                    SetShaderValue(pixelShader, pixelSizeLoc, &currentPixelSize, SHADER_UNIFORM_FLOAT);
                } else {
                    currentPixelSize = 1.0f;
                    pixelDone = true;
                }

                if (fadeAlpha > 0.0f) {
                    fadeAlpha -= 8.0f;
                } else {
                    fadeAlpha = 0.0f;
                    fadeDone = true;
                }

                if (pixelDone && fadeDone) {
                    currentState = STATE_TITLE_MM;
                }
            }
                break;

            case STATE_TITLE_MM:
                if (!isMenuMusicPlaying) {
                    PlayMusicStream(menuMusic);
                    isMenuMusicPlaying = true;
                }
                if (IsKeyPressed(KEY_ENTER)){
                    currentState = STATE_MENU;
                }
                break;
            
            case STATE_MENU:
                if (IsKeyPressed(KEY_DOWN)) {
                    selectedOption++;
                    if (selectedOption >= MENU_OPTIONS) selectedOption = 0;
                }

                if (IsKeyPressed(KEY_UP)) {
                    selectedOption--;
                    if (selectedOption < 0) selectedOption = MENU_OPTIONS - 1;
                }

                if (IsKeyPressed(KEY_ENTER)) {
                    switch (selectedOption) {
                        case 0:
                            currentState = STATE_QUICKPLAY_MENU;
                            selectedOption = 0;
                            break;

                        case 1:
                            break;

                        case 2:
                            break;

                        case 3:
                            break;
                        
                        case 4:
                            running = false;
                            break;
                    }
                }
                break;

            case STATE_QUICKPLAY_MENU:
                if (IsKeyPressed(KEY_DOWN)) {
                    selectedOption++;
                    if (selectedOption >= QP_OPTIONS) selectedOption = 0;
                }

                if (IsKeyPressed(KEY_UP)) {
                    selectedOption--;
                    if (selectedOption < 0) selectedOption = QP_OPTIONS - 1;
                }

                if (IsKeyPressed(KEY_ENTER)) {
                    switch (selectedOption) {
                        case 0:
                            GameScene_Init();
                            if (isMenuMusicPlaying) {
                                StopMusicStream(menuMusic);
                                isMenuMusicPlaying = false;
                            }
                            currentState = STATE_GAMEPLAY;
                            break;
                        case 1:
                            break;
                        case 2:
                            currentState = STATE_MENU;
                            selectedOption = 0;
                            break;
                    }
                }
                break;

            case STATE_GAMEPLAY:
                GameScene_Update();
                break;
        }

        BeginTextureMode(target);
            if (currentState == STATE_REVEAL_MM || currentState == STATE_TITLE_MM || currentState == STATE_MENU || currentState == STATE_QUICKPLAY_MENU) {
                BeginShaderMode(gradientShader);

                SetShaderValue(gradientShader, resLoc, &resolution, SHADER_UNIFORM_VEC2);
                SetShaderValue(gradientShader, centerLoc, centerColor, SHADER_UNIFORM_VEC3);
                SetShaderValue(gradientShader, edgeLoc, edgeColor, SHADER_UNIFORM_VEC3);

                DrawRectangle(0, 0, screenWidth, screenHeight, WHITE);

                EndShaderMode();
            } else {
                ClearBackground(BLACK);
            }

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
                } break;

                case STATE_REVEAL_MM:
                case STATE_TITLE_MM:
                    DrawInitialBackground(screenWidth, screenHeight, titleBGs, mmLogo, MMlogoScale);
                    
                    Rectangle sourceRec = { 0.0f, 0.0f, (float)mmLogo.width, (float)mmLogo.height };
                    float scaledWidth = (float)mmLogo.width * MMlogoScale;
                    float scaledHeight = (float)mmLogo.height * MMlogoScale;
                    Rectangle destRec = {
                        (screenWidth - scaledWidth) / 2.0f,
                        (screenHeight - scaledHeight) / 2.0f,
                        scaledWidth,
                        scaledHeight
                    };

                    Vector2 origin = { 0.0f, 0.0f };
                    DrawTexturePro(mmLogo, sourceRec, destRec, origin, 0.0f, WHITE);
                    break;

                case STATE_MENU:
                {
                    DrawInitialBackground(screenWidth, screenHeight, titleBGs, mmLogo, MMlogoScale);

                    const char *menuTitle = "MAIN MENU";
                    int titleSize = 60;
                    int titleWidth = MeasureText(menuTitle, titleSize);
                    DrawText(menuTitle,
                        (screenWidth - titleWidth) / 2,
                        screenHeight * 0.18f,
                        titleSize,
                        WHITE
                    );

                    for (int i = 0; i < MENU_OPTIONS; i++) {
                        int textWidth = MeasureText(menuOptions[i], optionFontSize);
                        
                        Color optionColor = (i == selectedOption)
                            ? (Color){255, 255, 200, 255}
                            : (Color){200, 200, 255, 255};

                        DrawText(
                            menuOptions[i],
                            (screenWidth - textWidth) / 2,
                            screenHeight * 0.35f + i * 60,
                            optionFontSize,
                            optionColor
                        );

                        if (i == selectedOption) {
                            DrawTriangle(
                                (Vector2){ (screenWidth - textWidth) / 2 - 30, screenHeight * 0.35f + i * 60 + 20 },
                                (Vector2){ (screenWidth - textWidth) / 2 - 10, screenHeight * 0.35f + i * 60 + 10 },
                                (Vector2){ (screenWidth - textWidth) / 2 - 10, screenHeight * 0.35f + i * 60 + 30 },
                                optionColor
                            );
                        }
                    }
                }
                break;

                case STATE_QUICKPLAY_MENU:
                    DrawInitialBackground(screenWidth, screenHeight, titleBGs, mmLogo, MMlogoScale);

                    for (int i = 0; i < 3; i++) {
                        int textWidth = MeasureText(QP_options[i], optionFontSize);
                        
                        Color optionColor = (i == selectedOption)
                            ? (Color){255, 255, 200, 255}
                            : (Color){200, 200, 255, 255};

                        DrawText(
                            QP_options[i],
                            (screenWidth - textWidth) / 2,
                            screenHeight * 0.35f + i * 60,
                            optionFontSize,
                            optionColor
                        );

                        if (i == selectedOption) {
                            DrawTriangle(
                                (Vector2){ (screenWidth - textWidth) / 2 - 30, screenHeight * 0.35f + i * 60 + 20 },
                                (Vector2){ (screenWidth - textWidth) / 2 - 10, screenHeight * 0.35f + i * 60 + 10 },
                                (Vector2){ (screenWidth - textWidth) / 2 - 10, screenHeight * 0.35f + i * 60 + 30 },
                                optionColor
                            );
                        }
                    }
                    break;
                
                case STATE_GAMEPLAY:
                    GameScene_Draw();
                    break;
            }
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);

            if (currentState == STATE_REVEAL_MM) {
                BeginShaderMode(pixelShader);
                    DrawTextureRec(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2){ 0, 0 }, WHITE);
                EndShaderMode();
            } else {
                DrawTextureRec(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2){ 0, 0 }, WHITE);
            }

            if (currentState == STATE_SPLASH_FADE_IN || currentState == STATE_FADE_OUT || currentState == STATE_REVEAL_MM) {
                DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 0, 0, 0, (unsigned char)fadeAlpha });
            }

            if (currentState == STATE_TITLE_MM) {
                bool showText = fmod(GetTime(), 1.0) < 0.5;
                if (showText) {
                    DrawText(text, textPosition.x, textPosition.y, fontSize, RAYWHITE);
                }
            }

        EndDrawing();
    }
    
    GameScene_Unload();
    UnloadRenderTexture(target);
    UnloadShader(pixelShader);
    UnloadShader(gradientShader);
    UnloadTexture(mmLogo);
    UnloadTexture(cesarLogo);
    UnloadTexture(titlebg1);
    UnloadTexture(titlebg2);
    UnloadTexture(titlebg3);
    UnloadTexture(titlebg4);
    UnloadTexture(titlebg5);
    UnloadTexture(titlebg6);
    UnloadTexture(titlebg7);
    UnloadTexture(titlebg8);
    UnloadTexture(titlebg9);
    UnloadTexture(titlebg10);
    UnloadTexture(titlebg11);
    UnloadTexture(titlebg12);
    UnloadTexture(titlebg13);
    UnloadTexture(titlebg14);
    UnloadTexture(titlebg15);
    UnloadTexture(titlebg16);
    UnloadTexture(titlebg17);
    UnloadTexture(titlebg18);
    UnloadImage(icon);

    StopMusicStream(menuMusic);
    UnloadMusicStream(menuMusic);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}

void DrawRadialBackground(int screenWidth, int screenHeight) {
    Color darkBlue = (Color){8, 14, 25, 255};
    ClearBackground(darkBlue);

    Vector2 center = { screenWidth / 2.0f, screenHeight / 2.0f };
    float maxRadius = screenWidth * 0.6f;

    for (int r = (int)maxRadius; r > 0; r--) {
        float t = (float)r / maxRadius;
        unsigned char brightness = (unsigned char)(50 + 70 * t);
        Color color = (Color){brightness / 2, brightness / 2 + 20, 120 - (unsigned char)(70 * t), 255};
        DrawCircleV(center, r, color);
    }
}

void DrawInitialBackground(int screenWidth, int screenHeight, TitleBG *titleBGs, Texture2D mmLogo, float MMlogoScale) {
    float thickness = 4.0f;
    Color lineColor = (Color){ 52, 62, 102, 255 };

    DrawLineEx(
        (Vector2){screenWidth * 0.088f, screenHeight * 0.09f},
        (Vector2){screenWidth * 0.088f, screenHeight * 0.6f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.088f, screenHeight * 0.6f},
        (Vector2){screenWidth * 0.159f, screenHeight * 0.6f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.29f, screenHeight * 0.91f},
        (Vector2){screenWidth * 0.72f, screenHeight * 0.91f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.265f, screenHeight * 0.09f},
        (Vector2){screenWidth * 0.84f, screenHeight * 0.09f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.84f, screenHeight * 0.09f},
        (Vector2){screenWidth * 0.84f, screenHeight * 0.2f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.912f, screenHeight * 0.55f},
        (Vector2){screenWidth * 0.912f, screenHeight * 0.91f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.83f, screenHeight * 0.55f},
        (Vector2){screenWidth * 0.912f, screenHeight * 0.55f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.24f, screenHeight * 0.55f},
        (Vector2){screenWidth * 0.24f, screenHeight * 0.76f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.3f, screenHeight * 0.26f},
        (Vector2){screenWidth * 0.38f, screenHeight * 0.26f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.46f, screenHeight * 0.15f},
        (Vector2){screenWidth * 0.46f, screenHeight * 0.21f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.37f, screenHeight * 0.15f},
        (Vector2){screenWidth * 0.46f, screenHeight * 0.15f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.53f, screenHeight * 0.79f},
        (Vector2){screenWidth * 0.53f, screenHeight * 0.83f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.53f, screenHeight * 0.83f},
        (Vector2){screenWidth * 0.59f, screenHeight * 0.83f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.73f, screenHeight * 0.25f},
        (Vector2){screenWidth * 0.73f, screenHeight * 0.41f},
        thickness, lineColor);

    DrawLineEx(
        (Vector2){screenWidth * 0.64f, screenHeight * 0.25f},
        (Vector2){screenWidth * 0.73f, screenHeight * 0.25f},
        thickness, lineColor);

    for (int i = 0; i < 20; i++) {
        Texture2D tex = titleBGs[i].texture;
        float scale = titleBGs[i].scale;

        float texWidth = tex.width * scale;
        float texHeight = tex.height * scale;

        Vector2 position = {
            screenWidth * titleBGs[i].positionRatio.x - texWidth / 2.0f,
            screenHeight * titleBGs[i].positionRatio.y - texHeight / 2.0f
        };

        DrawTextureEx(tex, position, 0.0f, scale, WHITE);
    }

    Rectangle sourceRec = { 0.0f, 0.0f, (float)mmLogo.width, (float)mmLogo.height };
    float scaledWidth = (float)mmLogo.width * MMlogoScale;
    float scaledHeight = (float)mmLogo.height * MMlogoScale;
    Rectangle destRec = {
        (screenWidth - scaledWidth) / 2.0f,
        (screenHeight - scaledHeight) / 2.0f,
        scaledWidth,
        scaledHeight
    };

    Vector2 origin = { 0.0f, 0.0f };
    DrawTexturePro(mmLogo, sourceRec, destRec, origin, 0.0f, (Color){ 52, 62, 102, 255 });
}