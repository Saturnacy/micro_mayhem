#include "game_scene.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>

#define PLAYER_HALF_WIDTH 20
#define PLAYER_HEIGHT 60

typedef enum {
    SCENE_STATE_START,
    SCENE_STATE_PLAY,
    SCENE_STATE_ROUND_END,
    SCENE_STATE_GAME_OVER
} InternalSceneState;

static Player *player1;
static Player *player2;
static HitboxNode *activeHitboxes = NULL;
static InternalSceneState sceneState;
static int countdownTimer = 0;
static int matchWinner = 0;
static Texture2D texGuiFrame;
static Texture2D texSyringeEmptyL, texSyringeFullL;
static Texture2D texSyringeEmptyR, texSyringeFullR;
static Texture2D texPillEmptyL, texPillFullL;
static Texture2D texPillEmptyR, texPillFullR;
static Texture2D texTabletActive, texTabletInactive;

static Moveset* LoadMoveSetFromFile(const char *filename) {
    Moveset *moves = (Moveset*)malloc(sizeof(Moveset));
    
    if (moves == NULL) return NULL;

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        TraceLog(LOG_WARNING, "Moveset file not found! creating a placeholder: %s", filename);
        
        moves->sideLight    = (Move){ 5, 10, 15, (Rectangle){ 20, -10, 30, 20 }, (Vector2){ 3, 0 }, 5 };
        moves->upLight      = (Move){ 6, 12, 18, (Rectangle){ -10, -40, 20, 30 }, (Vector2){ 0, -5 }, 6 };
        moves->downLight    = (Move){ 4, 8, 12, (Rectangle){ 20, 10, 25, 15 }, (Vector2){ 2, 0 }, 4 };
        moves->airSideLight = (Move){ 5, 10, 15, (Rectangle){ 20, -10, 30, 20 }, (Vector2){ 3, 0 }, 5 };
        moves->airUpLight   = (Move){ 7, 14, 20, (Rectangle){ -15, -45, 15, 20 }, (Vector2){ 0, -4 }, 7 };
        moves->airDownLight = (Move){ 10, 30, 20, (Rectangle){ -20, -20, 40, 40 }, (Vector2){ 4, 4 }, 8 };

        file = fopen(filename, "wb");
        if (file != NULL) {
            fwrite(moves, sizeof(Moveset), 1, file);
            fclose(file);
        }
    } else {
        fread(moves, sizeof(Moveset), 1, file);
        fclose(file);
    }
    return moves;
}

static void SpawnHitbox(Player *attacker, Move move) {
    HitboxNode *newNode = (HitboxNode*)malloc(sizeof(HitboxNode));
    if (newNode == NULL) return;

    Rectangle hitboxRect = move.hitbox;

    if (attacker->isFlipped) {
        hitboxRect.x = -hitboxRect.x - hitboxRect.width; 
    }

    newNode->size.x = attacker->position.x + hitboxRect.x;
    newNode->size.y = attacker->position.y + hitboxRect.y;
    newNode->size.width = hitboxRect.width;
    newNode->size.height = hitboxRect.height;
    newNode->damage = move.damage;
    newNode->knockback = move.knockback;
    newNode->lifetime = move.activeFrames;
    newNode->isPlayer1 = (attacker == player1); 
    newNode->next = activeHitboxes;

    activeHitboxes = newNode;
}

static void UpdateHuman(Player *player, float dt) {
    if (player->state == PLAYER_STATE_ATTACK) {
        player->attackFrameCounter++;

        Move *move = (player->currentMove != NULL) ? player->currentMove : &player->moves->sideLight;
        
        int totalAttackFrames = move->startupFrames +
                                move->activeFrames +
                                move->recoveryFrames;
                                
        if (player->attackFrameCounter > totalAttackFrames) {
            player->state = PLAYER_STATE_IDLE;
            player->currentMove = NULL;
        }
        
        return; 
    }

    if (!player->isGrounded) {
        player->velocity.y += 0.5f;
        player->state = PLAYER_STATE_FALL;
    }

    player->position = Vector2Add(player->position, player->velocity);

    int screenWidth = GAME_WIDTH;   
    int screenHeight = GAME_HEIGHT; 

    if (player->position.x - PLAYER_HALF_WIDTH < 0) {
        player->position.x = PLAYER_HALF_WIDTH;
        player->velocity.x = 0; 
    }

    if (player->position.x + PLAYER_HALF_WIDTH > screenWidth) {
        player->position.x = screenWidth - PLAYER_HALF_WIDTH;
        player->velocity.x = 0;
    }

    if (player->position.y > 540) {
        player->position.y = 540;
        player->velocity.y = 0;
        player->isGrounded = true;
        if (player->state == PLAYER_STATE_FALL) player->state = PLAYER_STATE_IDLE;
    }

    player->velocity.x = 0;
    if (IsKeyDown(KEY_A)) { player->velocity.x = -5.0f; player->isFlipped = true; }
    if (IsKeyDown(KEY_D)) { player->velocity.x = 5.0f; player->isFlipped = false; }
    
    if (player->velocity.x != 0 && player->isGrounded) player->state = PLAYER_STATE_WALK;
    else if (player->isGrounded) player->state = PLAYER_STATE_IDLE;
    
    if (IsKeyPressed(KEY_SPACE) && player->isGrounded) {
        player->velocity.y = -12.0f;
        player->isGrounded = false;
        player->state = PLAYER_STATE_JUMP;
    }

    if (IsKeyPressed(KEY_J) && IsKeyDown(KEY_A)) {
        player->isFlipped = true;
        player->state = PLAYER_STATE_ATTACK;
        player->attackFrameCounter = 0;

        if (player->isGrounded) {
            SpawnHitbox(player, player->moves->sideLight);
            player->currentMove = &player->moves->sideLight;
        } else {
            SpawnHitbox(player, player->moves->airSideLight);
            player->currentMove = &player->moves->airSideLight;
        }
        
    }
    else if (IsKeyPressed(KEY_J) && IsKeyDown(KEY_D)) {
        player->isFlipped = false;
        player->state = PLAYER_STATE_ATTACK;
        player->attackFrameCounter = 0;

        if (player->isGrounded) {
            SpawnHitbox(player, player->moves->sideLight);
            player->currentMove = &player->moves->sideLight;
        } else {
            SpawnHitbox(player, player->moves->airSideLight);
            player->currentMove = &player->moves->airSideLight;
        }
    }

    else if (IsKeyPressed(KEY_J) && IsKeyDown(KEY_W)) {
        player->state = PLAYER_STATE_ATTACK;
        player->attackFrameCounter = 0;
        if (player->isGrounded) {
            SpawnHitbox(player, player->moves->upLight);
            player->currentMove = &player->moves->upLight;
        } else {
            SpawnHitbox(player, player->moves->airUpLight);
            player->currentMove = &player->moves->airUpLight;
        }
    }

    else if (IsKeyPressed(KEY_J) && IsKeyDown(KEY_S)) {
        player->state = PLAYER_STATE_ATTACK;
        player->attackFrameCounter = 0;
        if (player->isGrounded) {
            SpawnHitbox(player, player->moves->downLight);
            player->currentMove = &player->moves->downLight;
        } else {
            SpawnHitbox(player, player->moves->airDownLight);
            player->currentMove = &player->moves->airDownLight;
        }
    }

    else if (IsKeyPressed(KEY_J)) {
        player->state = PLAYER_STATE_ATTACK;
        player->attackFrameCounter = 0;
        if (player->isGrounded) {
            SpawnHitbox(player, player->moves->sideLight);
            player->currentMove = &player->moves->sideLight;
        } else {
            SpawnHitbox(player, player->moves->airSideLight);
            player->currentMove = &player->moves->airSideLight;
        }
    }

    if (IsKeyPressed(KEY_C)) player1->currentHealth -= 10;
    if (IsKeyPressed(KEY_V)) player1->currentUlt++;
    if (IsKeyPressed(KEY_B)) player1->roundsWon++;
}

static void UpdateHitboxes(void) {
    HitboxNode *current = activeHitboxes;
    HitboxNode *prev = NULL;

    while (current != NULL) {
        current->lifetime--;

        if (current->lifetime <= 0) {
            if (prev == NULL) activeHitboxes = current->next;
            else prev->next = current->next;

            HitboxNode *toFree = current;
            current = current->next;
            free(toFree);
        } else {
            prev = current;
            current = current->next;
        }
    }
}

static void UpdateAI(Player *ai, Player *target, float dt) {
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
                            SpawnHitbox(ai, ai->moves->airUpLight);
                            ai->currentMove = &ai->moves->airUpLight;
                        }
                        ai->aiState = AI_STATE_THINKING;
                    }
                    
                    else if (decision < 20 && ai->isGrounded) {
                        ai->velocity.y = -12.0f;
                        ai->isGrounded = false;
                        ai->state = PLAYER_STATE_JUMP;
                        if (distanceX > 0) ai->velocity.x = 4.0f;
                        else ai->velocity.x = -4.0f;
                        ai->aiState = AI_STATE_APPROACH;
                    }

                    else if (fabs(distanceX) < 100) { 
                        if (decision < 70) ai->aiState = AI_STATE_ATTACK;
                        else ai->aiState = AI_STATE_APPROACH;
                    } 
                    
                    else { 
                        if (decision < 80) ai->aiState = AI_STATE_APPROACH;
                        else ai->aiState = AI_STATE_THINKING;
                    }
                }
                break;

            case AI_STATE_APPROACH:
                if (distanceX > 0) {
                    ai->velocity.x = 5.0f; 
                    ai->isFlipped = false;
                } else {
                    ai->velocity.x = -5.0f;
                    ai->isFlipped = true;
                }

                if (!ai->isGrounded && distanceY > 0 && fabs(distanceX) < 60) {
                    ai->state = PLAYER_STATE_ATTACK;
                    ai->attackFrameCounter = 0;
                    SpawnHitbox(ai, ai->moves->airDownLight);
                    ai->currentMove = &ai->moves->airDownLight;
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

                    if (attackType == 0) {
                        SpawnHitbox(ai, ai->moves->sideLight);
                        ai->currentMove = &ai->moves->sideLight;
                    } else if (attackType == 1) {
                        SpawnHitbox(ai, ai->moves->upLight);
                        ai->currentMove = &ai->moves->upLight;
                    } else {
                        SpawnHitbox(ai, ai->moves->downLight);
                        ai->currentMove = &ai->moves->downLight;
                    }
                    
                    ai->aiState = AI_STATE_THINKING;
                    ai->aiTimer = 0;
                }
                break;
            
            case AI_STATE_FLEE:
                ai->aiTimer++;
                
                if (distanceX > 0) {
                    ai->velocity.x = -5.0f;
                    ai->isFlipped = true;
                } else {
                    ai->velocity.x = 5.0f;
                    ai->isFlipped = false;
                }
                
                if (ai->aiTimer > 18) {
                    ai->aiState = AI_STATE_THINKING;
                    ai->aiTimer = 0;
                }
                break;
        }
    }

    if (ai->state == PLAYER_STATE_ATTACK) {
        ai->attackFrameCounter++;
        Move *move = (ai->currentMove != NULL) ? ai->currentMove : &ai->moves->sideLight;
        int totalAttackFrames = move->startupFrames + move->activeFrames + move->recoveryFrames;
                                
        if (ai->attackFrameCounter > totalAttackFrames) {
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

    int screenWidth = GAME_WIDTH;
    int screenHeight = GAME_HEIGHT;

    if (ai->position.x - PLAYER_HALF_WIDTH < 0) {
        ai->position.x = PLAYER_HALF_WIDTH;
        ai->velocity.x = 0;
    }

    if (ai->position.x + PLAYER_HALF_WIDTH > screenWidth) {
        ai->position.x = screenWidth - PLAYER_HALF_WIDTH;
        ai->velocity.x = 0;
    }

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

    player2->position = (Vector2){ 800, 540 };
    player2->velocity = (Vector2){ 0, 0 };
    player2->state = PLAYER_STATE_IDLE;
    player2->aiState = AI_STATE_THINKING;
    player2->currentHealth = player2->maxHealth;
    player2->isGrounded = false;
    player2->currentMove = NULL;
    player2->isFlipped = true;

    HitboxNode *current = activeHitboxes;
    while (current != NULL) {
        HitboxNode *toFree = current;
        current = current->next;
        free(toFree);
    }
    activeHitboxes = NULL;

    countdownTimer = 0;
    sceneState = SCENE_STATE_START;
}

static void CheckCollisions(void) {
    HitboxNode *current = activeHitboxes;
    HitboxNode *prev = NULL;

    Rectangle p1Body = { player1->position.x - 20, player1->position.y, 40, 60 };
    Rectangle p2Body = { player2->position.x - 20, player2->position.y, 40, 60 };

    while (current != NULL) {
        bool hitHappened = false;

        if (current->isPlayer1) {
            if (CheckCollisionRecs(current->size, p2Body)) {
                player2->currentHealth -= current->damage;
                if (player1->position.x < player2->position.x) 
                    player2->velocity.x += current->knockback.x;
                else 
                    player2->velocity.x -= current->knockback.x;
                
                player2->velocity.y -= current->knockback.y;
                
                player2->state = PLAYER_STATE_FALL;
                hitHappened = true;
            }
        } 

        else {
            if (CheckCollisionRecs(current->size, p1Body)) {
                player1->currentHealth -= current->damage;
                
                if (player2->position.x < player1->position.x) 
                    player1->velocity.x += current->knockback.x;
                else 
                    player1->velocity.x -= current->knockback.x;

                player1->velocity.y -= current->knockback.y;

                player1->state = PLAYER_STATE_FALL;
                hitHappened = true;
            }
        }

        if (hitHappened) {
            HitboxNode *toFree = current;
            if (prev == NULL) activeHitboxes = current->next;
            else prev->next = current->next;
            
            current = current->next;
            free(toFree);
        } else {
            prev = current;
            current = current->next;
        }
    }
}

void GameScene_Init(void) {
    sceneState = SCENE_STATE_START;
    matchWinner = 0;
    countdownTimer = 0;

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
    player1->roundsWon = 0;
    
    player1->moves = LoadMoveSetFromFile("bacteriophage.moves");

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
    player2->roundsWon = 0;

    player2->moves = LoadMoveSetFromFile("bacteriophage.moves");

    player2->isCPU = true;
    player2->aiState = AI_STATE_THINKING;
    player2->aiTimer = 0;

    activeHitboxes = NULL;

    texGuiFrame = LoadTexture("assets/gui_frame.png");
    
    texSyringeEmptyL = LoadTexture("assets/lsyringe_empty.png");
    texSyringeFullL  = LoadTexture("assets/lsyringe_full.png");
    texSyringeEmptyR = LoadTexture("assets/rsyringe_empty.png");
    texSyringeFullR  = LoadTexture("assets/rsyringe_full.png");
    
    texPillEmptyL = LoadTexture("assets/lpill_empty.png");
    texPillFullL  = LoadTexture("assets/lpill_full.png");
    texPillEmptyR = LoadTexture("assets/rpill_empty.png");
    texPillFullR  = LoadTexture("assets/rpill_full.png");
    
    texTabletActive   = LoadTexture("assets/tablet_active.png");
    texTabletInactive = LoadTexture("assets/tablet_inactive.png");
}

int GameScene_Update(void) {
    float dt = GetFrameTime();

    switch (sceneState) {
        case SCENE_STATE_START:
            countdownTimer++;
            if (countdownTimer > 180) {
                sceneState = SCENE_STATE_PLAY;
            }
            break;
            
        case SCENE_STATE_PLAY:
            UpdateHuman(player1, dt);
            UpdateAI(player2, player1, dt);
            UpdateHitboxes();
            CheckCollisions();

            if (player1->currentHealth <= 0 || player2->currentHealth <= 0) {
                
                if (player1->currentHealth <= 0) player2->roundsWon++;
                else if (player2->currentHealth <= 0) player1->roundsWon++;

                sceneState = SCENE_STATE_ROUND_END;
                countdownTimer = 0;
            }
            break;

        case SCENE_STATE_ROUND_END:
            countdownTimer++;
            if (countdownTimer > 120) {
                if (player1->roundsWon >= 3) {
                    matchWinner = 1;
                    sceneState = SCENE_STATE_GAME_OVER;
                } 
                else if (player2->roundsWon >= 3) {
                    matchWinner = 2;
                    sceneState = SCENE_STATE_GAME_OVER;
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

    if (player1->state != PLAYER_STATE_ATTACK) {
        DrawRectangleLinesEx(
            (Rectangle){ player1->position.x - 20, player1->position.y, 40, 60},
            2.0f,
            WHITE
        );
    } else {
        DrawRectangleLinesEx(
            (Rectangle){ player1->position.x - 20, player1->position.y, 40, 60},
            2.0f,
            RED
        );
    }

    if (player2->state != PLAYER_STATE_ATTACK) {
        DrawRectangleLinesEx(
            (Rectangle){ player2->position.x - 20, player2->position.y, 40, 60},
            2.0f,
            BLUE
        );
    } else {
        DrawRectangleLinesEx(
            (Rectangle){ player2->position.x - 20, player2->position.y, 40, 60},
            2.0f,
            ORANGE
        );
    }

    HitboxNode *current = activeHitboxes;
    
    while (current != NULL) {
        DrawRectangleLinesEx(current->size, 2.0f, RED);
        current = current->next;
    }

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
    Rectangle sourceRecL = { 0, 0, texSyringeFullL.width * healthPct1, (float)texSyringeFullL.height };
    Rectangle destRecL   = { offSyringeXL, offSyringeY, (texSyringeFullL.width * uiScale) * healthPct1, texSyringeFullL.height * uiScale };
    DrawTexturePro(texSyringeFullL, sourceRecL, destRecL, (Vector2){0,0}, 0.0f, WHITE);

    DrawTextureEx(texSyringeEmptyR, (Vector2){offSyringeXR, offSyringeY}, 0.0f, uiScale, WHITE);
    float healthPct2 = player2->currentHealth / player2->maxHealth;
    
    Rectangle sourceRecR = { 
        texSyringeFullR.width * (1.0f - healthPct2),
        0, 
        texSyringeFullR.width * healthPct2,
        (float)texSyringeFullR.height 
    };
    Rectangle destRecR = { 
        offSyringeXR + (texSyringeFullR.width * uiScale * (1.0f - healthPct2)),
        offSyringeY, 
        (texSyringeFullR.width * uiScale) * healthPct2, 
        texSyringeFullR.height * uiScale 
    };
    DrawTexturePro(texSyringeFullR, sourceRecR, destRecR, (Vector2){0,0}, 0.0f, WHITE);

    float pillSpacing = 22 * uiScale;
    float offPillY = startY + (71 * uiScale);
    float offPillXL = startX + (55 * uiScale);
    float offPillXR = startX + frameW - (55 * uiScale);

    for (int i = 0; i < 8; i++) {
        Texture2D tex = (i < player1->currentUlt) ? texPillFullL : texPillEmptyL;
        DrawTextureEx(tex, (Vector2){
            offPillXL + i * pillSpacing,
            offPillY
        }, 0, uiScale, WHITE);
    }

    for (int i = 0; i < 8; i++) {
        Texture2D tex = (i < player2->currentUlt) ? texPillFullR : texPillEmptyR;
        DrawTextureEx(tex, (Vector2){
            offPillXR - ((i + 1) * pillSpacing),
            offPillY
        }, 0, uiScale, WHITE);
    }

    float pillTotalWidth = 7 * (22 * uiScale);
    float offTabletY = startY + (68 * uiScale);
    float tabletSpacing = 5 * uiScale;

    float oneTabletW = texTabletActive.width * uiScale;
    float tabletsWidth = (oneTabletW * 3) + (tabletSpacing * 2);

    float centerX = startX + frameW / 2.0f;
    float tabletOffset = 20 * uiScale;

    float offTabletXL = centerX - tabletOffset - tabletsWidth;
    float offTabletXR = centerX + tabletOffset;


    for (int i = 0; i < 3; i++) {
        Texture2D tex = (i < player1->roundsWon) ? texTabletActive : texTabletInactive;
        DrawTextureEx(tex,
            (Vector2){ offTabletXL + i * (oneTabletW + tabletSpacing), offTabletY },
            0, uiScale, WHITE
        );
    }


    for (int i = 0; i < 3; i++) {
        Texture2D tex = (i < player2->roundsWon) ? texTabletActive : texTabletInactive;
        DrawTextureEx(tex,
            (Vector2){ offTabletXR + i * (oneTabletW + tabletSpacing), offTabletY },
            0, uiScale, WHITE
        );
    }

    if (sceneState == SCENE_STATE_START) {
        const char* countdownText = "";
        if (countdownTimer < 60) countdownText = "3";
        else if (countdownTimer < 120) countdownText = "2";
        else if (countdownTimer < 180) countdownText = "1";
        else countdownText = "FIGHT!";
        
        int fontSize = 80;
        int textWidth = MeasureText(countdownText, fontSize);
        DrawText(countdownText, (GAME_WIDTH - textWidth)/2, (GAME_HEIGHT - fontSize)/2, fontSize, YELLOW);
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
    
    HitboxNode *current = activeHitboxes;
    while (current != NULL) {
        HitboxNode *toFree = current;
        current = current->next;
        free(toFree);
    }
    activeHitboxes = NULL;
}