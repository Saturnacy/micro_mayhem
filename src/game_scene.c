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

typedef struct VfxNode {
    Vector2 position;
    float rotation;
    int frameWidth;
    int frameHeight;
    int currentFrame;
    int totalFrames;
    float animTimer;
    float animSpeed;
    Texture2D texture;
    float scale;
    struct VfxNode *next;
} VfxNode;

static Font hudFont;
static Font mainFont;
static Texture2D texRocketVfx;
static VfxNode *activeVfx = NULL;
static bool isMultiplayerMode = false;
static Player *player1;
static Player *player2;
static InternalSceneState sceneState;
static int countdownTimer = 0;
static int matchWinner = 0;
static int fightBannerTimer = 0;
static int currentLanguage = 0;
static int pauseOption = 0;
const char* pauseOptionsText[] = { "RESUME", "SETTINGS", "QUIT MATCH" };
static Texture2D texBackground;
static Texture2D texPoisonCloud;
static Texture2D texExplosion;

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
        default: return "assets/data/bacteriophage.json";
    }
}

static const char* GetCharacterName(int charID) {
    switch(charID) {
        case 0: return "BACTERIOPHAGE";
        case 1: return "AMOEBA";
        default: return "UNKNOWN";
    }
}

static void SpawnVfx(Vector2 pos, float rotation, Texture2D tex, int frames, float speed, float scale) {
    VfxNode *newNode = (VfxNode*)malloc(sizeof(VfxNode));
    if (!newNode) return;

    newNode->position = pos;
    newNode->rotation = rotation;
    newNode->texture = tex;
    newNode->totalFrames = frames; 
    newNode->frameWidth = tex.width / frames;
    newNode->frameHeight = tex.height;
    newNode->currentFrame = 0;
    newNode->animTimer = 0.0f;
    newNode->animSpeed = speed; 
    newNode->scale = scale;

    newNode->next = activeVfx;
    activeVfx = newNode;
}

static void UpdateVfx(float dt) {
    VfxNode *current = activeVfx;
    VfxNode *prev = NULL;

    while (current != NULL) {
        current->animTimer += dt;
        if (current->animTimer >= current->animSpeed) {
            current->animTimer = 0.0f;
            current->currentFrame++;
        }

        if (current->currentFrame >= current->totalFrames) {
            VfxNode *toFree = current;
            if (prev) prev->next = current->next;
            else activeVfx = current->next;
            current = current->next;
            free(toFree);
        } else {
            prev = current;
            current = current->next;
        }
    }
}

static void DrawVfx(void) {
    for (VfxNode *vfx = activeVfx; vfx != NULL; vfx = vfx->next) {
        if (vfx->texture.id == 0) continue;

        int frameX = vfx->currentFrame; 

        Rectangle sourceRec = {
            (float)frameX * vfx->frameWidth,
            0.0f,
            (float)vfx->frameWidth,
            (float)vfx->frameHeight
        };

        Rectangle destRec = {
            vfx->position.x,
            vfx->position.y,
            vfx->frameWidth * vfx->scale,
            vfx->frameHeight * vfx->scale
        };

        Vector2 origin = { destRec.width / 2.0f, destRec.height / 2.0f };

        DrawTexturePro(vfx->texture, sourceRec, destRec, origin, vfx->rotation, WHITE);
    }
}

static void Vfx_Cleanup(void) {
    while (activeVfx) {
        VfxNode *n = activeVfx;
        activeVfx = n->next;
        free(n);
    }
}

static void UpdateHuman(Player *player, float dt, InputConfig input) {
    Combat_ApplyStatus(player, dt);
    bool isP1 = (player == player1);

    if (player->state == PLAYER_STATE_HURT) {
        player->velocity.x *= 0.90f;
        
        if (player->attackFrameCounter > 0) {
            player->attackFrameCounter--;
        } else {
            player->state = PLAYER_STATE_FALL;
        }
        
        if (fabs(player->velocity.x) < 0.1f) player->velocity.x = 0;
    }

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

        if (move == &player->moves->specialSide || 
            move == &player->moves->specialUp || 
            move == &player->moves->ultimate) {
            
            player->vfxSpawnTimer += dt;
            if (player->vfxSpawnTimer >= 0.4f) {
                player->vfxSpawnTimer = 0.0f;
                Vector2 spawnPos = player->position;
                float rotation = 0.0f;

                if (move == &player->moves->specialSide) {
                    spawnPos.x += player->isFlipped ? 40 : -40;
                    spawnPos.y -= 30; 
                    rotation = player->isFlipped ? -90.0f : 90.0f;
                } else if (move == &player->moves->specialUp) {
                    spawnPos.y += 20; 
                } else if (move == &player->moves->ultimate) {
                    if (player->velocity.y > 0) { spawnPos.y -= 70; rotation = 180.0f; }
                    else spawnPos.y += 20;
                }
                SpawnVfx(spawnPos, rotation, texRocketVfx, 11, 0.05f, 2.5f); 
            }
        } else {
             player->vfxSpawnTimer = 0.4f;
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

        if (player->position.y < GROUND_LEVEL) {
            player->isGrounded = false;
        }

        if (player->position.x - PLAYER_HALF_WIDTH < 0) {
            player->position.x = PLAYER_HALF_WIDTH;
        }
        if (player->position.x + PLAYER_HALF_WIDTH > GAME_WIDTH) {
            player->position.x = GAME_WIDTH - PLAYER_HALF_WIDTH;
        }
        
        if (player->position.y > GROUND_LEVEL) { 
             if (move->selfVelocity.y > 0 || move->fallSpeed > 0) {
                 
                 if (move->type == MOVE_TYPE_ULTIMATE_FALL) {
                     Move explosion = {0};
                     explosion.type = MOVE_TYPE_ULTIMATE; 
                     explosion.hitbox = (Rectangle){ -400, -500, 800, 600 }; 
                     explosion.damage = 40.0f;
                     explosion.knockback = (Vector2){ 25.0f, -25.0f };
                     explosion.activeFrames = 10;
                     Combat_TryExecuteMove(player, &explosion, isP1);

                     Vector2 explosionPos = { player->position.x, GROUND_LEVEL - 30 };
                     SpawnVfx(explosionPos, 0.0f, texExplosion, 4, 0.08f, 12.0f);
                 }

                 player->position.y = GROUND_LEVEL;
                 player->state = PLAYER_STATE_IDLE;
                 player->currentMove = NULL;
                 player->velocity = (Vector2){0,0};
                 player->hasUsedAirSpecial = false;
                 return;
             }
        }

        int totalAttackFrames = move->startupFrames + move->activeFrames + move->recoveryFrames;     
        if (player->attackFrameCounter > totalAttackFrames) {
            if (player->isGrounded) {
                player->state = PLAYER_STATE_IDLE;
                player->velocity = (Vector2){0,0};
            } else {
                player->state = PLAYER_STATE_FALL;
                player->velocity.x *= 0.5f; 
            }
            player->currentMove = NULL;
        }
        return;
    }
    
    player->vfxSpawnTimer = 0.4f;

    if (!player->isGrounded) {
        player->velocity.y += 0.5f;
        
        if (player->state != PLAYER_STATE_HURT) {
            player->state = PLAYER_STATE_FALL;
        }
    }

    player->position = Vector2Add(player->position, player->velocity);

    if (player->position.x - PLAYER_HALF_WIDTH < 0) {
        player->position.x = PLAYER_HALF_WIDTH;
        player->velocity.x = 0; 
    }
    if (player->position.x + PLAYER_HALF_WIDTH > GAME_WIDTH) {
        player->position.x = GAME_WIDTH - PLAYER_HALF_WIDTH;
        player->velocity.x = 0;
    }
    
    if (player->position.y > GROUND_LEVEL) {
        player->position.y = GROUND_LEVEL;
        player->velocity.y = 0;
        player->isGrounded = true;
        player->hasUsedAirSpecial = false;
        
        if (player->state == PLAYER_STATE_FALL || player->state == PLAYER_STATE_HURT) {
            player->state = PLAYER_STATE_IDLE;
            player->velocity.x = 0;
        }
    }

    if (player->state != PLAYER_STATE_HURT) {
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
                if (isSpecial) {
                    if (!player->hasUsedAirSpecial || player->isGrounded) {
                        selectedMove = &player->moves->specialUp;
                    }
                } 
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

                if (selectedMove == &player->moves->specialUp && !player->isGrounded) {
                    player->hasUsedAirSpecial = true;
                }

                if (selectedMove == &player->moves->ultimate) {
                    player->ultCharge = 0.0f;
                    player->currentUlt = 0;
                }
            } else {
                player->state = PLAYER_STATE_IDLE;
            }
        }
    }
}

static void UpdateAI(Player *ai, Player *target, float dt) {
    Combat_ApplyStatus(ai, dt);

    if (ai->state == PLAYER_STATE_HURT) {
        ai->velocity.x *= 0.90f; 
        
        if (ai->attackFrameCounter > 0) {
            ai->attackFrameCounter--;
        } else {
            ai->state = PLAYER_STATE_FALL;
        }

        if (fabs(ai->velocity.x) < 0.1f) ai->velocity.x = 0;
    }

    float distanceX = target->position.x - ai->position.x;
    float distanceY = target->position.y - ai->position.y;
    
    bool nearLeftWall = (ai->position.x < 150.0f);
    bool nearRightWall = (ai->position.x > GAME_WIDTH - 150.0f);
    bool isCornered = nearLeftWall || nearRightWall;

    if (target->state == PLAYER_STATE_ATTACK && ai->state != PLAYER_STATE_ATTACK) {
        if (fabs(distanceX) < 150.0f) {
            if (isCornered) {
                ai->aiState = AI_STATE_ATTACK;
                ai->aiTimer = 0;
            } else {
                ai->aiState = AI_STATE_FLEE;
                ai->aiTimer = 0;
            }
        }
    }

    if (ai->state != PLAYER_STATE_ATTACK && ai->state != PLAYER_STATE_HURT) { 
        ai->vfxSpawnTimer = 0.4f;

        switch (ai->aiState) {
            case AI_STATE_THINKING:
                ai->velocity.x = 0;
                ai->aiTimer++;
                if (ai->aiTimer > 30) { 
                    ai->aiTimer = 0;
                    int decision = GetRandomValue(1, 100);
                    
                    if (isCornered && fabs(distanceX) < 200.0f) {
                        if (ai->isGrounded) {
                            ai->velocity.y = -12.0f;
                            ai->isGrounded = false;
                            ai->state = PLAYER_STATE_JUMP;
                            ai->velocity.x = nearLeftWall ? 5.0f : -5.0f;
                        }
                        ai->aiState = AI_STATE_APPROACH;
                    }
                    else if (distanceY < -50 && fabs(distanceX) < 100) {
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
                    else if (fabs(distanceX) < 120) { 
                        if (decision < 80) ai->aiState = AI_STATE_ATTACK; else ai->aiState = AI_STATE_APPROACH;
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
                else if (fabs(distanceX) < 90 && ai->isGrounded) {
                    ai->aiState = AI_STATE_ATTACK;
                }
                break;

            case AI_STATE_ATTACK:
                {
                    ai->state = PLAYER_STATE_ATTACK;
                    ai->attackFrameCounter = 0;
                    Move *move = NULL;
                    int randAttack = GetRandomValue(0, 100);

                    if (ai->currentUlt >= ai->maxUlt && randAttack < 20) {
                        move = &ai->moves->ultimate;
                        ai->ultLaunchPos = ai->position;
                        ai->currentUlt = 0;
                        ai->ultCharge = 0.0f;
                    }
                    else if (randAttack < 50) {
                        int specialType = GetRandomValue(0, 3);
                        if (specialType == 0) move = &ai->moves->specialNeutral;
                        else if (specialType == 1) move = &ai->moves->specialSide;
                        else if (specialType == 2) {
                            if (!ai->hasUsedAirSpecial || ai->isGrounded) {
                                move = &ai->moves->specialUp;
                            } else {
                                move = &ai->moves->airUp;
                            }
                        }
                        else move = &ai->moves->specialDown;
                    }
                    else {
                        int basicType = GetRandomValue(0, 2);
                        if (basicType == 0) move = &ai->moves->sideGround;
                        else if (basicType == 1) move = &ai->moves->upGround;
                        else move = &ai->moves->downGround;
                    }
                    
                    if (move != NULL && GetTime() - move->lastUsedTime < move->cooldown) {
                        move = &ai->moves->sideGround;
                    }

                    if (move != NULL) {
                        move->lastUsedTime = GetTime();
                        Combat_TryExecuteMove(ai, move, false);
                        ai->currentMove = move;
                        
                        if (move == &ai->moves->specialUp && !ai->isGrounded) {
                            ai->hasUsedAirSpecial = true;
                        }
                    }
                    
                    ai->aiState = AI_STATE_THINKING;
                    ai->aiTimer = 0;
                }
                break;
            
            case AI_STATE_FLEE:
                if (isCornered) {
                    ai->aiState = AI_STATE_ATTACK;
                } else {
                    ai->aiTimer++;
                    if (distanceX > 0) { ai->velocity.x = -5.0f; ai->isFlipped = true; } 
                    else { ai->velocity.x = 5.0f; ai->isFlipped = false; }
                    
                    if (ai->aiTimer > 18) {
                        ai->aiState = AI_STATE_THINKING;
                        ai->aiTimer = 0;
                    }
                }
                break;
        }
    }

    if (ai->state == PLAYER_STATE_ATTACK) {
        ai->attackFrameCounter++;
        Move *move = (ai->currentMove != NULL) ? ai->currentMove : &ai->moves->sideGround;
        
        if (move->selfVelocity.x != 0 || move->selfVelocity.y != 0) {
            float dir = ai->isFlipped ? -1.0f : 1.0f;
            ai->velocity.x = move->selfVelocity.x * dir;
            ai->velocity.y = move->selfVelocity.y;
        }

        if (move == &ai->moves->specialSide || 
            move == &ai->moves->specialUp || 
            move == &ai->moves->ultimate) {
            
            ai->vfxSpawnTimer += dt;
            if (ai->vfxSpawnTimer >= 0.4f) {
                ai->vfxSpawnTimer = 0.0f;
                Vector2 spawnPos = ai->position;
                float rotation = 0.0f;
                if (move == &ai->moves->specialSide) {
                    spawnPos.x += ai->isFlipped ? 40 : -40;
                    spawnPos.y -= 30;
                    rotation = ai->isFlipped ? -90.0f : 90.0f;
                } else if (move == &ai->moves->specialUp) {
                    spawnPos.y += 20; 
                } else if (move == &ai->moves->ultimate) {
                    if (ai->velocity.y > 0) { spawnPos.y -= 70; rotation = 180.0f; }
                    else spawnPos.y += 20;
                }
                SpawnVfx(spawnPos, rotation, texRocketVfx, 11, 0.05f, 2.5f);
            }
        } else {
             ai->vfxSpawnTimer = 0.4f;
        }

        if (move->type == MOVE_TYPE_ULTIMATE_FALL) {
            int peakFrame = 40;
            int hangTime = 60;

            if (ai->attackFrameCounter == peakFrame) {
                Move cloud = {0};
                cloud.type = MOVE_TYPE_TRAP;
                cloud.hitbox = (Rectangle){ -300, -300, 600, 600 }; 
                cloud.damage = 2.0f;
                cloud.effect = EFFECT_POISON;
                cloud.effectDuration = 5.0f;
                cloud.trapDuration = 5.0f;

                Vector2 currentPos = ai->position;
                ai->position = ai->ultLaunchPos;
                Combat_TryExecuteMove(ai, &cloud, false);
                ai->position = currentPos;
            }

            if (ai->attackFrameCounter >= peakFrame && ai->attackFrameCounter < (peakFrame + hangTime)) {
                ai->velocity.y = 0; 
                ai->velocity.x = 0;
            }
            else if (ai->attackFrameCounter >= (peakFrame + hangTime)) {
                ai->velocity.y = move->fallSpeed;
            }
        }

        ai->position = Vector2Add(ai->position, ai->velocity);

        if (ai->position.y < GROUND_LEVEL) {
            ai->isGrounded = false;
        }

        if (ai->position.x - PLAYER_HALF_WIDTH < 0) ai->position.x = PLAYER_HALF_WIDTH;
        if (ai->position.x + PLAYER_HALF_WIDTH > GAME_WIDTH) ai->position.x = GAME_WIDTH - PLAYER_HALF_WIDTH;

        if (ai->position.y > GROUND_LEVEL) {
            if (move->selfVelocity.y > 0 || move->fallSpeed > 0) {
                 
                 if (move->type == MOVE_TYPE_ULTIMATE_FALL) {
                     Move explosion = {0};
                     explosion.type = MOVE_TYPE_ULTIMATE; 
                     explosion.hitbox = (Rectangle){ -400, -500, 800, 600 }; 
                     explosion.damage = 40.0f;
                     explosion.knockback = (Vector2){ 25.0f, -25.0f };
                     explosion.activeFrames = 10;
                     Combat_TryExecuteMove(ai, &explosion, false);

                     Vector2 explosionPos = { ai->position.x, GROUND_LEVEL - 30 };
                     SpawnVfx(explosionPos, 0.0f, texExplosion, 4, 0.08f, 12.0f);
                 }

                 ai->position.y = GROUND_LEVEL;
                 ai->state = PLAYER_STATE_IDLE;
                 ai->currentMove = NULL;
                 ai->velocity = (Vector2){0,0};
                 ai->hasUsedAirSpecial = false;
                 return;
             }
        }

        int totalFrames = move->startupFrames + move->activeFrames + move->recoveryFrames;
        if (ai->attackFrameCounter > totalFrames) {
            if (ai->isGrounded) {
                ai->state = PLAYER_STATE_IDLE;
                ai->velocity = (Vector2){0,0};
            } else {
                ai->state = PLAYER_STATE_FALL;
                ai->velocity.x *= 0.5f;
            }
            ai->currentMove = NULL;
        }
        return;
    }

    if (!ai->isGrounded) {
        ai->velocity.y += 0.5f;
        if (ai->state != PLAYER_STATE_HURT) {
            ai->state = PLAYER_STATE_FALL;
        }
    }
    ai->position = Vector2Add(ai->position, ai->velocity);

    if (ai->position.x - PLAYER_HALF_WIDTH < 0) { ai->position.x = PLAYER_HALF_WIDTH; ai->velocity.x = 0; }
    if (ai->position.x + PLAYER_HALF_WIDTH > GAME_WIDTH) { ai->position.x = GAME_WIDTH - PLAYER_HALF_WIDTH; ai->velocity.x = 0; }
    
    if (ai->position.y > GROUND_LEVEL) {
        ai->position.y = GROUND_LEVEL;
        ai->velocity.y = 0;
        ai->isGrounded = true;
        ai->hasUsedAirSpecial = false;
        if (ai->state == PLAYER_STATE_FALL || ai->state == PLAYER_STATE_HURT) {
            ai->state = PLAYER_STATE_IDLE;
            ai->velocity.x = 0;
        }
    }
    
    if (ai->state != PLAYER_STATE_HURT) {
        if (ai->velocity.x != 0 && ai->isGrounded) ai->state = PLAYER_STATE_WALK;
        else if (ai->isGrounded && ai->state != PLAYER_STATE_JUMP) ai->state = PLAYER_STATE_IDLE;
    }
}

static void ResetRound(void) {
    player1->position = (Vector2){ 400, GROUND_LEVEL };
    player1->velocity = (Vector2){ 0, 0 };
    player1->state = PLAYER_STATE_IDLE;
    player1->currentHealth = player1->maxHealth;
    player1->isGrounded = false;
    player1->currentMove = NULL;
    player1->isFlipped = false;
    player1->poisonTimer = 0;
    player1->hasUsedAirSpecial = false;

    player2->position = (Vector2){ 800, GROUND_LEVEL };
    player2->velocity = (Vector2){ 0, 0 };
    player2->state = PLAYER_STATE_IDLE;
    player2->currentHealth = player2->maxHealth;
    player2->isGrounded = false;
    player2->currentMove = NULL;
    player2->isFlipped = true;
    player2->poisonTimer = 0;
    player2->hasUsedAirSpecial = false;

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

void GameScene_SetMainFont(Font font) {
    mainFont = font;
}

void DrawPlayerSprite(Player *p, Color tint) {
    if (p->spriteSheet.id == 0) return;

    int columns = 5;
    int frameX = p->currentAnimIndex % columns;
    int frameY = p->currentAnimIndex / columns;

    Rectangle sourceRec = {
        (float)frameX * p->frameWidth,
        (float)frameY * p->frameHeight,
        (float)p->frameWidth,
        (float)p->frameHeight
    };

    if (p->isFlipped) sourceRec.width = -sourceRec.width;

    float scale = 3.0f; 

    Rectangle destRec = {
        p->position.x, 
        p->position.y, 
        p->frameWidth * scale,
        p->frameHeight * scale 
    };
    
    float feetOffset = 38.0f; 

    Vector2 origin = { 
        destRec.width / 2.0f, 
        destRec.height - feetOffset 
    }; 
    
    float rotation = 0.0f;

    if (p->state == PLAYER_STATE_ATTACK && p->currentAnimIndex == 19) {
        origin.y = destRec.height / 2.0f; 
        
        destRec.y -= (destRec.height / 2.0f) - feetOffset; 

        if (p->currentMove == &p->moves->specialSide) {
            rotation = p->isFlipped ? -90.0f : 90.0f;
        }
        else if (p->currentMove == &p->moves->specialUp) {
            rotation = 0.0f;
        }
        else if (p->currentMove == &p->moves->ultimate) {
            if (p->velocity.y > 0) {
                 rotation = 180.0f;
            } else {
                 rotation = 0.0f;
            }
        }
    }

    DrawTexturePro(p->spriteSheet, sourceRec, destRec, origin, rotation, tint);
}

void UpdatePlayerAnimation(Player *p, float dt) {
    int start = 0;
    int len = 1;
    float speed = 0.12f;
    bool loop = true;

    if (p->state == PLAYER_STATE_HURT) {
        start = 24; len = 1; loop = false;
    }
    else if (p->state == PLAYER_STATE_ATTACK) {
        loop = false;
        speed = 0.08f;

        if (p->currentMove == &p->moves->specialSide || 
            p->currentMove == &p->moves->specialUp || 
            p->currentMove == &p->moves->ultimate) {
            
            start = 19;
            len = 1; 
            loop = true;
        }

        else if (p->currentMove == &p->moves->sideGround)      { start = 12; len = 2; }
        else if (p->currentMove == &p->moves->upGround)   { start = 14; len = 3; }
        else if (p->currentMove == &p->moves->neutralGround) { start = 17; len = 2; }
        else if (p->currentMove == &p->moves->airSide)    { start = 20; len = 1; }
        else if (p->currentMove == &p->moves->airUp)      { start = 21; len = 1; }
        else if (p->currentMove == &p->moves->airDown)    { start = 22; len = 2; }
        else { 
            start = 12; len = 2;
        }
    }
    else if (p->state == PLAYER_STATE_JUMP) {
        start = 6; len = 4; speed = 0.1f; loop = false; 
    }
    else if (p->state == PLAYER_STATE_FALL) {
        start = 10; len = 2; speed = 0.15f;
    }
    else if (p->state == PLAYER_STATE_WALK) {
        start = 2; len = 4; speed = 0.12f;
    }
    else {
        start = 0; len = 2; speed = 0.3f;
    }

    if (p->animStartFrame != start) {
        p->animStartFrame = start;
        p->animLength = len;
        p->currentAnimIndex = start;
        p->animTimer = 0;
        p->animSpeed = speed;
        p->loopAnim = loop;
    }

    p->animTimer += dt;
    if (p->animTimer >= p->animSpeed) {
        p->animTimer = 0;
        p->currentAnimIndex++;

        if (p->currentAnimIndex >= p->animStartFrame + p->animLength) {
            if (p->loopAnim) {
                p->currentAnimIndex = p->animStartFrame;
            } else {
                p->currentAnimIndex = p->animStartFrame + p->animLength - 1;
            }
        }
    }
}

void GameScene_SetLanguage(int lang) {
    currentLanguage = lang;
}

void GameScene_Init(int p1CharacterID, int p2CharacterID) {
    sceneState = SCENE_STATE_START;
    matchWinner = 0;
    countdownTimer = 0;
    fightBannerTimer = 0;

    player1 = (Player*)malloc(sizeof(Player));
    player1->position = (Vector2){ 400, GROUND_LEVEL };
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
    player1->hasUsedAirSpecial = false;
    
    player1->moves = LoadMovesetFromJSON(GetCharacterJSON(p1CharacterID));
    TextCopy(player1->name, GetCharacterName(p1CharacterID));

    player2 = (Player*)malloc(sizeof(Player));
    player2->position = (Vector2){ 800, GROUND_LEVEL };
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
    player2->hasUsedAirSpecial = false;

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

    texBackground = LoadTexture("assets/matchbg.png");
     SetTextureFilter(texBackground, TEXTURE_FILTER_POINT);

    player1->spriteSheet = LoadTexture("assets/Bacteriofago.png");
    player2->spriteSheet = LoadTexture("assets/Bacteriofago.png");
    SetTextureFilter(player1->spriteSheet, TEXTURE_FILTER_POINT);

    texRocketVfx = LoadTexture("assets/rocketfx.png");
    SetTextureFilter(texRocketVfx, TEXTURE_FILTER_POINT);

    texPoisonCloud = LoadTexture("assets/poison_cloud.png");
    SetTextureFilter(texPoisonCloud, TEXTURE_FILTER_POINT);

    texExplosion = LoadTexture("assets/explosion.png");
    SetTextureFilter(texExplosion, TEXTURE_FILTER_POINT);

    player1->vfxSpawnTimer = 0.7f;
    player2->vfxSpawnTimer = 0.7f;

    player1->frameWidth = player1->spriteSheet.width / 5;
    player1->frameHeight = player1->spriteSheet.height / 5;

    player1->animTimer = 0.0f;
    player1->animSpeed = 0.15f;
    player1->currentAnimIndex = 0;
    player1->animStartFrame = 0;
    player1->animLength = 2;

    player2->frameWidth = player2->spriteSheet.width / 5;
    player2->frameHeight = player2->spriteSheet.height / 5;

    player2->animTimer = 0.0f;
    player2->animSpeed = 0.15f;
    player2->currentAnimIndex = 0;
    player2->animStartFrame = 0;
    player2->animLength = 2;
}

int GameScene_Update(void) {
    float dt = GetFrameTime();

    UpdateVfx(dt);

    UpdatePlayerAnimation(player1, dt);
    UpdatePlayerAnimation(player2, dt);

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
                    return 2;
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
    Rectangle sourceRec = { 0.0f, 0.0f, (float)texBackground.width, (float)texBackground.height };
    Rectangle destRec   = { 0.0f, 0.0f, (float)GAME_WIDTH, (float)GAME_HEIGHT };
    Vector2 origin      = { 0.0f, 0.0f };
    DrawTexturePro(texBackground, sourceRec, destRec, origin, 0.0f, WHITE);

    DrawPlayerSprite(player1, WHITE);
    DrawPlayerSprite(player2, (Color){200, 200, 255, 255});
    
    DrawVfx();
    
    Color p1Color = (player1->state == PLAYER_STATE_ATTACK) ? RED : GREEN;
    DrawRectangleLinesEx((Rectangle){ 
        player1->position.x - 20, 
        player1->position.y - 60,
        40, 
        60
    }, 2.0f, p1Color);

    Color p2Color = (player2->state == PLAYER_STATE_ATTACK) ? ORANGE : BLUE;
    DrawRectangleLinesEx((Rectangle){ 
        player2->position.x - 20, 
        player2->position.y - 60,
        40, 
        60
    }, 2.0f, p2Color);

    Combat_Draw(texPoisonCloud);

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
        
        float cdSize = hudFont.baseSize * 4.0f;
        float cdSpacing = 4.0f;
        Vector2 txtSize = MeasureTextEx(hudFont, countdownText, cdSize, cdSpacing);
        
        DrawTextEx(hudFont, countdownText, 
            (Vector2){(GAME_WIDTH - txtSize.x)/2, (GAME_HEIGHT - txtSize.y)/2}, 
            cdSize, cdSpacing, YELLOW);
    }

    if (sceneState == SCENE_STATE_PLAY && fightBannerTimer < 60) {
        const char* fightText = (currentLanguage == 0) ? "FIGHT!" : "LUTEM!";
        
        float fSize = hudFont.baseSize * 5.0f;
        float fSpacing = 5.0f;
        Vector2 txtSize = MeasureTextEx(hudFont, fightText, fSize, fSpacing);
        
        Color fightColor = (fightBannerTimer % 10 < 5) ? RED : ORANGE;
        
        DrawTextEx(hudFont, fightText, 
            (Vector2){(GAME_WIDTH - txtSize.x)/2, (GAME_HEIGHT - txtSize.y)/2}, 
            fSize, fSpacing, fightColor);
    }
    else if (sceneState == SCENE_STATE_ROUND_END) {
        const char* text = "KO!";
        float kSize = hudFont.baseSize * 5.0f;
        float kSpacing = 5.0f;
        Vector2 txtSize = MeasureTextEx(hudFont, text, kSize, kSpacing);

        DrawTextEx(hudFont, text, 
            (Vector2){(GAME_WIDTH - txtSize.x)/2, (GAME_HEIGHT - txtSize.y)/2}, 
            kSize, kSpacing, RED);
    }
    else if (sceneState == SCENE_STATE_GAME_OVER) {
        DrawRectangle(0, 0, GAME_WIDTH, GAME_HEIGHT, (Color){0,0,0, 200});
        
        char wText[50];
        if (currentLanguage == 0) {
            sprintf(wText, (matchWinner == 1) ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!");
        } else {
            sprintf(wText, (matchWinner == 1) ? "JOGADOR 1 VENCEU!" : "JOGADOR 2 VENCEU!");
        }
        
        Color wColor = (matchWinner == 1) ? GREEN : BLUE;
        
        float wSize = hudFont.baseSize * 3.0f;
        float wSpacing = 3.0f;
        Vector2 wTxtSize = MeasureTextEx(hudFont, wText, wSize, wSpacing);

        DrawTextEx(hudFont, wText, 
            (Vector2){(GAME_WIDTH - wTxtSize.x)/2, GAME_HEIGHT/2 - 50}, 
            wSize, wSpacing, wColor);

        const char* subText = (currentLanguage == 0) ? "Press ENTER to Return" : "Pressione ENTER para Voltar";
        float subSize = mainFont.baseSize * 1.5f; 
        Vector2 subTxtSize = MeasureTextEx(mainFont, subText, subSize, 2.0f);

        DrawTextEx(mainFont, subText, 
            (Vector2){(GAME_WIDTH - subTxtSize.x)/2, GAME_HEIGHT/2 + 40}, 
            subSize, 2.0f, RAYWHITE);
    }

    if (sceneState == SCENE_STATE_PAUSED) {
        DrawRectangle(0, 0, GAME_WIDTH, GAME_HEIGHT, (Color){ 0, 0, 0, 150 });

        float menuW = 500;
        float menuH = 300;
        float menuX = (GAME_WIDTH - menuW) / 2;
        float menuY = (GAME_HEIGHT - menuH) / 2;
        
        DrawRectangle(menuX, menuY, menuW, menuH, (Color){ 20, 30, 60, 240 });
        DrawRectangleLinesEx((Rectangle){menuX, menuY, menuW, menuH}, 4, SKYBLUE);

        const char* title = (currentLanguage == 0) ? "PAUSED" : "PAUSA";
        float tSize = mainFont.baseSize * 2.0f;
        float tSpacing = 2.0f;
        Vector2 tTxtSize = MeasureTextEx(mainFont, title, tSize, tSpacing);

        DrawTextEx(mainFont, title, 
            (Vector2){(GAME_WIDTH - tTxtSize.x)/2, menuY + 30}, 
            tSize, tSpacing, WHITE);

        int startOptY = menuY + 120;
        int spacing = 50;

        const char* optsEN[] = { "RESUME", "SETTINGS", "QUIT MATCH" };
        const char* optsPT[] = { "CONTINUAR", "CONFIGURAÇÕES", "SAIR DA PARTIDA" };
        const char** currentOpts = (currentLanguage == 0) ? optsEN : optsPT;

        for (int i = 0; i < 3; i++) {
            Color color = (i == pauseOption) ? YELLOW : GRAY;
            
            float optSize = (i == pauseOption) ? mainFont.baseSize * 1.5f : mainFont.baseSize * 1.2f;
            
            const char* txt = currentOpts[i];
            
            Vector2 optTxtSize = MeasureTextEx(mainFont, txt, optSize, 2.0f);
            float drawX = (GAME_WIDTH - optTxtSize.x)/2;
            
            DrawTextEx(mainFont, txt, (Vector2){drawX, startOptY + (i * spacing)}, optSize, 2.0f, color);
        }
    }
}

void GameScene_Unload(void) {
    UnloadTexture(texBackground);
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

    UnloadTexture(texRocketVfx);
    UnloadTexture(texPoisonCloud);
    UnloadTexture(texExplosion);
    Vfx_Cleanup();

    Combat_Cleanup();
}