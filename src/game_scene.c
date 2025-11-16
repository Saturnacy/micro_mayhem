#include "game_scene.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    SCENE_STATE_START,
    SCENE_STATE_PLAY
} InternalSceneState;

static Player *player1;
static Player *player2;
static HitboxNode *activeHitboxes = NULL;
static InternalSceneState sceneState;
static int countdownTimer = 0;

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

static void SpawnHitbox(Move move) {
    HitboxNode *newNode = (HitboxNode*)malloc(sizeof(HitboxNode));
    if (newNode == NULL) return;

    Rectangle hitboxRect = move.hitbox;
    if (player1->isFlipped) {
        hitboxRect.x = -hitboxRect.x - hitboxRect.width; 
    }

    newNode->size.x = player1->position.x + hitboxRect.x;
    newNode->size.y = player1->position.y + hitboxRect.y;
    newNode->size.width = hitboxRect.width;
    newNode->size.height = hitboxRect.height;
    newNode->damage = move.damage;
    newNode->knockback = move.knockback;
    newNode->lifetime = move.activeFrames;

    newNode->next = activeHitboxes;
    activeHitboxes = newNode;
}

static void UpdateHuman(Player *player, float dt) {
    if (player->state == PLAYER_STATE_ATTACK) {
        player->attackFrameCounter++;
        
        int totalAttackFrames = player->moves->sideLight.startupFrames +
                                player->moves->sideLight.activeFrames +
                                player->moves->sideLight.recoveryFrames;
                                
        if (player->attackFrameCounter > totalAttackFrames) {
            player->state = PLAYER_STATE_IDLE;
        }
        
        return; 
    }

    if (!player->isGrounded) {
        player->velocity.y += 0.5f;
        player->state = PLAYER_STATE_FALL;
    }

    player->position = Vector2Add(player->position, player->velocity);

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
        if (player->isGrounded) SpawnHitbox(player->moves->sideLight);
        else SpawnHitbox(player->moves->airSideLight);
    }
    else if (IsKeyPressed(KEY_J) && IsKeyDown(KEY_D)) {
        player->isFlipped = false;
        player->state = PLAYER_STATE_ATTACK;
        player->attackFrameCounter = 0;
        if (player->isGrounded) SpawnHitbox(player->moves->sideLight);
        else SpawnHitbox(player->moves->airSideLight);
    }
    else if (IsKeyPressed(KEY_J) && IsKeyDown(KEY_W)) {
        player->state = PLAYER_STATE_ATTACK;
        player->attackFrameCounter = 0;
        if (player->isGrounded) SpawnHitbox(player->moves->upLight);
        else SpawnHitbox(player->moves->airUpLight);
    }
    else if (IsKeyPressed(KEY_J) && IsKeyDown(KEY_S)) {
        player->state = PLAYER_STATE_ATTACK;
        player->attackFrameCounter = 0;
        if (player->isGrounded) SpawnHitbox(player->moves->downLight);
        else SpawnHitbox(player->moves->airDownLight);
    }
    else if (IsKeyPressed(KEY_J)) {
        player->state = PLAYER_STATE_ATTACK;
        player->attackFrameCounter = 0;
        if (player->isGrounded) SpawnHitbox(player->moves->sideLight);
        else SpawnHitbox(player->moves->airSideLight);
    }
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
    if (ai->state == PLAYER_STATE_IDLE || ai->state == PLAYER_STATE_WALK) {
        float distanceX = target->position.x - ai->position.x;

        switch (ai->aiState) {
            case AI_STATE_THINKING:
                ai->velocity.x = 0;
                ai->aiTimer++;
                
                if (ai->aiTimer > 60) {
                    ai->aiTimer = 0;
                    
                    int decision = GetRandomValue(1, 100);
                    
                    if (fabs(distanceX) < 100) {
                        if (decision < 70) ai->aiState = AI_STATE_ATTACK;
                        else ai->aiState = AI_STATE_APPROACH;
                    } else {
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
                
                if (fabs(distanceX) < 80) {
                    ai->aiState = AI_STATE_ATTACK;
                }
                break;

            case AI_STATE_ATTACK:
                int attackType = GetRandomValue(0, 2);
                ai->state = PLAYER_STATE_ATTACK;
                ai->attackFrameCounter = 0;

                if (ai->isGrounded) {
                    if (attackType == 0) SpawnHitbox(ai->moves->sideLight);
                    else if (attackType == 1) SpawnHitbox(ai->moves->upLight);
                    else SpawnHitbox(ai->moves->downLight);
                } else {
                    //por enquanto ela só ataca no chão
                     SpawnHitbox(ai->moves->sideLight);
                }
                
                ai->aiState = AI_STATE_THINKING;
                ai->aiTimer = 0;
                break;
        }
    }

    if (ai->state == PLAYER_STATE_ATTACK) {
        ai->attackFrameCounter++;
        
        int totalAttackFrames = ai->moves->sideLight.startupFrames +
                                ai->moves->sideLight.activeFrames +
                                ai->moves->sideLight.recoveryFrames;
                                
        if (ai->attackFrameCounter > totalAttackFrames) {
            ai->state = PLAYER_STATE_IDLE;
        }
        return;
    }

    if (!ai->isGrounded) {
        ai->velocity.y += 0.5f;
        ai->state = PLAYER_STATE_FALL;
    }

    ai->position = Vector2Add(ai->position, ai->velocity);

    if (ai->position.y > 540) {
        ai->position.y = 540;
        ai->velocity.y = 0;
        ai->isGrounded = true;
        if (ai->state == PLAYER_STATE_FALL) ai->state = PLAYER_STATE_IDLE;
    }
    
    if (ai->velocity.x != 0 && ai->isGrounded) ai->state = PLAYER_STATE_WALK;
    else if (ai->isGrounded && ai->state != PLAYER_STATE_JUMP) ai->state = PLAYER_STATE_IDLE;
}

void GameScene_Init(void) {
    sceneState = SCENE_STATE_START;
    countdownTimer = 0;

    player1 = (Player*)malloc(sizeof(Player));
    player1->position = (Vector2){ 400, 540 };
    player1->velocity = (Vector2){ 0, 0 };
    player1->isGrounded = false;
    player1->isFlipped = false;
    player1->state = PLAYER_STATE_IDLE;
    player1->attackFrameCounter = 0;
    
    player1->moves = LoadMoveSetFromFile("bacteriophage.moves");

    player2 = (Player*)malloc(sizeof(Player));
    player2->position = (Vector2){ 800, 540 };
    player2->velocity = (Vector2){ 0, 0 };
    player2->isGrounded = false;
    player2->isFlipped = true;
    player2->state = PLAYER_STATE_IDLE;
    player2->attackFrameCounter = 0;
    player2->moves = LoadMoveSetFromFile("bacteriophage.moves");
    player2->isCPU = true;
    player2->aiState = AI_STATE_THINKING;
    player2->aiTimer = 0;

    activeHitboxes = NULL;
}

void GameScene_Update(void) {
    float dt = GetFrameTime();
    
    switch (sceneState) {
        case SCENE_STATE_START:
            countdownTimer++;
            if (countdownTimer > 240) {
                sceneState = SCENE_STATE_PLAY;
            }
            break;
            
        case SCENE_STATE_PLAY:
            UpdateHuman(player1, dt);
            UpdateAI(player2, player1, dt);
            UpdateHitboxes();
            break;
    }
}

void GameScene_Draw(void) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), SKYBLUE);
    DrawRectangle(0, 600, GetScreenWidth(), 120, GREEN);

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

    if (sceneState == SCENE_STATE_START) {
        const char* countdownText = "";
        
        if (countdownTimer < 60) countdownText = "3";
        else if (countdownTimer < 120) countdownText = "2";
        else if (countdownTimer < 180) countdownText = "1";
        else countdownText = "FIGHT!";
        
        int fontSize = 80;
        int textWidth = MeasureText(countdownText, fontSize);
        
        DrawText(
            countdownText,
            (GetScreenWidth() - textWidth) / 2,
            (GetScreenHeight() - fontSize) / 2,
            fontSize,
            YELLOW
        );
    }
}

void GameScene_Unload(void) {
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