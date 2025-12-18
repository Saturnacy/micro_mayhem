#ifndef GAME_SCENE_H
#define GAME_SCENE_H

#include "raylib.h"

#define GAME_WIDTH 1200
#define GAME_HEIGHT 720
#define GROUND_LEVEL 640.0f

// --- ENUMS ---

typedef enum {
    MOVE_TYPE_MELEE,
    MOVE_TYPE_PROJECTILE,
    MOVE_TYPE_PROJECTILE_INSTANT,
    MOVE_TYPE_TRAP,
    MOVE_TYPE_TRAP_PROJECTILE,
    MOVE_TYPE_GRAB,
    MOVE_TYPE_ULTIMATE,
    MOVE_TYPE_ULTIMATE_FALL
} MoveType;

typedef enum {
    EFFECT_NONE,
    EFFECT_POISON,
    EFFECT_SLOW
} MoveEffect;

typedef enum {
    PLAYER_STATE_IDLE,
    PLAYER_STATE_WALK,
    PLAYER_STATE_JUMP,
    PLAYER_STATE_FALL,
    PLAYER_STATE_ATTACK,
    PLAYER_STATE_HURT,
    PLAYER_STATE_DEAD
} PlayerState;

typedef enum {
    AI_STATE_THINKING,
    AI_STATE_APPROACH,
    AI_STATE_ATTACK,
    AI_STATE_FLEE
} AIState;

typedef enum {
    LANG_EN,
    LANG_PT
} GameLanguage;

// --- STRUCTS DE DADOS (Moveset, Input, Settings) ---

typedef struct Move {
    char name[32];
    int startupFrames;
    int activeFrames;
    int recoveryFrames;
    Rectangle hitbox;
    Vector2 knockback;
    float damage;

    MoveType type;
    MoveEffect effect;
    float effectDuration;
    Vector2 projectileSpeed;

    Vector2 selfVelocity;
    float steerSpeed;
    float fallSpeed;
    bool multiHit;
    bool canCombo;
    int maxCombo;
    
    float cooldown;
    float lastUsedTime;
    float trapDuration;
} Move;

typedef struct Moveset {
    Move sideGround, upGround, downGround, neutralGround;
    Move airSide, airUp, airDown, airNeutral;
    Move specialNeutral, specialSide, specialUp, specialDown;
    Move ultimate;
} Moveset;

typedef struct {
    KeyboardKey left;
    KeyboardKey right;
    KeyboardKey up;
    KeyboardKey down;
    KeyboardKey jump;
    KeyboardKey attack;
    KeyboardKey special;
} InputConfig;

typedef struct {
    float masterVolume;     
    float musicVolume;      
    float sfxVolume;        
    int resolutionIndex;    
    bool fullscreen;
    GameLanguage language;
} GameSettings;

// --- STRUCTS DO JOGO (Player e Objetos de Combate) ---

typedef struct Player {
    Texture2D spriteSheet;
    int frameWidth;
    int frameHeight;
    int currentAnimIndex;
    int animStartFrame;
    int animLength;
    float vfxSpawnTimer;
    float animTimer;
    float animSpeed;
    bool loopAnim;
    char name[32];
    Vector2 position;
    Vector2 ultLaunchPos;
    Vector2 velocity;
    bool isGrounded;
    bool isFlipped;
    PlayerState state;
    int attackFrameCounter;
    Moveset *moves;
    Move *currentMove;
    int characterID;
    
    float health;
    float maxHealth;
    float currentHealth;
    int maxUlt;
    int currentUlt;
    float ultCharge;
    float maxUltCharge;
    float chargePerPill;
    int roundsWon;
    float poisonTimer;
    bool hasUsedAirSpecial;

    bool isCPU;
    AIState aiState;
    int aiTimer;
} Player;

typedef struct HitboxNode {
    Rectangle size;
    float damage;
    Vector2 knockback;
    int lifetime;
    bool isPlayer1;
    struct HitboxNode *next;
    float relX; 
    float relY;
    
    MoveEffect effect;
    float effectDuration;
    MoveType moveType;
} HitboxNode;

typedef struct ProjectileNode {
    Vector2 position;
    Vector2 velocity;
    Rectangle size;
    float damage;
    Vector2 knockback;
    int lifetime;
    bool isPlayer1;
    struct ProjectileNode *next;
    
    bool spawnTrapOnGround;
    float trapDuration;
    
    MoveEffect effect;
    float effectDuration;
    MoveType moveType;
} ProjectileNode;

typedef struct TrapNode {
    Rectangle area;
    float damage;
    float duration;
    bool isPlayer1;
    struct TrapNode *next;
    
    MoveEffect effect;
    MoveType moveType;
} TrapNode;

// --- PROTÓTIPOS DE FUNÇÕES ---
void GameScene_Init(int p1CharacterID, int p2CharacterID);
int GameScene_Update(void);
void GameScene_Draw(void);
void GameScene_Unload(void);
void GameScene_SetMultiplayer(bool enabled);
void GameScene_SetFont(Font font);
void GameScene_SetMainFont(Font font);
void GameScene_SetLanguage(int lang);

// Sistema de Combate
void Combat_Init(void);
void Combat_Update(Player *p1, Player *p2);
void Combat_Draw(Player *p1, Player *p2, Texture2D poisonTex, Texture2D dnaTex, Texture2D amoebaTex, Texture2D sporeTex);
void Combat_Cleanup(void);
void Combat_TryExecuteMove(Player *player, Move *move, bool isPlayer1);
void Combat_ApplyStatus(Player *player, float dt);
void PlayHurtSound(void);
void SpawnVfx(Vector2 pos, float rotation, Texture2D tex, int frames, float speed, float scale);

extern Texture2D texHitVfx;

Moveset* LoadMovesetFromJSON(const char *filename);

#endif