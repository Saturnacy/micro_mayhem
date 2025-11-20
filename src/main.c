#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include "game_scene.h"

#define MENU_OPTIONS 5
#define QP_OPTIONS 3
#define BG_COUNT 20
#define UNIQUE_BG_COUNT 18
#define SETTINGS_OPTIONS 6

typedef enum {
    STATE_SPLASH_FADE_IN,
    STATE_SPLASH_CESAR,
    STATE_FADE_OUT,
    STATE_REVEAL_MM,
    STATE_TITLE_MM,
    STATE_MENU,
    STATE_QUICKPLAY_MENU,
    STATE_SETTINGS,
    STATE_CREDITS,
    STATE_GAMEPLAY
} GameState;

typedef struct {
    Texture2D texture;
    Vector2 positionRatio;
    float scale;
} TitleBG;

void LoadTexturesInLoop(Texture2D textures[], const char *paths[], int count) {
    for (int i = 0; i < count; i++) {
        textures[i] = LoadTexture(paths[i]);
    }
}

void UnloadTexturesInLoop(Texture2D textures[], int count) {
    for (int i = 0; i < count; i++) {
        UnloadTexture(textures[i]);
    }
}

void DrawRadialBackground(int screenWidth, int screenHeight);
void DrawInitialBackground(int screenWidth, int screenHeight, TitleBG *titleBGs, Texture2D mmLogo, float MMlogoScale);

const char *TitleBGPaths[UNIQUE_BG_COUNT] = {
    "assets/title_bg1.png", "assets/title_bg2.png", "assets/title_bg3.png", "assets/title_bg4.png",
    "assets/title_bg5.png", "assets/title_bg6.png", "assets/title_bg7.png", "assets/title_bg8.png",
    "assets/title_bg9.png", "assets/title_bg10.png", "assets/title_bg11.png", "assets/title_bg12.png",
    "assets/title_bg13.png", "assets/title_bg14.png", "assets/title_bg15.png", "assets/title_bg16.png",
    "assets/title_bg17.png", "assets/title_bg18.png"
};

int main(void) {
    // Resoluções suportadas
    const int resWidths[] = { 1200, 1366, 1920 };
    const int resHeights[] = { 720, 768, 1080 };
    int maxResolutions = 3;

    // Inicializa com a primeira resolução
    int screenWidth = resWidths[0];
    int screenHeight = resHeights[0];

    InitWindow(screenWidth, screenHeight, "Micro Mayhem");
    InitAudioDevice();
    SetTargetFPS(60);

    GameState currentState = STATE_SPLASH_FADE_IN;

    Image icon = LoadImage("assets/exe_icon.png");
    SetWindowIcon(icon);

    Texture2D cesarLogo = LoadTexture("assets/cesar_logo.png");
    Texture2D mmLogo = LoadTexture("assets/title.png");
    
    Texture2D uniqueTitleBGs[UNIQUE_BG_COUNT];
    LoadTexturesInLoop(uniqueTitleBGs, TitleBGPaths, UNIQUE_BG_COUNT);

    Music menuMusic = LoadMusicStream("../assets/audio/menu_principal.ogg");
    menuMusic.looping = true;
    SetMasterVolume(1.0f);
    bool isMenuMusicPlaying = false;

    TitleBG titleBGs[BG_COUNT] = {
        { uniqueTitleBGs[0], {0.34f, 0.195f}, 2.6f },
        { uniqueTitleBGs[1], {0.35f, 0.79f}, 3.0f },
        { uniqueTitleBGs[2], {0.8f, 0.36f}, 2.7f },
        { uniqueTitleBGs[3], {0.56f, 0.17f}, 2.7f },
        { uniqueTitleBGs[4], {0.73f, 0.18f}, 2.5f },
        { uniqueTitleBGs[5], {0.39f, 0.79f}, 3.0f },
        { uniqueTitleBGs[6], {0.88f, 0.83f}, 2.5f },
        { uniqueTitleBGs[7], {0.88f, 0.26f}, 2.9f },
        { uniqueTitleBGs[8], {0.125f, 0.72f}, 2.7f },
        { uniqueTitleBGs[9], {0.16f, 0.17f}, 2.7f },
        { uniqueTitleBGs[10], {0.12f, 0.42f}, 2.8f },
        { uniqueTitleBGs[11], {0.28f, 0.77f}, 2.7f },
        { uniqueTitleBGs[12], {0.67f, 0.81f}, 2.8f },
        { uniqueTitleBGs[13], {0.895f, 0.47f}, 2.8f },
        { uniqueTitleBGs[14], {0.21f, 0.82f}, 2.5f },
        { uniqueTitleBGs[15], {0.86f, 0.47f}, 2.8f },
        { uniqueTitleBGs[15], {0.825f, 0.47f}, 2.8f }, 
        { uniqueTitleBGs[16], {0.615f, 0.82f}, 2.2f },
        { uniqueTitleBGs[17], {0.22f, 0.35f}, 1.6f }, 
        { uniqueTitleBGs[17], {0.78f, 0.72f}, 1.5f }, 
    };

    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT); 

    Shader pixelShader = LoadShader(0, "assets/shaders/pixelizer.fs");
    Shader gradientShader = LoadShader(0, "assets/shaders/radial_gradient.fs");
    
    int resLoc = GetShaderLocation(gradientShader, "resolution");
    int centerLoc = GetShaderLocation(gradientShader, "colorCenter");
    int edgeLoc = GetShaderLocation(gradientShader, "colorEdge");

    Vector2 renderResolution = { (float)GAME_WIDTH, (float)GAME_HEIGHT };

    float centerColor[3] = { 11/255.0f, 22/255.0f, 79/255.0f };
    float edgeColor[3] = { 9/255.0f, 15/255.0f, 29/255.0f };

    int pixelSizeLoc = GetShaderLocation(pixelShader, "pixelSize");
    int renderSizeLoc = GetShaderLocation(pixelShader, "renderSize");

    float renderSize[2] = { (float)GAME_WIDTH, (float)GAME_HEIGHT };
    SetShaderValue(pixelShader, renderSizeLoc, renderSize, SHADER_UNIFORM_VEC2);

    char *text = "Press ENTER to begin";
    int fontSize = 30;
    int textWidth = MeasureText(text, fontSize);
    
    Vector2 textPosition = {
        (1200 - textWidth) / 2.0f,
        (720 / 2.0f) + 150
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

    int selectedOption = 0;
    int optionFontSize = 40;

    const char* text_menu_pt[] = { "Jogo Rapido", "Arcade", "Creditos", "Opcoes", "Sair" };
    const char* text_menu_en[] = { "Quick Play", "Arcade", "Credits", "Settings", "Quit Game" };
    
    const char* text_settings_pt[] = { "Volume Geral", "Musica", "Efeitos (SFX)", "Resolucao", "Idioma: PT-BR", "Voltar" };
    const char* text_settings_en[] = { "Master Volume", "Music", "SFX", "Resolution", "Language: ENG", "Return" };

    const char* text_qp_pt[] = { "Um Jogador", "Multijogador", "Voltar" };
    const char* text_qp_en[] = { "Singleplayer", "Multiplayer", "Return" };
    
    // Estado inicial das Configurações
    GameSettings settings = { 
        1.0f, 1.0f, 1.0f,   // Volumes
        0,                  // Resolução (Indice 0 = 1200x720)
        false,              // Fullscreen
        LANG_PT             // Começa em Português
    };
    SetMasterVolume(settings.masterVolume);

    while (running && !WindowShouldClose()) {
        
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
            settings.fullscreen = !settings.fullscreen;
        }

        // Atualiza musica continuamente
        if (isMenuMusicPlaying) {
            SetMusicVolume(menuMusic, settings.musicVolume);
            UpdateMusicStream(menuMusic);
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
                            currentState = STATE_CREDITS;
                            break;
                        case 3:
                            currentState = STATE_SETTINGS;
                            selectedOption = 0;
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

            
            case STATE_SETTINGS:
                if (IsKeyPressed(KEY_DOWN)) {
                    selectedOption++;
                    if (selectedOption >= SETTINGS_OPTIONS) selectedOption = 0;
                }
                if (IsKeyPressed(KEY_UP)) {
                    selectedOption--;
                    if (selectedOption < 0) selectedOption = SETTINGS_OPTIONS - 1;
                }

                // Esquerda/Direita para mudar valores
                if (IsKeyPressed(KEY_LEFT)) {
                    switch(selectedOption) {
                        case 0: settings.masterVolume -= 0.1f; break;
                        case 1: settings.musicVolume -= 0.1f; break;
                        case 2: settings.sfxVolume -= 0.1f; break;
                        case 3: // Resolução
                            settings.resolutionIndex--;
                            if (settings.resolutionIndex < 0) settings.resolutionIndex = maxResolutions - 1;
                            SetWindowSize(resWidths[settings.resolutionIndex], resHeights[settings.resolutionIndex]);
                            SetWindowPosition((GetMonitorWidth(0) - resWidths[settings.resolutionIndex])/2, (GetMonitorHeight(0) - resHeights[settings.resolutionIndex])/2);
                            break;
                        case 4: settings.language = LANG_EN; break;
                    }
                }
                if (IsKeyPressed(KEY_RIGHT)) {
                    switch(selectedOption) {
                        case 0: settings.masterVolume += 0.1f; break;
                        case 1: settings.musicVolume += 0.1f; break;
                        case 2: settings.sfxVolume += 0.1f; break;
                        case 3: // Resolução
                            settings.resolutionIndex++;
                            if (settings.resolutionIndex >= maxResolutions) settings.resolutionIndex = 0;
                            SetWindowSize(resWidths[settings.resolutionIndex], resHeights[settings.resolutionIndex]);
                            SetWindowPosition((GetMonitorWidth(0) - resWidths[settings.resolutionIndex])/2, (GetMonitorHeight(0) - resHeights[settings.resolutionIndex])/2);
                            break;
                        case 4: settings.language = LANG_PT; break;
                    }
                }

                // Travar Volumes
                if (settings.masterVolume > 1.0f) settings.masterVolume = 1.0f;
                if (settings.masterVolume < 0.0f) settings.masterVolume = 0.0f;
                if (settings.musicVolume > 1.0f) settings.musicVolume = 1.0f;
                if (settings.musicVolume < 0.0f) settings.musicVolume = 0.0f;
                if (settings.sfxVolume > 1.0f) settings.sfxVolume = 1.0f;
                if (settings.sfxVolume < 0.0f) settings.sfxVolume = 0.0f;
                
                SetMasterVolume(settings.masterVolume);

                // Botão Voltar
                if (IsKeyPressed(KEY_ENTER)) {
                    if (selectedOption == 5) {
                        currentState = STATE_MENU;
                        selectedOption = 3;
                    }
                }
                break;

            case STATE_CREDITS:
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
                    currentState = STATE_MENU;
                    selectedOption = 2;
                }
                break;

            case STATE_GAMEPLAY:
                int gameResult = GameScene_Update();
                if (gameResult == 1) {
                    currentState = STATE_QUICKPLAY_MENU;
                    selectedOption = 0;
                }
                break;
        }

        BeginTextureMode(target);
            if (currentState == STATE_REVEAL_MM || currentState == STATE_TITLE_MM || currentState == STATE_MENU || currentState == STATE_QUICKPLAY_MENU) {
                BeginShaderMode(gradientShader);
                SetShaderValue(gradientShader, resLoc, &renderResolution, SHADER_UNIFORM_VEC2);
                SetShaderValue(gradientShader, centerLoc, centerColor, SHADER_UNIFORM_VEC3);
                SetShaderValue(gradientShader, edgeLoc, edgeColor, SHADER_UNIFORM_VEC3);
                DrawRectangle(0, 0, 1200, 720, WHITE);
                EndShaderMode();
            } else if (currentState == STATE_SETTINGS || currentState == STATE_CREDITS) {
                // Fundo diferente para Settings/Credits
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
                        (1200 - scaledWidth) / 2.0f,
                        (720 - scaledHeight) / 2.0f,
                        scaledWidth,
                        scaledHeight
                    };
                    Vector2 origin = { 0.0f, 0.0f };
                    DrawTexturePro(cesarLogo, sourceRec, destRec, origin, 0.0f, WHITE);
                } break;

                case STATE_REVEAL_MM:
                case STATE_TITLE_MM:
                    DrawInitialBackground(1200, 720, titleBGs, mmLogo, MMlogoScale);
                    
                    Rectangle sourceRec = { 0.0f, 0.0f, (float)mmLogo.width, (float)mmLogo.height };
                    float scaledWidth = (float)mmLogo.width * MMlogoScale;
                    float scaledHeight = (float)mmLogo.height * MMlogoScale;
                    Rectangle destRec = {
                        (1200 - scaledWidth) / 2.0f,
                        (720 - scaledHeight) / 2.0f,
                        scaledWidth,
                        scaledHeight
                    };
                    Vector2 origin = { 0.0f, 0.0f };
                    DrawTexturePro(mmLogo, sourceRec, destRec, origin, 0.0f, WHITE);
                    break;

                case STATE_MENU:
                {
                    DrawInitialBackground(1200, 720, titleBGs, mmLogo, MMlogoScale);

                    // Seleciona texto com base na lingua
                    const char **currentText = (settings.language == LANG_EN) ? text_menu_en : text_menu_pt;
                    const char *menuTitle = (settings.language == LANG_EN) ? "MAIN MENU" : "MENU PRINCIPAL";

                    int titleSize = 60;
                    int titleWidth = MeasureText(menuTitle, titleSize);
                    DrawText(menuTitle, (1200 - titleWidth) / 2, 720 * 0.18f, titleSize, WHITE);

                    for (int i = 0; i < MENU_OPTIONS; i++) {
                        int textWidth = MeasureText(currentText[i], optionFontSize);
                        Color optionColor = (i == selectedOption)
                            ? (Color){255, 255, 200, 255}
                            : (Color){200, 200, 255, 255};

                        DrawText(currentText[i], (1200 - textWidth) / 2, 720 * 0.35f + i * 60, optionFontSize, optionColor);

                        if (i == selectedOption) {
                            DrawTriangle(
                                (Vector2){ (1200 - textWidth) / 2 - 30, 720 * 0.35f + i * 60 + 20 },
                                (Vector2){ (1200 - textWidth) / 2 - 10, 720 * 0.35f + i * 60 + 10 },
                                (Vector2){ (1200 - textWidth) / 2 - 10, 720 * 0.35f + i * 60 + 30 },
                                optionColor
                            );
                        }
                    }
                }
                break;

                case STATE_SETTINGS:
                {
                    ClearBackground((Color){10, 12, 30, 255});
                    const char **currentSetText = (settings.language == LANG_EN) ? text_settings_en : text_settings_pt;
                    
                    DrawText((settings.language == LANG_EN) ? "SETTINGS" : "CONFIGURACOES", 100, 60, 60, WHITE);

                    for (int i = 0; i < SETTINGS_OPTIONS; i++) {
                        Color color = (i == selectedOption) ? YELLOW : GRAY;
                        DrawText(currentSetText[i], 100, 180 + (i * 70), 40, color);

                        char valText[40];
                        sprintf(valText, ""); 

                        if (i == 0) sprintf(valText, "< %.0f%% >", settings.masterVolume * 100);
                        else if (i == 1) sprintf(valText, "< %.0f%% >", settings.musicVolume * 100);
                        else if (i == 2) sprintf(valText, "< %.0f%% >", settings.sfxVolume * 100);
                        else if (i == 3) sprintf(valText, "< %dx%d >", resWidths[settings.resolutionIndex], resHeights[settings.resolutionIndex]);

                        DrawText(valText, 500, 180 + (i * 70), 40, WHITE);
                    }
                }
                break;

                case STATE_CREDITS:
                {
                    ClearBackground((Color){5, 5, 10, 255});
                    DrawText("CREDITS", 100, 60, 60, WHITE);
                    
                    DrawText("Superiores:", 100, 180, 30, GRAY);
                    DrawText("Saturnacy - Chefão", 100, 220, 40, WHITE);
                    
                    DrawText("Escória:", 100, 300, 30, GRAY);
                    DrawText("LipeC - Escravo", 100, 340, 40, WHITE);
                    DrawText("Davi - Escravo", 100, 380, 40, WHITE);
                    
                    const char* returnText = (settings.language == LANG_EN) ? "Press ENTER to Return" : "Pressione ENTER para Voltar";
                    DrawText(returnText, 100, 600, 30, DARKGRAY);
                }
                break;

                case STATE_QUICKPLAY_MENU:
                    DrawInitialBackground(1200, 720, titleBGs, mmLogo, MMlogoScale);
                    
                    const char **qpText = (settings.language == LANG_EN) ? text_qp_en : text_qp_pt;

                    for (int i = 0; i < 3; i++) {
                        int textWidth = MeasureText(qpText[i], optionFontSize);
                        Color optionColor = (i == selectedOption) ? (Color){255, 255, 200, 255} : (Color){200, 200, 255, 255};

                        DrawText(qpText[i], (1200 - textWidth) / 2, 720 * 0.35f + i * 60, optionFontSize, optionColor);

                        if (i == selectedOption) {
                            DrawTriangle(
                                (Vector2){ (1200 - textWidth) / 2 - 30, 720 * 0.35f + i * 60 + 20 },
                                (Vector2){ (1200 - textWidth) / 2 - 10, 720 * 0.35f + i * 60 + 10 },
                                (Vector2){ (1200 - textWidth) / 2 - 10, 720 * 0.35f + i * 60 + 30 },
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

            Rectangle sourceRect = { 0, 0, (float)target.texture.width, (float)-target.texture.height };
            Rectangle destRect = { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };
            Vector2 origin = { 0, 0 };

            if (currentState == STATE_REVEAL_MM) {
                BeginShaderMode(pixelShader);
                    DrawTexturePro(target.texture, sourceRect, destRect, origin, 0.0f, WHITE);
                EndShaderMode();
            } else {
                DrawTexturePro(target.texture, sourceRect, destRect, origin, 0.0f, WHITE);
            }

            if (currentState == STATE_SPLASH_FADE_IN || currentState == STATE_FADE_OUT || currentState == STATE_REVEAL_MM) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 0, 0, 0, (unsigned char)fadeAlpha });
            }

            if (currentState == STATE_TITLE_MM) {
                bool showText = fmod(GetTime(), 1.0) < 0.5;
                if (showText) {
                    // Ajuste de escala para o texto "Press Enter"
                    float scaleX = (float)GetScreenWidth() / 1200.0f;
                    float scaleY = (float)GetScreenHeight() / 720.0f;
                    DrawText(text, textPosition.x * scaleX, textPosition.y * scaleY, fontSize * scaleX, RAYWHITE);
                }
            }

        EndDrawing();
    }
    
    GameScene_Unload();
    
    UnloadTexturesInLoop(uniqueTitleBGs, UNIQUE_BG_COUNT);

    UnloadRenderTexture(target);
    UnloadShader(pixelShader);
    UnloadShader(gradientShader);
    UnloadTexture(mmLogo);
    UnloadTexture(cesarLogo);
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

    DrawLineEx((Vector2){screenWidth * 0.088f, screenHeight * 0.09f}, (Vector2){screenWidth * 0.088f, screenHeight * 0.6f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.088f, screenHeight * 0.6f}, (Vector2){screenWidth * 0.159f, screenHeight * 0.6f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.29f, screenHeight * 0.91f}, (Vector2){screenWidth * 0.72f, screenHeight * 0.91f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.265f, screenHeight * 0.09f}, (Vector2){screenWidth * 0.84f, screenHeight * 0.09f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.84f, screenHeight * 0.09f}, (Vector2){screenWidth * 0.84f, screenHeight * 0.2f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.912f, screenHeight * 0.55f}, (Vector2){screenWidth * 0.912f, screenHeight * 0.91f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.83f, screenHeight * 0.55f}, (Vector2){screenWidth * 0.912f, screenHeight * 0.55f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.24f, screenHeight * 0.55f}, (Vector2){screenWidth * 0.24f, screenHeight * 0.76f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.3f, screenHeight * 0.26f}, (Vector2){screenWidth * 0.38f, screenHeight * 0.26f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.46f, screenHeight * 0.15f}, (Vector2){screenWidth * 0.46f, screenHeight * 0.21f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.37f, screenHeight * 0.15f}, (Vector2){screenWidth * 0.46f, screenHeight * 0.15f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.53f, screenHeight * 0.79f}, (Vector2){screenWidth * 0.53f, screenHeight * 0.83f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.53f, screenHeight * 0.83f}, (Vector2){screenWidth * 0.59f, screenHeight * 0.83f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.73f, screenHeight * 0.25f}, (Vector2){screenWidth * 0.73f, screenHeight * 0.41f}, thickness, lineColor);
    DrawLineEx((Vector2){screenWidth * 0.64f, screenHeight * 0.25f}, (Vector2){screenWidth * 0.73f, screenHeight * 0.25f}, thickness, lineColor);

    for (int i = 0; i < BG_COUNT; i++) {
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