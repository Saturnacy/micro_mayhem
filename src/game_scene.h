#ifndef GAME_SCENE_H
#define GAME_SCENE_H

#include "raylib.h"

typedef struct Move {
    int startupFrames;
    int activeFrames;
    int recoveryFrames;
    Rectangle hitbox;
    Vector2 knockback;
    float damage;
} Move;

typedef struct Moveset {
    Move sideLight;
    Move upLight;
    Move downLight;
    Move airSideLight;
    Move airUpLight;
    Move airDownLight;
} Moveset;

typedef enum {
    PLAYER_STATE_IDLE,
    PLAYER_STATE_WALK,
    PLAYER_STATE_JUMP,
    PLAYER_STATE_FALL,
    PLAYER_STATE_ATTACK
} PlayerState;

typedef enum {
    AI_STATE_THINKING,
    AI_STATE_APPROACH,
    AI_STATE_ATTACK,
    AI_STATE_FLEE
} AIState;

typedef struct Player {
    Vector2 position;
    Vector2 velocity;
    bool isGrounded;
    bool isFlipped;
    PlayerState state;
    int attackFrameCounter;
    Moveset *moves;

    bool isCPU;
    AIState aiState;
    int aiTimer;
} Player;

typedef struct HitboxNode {
    Rectangle size;
    float damage;
    Vector2 knockback;
    int lifetime;
    struct HitboxNode *next;
} HitboxNode;

void GameScene_Init(void);
void GameScene_Update(void);
void GameScene_Draw(void);
void GameScene_Unload(void);

#endif