#include "game_scene.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ParseMove(cJSON *json, Move *move) {
    if (!json) return;

    move->type = MOVE_TYPE_MELEE;
    move->effect = EFFECT_NONE;
    move->cooldown = 0;
    move->lastUsedTime = -100.0f;
    move->trapDuration = 0;

    move->selfVelocity = (Vector2){0,0};
    move->steerSpeed = 0;
    move->fallSpeed = 0;
    move->multiHit = false;
    move->canCombo = false;
    move->maxCombo = 0;

    cJSON *name = cJSON_GetObjectItem(json, "name");
    if (name) strncpy(move->name, name->valuestring, 31);

    cJSON *dmg = cJSON_GetObjectItem(json, "damage");
    if (dmg) move->damage = dmg->valuedouble;
    
    cJSON *startup = cJSON_GetObjectItem(json, "startup");
    if (startup) move->startupFrames = startup->valueint;
    
    cJSON *active = cJSON_GetObjectItem(json, "active");
    if (active) move->activeFrames = active->valueint;
    
    cJSON *recovery = cJSON_GetObjectItem(json, "recovery");
    if (recovery) move->recoveryFrames = recovery->valueint;

    cJSON *kb = cJSON_GetObjectItem(json, "knockback");
    if (kb) {
        move->knockback.x = cJSON_GetObjectItem(kb, "x")->valuedouble;
        move->knockback.y = cJSON_GetObjectItem(kb, "y")->valuedouble;
    }

    cJSON *hb = cJSON_GetObjectItem(json, "hitbox");
    if (hb) {
        move->hitbox.x = cJSON_GetObjectItem(hb, "x")->valuedouble;
        move->hitbox.y = cJSON_GetObjectItem(hb, "y")->valuedouble;
        move->hitbox.width = cJSON_GetObjectItem(hb, "w")->valuedouble;
        move->hitbox.height = cJSON_GetObjectItem(hb, "h")->valuedouble;
    }

    cJSON *effect = cJSON_GetObjectItem(json, "effect");
    if (effect) {
        if (strcmp(effect->valuestring, "POISON") == 0) move->effect = EFFECT_POISON;
        else if (strcmp(effect->valuestring, "SLOW") == 0) move->effect = EFFECT_SLOW;
    }

    cJSON *duration = cJSON_GetObjectItem(json, "effect_duration");
    if (duration) move->effectDuration = (float)duration->valuedouble;

    cJSON *trapDur = cJSON_GetObjectItem(json, "trap_duration");
    if (trapDur) move->trapDuration = (float)trapDur->valuedouble;

    cJSON *cd = cJSON_GetObjectItem(json, "cooldown");
    if (cd) move->cooldown = (float)cd->valuedouble;

    cJSON *projSpeed = cJSON_GetObjectItem(json, "projectile_speed");
    if (projSpeed) {
        move->projectileSpeed.x = cJSON_GetObjectItem(projSpeed, "x")->valuedouble;
        move->projectileSpeed.y = cJSON_GetObjectItem(projSpeed, "y")->valuedouble;
    }

    cJSON *selfVel = cJSON_GetObjectItem(json, "self_velocity");
    if (selfVel) {
        move->selfVelocity.x = cJSON_GetObjectItem(selfVel, "x")->valuedouble;
        move->selfVelocity.y = cJSON_GetObjectItem(selfVel, "y")->valuedouble;
    }

    cJSON *steer = cJSON_GetObjectItem(json, "steer_speed");
    if (steer) move->steerSpeed = (float)steer->valuedouble;

    cJSON *fall = cJSON_GetObjectItem(json, "fall_speed");
    if (fall) move->fallSpeed = (float)fall->valuedouble;

    cJSON *multi = cJSON_GetObjectItem(json, "multi_hit");
    if (multi) move->multiHit = multi->valueint;

    cJSON *combo = cJSON_GetObjectItem(json, "can_combo");
    if (combo) move->canCombo = combo->valueint;

    cJSON *type = cJSON_GetObjectItem(json, "type");
    if (type) {
        if (strcmp(type->valuestring, "PROJECTILE") == 0) move->type = MOVE_TYPE_PROJECTILE;
        else if (strcmp(type->valuestring, "PROJECTILE_INSTANT") == 0) move->type = MOVE_TYPE_PROJECTILE_INSTANT;
        else if (strcmp(type->valuestring, "TRAP") == 0) move->type = MOVE_TYPE_TRAP;
        else if (strcmp(type->valuestring, "TRAP_PROJECTILE") == 0) move->type = MOVE_TYPE_TRAP_PROJECTILE;
        else if (strcmp(type->valuestring, "GRAB") == 0) move->type = MOVE_TYPE_GRAB;
        else if (strcmp(type->valuestring, "ULTIMATE") == 0) move->type = MOVE_TYPE_ULTIMATE;
        else if (strcmp(type->valuestring, "ULTIMATE_FALL") == 0) move->type = MOVE_TYPE_ULTIMATE_FALL;
        else move->type = MOVE_TYPE_MELEE;
    }
}

Moveset* LoadMovesetFromJSON(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("ERRO: Nao foi possivel abrir %s\n", filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = (char*)malloc(length + 1);
    fread(data, 1, length, f);
    data[length] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(data);
    if (!root) {
        printf("ERRO: JSON invalido em %s\n", filename);
        free(data);
        return NULL;
    }

    Moveset *moveset = (Moveset*)malloc(sizeof(Moveset));
    cJSON *moves = cJSON_GetObjectItem(root, "moves");

    ParseMove(cJSON_GetObjectItem(moves, "side_ground"), &moveset->sideGround);
    ParseMove(cJSON_GetObjectItem(moves, "up_ground"), &moveset->upGround);
    ParseMove(cJSON_GetObjectItem(moves, "down_ground"), &moveset->downGround);
    ParseMove(cJSON_GetObjectItem(moves, "neutral_ground"), &moveset->neutralGround);
    
    ParseMove(cJSON_GetObjectItem(moves, "air_side"), &moveset->airSide);
    ParseMove(cJSON_GetObjectItem(moves, "air_up"), &moveset->airUp);
    ParseMove(cJSON_GetObjectItem(moves, "air_down"), &moveset->airDown);
    ParseMove(cJSON_GetObjectItem(moves, "air_neutral"), &moveset->airNeutral);

    ParseMove(cJSON_GetObjectItem(moves, "special_neutral"), &moveset->specialNeutral);
    ParseMove(cJSON_GetObjectItem(moves, "special_side"), &moveset->specialSide);
    ParseMove(cJSON_GetObjectItem(moves, "special_up"), &moveset->specialUp);
    ParseMove(cJSON_GetObjectItem(moves, "special_down"), &moveset->specialDown);
    
    ParseMove(cJSON_GetObjectItem(moves, "ultimate"), &moveset->ultimate);

    cJSON_Delete(root);
    free(data);
    
    printf("Moveset do Bacteriofago carregado com sucesso!\n");
    return moveset;
}