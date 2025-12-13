#include "game_scene.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>

#define BODY_WIDTH 50.0f
#define BODY_HEIGHT 90.0f

static HitboxNode *activeHitboxes = NULL;
static ProjectileNode *activeProjectiles = NULL;
static TrapNode *activeTraps = NULL;

static void SpawnHitbox(Player *attacker, Move *move, bool isPlayer1) {
    HitboxNode *newNode = (HitboxNode*)malloc(sizeof(HitboxNode));
    if (!newNode) return;

    Rectangle hitboxRect = move->hitbox;
    
    float offsetX = attacker->isFlipped ? (-hitboxRect.x - hitboxRect.width) : hitboxRect.x;
    
    newNode->relX = offsetX;
    newNode->relY = hitboxRect.y;

    newNode->size = (Rectangle){ 
        attacker->position.x + newNode->relX, 
        attacker->position.y + newNode->relY, 
        hitboxRect.width, 
        hitboxRect.height 
    };

    newNode->damage = move->damage;
    newNode->knockback = move->knockback;
    newNode->lifetime = move->activeFrames;
    newNode->isPlayer1 = isPlayer1; 
    newNode->effect = move->effect;
    newNode->effectDuration = move->effectDuration;
    newNode->moveType = move->type;
    
    newNode->next = activeHitboxes;
    activeHitboxes = newNode;
}

static void SpawnProjectile(Player *attacker, Move *move, bool isPlayer1) {
    ProjectileNode *p = (ProjectileNode*)malloc(sizeof(ProjectileNode));
    if (!p) return;

    float dir = attacker->isFlipped ? -1.0f : 1.0f;
    float offsetX = attacker->isFlipped ? (-move->hitbox.x - move->hitbox.width) : move->hitbox.x;
    
    p->position = (Vector2){ attacker->position.x + offsetX, attacker->position.y + move->hitbox.y };
    p->velocity = (Vector2){ move->projectileSpeed.x * dir, move->projectileSpeed.y };
    p->size = move->hitbox;
    p->damage = move->damage;
    p->knockback = move->knockback;
    p->lifetime = (move->type == MOVE_TYPE_PROJECTILE_INSTANT) ? 10 : 180;
    
    p->spawnTrapOnGround = (move->type == MOVE_TYPE_TRAP_PROJECTILE);
    p->trapDuration = (move->type == MOVE_TYPE_TRAP_PROJECTILE) ? 600 : 0;
    p->effect = move->effect;
    p->effectDuration = move->effectDuration;
    p->moveType = move->type;
    
    p->isPlayer1 = isPlayer1;

    p->next = activeProjectiles;
    activeProjectiles = p;
}

static void SpawnTrap(Vector2 pos, Rectangle size, float damage, float duration, bool isP1, MoveEffect effect, MoveType type) {
    TrapNode *t = (TrapNode*)malloc(sizeof(TrapNode));
    if (!t) return;

    t->area = size;
    t->area.x = pos.x;
    t->area.y = pos.y;
    t->damage = damage;
    t->duration = duration;
    t->isPlayer1 = isP1;
    t->effect = effect;
    t->moveType = type;
    
    t->next = activeTraps;
    activeTraps = t;
}

void Combat_TryExecuteMove(Player *player, Move *move, bool isPlayer1) {
    if (move->type == MOVE_TYPE_TRAP_PROJECTILE) {
        if (player->isGrounded) {
            Rectangle trapRect = move->hitbox;
            float dir = player->isFlipped ? -1.0f : 1.0f;
            float offsetX = player->isFlipped ? (-trapRect.x - trapRect.width) : trapRect.x;
            Vector2 pos = { player->position.x + offsetX, GROUND_LEVEL - trapRect.height }; 
            SpawnTrap(pos, trapRect, move->damage, move->trapDuration * 60, isPlayer1, move->effect, move->type);
        } else {
            SpawnProjectile(player, move, isPlayer1);
        }
    }
    else if (move->type == MOVE_TYPE_PROJECTILE || move->type == MOVE_TYPE_PROJECTILE_INSTANT) {
        SpawnProjectile(player, move, isPlayer1);
    }
    else if (move->type == MOVE_TYPE_TRAP) {
        Rectangle trapRect = move->hitbox;
        float dir = player->isFlipped ? -1.0f : 1.0f;
        float offsetX = player->isFlipped ? (-trapRect.x - trapRect.width) : trapRect.x;
        Vector2 pos = { player->position.x + offsetX, player->position.y + trapRect.y };
        SpawnTrap(pos, trapRect, move->damage, move->effectDuration * 60, isPlayer1, move->effect, move->type);
    }
    else {
        SpawnHitbox(player, move, isPlayer1);
    }
}

void Combat_Update(Player *p1, Player *p2) {
    ProjectileNode *proj = activeProjectiles;
    ProjectileNode *prevProj = NULL;
    while (proj != NULL) {
        proj->position.x += proj->velocity.x;
        proj->position.y += proj->velocity.y;
        proj->lifetime--;

        bool hitGround = false;
        if (proj->spawnTrapOnGround && proj->position.y >= GROUND_LEVEL - proj->size.height) {
            hitGround = true;
            SpawnTrap(proj->position, proj->size, proj->damage, proj->trapDuration, proj->isPlayer1, proj->effect, proj->moveType);
        }

        if (proj->lifetime <= 0 || hitGround || proj->position.x < -200 || proj->position.x > GAME_WIDTH + 200) {
            ProjectileNode *toFree = proj;
            if (prevProj) prevProj->next = proj->next; else activeProjectiles = proj->next;
            proj = proj->next;
            free(toFree);
        } else {
            prevProj = proj;
            proj = proj->next;
        }
    }

    TrapNode *trap = activeTraps;
    TrapNode *prevTrap = NULL;
    while (trap != NULL) {
        trap->duration--;
        if (trap->duration <= 0) {
            TrapNode *toFree = trap;
            if (prevTrap) prevTrap->next = trap->next; else activeTraps = trap->next;
            trap = trap->next;
            free(toFree);
        } else {
            prevTrap = trap;
            trap = trap->next;
        }
    }

    HitboxNode *hb = activeHitboxes;
    HitboxNode *prevHb = NULL;
    while (hb != NULL) {
        hb->lifetime--;

        Player *owner = hb->isPlayer1 ? p1 : p2;
        
        hb->size.x = owner->position.x + hb->relX;
        hb->size.y = owner->position.y + hb->relY;

        if (hb->size.x < 0) {
            hb->size.width += hb->size.x;
            hb->size.x = 0;
        }
        if (hb->size.x + hb->size.width > GAME_WIDTH) {
            hb->size.width = GAME_WIDTH - hb->size.x;
        }
        if (hb->size.y + hb->size.height > GROUND_LEVEL) {
            hb->size.height = GROUND_LEVEL - hb->size.y;
        }

        if (hb->lifetime <= 0 || hb->size.width <= 0 || hb->size.height <= 0) {
            HitboxNode *toFree = hb;
            if (prevHb) prevHb->next = hb->next; else activeHitboxes = hb->next;
            hb = hb->next;
            free(toFree);
        } else {
            prevHb = hb;
            hb = hb->next;
        }
    }

    Rectangle body1 = { 
        p1->position.x - (BODY_WIDTH/2), 
        p1->position.y - BODY_HEIGHT, 
        BODY_WIDTH, 
        BODY_HEIGHT 
    };
    Rectangle body2 = { 
        p2->position.x - (BODY_WIDTH/2), 
        p2->position.y - BODY_HEIGHT, 
        BODY_WIDTH, 
        BODY_HEIGHT 
    };

    proj = activeProjectiles;
    prevProj = NULL;
    while (proj != NULL) {
        Player *victim = proj->isPlayer1 ? p2 : p1;
        Rectangle victimBody = proj->isPlayer1 ? body2 : body1;
        
        if (CheckCollisionRecs((Rectangle){proj->position.x, proj->position.y, proj->size.width, proj->size.height}, victimBody)) {
            victim->currentHealth -= proj->damage;

            if (proj->effect != EFFECT_POISON) {
                Vector2 spawnPos = {
                    victimBody.x + victimBody.width / 2.0f,
                    victimBody.y + victimBody.height / 2.0f
                };
                SpawnVfx(spawnPos, 0.0f, texHitVfx, 8, 0.04f, 1.0f);
                PlayHurtSound();
            }

            if (proj->moveType != MOVE_TYPE_ULTIMATE && proj->moveType != MOVE_TYPE_ULTIMATE_FALL) {
                float gainAttacker = proj->damage * 0.8f;
                float gainVictim = proj->damage * 0.5f;
            
                Player *attacker = proj->isPlayer1 ? p1 : p2;
                attacker->ultCharge += gainAttacker;
                victim->ultCharge += gainVictim;
                
                if (attacker->ultCharge > attacker->maxUltCharge) attacker->ultCharge = attacker->maxUltCharge;
                if (victim->ultCharge > victim->maxUltCharge) victim->ultCharge = victim->maxUltCharge;

                attacker->currentUlt = (int)(attacker->ultCharge / attacker->chargePerPill);
                victim->currentUlt = (int)(victim->ultCharge / victim->chargePerPill);

                ProjectileNode *toFree = proj;
                if (prevProj) prevProj->next = proj->next; else activeProjectiles = proj->next;
                proj = proj->next;
                free(toFree);
            }
        } else {
            prevProj = proj;
            proj = proj->next;
        }
    }

    hb = activeHitboxes;
    prevHb = NULL;
    while (hb != NULL) {
        Player *victim = hb->isPlayer1 ? p2 : p1;
        Player *attacker = hb->isPlayer1 ? p1 : p2;
        Rectangle victimBody = hb->isPlayer1 ? body2 : body1;
        
        if (CheckCollisionRecs(hb->size, victimBody)) {
            if (hb->moveType == MOVE_TYPE_ULTIMATE && attacker->characterID == 1) {
                
                if (hb->lifetime % 20 == 0) {
                    victim->currentHealth -= hb->damage;
                    
                    Vector2 spawnPos = {
                        victimBody.x + victimBody.width / 2.0f,
                        victimBody.y + victimBody.height / 2.0f
                    };
                    SpawnVfx(spawnPos, 0.0f, texHitVfx, 8, 0.04f, 1.0f);
                    PlayHurtSound();
                    
                    float kbDir = (attacker->position.x < victim->position.x) ? 2.0f : -2.0f;
                    victim->velocity.x = hb->knockback.x * kbDir;
                    victim->velocity.y = hb->knockback.y;
                    
                    victim->state = PLAYER_STATE_HURT;
                    victim->attackFrameCounter = 15;
                }

                prevHb = hb;
                hb = hb->next;
                continue; 
            }

            victim->currentHealth -= hb->damage;
            if (hb->effect == EFFECT_POISON) victim->poisonTimer = hb->effectDuration;

            if (hb->effect != EFFECT_POISON) {
                Vector2 spawnPos = {
                    victimBody.x + victimBody.width / 2.0f,
                    victimBody.y + victimBody.height / 2.0f
                };
                SpawnVfx(spawnPos, 0.0f, texHitVfx, 8, 0.04f, 1.0f);
                PlayHurtSound();
            }

            if (hb->moveType != MOVE_TYPE_ULTIMATE && hb->moveType != MOVE_TYPE_ULTIMATE_FALL) {
                float gainAttacker = hb->damage * 5.0f; 
                float gainVictim = hb->damage * 2.0f;   

                attacker->ultCharge += gainAttacker;
                victim->ultCharge += gainVictim;

                if (attacker->ultCharge > attacker->maxUltCharge) attacker->ultCharge = attacker->maxUltCharge;
                if (victim->ultCharge > victim->maxUltCharge) victim->ultCharge = victim->maxUltCharge;

                attacker->currentUlt = (int)(attacker->ultCharge / attacker->chargePerPill);
                victim->currentUlt = (int)(victim->ultCharge / victim->chargePerPill);
            }
            
            float kbDir = (p1->position.x < p2->position.x) ? 1.0f : -1.0f;
            if (!hb->isPlayer1) kbDir *= -1;

            victim->velocity.x = hb->knockback.x * kbDir;
            
            if (!attacker->isGrounded && attacker->position.y < victim->position.y - 30.0f) {
                victim->velocity.y = fabs(hb->knockback.y); 
            }

            else {
                if (fabs(hb->knockback.y) > 0.1f) {
                    victim->velocity.y = hb->knockback.y;
                }
            }
            
            victim->state = PLAYER_STATE_HURT;
            victim->attackFrameCounter = 30; 

            HitboxNode *toFree = hb;
            if (prevHb) prevHb->next = hb->next; else activeHitboxes = hb->next;
            hb = hb->next;
            free(toFree);
        } else {
            prevHb = hb;
            hb = hb->next;
        }
    }
    
    trap = activeTraps;
    while (trap != NULL) {
        Player *victim = trap->isPlayer1 ? p2 : p1;
        Rectangle victimBody = trap->isPlayer1 ? body2 : body1;
        if (CheckCollisionRecs(trap->area, victimBody)) {
            if ((int)trap->duration % 60 == 0) {
                victim->currentHealth -= trap->damage;
                if (trap->effect == EFFECT_POISON) victim->poisonTimer = 5.0f;
                
                if (trap->effect != EFFECT_POISON) {
                    Vector2 spawnPos = {
                        victimBody.x + victimBody.width / 2.0f,
                        victimBody.y + victimBody.height / 2.0f
                    };
                    SpawnVfx(spawnPos, 0.0f, texHitVfx, 8, 0.04f, 1.0f);
                    PlayHurtSound();
                }
            }
        }
        trap = trap->next;
    }
}

void Combat_ApplyStatus(Player *player, float dt) {
    if (player->poisonTimer > 0) {
        player->poisonTimer -= dt;
        player->currentHealth -= 5.0f * dt;
    }
}

void Combat_Draw(Texture2D poisonTex, Texture2D dnaTex, Texture2D amoebaTex) {
    int totalFrames = 6;
    int currentFrame = (int)(GetTime() * 10.0f) % totalFrames;
    
    float frameW = (float)poisonTex.width / totalFrames;
    float frameH = (float)poisonTex.height;

    Rectangle sourceRec = {
        currentFrame * frameW,
        0.0f,
        frameW,
        frameH
    };

    for (TrapNode *t = activeTraps; t != NULL; t = t->next) {
        if (t->effect == EFFECT_POISON || t->moveType == MOVE_TYPE_TRAP) {

            Color cloudColor;
            if (t->moveType == MOVE_TYPE_TRAP) cloudColor = Fade(WHITE, 0.8f);
            else cloudColor = Fade(WHITE, 0.6f);

            int totalFrames = 6;
            float frameWidth = (float)poisonTex.width / totalFrames;
            float frameHeight = (float)poisonTex.height;

            int currentFrame = (int)(GetTime() * 10.0f) % totalFrames;

            Rectangle sourceRec = { 
                currentFrame * frameWidth, 
                0.0f, 
                frameWidth, 
                frameHeight 
            };

            float scale = 3.0f;

            if (t->area.width > 100.0f) {
                scale = (t->area.width / frameWidth) * 1.2f;
            }
            
            float drawWidth = frameWidth * scale;   
            float drawHeight = frameHeight * scale;

            float centerX = t->area.x + (t->area.width / 2.0f);
            float centerY = t->area.y + (t->area.height / 2.0f);

            Rectangle destRec = {
                centerX,
                centerY,
                drawWidth,
                drawHeight
            };

            Vector2 origin = { drawWidth / 2.0f, drawHeight / 2.0f };

            DrawTexturePro(poisonTex, sourceRec, destRec, origin, 0.0f, cloudColor);
        }
        else {
            DrawRectangleRec(t->area, Fade(GREEN, 0.5f));
        }
    }
    for (ProjectileNode *p = activeProjectiles; p != NULL; p = p->next) {
        
        Texture2D spriteToUse = {0};
        bool shouldUseSprite = false;

        if (p->effect == EFFECT_POISON || p->moveType == MOVE_TYPE_TRAP_PROJECTILE) {
            spriteToUse = dnaTex;
            shouldUseSprite = true;
        }

        else if (p->effect == EFFECT_SLOW) {
            spriteToUse = amoebaTex;
            shouldUseSprite = true;
        }

        if (shouldUseSprite) {
            Rectangle sourceRec = { 0.0f, 0.0f, (float)spriteToUse.width, (float)spriteToUse.height };

            float scale = 3.5f; 
            
            float drawWidth = (float)spriteToUse.width * scale;
            float drawHeight = (float)spriteToUse.height * scale;

            Rectangle destRec = {
                p->position.x + (p->size.width / 2.0f),
                p->position.y + (p->size.height / 2.0f),
                drawWidth,
                drawHeight
            };
        
            Vector2 origin = { drawWidth / 2.0f, drawHeight / 2.0f };
            
            float rotation = 0.0f;
            if (fabs(p->velocity.x) > 0.1f) {
                 rotation = 90.0f; 
            }

            DrawTexturePro(spriteToUse, sourceRec, destRec, origin, rotation, WHITE);
        } 
        else {
            DrawRectangle(p->position.x, p->position.y, p->size.width, p->size.height, YELLOW);
        }
    }
}

void Combat_Cleanup(void) {
    while (activeHitboxes) { HitboxNode *n = activeHitboxes; activeHitboxes=n->next; free(n); }
    while (activeProjectiles) { ProjectileNode *n = activeProjectiles; activeProjectiles=n->next; free(n); }
    while (activeTraps) { TrapNode *n = activeTraps; activeTraps=n->next; free(n); }
}

void Combat_Init(void) {
    Combat_Cleanup();
}