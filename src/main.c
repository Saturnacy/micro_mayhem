#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>
#include "game_scene.h"
#include "custom_fonts.h"

#define MENU_OPTIONS 5
#define QP_OPTIONS 3
#define BG_COUNT 20
#define UNIQUE_BG_COUNT 18
#define SETTINGS_OPTIONS 8
#define CHAR_COUNT 12
#define CONFIG_FILE "game_settings.bin"

typedef enum {
    STATE_SPLASH_FADE_IN,
    STATE_SPLASH_CESAR,
    STATE_FADE_OUT,
    STATE_REVEAL_MM,
    STATE_TITLE_MM,
    STATE_MENU,
    STATE_QUICKPLAY_MENU,
    STATE_CHARACTER_SELECT,
    STATE_SETTINGS,
    STATE_CREDITS,
    STATE_GAMEPLAY,
    STATE_DEMO_LOCK
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

void SaveGameSettings(GameSettings *settings) {
    FILE *file = fopen(CONFIG_FILE, "wb");
    
    if (file != NULL) {
        fwrite(settings, sizeof(GameSettings), 1, file);
        fclose(file);
    } else {
        printf("SETTINGS: Erro ao salvar configuracoes!\n");
    }
}

void LoadGameSettings(GameSettings *settings) {
    FILE *file = fopen(CONFIG_FILE, "rb");
    
    if (file != NULL) {
        fread(settings, sizeof(GameSettings), 1, file);
        fclose(file);
    } else {
        printf("SETTINGS: Arquivo de configuraçao nao encontrado. Usando padroes.\n");
    }
}

const char *TitleBGPaths[UNIQUE_BG_COUNT] = {
    "assets/title_bg1.png", "assets/title_bg2.png", "assets/title_bg3.png", "assets/title_bg4.png",
    "assets/title_bg5.png", "assets/title_bg6.png", "assets/title_bg7.png", "assets/title_bg8.png",
    "assets/title_bg9.png", "assets/title_bg10.png", "assets/title_bg11.png", "assets/title_bg12.png",
    "assets/title_bg13.png", "assets/title_bg14.png", "assets/title_bg15.png", "assets/title_bg16.png",
    "assets/title_bg17.png", "assets/title_bg18.png"
};

bool isMultiplayer = false;
const char* charNamesEN[CHAR_COUNT] = { 
    "BACTERIOPHAGE", "AMOEBA", "TARDIGRADE", "STENTOR", 
    "PARAMECIUM", "EUGLENA", "NEMATODE", "ROTIFER",
    "DINOFLAGELLATE", "DAPHNIA", "HYDRA", "ARCHEON" 
};

const char* charNamesPT[CHAR_COUNT] = { 
    "BACTERIOFAGO", "AMEBA", "TARDIGRADO", "STENTOR", 
    "PARAMECIO", "EUGLENA", "NEMATODEO", "ROTIFERO",
    "DINOFLAGELADO", "DAPHNIA", "HIDRA", "ARQUEA" 
};
int p1Selection = 0;
int p2Selection = 0;
bool isSelectingP2 = false;
int inputDelayTimer = 0;

int statsHP[CHAR_COUNT]    = { 0, 2, 2, 0, 0, 0, 0, 0, 0, 1, 2, 1 };
int statsSTR[CHAR_COUNT]   = { 1, 1, 2, 2, 1, 1, 0, 1, 1, 1, 2, 0 };
int statsSPD[CHAR_COUNT]   = { 2, 1, 0, 1, 2, 0, 1, 1, 0, 2, 1, 2 };

const char* statLabels[] = { "LOW", "MED", "HIGH" };
Color statColors[] = { RED, YELLOW, GREEN };

int main(void) {
    // =========================================================
    // 1. INICIALIZAÇÃO DO SISTEMA E JANELA
    // =========================================================
    const int resWidths[] = { 1200, 1366, 1920 };
    const int resHeights[] = { 720, 768, 1080 };
    int maxResolutions = 3;
    
    GameSettings settings = { 
        1.0f, 1.0f, 1.0f,
        0,
        false,
        LANG_EN
    };

    LoadGameSettings(&settings);

    if (settings.resolutionIndex < 0 || settings.resolutionIndex >= maxResolutions) {
        settings.resolutionIndex = 0;
    }
    if (settings.masterVolume < 0) settings.masterVolume = 0.5f;

    int screenWidth = resWidths[settings.resolutionIndex];
    int screenHeight = resHeights[settings.resolutionIndex];

    InitWindow(screenWidth, screenHeight, "Micro Mayhem");
    
    if (settings.fullscreen) {
        ToggleFullscreen();
    }

    SetTargetFPS(60);
    
    Image icon = LoadImage("assets/exe_icon.png");
    SetWindowIcon(icon);

    // =========================================================
    // 2. SISTEMA DE ÁUDIO
    // =========================================================
    InitAudioDevice();
    
    SetMasterVolume(settings.masterVolume);
    
    Music menuMusic = LoadMusicStream("assets/audio/main_menu.ogg");
    menuMusic.looping = true;
    SetMusicVolume(menuMusic, settings.musicVolume); 
    bool isMenuMusicPlaying = false;

    Sound sndSelect = LoadSound("assets/audio/select.mp3");
    Sound sndSelected = LoadSound("assets/audio/confirm.mp3");

    Music cssMusic = LoadMusicStream("assets/audio/css_music.ogg");
    cssMusic.looping = true;

    Music fightMusic = LoadMusicStream("assets/audio/fight.ogg");
    fightMusic.looping = true;

    // =========================================================
    // 3. SHADERS E RENDER TEXTURE (SISTEMA VISUAL)
    // =========================================================
    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT); 

    Shader pixelShader = LoadShader(0, "assets/shaders/pixelizer.fs");
    int pixelSizeLoc = GetShaderLocation(pixelShader, "pixelSize");
    int renderSizeLoc = GetShaderLocation(pixelShader, "renderSize");
    float renderSize[2] = { (float)GAME_WIDTH, (float)GAME_HEIGHT };
    SetShaderValue(pixelShader, renderSizeLoc, renderSize, SHADER_UNIFORM_VEC2);
    float currentPixelSize = 20.0f;

    Shader gradientShader = LoadShader(0, "assets/shaders/radial_gradient.fs");
    int resLoc = GetShaderLocation(gradientShader, "resolution");
    int centerLoc = GetShaderLocation(gradientShader, "colorCenter");
    int edgeLoc = GetShaderLocation(gradientShader, "colorEdge");
    Vector2 renderResolution = { (float)GAME_WIDTH, (float)GAME_HEIGHT };
    float centerColor[3] = { 11/255.0f, 22/255.0f, 79/255.0f };
    float edgeColor[3] = { 9/255.0f, 15/255.0f, 29/255.0f };

    // =========================================================
    // 4. ASSETS: LOGOS E FONTES
    // =========================================================
    Texture2D cesarLogo = LoadTexture("assets/cesar_logo.png");
    Texture2D mmLogo = LoadTexture("assets/title.png");
    
    Font gameFont = LoadGameFont("assets/game_font.png");
    Font mainFont = LoadMainFont("assets/main_font.png");
    GameScene_SetFont(gameFont);
    GameScene_SetMainFont(mainFont);

    // =========================================================
    // 5. ASSETS: BACKGROUNDS (ARRAYS E CARREGAMENTO)
    // =========================================================
    Texture2D uniqueTitleBGs[UNIQUE_BG_COUNT];
    LoadTexturesInLoop(uniqueTitleBGs, TitleBGPaths, UNIQUE_BG_COUNT);

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

    Texture2D charSelectBg;
    charSelectBg = LoadTexture("assets/css_bg.png"); 
    SetTextureFilter(charSelectBg, TEXTURE_FILTER_POINT);

    static Texture2D texBackground;

    Texture2D charBoxTex;
    charBoxTex = LoadTexture("assets/character_box.png");
    SetTextureFilter(charBoxTex, TEXTURE_FILTER_POINT);

    Texture2D lockedBoxTex = LoadTexture("assets/locked_character_box.png");
    SetTextureFilter(lockedBoxTex, TEXTURE_FILTER_POINT);

    Texture2D infoBoxTex;
    infoBoxTex = LoadTexture("assets/infobox.png");
    SetTextureFilter(infoBoxTex, TEXTURE_FILTER_POINT);

    Texture2D bactIcon = LoadTexture("assets/bacteriophage_icon.png");
    SetTextureFilter(bactIcon, TEXTURE_FILTER_POINT);

    Texture2D amoebaIcon = LoadTexture("assets/amoeba_icon.png");
    SetTextureFilter(amoebaIcon, TEXTURE_FILTER_POINT);

    Texture2D boxArtTex = LoadTexture("assets/boxart.png");

    // =========================================================
    // 6. CONFIGURAÇÃO DE UI, TEXTO E ESCALAS
    // =========================================================
    const char *title_text = "PRESS ENTER TO BEGIN";
    float fontSize = gameFont.baseSize * 1.5f; 
    float fontSpacing = 3.0f;
    Vector2 textSize = MeasureTextEx(gameFont, title_text, fontSize, fontSpacing);
    Vector2 textPosition = {
        (1200 - textSize.x) / 2.0f,
        (720 / 1.65f) + 150
    };

    float mainFontSpacing = 2.0f;
    float scaleTitle = 3.0f;
    float scaleOptions = 1.3f;
    float scaleSmall = 1.5f;

    float fontSizeTitle = mainFont.baseSize * scaleTitle;
    float fontSizeOption = mainFont.baseSize * scaleOptions;
    float fontSizeSmall = mainFont.baseSize * scaleSmall;
    
    float CESARlogoscale = 2.0f;
    float MMlogoScale = 2.0f;

    const char* demo_lock_pt = 
        "Compre uma cópia oficial de Micro Mayhem para\naproveitar tudo!\n\n"
        "+ 10 personagens únicos!\n"
        "+ 1 modo história (arcade)!\n"
        "+ Extras (arte conceitual e galeria!)\n"
        "+ Configurações mais ambiciosas!\n"
        "+ CPU melhorada!\n"
        "+ Novos microambientes!\n"
        "+ MUITO MAIS!\n\n"
        "Por enquanto, esse recurso não está\ndisponível na demo!";

    const char* demo_lock_en = 
        "Buy an official copy of Micro Mayhem to\nenjoy everything!\n\n"
        "+ 10 unique characters!\n"
        "+ 1 Story Mode (Arcade)!\n"
        "+ Extras (Concept Art & Gallery!)\n"
        "+ More ambitious settings!\n"
        "+ Improved CPU AI!\n"
        "+ New Micro-Environments!\n"
        "+ MUCH MORE!\n\n"
        "For now, this feature is not available\nin the demo!";

    // =========================================================
    // 7. DADOS DO MENU PRINCIPAL (TEXTOS E ÍCONES)
    // =========================================================
    const char* text_menu_pt[] = { "Jogo Rápido", "Arcade", "Extras", "Opções", "Sair" };
    const char* text_menu_en[] = { "Quick Play", "Arcade", "Extras", "Settings", "Quit Game" };
    
    const char *iconPaths[MENU_OPTIONS] = {
        "assets/QP_icon.png",
        "assets/Arcade_icon.png",
        "assets/Extras_icon.png",
        "assets/Settings_icon.png",
        "assets/QG_icon.png"
    };

    Texture2D menuIcons[MENU_OPTIONS];
    for (int i = 0; i < MENU_OPTIONS; i++) {
        menuIcons[i] = LoadTexture(iconPaths[i]);
        SetTextureFilter(menuIcons[i], TEXTURE_FILTER_POINT);
        SetTextureWrap(menuIcons[i], TEXTURE_WRAP_CLAMP);
    }

    float iconScales[MENU_OPTIONS] = { 3.0f, 3.0f, 3.0f, 3.0f, 3.0f };
    float baseScale = 3.0f;
    float selectedScale = 4.0f;
    float floatSpeed = 6.0f;
    float floatAmp = 5.0f;
    int selectedOption = 0;

    Texture2D qpIcons[2];
    qpIcons[0] = LoadTexture("assets/SP_icon.png");
    qpIcons[1] = LoadTexture("assets/MP_icon.png");

    for(int i=0; i<2; i++) {
        SetTextureFilter(qpIcons[i], TEXTURE_FILTER_POINT);
        SetTextureWrap(qpIcons[i], TEXTURE_WRAP_CLAMP);
    }

    float qpScales[3] = { 3.0f, 3.0f, 1.0f };

    // =========================================================
    // 8. DADOS DE SUB-MENUS (QUICK PLAY E SETTINGS)
    // =========================================================
    const char* text_qp_pt[] = { "Um Jogador", "Multijogador", "Voltar" };
    const char* text_qp_en[] = { "Singleplayer", "Multiplayer", "Return" };

    const char* text_settings_pt[] = {
        "Volume Geral",
        "Música",
        "Efeitos (SFX)",
        "Resolução",
        "Tela Cheia",
        "Idioma: PT-BR",
        "Créditos",
        "Voltar"
    };

    const char* text_settings_en[] = {
        "Master Volume",
        "Music",
        "SFX",
        "Resolution",
        "Fullscreen",
        "Language: ENG",
        "Credits",
        "Return"
    };

    // =========================================================
    // 9. ESTADO GLOBAL DO JOGO E SETTINGS
    // =========================================================
    GameState currentState = STATE_SPLASH_FADE_IN;
    GameState returnState = STATE_MENU;
    bool running = true;
    
    int frameCounter = 0;
    float fadeAlpha = 255.0f;

    SetMasterVolume(settings.masterVolume);

    // =========================================================
    // LOOP PRINCIPAL
    // =========================================================

    while (running && !WindowShouldClose()) {
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
            settings.fullscreen = !settings.fullscreen;
        }

        if (isMenuMusicPlaying) {
            SetMusicVolume(menuMusic, settings.musicVolume);
            UpdateMusicStream(menuMusic);
        }
        else if (currentState == STATE_CHARACTER_SELECT) {
            SetMusicVolume(cssMusic, settings.musicVolume);
            UpdateMusicStream(cssMusic);
        }
        else if (currentState == STATE_GAMEPLAY) {
            SetMusicVolume(fightMusic, settings.musicVolume);
            UpdateMusicStream(fightMusic);
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
                if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                    selectedOption++;
                    if (selectedOption >= MENU_OPTIONS) selectedOption = 0;
                    PlaySound(sndSelect);
                }

                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                    selectedOption--;
                    if (selectedOption < 0) selectedOption = MENU_OPTIONS - 1;
                    PlaySound(sndSelect);
                }

                if (IsKeyPressed(KEY_ENTER)) {
                    PlaySound(sndSelected);
                    switch (selectedOption) {
                        case 0:
                            currentState = STATE_QUICKPLAY_MENU;
                            selectedOption = 0;
                            break;
                        case 1:
                            returnState = STATE_MENU;
                            currentState = STATE_DEMO_LOCK;
                            break;
                        case 2:
                            returnState = STATE_MENU;
                            currentState = STATE_DEMO_LOCK;
                            break;
                        case 3:
                            returnState = STATE_MENU;
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
                if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                    selectedOption++;
                    if (selectedOption > 2) selectedOption = 0;
                    PlaySound(sndSelect);
                }

                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                    selectedOption--;
                    if (selectedOption < 0) selectedOption = 2;
                    PlaySound(sndSelect);
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    PlaySound(sndSelected);
                    switch (selectedOption) {
                        case 0:
                            StopMusicStream(menuMusic);
                            PlayMusicStream(cssMusic);
                            isMenuMusicPlaying = false;

                            GameScene_SetMultiplayer(false);
                            isMultiplayer = false;
                            
                            currentState = STATE_CHARACTER_SELECT;
                            p1Selection = 0;
                            p2Selection = 1;
                            isSelectingP2 = false;
                            inputDelayTimer = 30;
                            break;
                            
                        case 1:
                            StopMusicStream(menuMusic);
                            PlayMusicStream(cssMusic);
                            isMenuMusicPlaying = false;

                            GameScene_SetMultiplayer(true);
                            isMultiplayer = true;

                            currentState = STATE_CHARACTER_SELECT;
                            p1Selection = 0;
                            p2Selection = 0;
                            isSelectingP2 = false;
                            inputDelayTimer = 30;
                            break;
                            
                        case 2:
                            currentState = STATE_MENU;
                            selectedOption = 0;
                            break;
                    }
                }
                break;

                case STATE_CHARACTER_SELECT:
                {
                    if (inputDelayTimer > 0) {
                        inputDelayTimer--;
                    }

                    if (isSelectingP2) {
                        if (inputDelayTimer <= 0) {
                            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || 
                                IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) ||
                                IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) ||
                                IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                                    PlaySound(sndSelect);
                            }

                            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) p2Selection--;
                            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) p2Selection++;
                            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) p2Selection += 4;
                            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) p2Selection -= 4;

                            if (p2Selection < 0) p2Selection += CHAR_COUNT;
                            if (p2Selection >= CHAR_COUNT) p2Selection -= CHAR_COUNT;

                            if (IsKeyPressed(KEY_ENTER)) {
                                if (p2Selection > 1) {
                                    returnState = STATE_CHARACTER_SELECT; 
                                    currentState = STATE_DEMO_LOCK;
                                    PlaySound(sndSelected); 
                                } else {
                                    StopMusicStream(cssMusic);
                                    
                                    PlayMusicStream(fightMusic);
                                    
                                    GameScene_SetLanguage(settings.language);
                                    GameScene_Init(p1Selection, p2Selection); 
                                    currentState = STATE_GAMEPLAY;
                                }
                            }

                            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
                                isSelectingP2 = false; 
                                inputDelayTimer = 15;
                            }
                        }
                    }
                
                    else {
                        if (inputDelayTimer <= 0) {
                            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || 
                                IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) ||
                                IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) ||
                                IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                                    PlaySound(sndSelect);
                            }

                            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) p1Selection--;
                            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) p1Selection++;
                            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) p1Selection += 4;
                            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) p1Selection -= 4;

                            if (p1Selection < 0) p1Selection += CHAR_COUNT;
                            if (p1Selection >= CHAR_COUNT) p1Selection -= CHAR_COUNT;

                            if (IsKeyPressed(KEY_ENTER)) {
                                if (p1Selection > 1) {
                                    returnState = STATE_CHARACTER_SELECT;
                                    currentState = STATE_DEMO_LOCK;
                                    PlaySound(sndSelected);
                                } 
                                else {
                                    if (isMultiplayer) {
                                        isSelectingP2 = true;
                                        inputDelayTimer = 20;
                                    } else {
                                        StopMusicStream(cssMusic);
                                        
                                        PlayMusicStream(fightMusic);
                                        
                                        GameScene_SetLanguage(settings.language);

                                        int cpuID = GetRandomValue(0, 1);

                                        GameScene_Init(p1Selection, cpuID); 
                                        currentState = STATE_GAMEPLAY;
                                    }
                                }
                            }

                            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
                                StopMusicStream(cssMusic);
                                PlayMusicStream(menuMusic);
                                isMenuMusicPlaying = true;
                                currentState = STATE_QUICKPLAY_MENU;
                            }
                        }
                    }

                }
                break;
            
            case STATE_SETTINGS:
                if (IsKeyPressed(KEY_DOWN)) {
                    selectedOption++;
                    if (selectedOption >= SETTINGS_OPTIONS) selectedOption = 0;
                    PlaySound(sndSelect);
                }
                if (IsKeyPressed(KEY_UP)) {
                    selectedOption--;
                    if (selectedOption < 0) selectedOption = SETTINGS_OPTIONS - 1;
                    PlaySound(sndSelect);
                }

                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {

                    int dir = IsKeyPressed(KEY_RIGHT) ? 1 : -1;

                    PlaySound(sndSelect);

                    switch(selectedOption) {
                        case 0: settings.masterVolume += 0.1f * dir; break;
                        case 1: settings.musicVolume += 0.1f * dir; break;
                        case 2: settings.sfxVolume += 0.1f * dir; break;
                        case 3:
                            settings.resolutionIndex += dir;
                            if (settings.resolutionIndex >= maxResolutions) settings.resolutionIndex = 0;
                            if (settings.resolutionIndex < 0) settings.resolutionIndex = maxResolutions - 1;
                            
                            SetWindowSize(resWidths[settings.resolutionIndex], resHeights[settings.resolutionIndex]);
                            SetWindowPosition((GetMonitorWidth(0) - resWidths[settings.resolutionIndex])/2, (GetMonitorHeight(0) - resHeights[settings.resolutionIndex])/2);
                            break;
                        case 4:
                            ToggleFullscreen();
                            settings.fullscreen = IsWindowFullscreen(); 
                            break;
                        case 5:
                            settings.language = (settings.language == LANG_EN) ? LANG_PT : LANG_EN; 
                            break;
                    }
                }

                if (settings.masterVolume > 1.0f) settings.masterVolume = 1.0f;
                if (settings.masterVolume < 0.0f) settings.masterVolume = 0.0f;
                if (settings.musicVolume > 1.0f) settings.musicVolume = 1.0f;
                if (settings.musicVolume < 0.0f) settings.musicVolume = 0.0f;
                if (settings.sfxVolume > 1.0f) settings.sfxVolume = 1.0f;
                if (settings.sfxVolume < 0.0f) settings.sfxVolume = 0.0f;
                
                SetMasterVolume(settings.masterVolume);
                if (isMenuMusicPlaying) SetMusicVolume(menuMusic, settings.musicVolume);

                if (IsKeyPressed(KEY_ENTER)) {
                    PlaySound(sndSelected);
                    if (selectedOption == 4) {
                        ToggleFullscreen();
                        settings.fullscreen = IsWindowFullscreen();
                        SaveGameSettings(&settings);
                    }
                    else if (selectedOption == 6) {
                        currentState = STATE_CREDITS;
                    }
                    else if (selectedOption == 7) {
                        SaveGameSettings(&settings);
                        
                        currentState = returnState;
                        
                        if (returnState == STATE_MENU) selectedOption = 3;
                    }
                }
                break;

            case STATE_CREDITS:
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
                    currentState = STATE_MENU;
                    selectedOption = 0;
                }
                break;

            case STATE_GAMEPLAY:
                int gameResult = GameScene_Update();
                
                if (gameResult == 1) {
                    StopMusicStream(fightMusic);
                    
                    currentState = STATE_QUICKPLAY_MENU;
                    selectedOption = 0;
                    if (!isMenuMusicPlaying) {
                        PlayMusicStream(menuMusic);
                        isMenuMusicPlaying = true;
                    }
                }
                else if (gameResult == 2) {
                    returnState = STATE_GAMEPLAY;
                    currentState = STATE_SETTINGS;
                    selectedOption = 0; 
                }
                break;

            case STATE_DEMO_LOCK:
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_BACKSPACE)) {
                    currentState = returnState;
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

                    if (currentState == STATE_TITLE_MM) {
                        bool showText = fmod(GetTime(), 1.0) < 0.5;
                        if (showText) {
                            DrawTextEx(gameFont, title_text, textPosition, fontSize, fontSpacing, RAYWHITE);
                        }
                    }
                    break;

                case STATE_MENU:
                {
                    DrawInitialBackground(1200, 720, titleBGs, mmLogo, MMlogoScale);

                    const char **currentText = (settings.language == LANG_EN) ? text_menu_en : text_menu_pt;

                    Vector2 menuPositions[MENU_OPTIONS] = {
                        { 300, 320 },
                        { 600, 280 },
                        { 900, 320 },
                        { 450, 550 },
                        { 750, 550 }
                    };

                    float labelScale = 1.0f;
                    float labelSpacing = 1.5f;

                    for (int i = 0; i < MENU_OPTIONS; i++) {
                        float targetScale = (i == selectedOption) ? selectedScale : baseScale;
                        iconScales[i] = Lerp(iconScales[i], targetScale, 0.15f);

                        float yOffset = 0.0f;
                        if (i == selectedOption) {
                            yOffset = sinf(GetTime() * floatSpeed) * floatAmp;
                        }

                        Texture2D icon = menuIcons[i];
                        float scale = iconScales[i];

                        float scaledW = icon.width * scale;
                        float scaledH = icon.height * scale;

                        Vector2 basePos = menuPositions[i];

                        float drawX = basePos.x - (scaledW / 2.0f);
                        float drawY = basePos.y - (scaledH / 2.0f) + yOffset;

                        Color tint = (i == selectedOption) ? WHITE : LIGHTGRAY;

                        DrawTextureEx(icon, (Vector2){drawX, drawY}, 0.0f, scale, tint);
                    
                        const char* labelText = currentText[i];
                        Vector2 labelSize = MeasureTextEx(mainFont, labelText, mainFont.baseSize * labelScale, labelSpacing);

                        float textX = drawX + (scaledW / 2.0f) - (labelSize.x / 2.0f);
                        float textY = drawY + scaledH + 10.0f;

                        Color textColor = (i == selectedOption) ? (Color){255, 255, 150, 255} : GRAY;

                        DrawTextEx(mainFont, labelText, (Vector2){textX, textY}, mainFont.baseSize * labelScale, labelSpacing, textColor);
                    }
                }
                break;

                case STATE_SETTINGS:
                {
                    ClearBackground((Color){10, 12, 30, 255});
                    const char **currentSetText = (settings.language == LANG_EN) ? text_settings_en : text_settings_pt;
                    
                    const char* settTitle = (settings.language == LANG_EN) ? "Settings" : "Configurações";
                    DrawTextEx(mainFont, settTitle, (Vector2){100, 60}, fontSizeTitle, mainFontSpacing, WHITE);

                    for (int i = 0; i < SETTINGS_OPTIONS; i++) {
                        Color color = (i == selectedOption) ? YELLOW : GRAY;
                        DrawTextEx(mainFont, currentSetText[i], (Vector2){100, 180 + (i * 70)}, fontSizeOption, mainFontSpacing, color);

                        char valText[40];
                        sprintf(valText, ""); 

                        if (i == 0) sprintf(valText, "< %.0f%% >", settings.masterVolume * 100);
                        else if (i == 1) sprintf(valText, "< %.0f%% >", settings.musicVolume * 100);
                        else if (i == 2) sprintf(valText, "< %.0f%% >", settings.sfxVolume * 100);
                        else if (i == 3) sprintf(valText, "< %dx%d >", resWidths[settings.resolutionIndex], resHeights[settings.resolutionIndex]);
                        
                        if (i < 5) {
                             DrawTextEx(mainFont, valText, (Vector2){500, 180 + (i * 70)}, fontSizeOption, mainFontSpacing, WHITE);
                        }
                    }
                }
                break;

                case STATE_CREDITS:
                {
                    ClearBackground((Color){5, 5, 10, 255});

                    float scaleCredTitle = 2.5f;
                    float scaleCredHeader = 1.3f;
                    float scaleCredText = 1.0f;

                    float sizeTitle = mainFont.baseSize * scaleCredTitle;
                    float sizeHeader = mainFont.baseSize * scaleCredHeader;
                    float sizeText = mainFont.baseSize * scaleCredText;

                    const char* txtTitle = (settings.language == LANG_EN) ? "CREDITS" : "CREDITOS";
                    Vector2 mTitle = MeasureTextEx(mainFont, txtTitle, sizeTitle, mainFontSpacing);
                    DrawTextEx(mainFont, txtTitle, (Vector2){(1200 - mTitle.x)/2, 60}, sizeTitle, mainFontSpacing, WHITE);

                    const char* line1 = "João Pedro Pessôa - Lead Developer & Producer";
                    const char* line2 = "Davi de Lucena - Art Director";
                    const char* line3 = "Filipe Correia - QA Specialist";

                    Vector2 m1 = MeasureTextEx(mainFont, line1, sizeText, mainFontSpacing);
                    DrawTextEx(mainFont, line1, (Vector2){(1200 - m1.x)/2, 200}, sizeText, mainFontSpacing, WHITE);

                    Vector2 m2 = MeasureTextEx(mainFont, line2, sizeText, mainFontSpacing);
                    DrawTextEx(mainFont, line2, (Vector2){(1200 - m2.x)/2, 250}, sizeText, mainFontSpacing, WHITE);

                    Vector2 m3 = MeasureTextEx(mainFont, line3, sizeText, mainFontSpacing);
                    DrawTextEx(mainFont, line3, (Vector2){(1200 - m3.x)/2, 300}, sizeText, mainFontSpacing, WHITE);

                    const char* txtSpecial = (settings.language == LANG_EN) ? "Special Thanks" : "Agradecimentos Especiais";
                    Vector2 mSpecial = MeasureTextEx(mainFont, txtSpecial, sizeHeader, mainFontSpacing);
                    DrawTextEx(mainFont, txtSpecial, (Vector2){(1200 - mSpecial.x)/2, 420}, sizeHeader, mainFontSpacing, YELLOW);

                    const char* lineTiago = "Tiago Barros";
                    Vector2 mTiago = MeasureTextEx(mainFont, lineTiago, sizeText, mainFontSpacing);
                    DrawTextEx(mainFont, lineTiago, (Vector2){(1200 - mTiago.x)/2, 470}, sizeText, mainFontSpacing, WHITE);
                    
                    const char* lineDaniel = "Daniel Bezerra";
                    Vector2 mDaniel = MeasureTextEx(mainFont, lineDaniel, sizeText, mainFontSpacing);
                    DrawTextEx(mainFont, lineDaniel, (Vector2){(1200 - mDaniel.x)/2, 510}, sizeText, mainFontSpacing, WHITE);

                    const char* returnText = (settings.language == LANG_EN) ? "Press ENTER to Return" : "Pressione ENTER para Voltar";
                    Vector2 mRet = MeasureTextEx(mainFont, returnText, sizeText, mainFontSpacing);
                    DrawTextEx(mainFont, returnText, (Vector2){(1200 - mRet.x)/2, 620}, sizeText, mainFontSpacing, DARKGRAY);
                }
                break;

                case STATE_QUICKPLAY_MENU:
                {
                    DrawInitialBackground(1200, 720, titleBGs, mmLogo, MMlogoScale);
                    
                    const char **qpText = (settings.language == LANG_EN) ? text_qp_en : text_qp_pt;
                    const char *qpTitle = (settings.language == LANG_EN) ? "GAME MODE" : "MODO DE JOGO";

                    DrawTextEx(mainFont, qpTitle, (Vector2){(1200 - MeasureTextEx(mainFont, qpTitle, fontSizeTitle, mainFontSpacing).x) / 2, 50}, fontSizeTitle, mainFontSpacing, WHITE);

                    Vector2 qpPositions[3] = {
                        { 400, 360 },
                        { 800, 360 },
                        { 600, 600 }
                    };

                    for (int i = 0; i < 3; i++) {
                        float baseS = (i == 2) ? 1.0f : 3.0f;
                        float selS  = (i == 2) ? 1.2f : 4.0f;
                        
                        float targetScale = (i == selectedOption) ? selS : baseS;
                        qpScales[i] = Lerp(qpScales[i], targetScale, 0.15f);

                        float yOffset = 0.0f;
                        if (i == selectedOption) {
                            yOffset = sinf(GetTime() * floatSpeed) * floatAmp;
                        }

                        Color tint = (i == selectedOption) ? WHITE : LIGHTGRAY;
                        Color textColor = (i == selectedOption) ? (Color){255, 255, 150, 255} : GRAY;

                        if (i < 2) {
                            Texture2D icon = qpIcons[i];
                            float scale = qpScales[i];
                            
                            float scaledW = icon.width * scale;
                            float scaledH = icon.height * scale;
                            
                            float drawX = qpPositions[i].x - (scaledW / 2.0f);
                            float drawY = qpPositions[i].y - (scaledH / 2.0f) + yOffset;
                            
                            Vector2 drawPos = { roundf(drawX), roundf(drawY) };

                            DrawTextureEx(icon, drawPos, 0.0f, scale, tint);

                            const char* label = qpText[i];
                            float lblScale = 1.2f;
                            Vector2 lblSize = MeasureTextEx(mainFont, label, mainFont.baseSize * lblScale, 1.5f);
                            
                            float textX = drawX + (scaledW / 2.0f) - (lblSize.x / 2.0f);
                            float textY = drawY + scaledH + 15.0f;

                            DrawTextEx(mainFont, label, (Vector2){textX, textY}, mainFont.baseSize * lblScale, 1.5f, textColor);
                        }

                        else {
                            const char* label = qpText[i];
                            float scale = qpScales[i];
                            float fontSize = mainFont.baseSize * scale;

                            Vector2 lblSize = MeasureTextEx(mainFont, label, fontSize, 1.5f);
                            
                            float textX = qpPositions[i].x - (lblSize.x / 2.0f);
                            float textY = qpPositions[i].y - (lblSize.y / 2.0f) + yOffset;

                            DrawTextEx(mainFont, label, (Vector2){textX, textY}, fontSize, 1.5f, textColor);
                            
                            if (i == selectedOption) {
                                DrawTriangle(
                                    (Vector2){ textX - 25, textY + lblSize.y/2 },
                                    (Vector2){ textX - 10, textY + lblSize.y/2 - 10 },
                                    (Vector2){ textX - 10, textY + lblSize.y/2 + 10 },
                                    textColor
                                );
                            }
                        }
                    }
                }
                break;

                case STATE_CHARACTER_SELECT:
                {
                    Color bgTint = (Color){ 200, 200, 200, 255 }; 
                    DrawTexturePro(charSelectBg, (Rectangle){0, 0, charSelectBg.width, charSelectBg.height}, (Rectangle){0, 0, 1200, 720}, (Vector2){0, 0}, 0.0f, bgTint);

                    const char* title = (settings.language == LANG_EN) ? "SELECT YOUR MICROBE" : "SELECIONE SEU MICROBIO";
                    DrawTextEx(gameFont, title, (Vector2){(1200 - MeasureTextEx(gameFont, title, fontSizeTitle, fontSpacing).x)/2, 30}, fontSizeTitle, fontSpacing, WHITE);

                    int columns = 4;
                    int rows = 3;
                    float boxSize = 100.0f;
                    float spacing = 0.0f;
                
                    float gridWidth = (columns * boxSize) + ((columns - 1) * spacing);
                    float gridHeight = (rows * boxSize) + ((rows - 1) * spacing);
                
                    float startX = (1200 - gridWidth) / 2.0f;
                    float startY = (720 - gridHeight) / 2.0f;

                    for (int i = 0; i < CHAR_COUNT; i++) {
                        int col = i % columns;
                        int row = i / columns;
                        bool isLocked = (i > 1);

                        float drawX = startX + col * (boxSize + spacing);
                        float drawY = startY + row * (boxSize + spacing);
                        Rectangle boxRect = { drawX, drawY, boxSize, boxSize };

                        Texture2D texToDraw = isLocked ? lockedBoxTex : charBoxTex;

                        if (!isLocked) {
                            DrawRectangle(drawX + 10, drawY + 10, boxSize - 20, boxSize - 20, (Color){0, 0, 0, 100});
                        }

                        DrawTexturePro(texToDraw, 
                            (Rectangle){0, 0, texToDraw.width, texToDraw.height}, 
                            boxRect, (Vector2){0,0}, 0.0f, WHITE);

                        BeginScissorMode((int)drawX + 13, (int)drawY + 13, (int)boxSize - 4, (int)boxSize - 22);

                        if (i == 0) {
                            float iconScale = 3.5f;
                            float iconX = drawX + (boxSize - (bactIcon.width * iconScale)) / 2;
                            float iconY = drawY + (boxSize - (bactIcon.height * iconScale)) / 2;
                            DrawTextureEx(bactIcon, (Vector2){iconX, iconY}, 0.0f, iconScale, WHITE);
                        }

                        else if (i == 1) {
                            float iconScale = 3.5f;
                            float iconX = drawX + (boxSize - (amoebaIcon.width * iconScale)) / 2;
                            float iconY = drawY + (boxSize - (amoebaIcon.height * iconScale)) / 2;
                            DrawTextureEx(amoebaIcon, (Vector2){iconX, iconY}, 0.0f, iconScale, WHITE);
                        }

                        EndScissorMode();

                        if (i == p1Selection) {
                            DrawRectangleLinesEx(boxRect, 4.0f, RED);
                            if ((int)(GetTime() * 10) % 2 == 0) {
                                DrawRectangleLinesEx((Rectangle){drawX-4, drawY-4, boxSize+8, boxSize+8}, 2.0f, YELLOW);
                            }
                        }

                        bool showP2 = isSelectingP2 || (isMultiplayer && !isSelectingP2); 
                        if (showP2 && i == p2Selection) {
                            float offset = (i == p1Selection) ? 4.0f : 0.0f;
                            DrawRectangleLinesEx((Rectangle){drawX + offset, drawY + offset, boxSize - (offset*2), boxSize - (offset*2)}, 4.0f, BLUE);
                        }
                    }

                    float infoBoxW = 340; 
                    float infoBoxH = 190;
                    float infoBoxX = 40;  
                    float infoBoxY = 490; 

                    Rectangle infoDestRec = { infoBoxX, infoBoxY, infoBoxW, infoBoxH };

                    DrawTexturePro(infoBoxTex, 
                        (Rectangle){0, 0, infoBoxTex.width, infoBoxTex.height}, 
                        infoDestRec, (Vector2){0,0}, 0.0f, WHITE);

                    float textLeftMargin = infoBoxX + 25;
                    float textTopMargin = infoBoxY + 20;
                    float statsStartY = textTopMargin + 45; 
                    float lineHeight = 30.0f;               
                    float valueOffsetX = 160.0f; 
                    float nameFontSize = gameFont.baseSize;
                    
                    const char* lblSTR = (settings.language == LANG_EN) ? "STR:" : "FOR:";
                    const char* lblSPD = (settings.language == LANG_EN) ? "SPD:" : "VEL:";
                    const char* statLow = (settings.language == LANG_EN) ? "LOW" : "BAIXO";
                    const char* statMed = (settings.language == LANG_EN) ? "MED" : "MEDIO";
                    const char* statHigh = (settings.language == LANG_EN) ? "HIGH" : "ALTO";
                    const char* localizedLabels[] = { statLow, statMed, statHigh };
                    const char** namesToUse = (settings.language == LANG_EN) ? charNamesEN : charNamesPT;

                    if (p1Selection > 1) {
                         DrawTextEx(gameFont, "???", (Vector2){textLeftMargin, textTopMargin}, nameFontSize, fontSpacing, GRAY);
                         DrawTextEx(gameFont, "HP:", (Vector2){textLeftMargin, statsStartY}, nameFontSize, fontSpacing, WHITE);
                         DrawTextEx(gameFont, "???", (Vector2){textLeftMargin + valueOffsetX, statsStartY}, nameFontSize, fontSpacing, GRAY);
                         
                         DrawTextEx(gameFont, lblSTR, (Vector2){textLeftMargin, statsStartY + lineHeight}, nameFontSize, fontSpacing, WHITE);
                         DrawTextEx(gameFont, "???", (Vector2){textLeftMargin + valueOffsetX, statsStartY + lineHeight}, nameFontSize, fontSpacing, GRAY);

                         DrawTextEx(gameFont, lblSPD, (Vector2){textLeftMargin, statsStartY + (lineHeight * 2)}, nameFontSize, fontSpacing, WHITE);
                         DrawTextEx(gameFont, "???", (Vector2){textLeftMargin + valueOffsetX, statsStartY + (lineHeight * 2)}, nameFontSize, fontSpacing, GRAY);
                    } 
                    else {
                        const char* p1Txt = namesToUse[p1Selection];
                        DrawTextEx(gameFont, p1Txt, (Vector2){textLeftMargin, textTopMargin}, nameFontSize, fontSpacing, WHITE);

                        int hpVal = statsHP[p1Selection];
                        DrawTextEx(gameFont, "HP:", (Vector2){textLeftMargin, statsStartY}, nameFontSize, fontSpacing, WHITE);
                        DrawTextEx(gameFont, localizedLabels[hpVal], (Vector2){textLeftMargin + valueOffsetX, statsStartY}, nameFontSize, fontSpacing, statColors[hpVal]);

                        int strVal = statsSTR[p1Selection];
                        DrawTextEx(gameFont, lblSTR, (Vector2){textLeftMargin, statsStartY + lineHeight}, nameFontSize, fontSpacing, WHITE);
                        DrawTextEx(gameFont, localizedLabels[strVal], (Vector2){textLeftMargin + valueOffsetX, statsStartY + lineHeight}, nameFontSize, fontSpacing, statColors[strVal]);

                        int spdVal = statsSPD[p1Selection];
                        DrawTextEx(gameFont, lblSPD, (Vector2){textLeftMargin, statsStartY + (lineHeight * 2)}, nameFontSize, fontSpacing, WHITE);
                        DrawTextEx(gameFont, localizedLabels[spdVal], (Vector2){textLeftMargin + valueOffsetX, statsStartY + (lineHeight * 2)}, nameFontSize, fontSpacing, statColors[spdVal]);
                    }

                    DrawTextEx(gameFont, "P1", (Vector2){infoBoxX + infoBoxW - 40, infoBoxY + infoBoxH - 30}, 23, fontSpacing, RED);

                    if (isSelectingP2 || isMultiplayer) {
                        float p2BoxX = 1200 - infoBoxX - infoBoxW; 
                        Rectangle p2DestRec = { p2BoxX, infoBoxY, infoBoxW, infoBoxH };
                        Color p2Tint = isSelectingP2 ? WHITE : GRAY;
                        DrawTexturePro(infoBoxTex, (Rectangle){0, 0, infoBoxTex.width, infoBoxTex.height}, p2DestRec, (Vector2){0,0}, 0.0f, p2Tint);
                        float p2TextLeftMargin = p2BoxX + 25;

                        if (p2Selection > 1) {
                            DrawTextEx(gameFont, "???", (Vector2){p2TextLeftMargin, textTopMargin}, nameFontSize, fontSpacing, GRAY);
                            DrawTextEx(gameFont, "HP:", (Vector2){p2TextLeftMargin, statsStartY}, nameFontSize, fontSpacing, WHITE);
                            DrawTextEx(gameFont, "???", (Vector2){p2TextLeftMargin + valueOffsetX, statsStartY}, nameFontSize, fontSpacing, GRAY);
                            DrawTextEx(gameFont, lblSTR, (Vector2){p2TextLeftMargin, statsStartY + lineHeight}, nameFontSize, fontSpacing, WHITE);
                            DrawTextEx(gameFont, "???", (Vector2){p2TextLeftMargin + valueOffsetX, statsStartY + lineHeight}, nameFontSize, fontSpacing, GRAY);
                            DrawTextEx(gameFont, lblSPD, (Vector2){p2TextLeftMargin, statsStartY + (lineHeight * 2)}, nameFontSize, fontSpacing, WHITE);
                            DrawTextEx(gameFont, "???", (Vector2){p2TextLeftMargin + valueOffsetX, statsStartY + (lineHeight * 2)}, nameFontSize, fontSpacing, GRAY);
                        }
                        else {
                            const char* p2Txt = namesToUse[p2Selection];
                            DrawTextEx(gameFont, p2Txt, (Vector2){p2TextLeftMargin, textTopMargin}, nameFontSize, fontSpacing, WHITE);

                            int hpValP2 = statsHP[p2Selection];
                            int strValP2 = statsSTR[p2Selection];
                            int spdValP2 = statsSPD[p2Selection];

                            DrawTextEx(gameFont, "HP:", (Vector2){p2TextLeftMargin, statsStartY}, nameFontSize, fontSpacing, WHITE);
                            DrawTextEx(gameFont, localizedLabels[hpValP2], (Vector2){p2TextLeftMargin + valueOffsetX, statsStartY}, nameFontSize, fontSpacing, statColors[hpValP2]);

                            DrawTextEx(gameFont, lblSTR, (Vector2){p2TextLeftMargin, statsStartY + lineHeight}, nameFontSize, fontSpacing, WHITE);
                            DrawTextEx(gameFont, localizedLabels[strValP2], (Vector2){p2TextLeftMargin + valueOffsetX, statsStartY + lineHeight}, nameFontSize, fontSpacing, statColors[strValP2]);

                            DrawTextEx(gameFont, lblSPD, (Vector2){p2TextLeftMargin, statsStartY + (lineHeight * 2)}, nameFontSize, fontSpacing, WHITE);
                            DrawTextEx(gameFont, localizedLabels[spdValP2], (Vector2){p2TextLeftMargin + valueOffsetX, statsStartY + (lineHeight * 2)}, nameFontSize, fontSpacing, statColors[spdValP2]);
                        }
                        DrawTextEx(gameFont, "P2", (Vector2){p2BoxX + infoBoxW - 40, infoBoxY + infoBoxH - 30}, 23, fontSpacing, BLUE);
                    }
                        
                    const char* instrEN = isSelectingP2 ? "P2: CHOOSE CHARACTER" : "P1: CHOOSE CHARACTER";
                    const char* instrPT = isSelectingP2 ? "P2: ESCOLHA O PERSONAGEM" : "P1: ESCOLHA O PERSONAGEM";
                    const char* instr = (settings.language == LANG_EN) ? instrEN : instrPT;

                    DrawTextEx(gameFont, instr, (Vector2){(1200 - MeasureTextEx(gameFont, instr, fontSizeSmall, fontSpacing).x)/2, 690}, fontSizeSmall, fontSpacing, YELLOW);
                }   
                break;

                case STATE_DEMO_LOCK:
                {
                    ClearBackground((Color){15, 10, 25, 255});
                    
                    float scale = 0.6f; 
                    float rotation = -5.0f;
                    Rectangle sourceRec = {0, 0, (float)boxArtTex.width, (float)boxArtTex.height};
                    Rectangle destRec = {210, 360, (float)boxArtTex.width * scale, (float)boxArtTex.height * scale};
                    Vector2 origin = {(float)boxArtTex.width * scale / 2, (float)boxArtTex.height * scale / 2};
                    
                    DrawTexturePro(boxArtTex, sourceRec, destRec, origin, rotation, WHITE);

                    const char* textToShow = (settings.language == LANG_EN) ? demo_lock_en : demo_lock_pt;
                    
                    Vector2 textPos = { 500, 150 };
                    float fontSizeDemo = mainFont.baseSize * 0.75f;
                    float spacingDemo = 2.0f;
                
                    DrawTextEx(mainFont, textToShow, textPos, fontSizeDemo, spacingDemo, WHITE);

                    const char* returnTxt = (settings.language == LANG_EN) ? "Press ENTER to Return" : "Pressione ENTER para Voltar";
                    Vector2 mRet = MeasureTextEx(mainFont, returnTxt, fontSizeOption, spacingDemo);
                    DrawTextEx(mainFont, returnTxt, (Vector2){(1200 - mRet.x)/2, 650}, fontSizeOption, spacingDemo, YELLOW);
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

        EndDrawing();
    }
    
    SaveGameSettings(&settings);

    GameScene_Unload();
    
    UnloadTexturesInLoop(uniqueTitleBGs, UNIQUE_BG_COUNT);
    UnloadRenderTexture(target);
    UnloadShader(pixelShader);
    UnloadShader(gradientShader);
    UnloadTexture(mmLogo);
    UnloadTexture(cesarLogo);
    UnloadTexture(charSelectBg);
    UnloadTexture(charBoxTex);
    UnloadTexture(infoBoxTex);
    UnloadTexture(lockedBoxTex);
    UnloadTexture(boxArtTex);
    UnloadTexture(bactIcon);
    UnloadTexture(amoebaIcon);
    UnloadFont(mainFont);
    UnloadFont(gameFont);
    UnloadImage(icon);

    for (int i = 0; i < MENU_OPTIONS; i++) {
        UnloadTexture(menuIcons[i]);
    }

    UnloadTexture(qpIcons[0]);
    UnloadTexture(qpIcons[1]);

    StopMusicStream(menuMusic);
    UnloadMusicStream(menuMusic);
    StopMusicStream(cssMusic);
    UnloadMusicStream(cssMusic);
    StopMusicStream(fightMusic);
    UnloadMusicStream(fightMusic);
    UnloadSound(sndSelect);
    UnloadSound(sndSelected);

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

    float bgLines[15][4] = {
        {0.088f, 0.09f, 0.088f, 0.60f},
        {0.088f, 0.60f, 0.159f, 0.60f},
        {0.290f, 0.91f, 0.720f, 0.91f},
        {0.265f, 0.09f, 0.840f, 0.09f},
        {0.840f, 0.09f, 0.840f, 0.20f},
        {0.912f, 0.55f, 0.912f, 0.91f},
        {0.830f, 0.55f, 0.912f, 0.55f},
        {0.240f, 0.55f, 0.240f, 0.76f},
        {0.300f, 0.26f, 0.380f, 0.26f},
        {0.460f, 0.15f, 0.460f, 0.21f},
        {0.370f, 0.15f, 0.460f, 0.15f},
        {0.530f, 0.79f, 0.530f, 0.83f},
        {0.530f, 0.83f, 0.590f, 0.83f},
        {0.730f, 0.25f, 0.730f, 0.41f},
        {0.640f, 0.25f, 0.730f, 0.25f}
    };

    for (int i = 0; i < 15; i++) {
        Vector2 startPos = { screenWidth * bgLines[i][0], screenHeight * bgLines[i][1] };
        Vector2 endPos   = { screenWidth * bgLines[i][2], screenHeight * bgLines[i][3] };
    
        DrawLineEx(startPos, endPos, thickness, lineColor);
    }

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