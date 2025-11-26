#include "game_scene.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>

#define PLAYER_HALF_WIDTH 20
#define PLAYER_HEIGHT 60

typedef enum {
    SCENE_STATE_START,
    SCENE_STATE_PLAY,
    SCENE_STATE_PAUSED,
    SCENE_STATE_ROUND_END,
    SCENE_STATE_GAME_OVER
} InternalSceneState;

static Font hudFont;
static bool isMultiplayerMode = false;
static Player *player1;
static Player *player2;
static InternalSceneState sceneState;
static int countdownTimer = 0;
static int matchWinner = 0;
static int fightBannerTimer = 0;
static int pauseOption = 0;
const char* pauseOptionsText[] = { "RESUME", "SETTINGS", "QUIT MATCH" };

static Texture2D texGuiFrame;
static Texture2D texSyringeEmptyL, texSyringeFullL;
static Texture2D texSyringeEmptyR, texSyringeFullR;
static Texture2D texPillEmptyL, texPillFullL;
static Texture2D texPillEmptyR, texPillFullR;
static Texture2D texTabletActive, texTabletInactive;

static const char* GetCharacterJSON(int charID) {
    switch(charID) {
        case 0: return "assets/data/bacteriophage.json";
        case 1: return "assets/data/amoeba.json";
        case 2: return "assets/data/tardigrade.json";
        default: return "assets/data/bacteriophage.json";
    }
}

static const char* GetCharacterName(int charID) {
    switch(charID) {
        case 0: return "BACTERIOPHAGE";
        case 1: return "AMOEBA";
        case 2: return "TARDIGRADE";
        default: return "UNKNOWN";
    }
}

static void UpdateHuman(Player *player, float dt, InputConfig input) {
    Combat_ApplyStatus(player, dt);
    bool isP1 = (player == player1);

    if (player->state == PLAYER_STATE_ATTACK) {
        player->attackFrameCounter++;
        Move *move = (player->currentMove != NULL) ? player->currentMove : &player->moves->sideGround;
        
        if (move->canCombo && IsKeyPressed(input.attack)) {
            if (player->attackFrameCounter > (move->startupFrames + move->activeFrames)) {
                player->attackFrameCounter = 0; 
                Combat_TryExecuteMove(player, move, isP1);
                return;
            }
        }

        if (move->selfVelocity.x != 0 || move->selfVelocity.y != 0) {
            float dir = player->isFlipped ? -1.0f : 1.0f;
            player->velocity.x = move->selfVelocity.x * dir;
            player->velocity.y = move->selfVelocity.y;
        }

        if (move->steerSpeed > 0) {
            if (IsKeyDown(input.left))  player->position.x -= move->steerSpeed;
            if (IsKeyDown(input.right)) player->position.x += move->steerSpeed;
        }

        if (move->type == MOVE_TYPE_ULTIMATE_FALL) {
            int totalFrames = move->startupFrames + move->activeFrames;
            int peakFrame = 40;
            int hangTime = 60;

            if (player->attackFrameCounter == peakFrame) {
                Move cloud = {0};
                cloud.type = MOVE_TYPE_TRAP;
                cloud.hitbox = (Rectangle){ -300, -300, 600, 600 }; 
                
                cloud.damage = 2.0f;
                cloud.effect = EFFECT_POISON;
                cloud.effectDuration = 5.0f;
                cloud.trapDuration = 5.0f;

                Vector2 currentPos = player->position;
                player->position = player->ultLaunchPos;
                
                Combat_TryExecuteMove(player, &cloud, isP1);
                
                player->position = currentPos;
            }

            if (player->attackFrameCounter >= peakFrame && player->attackFrameCounter < (peakFrame + hangTime)) {
                player->velocity.y = 0; 
                player->velocity.x = 0;
            }
            else if (player->attackFrameCounter >= (peakFrame + hangTime)) {
                player->velocity.y = move->fallSpeed;
                
                if (IsKeyDown(input.left))  player->position.x -= move->steerSpeed;
                if (IsKeyDown(input.right)) player->position.x += move->steerSpeed;
            }
        }
        
        player->position = Vector2Add(player->position, player->velocity);
        
        if (player->position.y > 540) { 
             if (move->selfVelocity.y > 0 || move->fallSpeed > 0) {
                 
                 if (move->type == MOVE_TYPE_ULTIMATE_FALL) {
                     Move explosion = {0};
                     explosion.type = MOVE_TYPE_ULTIMATE; 
                     explosion.hitbox = (Rectangle){ -200, -150, 400, 300 }; 
                     explosion.damage = 40.0f;
                     explosion.knockback = (Vector2){ 25.0f, -25.0f };
                     explosion.activeFrames = 10;
                     
                     Combat_TryExecuteMove(player, &explosion, isP1);
                 }

                 player->position.y = 540;
                 player->state = PLAYER_STATE_IDLE;
                 player->currentMove = NULL;
                 player->velocity = (Vector2){0,0};
                 return;
             }
        }

        int totalAttackFrames = move->startupFrames + move->activeFrames + move->recoveryFrames;     
        if (player->attackFrameCounter > totalAttackFrames) {
            player->state = PLAYER_STATE_IDLE;
            player->currentMove = NULL;
            player->velocity = (Vector2){0,0};
        }
        return;
    }

    if (!player->isGrounded) {
        player->velocity.y += 0.5f;
        player->state = PLAYER_STATE_FALL;
    }

    player->position = Vector2Add(player->position, player->velocity);

    int screenWidth = GAME_WIDTH;   
    if (player->position.x - PLAYER_HALF_WIDTH < 0) {
        player->position.x = PLAYER_HALF_WIDTH;
        player->velocity.x = 0; 
    }
    if (player->position.x + PLAYER_HALF_WIDTH > screenWidth) {
        player->position.x = screenWidth - PLAYER_HALF_WIDTH;
        player->velocity.x = 0;
    }
    
    float groundLevel = 540.0f;
    if (player->position.y > groundLevel) {
        player->position.y = groundLevel;
        player->velocity.y = 0;
        player->isGrounded = true;
        if (player->state == PLAYER_STATE_FALL) player->state = PLAYER_STATE_IDLE;
    }

    player->velocity.x = 0;
    if (IsKeyDown(input.left)) { 
        player->velocity.x = -5.0f; 
        player->isFlipped = true; 
    }
    if (IsKeyDown(input.right)) { 
        player->velocity.x = 5.0f; 
        player->isFlipped = false; 
    }
    
    if (player->velocity.x != 0 && player->isGrounded) player->state = PLAYER_STATE_WALK;
    else if (player->isGrounded) player->state = PLAYER_STATE_IDLE;
    
    if (IsKeyPressed(input.jump) && player->isGrounded) {
        player->velocity.y = -12.0f;
        player->isGrounded = false;
        player->state = PLAYER_STATE_JUMP;
    }

    Move *selectedMove = NULL;
    bool isSpecial = false;

    if (IsKeyPressed(input.attack) || IsKeyPressed(input.special)) {
        isSpecial = IsKeyPressed(input.special);
        
        player->state = PLAYER_STATE_ATTACK;
        player->attackFrameCounter = 0;

        if (IsKeyDown(input.up)) {
            if (isSpecial) selectedMove = &player->moves->specialUp;
            else selectedMove = player->isGrounded ? &player->moves->upGround : &player->moves->airUp;
        }
        else if (IsKeyDown(input.down)) {
            if (isSpecial) selectedMove = &player->moves->specialDown;
            else selectedMove = player->isGrounded ? &player->moves->downGround : &player->moves->airDown;
        }
        else {
            bool movingSide = IsKeyDown(input.left) || IsKeyDown(input.right);
            
            if (IsKeyDown(input.left)) player->isFlipped = true;
            if (IsKeyDown(input.right)) player->isFlipped = false;

            if (isSpecial) {
                if (movingSide) {
                    selectedMove = &player->moves->specialSide;
                } else {
                    if (player->currentUlt >= player->maxUlt) {
                        selectedMove = &player->moves->ultimate;
                        player->ultLaunchPos = player->position;
                    } else {
                        selectedMove = &player->moves->specialNeutral;
                    }
                }
            } else {
                selectedMove = player->isGrounded ? (movingSide ? &player->moves->sideGround : &player->moves->neutralGround) 
                                                : (movingSide ? &player->moves->airSide : &player->moves->airNeutral);
            }
        }

        if (selectedMove != NULL) {
            if (GetTime() - selectedMove->lastUsedTime < selectedMove->cooldown) {
                player->state = PLAYER_STATE_IDLE;
                return;
            }
            selectedMove->lastUsedTime = GetTime();
            
            Combat_TryExecuteMove(player, selectedMove, isP1);
            player->currentMove = selectedMove;

            if (selectedMove == &player->moves->ultimate) {
                player->ultCharge = 0.0f;
                player->currentUlt = 0;
                printf("ULTIMATE ACTIVATED! Meter Reset.\n");
            }
        }
    }
}

static void UpdateAI(Player *ai, Player *target, float dt) {
    Combat_ApplyStatus(ai, dt);

    float distanceX = target->position.x - ai->position.x;
    float distanceY = target->position.y - ai->position.y;

    if (target->state == PLAYER_STATE_ATTACK && ai->state != PLAYER_STATE_ATTACK) {
        if (fabs(distanceX) < 150.0f) {
            ai->aiState = AI_STATE_FLEE;
            ai->aiTimer = 0;
        }
    }

    if (ai->state != PLAYER_STATE_ATTACK) { 
        switch (ai->aiState) {
            case AI_STATE_THINKING:
                ai->velocity.x = 0;
                ai->aiTimer++;
                if (ai->aiTimer > 30) { 
                    ai->aiTimer = 0;
                    int decision = GetRandomValue(1, 100);
                    if (distanceY < -50 && fabs(distanceX) < 100) {
                        if (ai->isGrounded) {
                            ai->velocity.y = -12.0f;
                            ai->isGrounded = false;
                            ai->state = PLAYER_STATE_JUMP;
                        } else {
                            ai->state = PLAYER_STATE_ATTACK;
                            ai->attackFrameCounter = 0;
                            Combat_TryExecuteMove(ai, &ai->moves->airUp, false);
                            ai->currentMove = &ai->moves->airUp;
                        }
                        ai->aiState = AI_STATE_THINKING;
                    }
                    else if (decision < 20 && ai->isGrounded) {
                        ai->velocity.y = -12.0f;
                        ai->isGrounded = false;
                        ai->state = PLAYER_STATE_JUMP;
                        if (distanceX > 0) ai->velocity.x = 4.0f; else ai->velocity.x = -4.0f;
                        ai->aiState = AI_STATE_APPROACH;
                    }
                    else if (fabs(distanceX) < 100) { 
                        if (decision < 70) ai->aiState = AI_STATE_ATTACK; else ai->aiState = AI_STATE_APPROACH;
                    } 
                    else { 
                        if (decision < 80) ai->aiState = AI_STATE_APPROACH; else ai->aiState = AI_STATE_THINKING;
                    }
                }
                break;

            case AI_STATE_APPROACH:
                if (distanceX > 0) { ai->velocity.x = 5.0f; ai->isFlipped = false; } 
                else { ai->velocity.x = -5.0f; ai->isFlipped = true; }

                if (!ai->isGrounded && distanceY > 0 && fabs(distanceX) < 60) {
                    ai->state = PLAYER_STATE_ATTACK;
                    ai->attackFrameCounter = 0;
                    Combat_TryExecuteMove(ai, &ai->moves->airDown, false);
                    ai->currentMove = &ai->moves->airDown;
                    ai->aiState = AI_STATE_THINKING;
                }
                else if (fabs(distanceX) < 80 && ai->isGrounded) {
                    ai->aiState = AI_STATE_ATTACK;
                }
                break;

            case AI_STATE_ATTACK:
                {
                    int attackType = GetRandomValue(0, 2);
                    ai->state = PLAYER_STATE_ATTACK;
                    ai->attackFrameCounter = 0;
                    Move *move = NULL;

                    if (attackType == 0) move = &ai->moves->sideGround;
                    else if (attackType == 1) move = &ai->moves->upGround;
                    else move = &ai->moves->downGround;
                    
                    Combat_TryExecuteMove(ai, move, false);
                    ai->currentMove = move;
                    
                    ai->aiState = AI_STATE_THINKING;
                    ai->aiTimer = 0;
                }
                break;
            
            case AI_STATE_FLEE:
                ai->aiTimer++;
                if (distanceX > 0) { ai->velocity.x = -5.0f; ai->isFlipped = true; } 
                else { ai->velocity.x = 5.0f; ai->isFlipped = false; }
                
                if (ai->aiTimer > 18) {
                    ai->aiState = AI_STATE_THINKING;
                    ai->aiTimer = 0;
                }
                break;
        }
    }

    if (ai->state == PLAYER_STATE_ATTACK) {
        ai->attackFrameCounter++;
        Move *move = (ai->currentMove != NULL) ? ai->currentMove : &ai->moves->sideGround;
        
        int totalFrames = move->startupFrames + move->activeFrames + move->recoveryFrames;
        if (ai->attackFrameCounter > totalFrames) {
            ai->state = PLAYER_STATE_IDLE;
            ai->currentMove = NULL;
        }
        return;
    }

    if (!ai->isGrounded) {
        ai->velocity.y += 0.5f;
        ai->state = PLAYER_STATE_FALL;
    }
    ai->position = Vector2Add(ai->position, ai->velocity);

    if (ai->position.x - PLAYER_HALF_WIDTH < 0) { ai->position.x = PLAYER_HALF_WIDTH; ai->velocity.x = 0; }
    if (ai->position.x + PLAYER_HALF_WIDTH > GAME_WIDTH) { ai->position.x = GAME_WIDTH - PLAYER_HALF_WIDTH; ai->velocity.x = 0; }
    if (ai->position.y > 540) {
        ai->position.y = 540;
        ai->velocity.y = 0;
        ai->isGrounded = true;
        if (ai->state == PLAYER_STATE_FALL) ai->state = PLAYER_STATE_IDLE;
    }
    
    if (ai->velocity.x != 0 && ai->isGrounded) ai->state = PLAYER_STATE_WALK;
    else if (ai->isGrounded && ai->state != PLAYER_STATE_JUMP) ai->state = PLAYER_STATE_IDLE;
}

static void ResetRound(void) {
    player1->position = (Vector2){ 400, 540 };
    player1->velocity = (Vector2){ 0, 0 };
    player1->state = PLAYER_STATE_IDLE;
    player1->currentHealth = player1->maxHealth;
    player1->isGrounded = false;
    player1->currentMove = NULL;
    player1->isFlipped = false;
    player1->poisonTimer = 0;

    player2->position = (Vector2){ 800, 540 };
    player2->velocity = (Vector2){ 0, 0 };
    player2->state = PLAYER_STATE_IDLE;
    player2->currentHealth = player2->maxHealth;
    player2->isGrounded = false;
    player2->currentMove = NULL;
    player2->isFlipped = true;
    player2->poisonTimer = 0;

    if (!isMultiplayerMode) {
        player2->aiState = AI_STATE_THINKING;
        player2->aiTimer = 0;
    }

    Combat_Cleanup(); 

    countdownTimer = 0;
    fightBannerTimer = 0;
    sceneState = SCENE_STATE_START;
}

void GameScene_SetMultiplayer(bool enabled) {
    isMultiplayerMode = enabled;
}

void GameScene_SetFont(Font font) {
    hudFont = font;
}

void GameScene_Init(int p1CharacterID, int p2CharacterID) {
    sceneState = SCENE_STATE_START;
    matchWinner = 0;
    countdownTimer = 0;
    fightBannerTimer = 0;

    player1 = (Player*)malloc(sizeof(Player));
    player1->position = (Vector2){ 400, 540 };
    player1->velocity = (Vector2){ 0, 0 };
    player1->isGrounded = false;
    player1->isFlipped = false;
    player1->state = PLAYER_STATE_IDLE;
    player1->attackFrameCounter = 0;
    player1->currentMove = NULL;
    player1->maxHealth = 100.0f;
    player1->currentHealth = 100.0f;
    player1->maxUlt = 8;
    player1->currentUlt = 0;
    player1->chargePerPill = 100.0f;
    player1->maxUltCharge = player1->maxUlt * player1->chargePerPill;
    player1->ultCharge = 0.0f;
    player1->roundsWon = 0;
    player1->poisonTimer = 0;
    
    player1->moves = LoadMovesetFromJSON(GetCharacterJSON(p1CharacterID));
    TextCopy(player1->name, GetCharacterName(p1CharacterID));

    player2 = (Player*)malloc(sizeof(Player));
    player2->position = (Vector2){ 800, 540 };
    player2->velocity = (Vector2){ 0, 0 };
    player2->isGrounded = false;
    player2->isFlipped = true;
    player2->state = PLAYER_STATE_IDLE;
    player2->attackFrameCounter = 0;
    player2->currentMove = NULL;
    player2->maxHealth = 100.0f;
    player2->currentHealth = 100.0f;
    player2->maxUlt = 8;
    player2->currentUlt = 0;
    player2->chargePerPill = 100.0f; 
    player2->maxUltCharge = player2->maxUlt * player2->chargePerPill;
    player2->ultCharge = 0.0f;
    player2->roundsWon = 0;
    player2->poisonTimer = 0;

    player2->moves = LoadMovesetFromJSON(GetCharacterJSON(p2CharacterID));
    TextCopy(player2->name, GetCharacterName(p2CharacterID));

    if (isMultiplayerMode) {
        player2->isCPU = false;
        player2->aiState = 0;
    } else {
        player2->isCPU = true;
        player2->aiState = AI_STATE_THINKING;
        player2->aiTimer = 0;
    }

    Combat_Init();

    texGuiFrame = LoadTexture("assets/gui_frame.png");
    texSyringeEmptyL = LoadTexture("assets/lsyringe_empty.png");
    texSyringeFullL = LoadTexture("assets/lsyringe_full.png");
    texSyringeEmptyR = LoadTexture("assets/rsyringe_empty.png");
    texSyringeFullR = LoadTexture("assets/rsyringe_full.png");
    texPillEmptyL = LoadTexture("assets/lpill_empty.png");
    texPillFullL = LoadTexture("assets/lpill_full.png");
    texPillEmptyR = LoadTexture("assets/rpill_empty.png");
    texPillFullR = LoadTexture("assets/rpill_full.png");
    texTabletActive = LoadTexture("assets/tablet_active.png");
    texTabletInactive = LoadTexture("assets/tablet_inactive.png");
}

int GameScene_Update(void) {
    float dt = GetFrameTime();

    if (IsKeyPressed(KEY_P)) {
        if (sceneState == SCENE_STATE_PLAY) {
            sceneState = SCENE_STATE_PAUSED;
            pauseOption = 0;
        }
        else if (sceneState == SCENE_STATE_PAUSED) {
            sceneState = SCENE_STATE_PLAY;
        }
    }

    switch (sceneState) {
        case SCENE_STATE_START:
            countdownTimer++;
            if (countdownTimer > 180) {
                sceneState = SCENE_STATE_PLAY;
            }
            break;
            
        case SCENE_STATE_PLAY:
            if (fightBannerTimer < 120) fightBannerTimer++;

            InputConfig p1Controls = { KEY_A, KEY_D, KEY_W, KEY_S, KEY_SPACE, KEY_J, KEY_K };
            UpdateHuman(player1, dt, p1Controls);

            if (isMultiplayerMode) {
                InputConfig p2Controls = { KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_KP_0, KEY_KP_1, KEY_KP_2 };
                UpdateHuman(player2, dt, p2Controls);
            } else {
                UpdateAI(player2, player1, dt);
            }

            // DEBUG KEYS
            if (IsKeyPressed(KEY_F2)) {
                player1->ultCharge = player1->maxUltCharge;
                player1->currentUlt = player1->maxUlt;
                printf("DEBUG: Player 1 Ult Maxed\n");
            }
            if (IsKeyPressed(KEY_F3)) {
                player1->roundsWon++;
                if (player1->roundsWon > 3) player1->roundsWon = 3;
                printf("DEBUG: Player 1 Rounds Won: %d\n", player1->roundsWon);
            }
            
            Combat_Update(player1, player2);

            if (player1->currentHealth <= 0 || player2->currentHealth <= 0) {
                if (player1->currentHealth <= 0) player2->roundsWon++;
                else if (player2->currentHealth <= 0) player1->roundsWon++;

                sceneState = SCENE_STATE_ROUND_END;
                countdownTimer = 0;
            }
            break;

        case SCENE_STATE_PAUSED:
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                pauseOption++;
                if (pauseOption > 2) pauseOption = 0;
            }
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                pauseOption--;
                if (pauseOption < 0) pauseOption = 2;
            }

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (pauseOption == 0) {
                    sceneState = SCENE_STATE_PLAY;
                }
                else if (pauseOption == 1) {
                    ToggleFullscreen(); 
                }
                else if (pauseOption == 2) {
                    return 1;
                }
            }
            break;

        case SCENE_STATE_ROUND_END:
            countdownTimer++;
            if (countdownTimer > 120) {
                if (player1->roundsWon >= 3 || player2->roundsWon >= 3) {
                    sceneState = SCENE_STATE_GAME_OVER;
                    matchWinner = (player1->roundsWon >= 3) ? 1 : 2;
                } 
                else {
                    ResetRound();
                }
            }
            break;

        case SCENE_STATE_GAME_OVER:
            if (IsKeyPressed(KEY_ENTER)) {
                return 1;
            }
            break;
    }
    return 0;
}

void GameScene_Draw(void) {
    DrawRectangle(0, 0, GAME_WIDTH, GAME_HEIGHT, SKYBLUE);
    DrawRectangle(0, 600, GAME_WIDTH, 120, GREEN);

    Color p1Color = (player1->state == PLAYER_STATE_ATTACK) ? RED : WHITE;
    DrawRectangleLinesEx((Rectangle){ player1->position.x - 20, player1->position.y, 40, 60}, 2.0f, p1Color);

    Color p2Color = (player2->state == PLAYER_STATE_ATTACK) ? ORANGE : BLUE;
    DrawRectangleLinesEx((Rectangle){ player2->position.x - 20, player2->position.y, 40, 60}, 2.0f, p2Color);

    Combat_Draw();

    float uiScale = 1.7f;
    float frameW = texGuiFrame.width * uiScale;
    float startX = (GAME_WIDTH - frameW) / 2.0f;
    float startY = 20.0f;

    DrawTextureEx(texGuiFrame, (Vector2){startX, startY}, 0.0f, uiScale, WHITE);

    float offSyringeY = startY + (33 * uiScale);
    float offSyringeXL = startX + (28 * uiScale);
    float offSyringeXR = startX + frameW - (texSyringeFullR.width * uiScale) - (28 * uiScale);

    DrawTextureEx(texSyringeEmptyL, (Vector2){offSyringeXL, offSyringeY}, 0.0f, uiScale, WHITE);
    
    float healthPct1 = player1->currentHealth / player1->maxHealth;
    if (healthPct1 < 0) healthPct1 = 0;
    Rectangle sourceRecL = { 0, 0, texSyringeFullL.width * healthPct1, (float)texSyringeFullL.height };
    Rectangle destRecL   = { offSyringeXL, offSyringeY, (texSyringeFullL.width * uiScale) * healthPct1, texSyringeFullL.height * uiScale };
    DrawTexturePro(texSyringeFullL, sourceRecL, destRecL, (Vector2){0,0}, 0.0f, WHITE);

    DrawTextureEx(texSyringeEmptyR, (Vector2){offSyringeXR, offSyringeY}, 0.0f, uiScale, WHITE);
    
    float healthPct2 = player2->currentHealth / player2->maxHealth;
    if (healthPct2 < 0) healthPct2 = 0;
    Rectangle sourceRecR = { texSyringeFullR.width * (1.0f - healthPct2), 0, texSyringeFullR.width * healthPct2, (float)texSyringeFullR.height };
    Rectangle destRecR = { offSyringeXR + (texSyringeFullR.width * uiScale * (1.0f - healthPct2)), offSyringeY, (texSyringeFullR.width * uiScale) * healthPct2, texSyringeFullR.height * uiScale };
    DrawTexturePro(texSyringeFullR, sourceRecR, destRecR, (Vector2){0,0}, 0.0f, WHITE);

    float pillSpacing = 22 * uiScale;
    float offPillY = startY + (71 * uiScale);
    float offPillXL = startX + (55 * uiScale);
    float offPillXR = startX + frameW - (55 * uiScale);

    for (int i = 0; i < player1->maxUlt; i++) {
        Vector2 pos = { offPillXL + i * pillSpacing, offPillY };
        
        DrawTextureEx(texPillEmptyL, pos, 0, uiScale, WHITE);

        float chargeNeededStart = i * player1->chargePerPill;
        float chargeNeededEnd = (i + 1) * player1->chargePerPill;
        
        if (player1->ultCharge >= chargeNeededEnd) {
            DrawTextureEx(texPillFullL, pos, 0, uiScale, WHITE);
        } 
        else if (player1->ultCharge > chargeNeededStart) {
            float fillAmount = (player1->ultCharge - chargeNeededStart) / player1->chargePerPill;
            
            Rectangle source = { 0, 0, texPillFullL.width * fillAmount, (float)texPillFullL.height };
            Rectangle dest = { pos.x, pos.y, (texPillFullL.width * uiScale) * fillAmount, texPillFullL.height * uiScale };
            DrawTexturePro(texPillFullL, source, dest, (Vector2){0,0}, 0.0f, WHITE);
        }
    }

    for (int i = 0; i < player2->maxUlt; i++) {
        Vector2 pos = { offPillXR - ((i + 1) * pillSpacing), offPillY };
        
        DrawTextureEx(texPillEmptyR, pos, 0, uiScale, WHITE);

        float chargeNeededStart = i * player2->chargePerPill;
        float chargeNeededEnd = (i + 1) * player2->chargePerPill;

        if (player2->ultCharge >= chargeNeededEnd) {
            DrawTextureEx(texPillFullR, pos, 0, uiScale, WHITE);
        } 
        else if (player2->ultCharge > chargeNeededStart) {
            float fillAmount = (player2->ultCharge - chargeNeededStart) / player2->chargePerPill;
            Rectangle source = { 
                texPillFullR.width * (1.0f - fillAmount),
                0, 
                texPillFullR.width * fillAmount, 
                (float)texPillFullR.height 
            };
            
            float drawnWidth = (texPillFullR.width * uiScale) * fillAmount;
            Rectangle dest = { 
                pos.x + (texPillFullR.width * uiScale) - drawnWidth,
                pos.y, 
                drawnWidth, 
                texPillFullR.height * uiScale 
            };
            
            DrawTexturePro(texPillFullR, source, dest, (Vector2){0,0}, 0.0f, WHITE);
        }
    }

    float offTabletY = startY + (68 * uiScale);
    float tabletSpacing = 5 * uiScale;
    float oneTabletW = texTabletActive.width * uiScale;
    float tabletsWidth = (oneTabletW * 3) + (tabletSpacing * 2);
    float centerX = startX + frameW / 2.0f;
    float tabletOffset = 20 * uiScale;
    float offTabletXL = centerX - tabletOffset - tabletsWidth;
    float offTabletXR = centerX + tabletOffset;
    float nameOffsetY = 20.0f;
    float nameOffsetX = 55.0f;

    for (int i = 0; i < 3; i++) {
        Texture2D tex = (i < player1->roundsWon) ? texTabletActive : texTabletInactive;
        DrawTextureEx(tex, (Vector2){ offTabletXL + i * (oneTabletW + tabletSpacing), offTabletY }, 0, uiScale, WHITE);
    }
    for (int i = 0; i < 3; i++) {
        Texture2D tex = (i < player2->roundsWon) ? texTabletActive : texTabletInactive;
        DrawTextureEx(tex, (Vector2){ offTabletXR + i * (oneTabletW + tabletSpacing), offTabletY }, 0, uiScale, WHITE);
    }

    float fontSize = hudFont.baseSize * 0.8f;
    float fontSpacing = 3.5f;
    Color nameColor = WHITE;

    Vector2 p1NamePos = { 
        startX + (nameOffsetX * uiScale),
        startY + (nameOffsetY * uiScale)
    };
    DrawTextEx(hudFont, player1->name, p1NamePos, fontSize, fontSpacing, nameColor);

    Vector2 p2NameSize = MeasureTextEx(hudFont, player2->name, fontSize, fontSpacing);
    Vector2 p2NamePos = { 
        (startX + frameW) - (nameOffsetX * uiScale) - p2NameSize.x,
        startY + (nameOffsetY * uiScale)
    };
    DrawTextEx(hudFont, player2->name, p2NamePos, fontSize, fontSpacing, nameColor);

    if (sceneState == SCENE_STATE_START) {
        const char* countdownText = "";
        if (countdownTimer < 60) countdownText = "3";
        else if (countdownTimer < 120) countdownText = "2";
        else if (countdownTimer < 180) countdownText = "1";
        
        int fontSize = 80;
        int textWidth = MeasureText(countdownText, fontSize);
        DrawText(countdownText, (GAME_WIDTH - textWidth)/2, (GAME_HEIGHT - fontSize)/2, fontSize, YELLOW);
    }

    if (sceneState == SCENE_STATE_PLAY && fightBannerTimer < 60) {
        const char* fightText = "FIGHT!";
        int fontSize = 100;
        int textWidth = MeasureText(fightText, fontSize);
        
        Color fightColor = (fightBannerTimer % 10 < 5) ? RED : ORANGE;
        
        DrawText(fightText, (GAME_WIDTH - textWidth)/2, (GAME_HEIGHT - fontSize)/2, fontSize, fightColor);
    }
    else if (sceneState == SCENE_STATE_ROUND_END) {
        const char* text = "KO!";
        int fontSize = 100;
        DrawText(text, (GAME_WIDTH - MeasureText(text, fontSize))/2, (GAME_HEIGHT - fontSize)/2, fontSize, RED);
    }
    else if (sceneState == SCENE_STATE_GAME_OVER) {
        DrawRectangle(0, 0, GAME_WIDTH, GAME_HEIGHT, (Color){0,0,0, 200});
        char* wText = (matchWinner == 1) ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!";
        Color wColor = (matchWinner == 1) ? GREEN : BLUE;
        
        int fontSize = 60;
        DrawText(wText, (GAME_WIDTH - MeasureText(wText, fontSize))/2, GAME_HEIGHT/2 - 50, fontSize, wColor);
        const char* subText = "Press ENTER to Return";
        DrawText(subText, (GAME_WIDTH - MeasureText(subText, 30))/2, GAME_HEIGHT/2 + 20, 30, RAYWHITE);
    }

    if (sceneState == SCENE_STATE_PAUSED) {
        DrawRectangle(0, 0, GAME_WIDTH, GAME_HEIGHT, (Color){ 0, 0, 0, 150 });

        float menuW = 400;
        float menuH = 300;
        float menuX = (GAME_WIDTH - menuW) / 2;
        float menuY = (GAME_HEIGHT - menuH) / 2;
        
        DrawRectangle(menuX, menuY, menuW, menuH, (Color){ 20, 30, 60, 240 });
        DrawRectangleLinesEx((Rectangle){menuX, menuY, menuW, menuH}, 4, SKYBLUE);

        const char* title = "PAUSED";
        int titleSize = 60;
        int titleW = MeasureText(title, titleSize);
        DrawText(title, (GAME_WIDTH - titleW)/2, menuY + 30, titleSize, WHITE);

        int startOptY = menuY + 120;
        int spacing = 60;

        for (int i = 0; i < 3; i++) {
            Color color = (i == pauseOption) ? YELLOW : GRAY;
            int fontSize = (i == pauseOption) ? 40 : 30;
            
            const char* txt = pauseOptionsText[i];
            int txtW = MeasureText(txt, fontSize);
            DrawText(txt, (GAME_WIDTH - txtW)/2, startOptY + (i * spacing), fontSize, color);

            if (i == pauseOption) {
                DrawText(">", (GAME_WIDTH - txtW)/2 - 30, startOptY + (i * spacing), fontSize, YELLOW);
            }
        }
    }
}

void GameScene_Unload(void) {
    UnloadTexture(texGuiFrame);
    UnloadTexture(texSyringeEmptyL); UnloadTexture(texSyringeFullL);
    UnloadTexture(texSyringeEmptyR); UnloadTexture(texSyringeFullR);
    UnloadTexture(texPillEmptyL);    UnloadTexture(texPillFullL);
    UnloadTexture(texPillEmptyR);    UnloadTexture(texPillFullR);
    UnloadTexture(texTabletActive);  UnloadTexture(texTabletInactive);

    if (player1 != NULL) {
        if (player1->moves != NULL) free(player1->moves);
        free(player1);
        player1 = NULL;
    }
    if (player2 != NULL) {
        if (player2->moves != NULL) free(player2->moves);
        free(player2);
        player2 = NULL;
    }

    Combat_Cleanup();
}