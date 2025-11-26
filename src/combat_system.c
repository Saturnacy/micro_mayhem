#include "game_scene.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>

static HitboxNode *activeHitboxes = NULL;
static ProjectileNode *activeProjectiles = NULL;
static TrapNode *activeTraps = NULL;

static void SpawnHitbox(Player *attacker, Move *move, bool isPlayer1) {
    HitboxNode *newNode = (HitboxNode*)malloc(sizeof(HitboxNode));
    if (!newNode) return;

    Rectangle hitboxRect = move->hitbox;
    if (attacker->isFlipped) hitboxRect.x = -hitboxRect.x - hitboxRect.width; 

    newNode->size = (Rectangle){ attacker->position.x + hitboxRect.x, attacker->position.y + hitboxRect.y, hitboxRect.width, hitboxRect.height };
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
            
            Vector2 pos = { player->position.x + offsetX, 540.0f - trapRect.height }; 
            
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
        if (proj->spawnTrapOnGround && proj->position.y >= 540 - proj->size.height) {
            hitGround = true;
            SpawnTrap(proj->position, proj->size, proj->damage, proj->trapDuration, proj->isPlayer1, proj->effect, proj->moveType);
        }

        if (proj->lifetime <= 0 || hitGround || proj->position.x < -200 || proj->position.x > 1400) {
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
        if (hb->lifetime <= 0) {
            HitboxNode *toFree = hb;
            if (prevHb) prevHb->next = hb->next; else activeHitboxes = hb->next;
            hb = hb->next;
            free(toFree);
        } else {
            prevHb = hb;
            hb = hb->next;
        }
    }

    Rectangle body1 = { p1->position.x - 20, p1->position.y, 40, 60 };
    Rectangle body2 = { p2->position.x - 20, p2->position.y, 40, 60 };

    proj = activeProjectiles;
    prevProj = NULL;
    while (proj != NULL) {
        Player *victim = proj->isPlayer1 ? p2 : p1;
        Rectangle victimBody = proj->isPlayer1 ? body2 : body1;
        bool hit = CheckCollisionRecs((Rectangle){proj->position.x, proj->position.y, proj->size.width, proj->size.height}, victimBody);
        
        if (hit) {
            victim->currentHealth -= proj->damage;

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
            victim->currentHealth -= hb->damage;
            if (hb->effect == EFFECT_POISON) victim->poisonTimer = hb->effectDuration;

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
            
            victim->velocity.x += hb->knockback.x * kbDir;
            victim->velocity.y -= hb->knockback.y;
            victim->state = PLAYER_STATE_HURT;

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

void Combat_Draw(void) {
    for (TrapNode *t = activeTraps; t != NULL; t = t->next) {
        if (t->effect == EFFECT_POISON) {
            Color cloudColor = (Color){ 50, 255, 50, 100 };
            Color outlineColor = (Color){ 20, 180, 20, 200 };
            
            float radius = t->area.width / 2.0f;
            Vector2 center = { t->area.x + radius, t->area.y + t->area.height / 2.0f };
            
            DrawCircleV(center, radius, cloudColor);
            DrawCircleLines(center.x, center.y, radius, outlineColor);
        } 
        else {
            Color trapColor = (Color){ 100, 0, 200, 100 };
            DrawRectangleRec(t->area, trapColor);
            DrawRectangleLinesEx(t->area, 2, GREEN);
        }
    }

    for (ProjectileNode *p = activeProjectiles; p != NULL; p = p->next) {
        DrawRectangle(p->position.x, p->position.y, p->size.width, p->size.height, YELLOW);
    }

    // Desenha Hitbox (Debug)
    for (HitboxNode *h = activeHitboxes; h != NULL; h = h->next) {
        DrawRectangleLinesEx(h->size, 2, RED);
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