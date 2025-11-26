#include "custom_fonts.h"
#include <stdlib.h>
#include <string.h>
#include "raylib.h"

Font LoadGameFont(const char* fileName) {
    Texture2D fontTexture = LoadTexture(fileName);

    SetTextureFilter(fontTexture, TEXTURE_FILTER_POINT);

    Font font = { 0 };
    font.baseSize = 23;
    font.texture = fontTexture;

    const char *charOrder = "ABCDEFGHIJKLMNOPQRSTUVWXYZ:1234567890?!";
    
    int totalGlyphs = strlen(charOrder) + 1;

    font.glyphCount = totalGlyphs; 
    font.glyphPadding = 0;
    font.recs = (Rectangle *)malloc(font.glyphCount * sizeof(Rectangle));
    font.glyphs = (GlyphInfo *)malloc(font.glyphCount * sizeof(GlyphInfo));

    int currentX = 0;
    int gap = 6;

    for (int i = 0; i < strlen(charOrder); i++) {
        char letter = charOrder[i];
        
        int width = 16;  
        int height = 21; 
        int offsetY = 0;

        if (letter == 'M' || letter == 'W') {
            width = 20;
            height = 20;
            offsetY = 1;
        }
        else if (letter == 'V') {
            width = 18;
            height = 20;
            offsetY = 1;
        }
        else if (letter == '0' || letter == '2' || (letter >= '4' && letter <= '9')) {
            width = 14;
        }
        else if (letter == '1') {
            width = 12;
        }
        else if (letter == ':') {
            width = 6;
        }
        else if (letter == 'I' || letter == '!') {
            width = 4;
        }
        else if (letter == 'Q') {
            height = 23;
        }

        font.recs[i].x = (float)currentX;
        font.recs[i].y = 0;
        font.recs[i].width = (float)width;
        font.recs[i].height = (float)height;

        font.glyphs[i].value = letter;
        font.glyphs[i].offsetX = 0;
        font.glyphs[i].offsetY = offsetY;
        font.glyphs[i].advanceX = width + 2;
        font.glyphs[i].image = (Image){ 0 }; 

        currentX += width + gap;
    }

    int spaceIndex = strlen(charOrder);
    font.recs[spaceIndex] = (Rectangle){ 0, 0, 0, 0 };
    font.glyphs[spaceIndex].value = 32;
    font.glyphs[spaceIndex].offsetX = 0;
    font.glyphs[spaceIndex].offsetY = 0;
    font.glyphs[spaceIndex].advanceX = 12;

    return font;
}

Font LoadMainFont(const char* fileName) {
    Texture2D fontTexture = LoadTexture(fileName);

    SetTextureFilter(fontTexture, TEXTURE_FILTER_POINT);

    Font font = { 0 };
    font.baseSize = 22; 
    font.texture = fontTexture;

    const char *charOrder = 
        "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
        "\xC1\xC0\xC2\xC3\xC7\xC9\xCA\xCD\xD3\xD4\xD5\xDA"
        "\xE1\xE0\xE2\xE3\xE7\xE9\xEA\xED\xF3\xF4\xF5\xFA";
    
    int totalChars = strlen(charOrder) + 1; 

    font.glyphCount = totalChars;
    font.recs = (Rectangle *)malloc(font.glyphCount * sizeof(Rectangle));
    font.glyphs = (GlyphInfo *)malloc(font.glyphCount * sizeof(GlyphInfo));

    int currentX = 0; 
    int pngGap = 4;

    font.recs[0] = (Rectangle){ 0, 0, 0, 0 };
    font.glyphs[0].value = 32; 
    font.glyphs[0].offsetX = 0;
    font.glyphs[0].offsetY = 0;
    font.glyphs[0].advanceX = 10;
    font.glyphs[0].image = (Image){ 0 };

    for (int i = 0; i < strlen(charOrder); i++) {
        unsigned char letter = (unsigned char)charOrder[i];
        int fontIndex = i + 1;

        int width = 18;
        int height = 22;
        int offsetY = 0;

        if (letter >= 'a' && letter <= 'z') {
            if (letter == 'w' || letter == 'x') { width = 20; height = 18; }
            
            else if (strchr("cefmorosuvz", letter)) { width = 18; height = 18; }
            
            else if (letter == 'a' || letter == 'n' || letter == 'g') { width = 17; height = 18; }
            
            else if (strchr("bdhkpq", letter)) { width = 16; } 
            else if (letter == 't') { width = 16; height = 18; }
            
            else if (letter == 'y') { width = 15; height = 18; }
            
            else if (letter == 'j') { width = 14; height = 20; }
            else if (letter == 'l') { width = 14; height = 20; }
            
            else if (letter == 'i') { width = 12; height = 20; }
            
            if (letter == 'b' || letter == 'd' || letter == 'p' || letter == 'q') height = 20; 
            if (letter == 'h' || letter == 'k') height = 18;

            offsetY = 22 - height;

            if (strchr("jpqgy", letter)) {
                offsetY = 4; 
            }
        }
        
        else if (letter >= 'A' && letter <= 'Z') {
            if (letter == 'I') width = 14;
            else if (letter == 'M' || letter == 'W') width = 22;
            else if (letter == 'Q') height = 24; 
            else if (letter == 'V') width = 20;
        }

        else if (letter >= 128) {
            if (letter == 0xE1 || letter == 0xE0 || letter == 0xE2) { width = 17; height = 26; }
            else if (letter == 0xE3) { width = 17; height = 25; }
            else if (letter == 0xE7) { width = 18; height = 24; }
            else if (letter == 0xE9 || letter == 0xEA) { width = 18; height = 26; }
            else if (letter == 0xED) { width = 12; height = 26; }
            else if (letter == 0xF3 || letter == 0xF4 || letter == 0xFA) { width = 18; height = 26; }
            else if (letter == 0xF5) { width = 18; height = 25; }

            else if (letter == 0xC1 || letter == 0xC0 || letter == 0xC2) { width = 18; height = 30; }
            else if (letter == 0xC3) { width = 18; height = 29; }
            else if (letter == 0xC7) { width = 18; height = 29; }
            else if (letter == 0xC9 || letter == 0xCA) { width = 18; height = 30; }
            else if (letter == 0xCD) { width = 14; height = 30; }
            else if (letter == 0xD3 || letter == 0xD4 || letter == 0xDA) { width = 18; height = 30; }
            else if (letter == 0xD5) { width = 18; height = 29; }

            if (letter == 0xE7) offsetY = 4;
            else if (letter == 0xC7) offsetY = 0;
            else offsetY = 22 - height;
        }

        else {
            if (letter == '{' || letter == '}') { width = 15; height = 23; offsetY = -1; }
            else if (letter == '|') { width = 6; height = 26; offsetY = -2; }
            else if (letter == '~') { width = 12; height = 6; offsetY = 8; }
            else if (strchr("!':;,.", letter)) {
                width = 6;
                if (letter == '\'') { height = 9; }                    
                else if (letter == ',') { height = 10; offsetY = 12; } 
                else if (letter == '.') { height = 6; offsetY = 16; }  
                else if (letter == ':') { height = 14; offsetY = 4; }  
                else if (letter == ';') { height = 18; offsetY = 2; }  
            }
            else if (strchr("()jt`", letter)) { 
                width = 10;
                if (letter == '`') { height = 10; offsetY = 0; } 
            }
            else if (letter == '[' || letter == ']') { width = 11; }
            else if (letter == '"') { width = 14; height = 9; }
            else if (strchr("%/?\\", letter)) { 
                width = 16;
                if (letter != '?') { height = 24; offsetY = -1; }
            }
            else if (letter == '&') width = 22;
            else if (letter == '@') width = 20;
            else if (letter == '^') { height = 12; offsetY = 0; }  
            else if (letter == '_') { height = 6; offsetY = 16; }  
            else if (letter == '4') { width = 17; }
            else if (letter == '$') { height = 26; offsetY = -2; }
            else if (letter == '*') { height = 20; }
            else if (letter == '+') { height = 18; offsetY = 2; }
            else if (letter == '=') { height = 14; offsetY = 4; }
            else if (letter == '-') { height = 6;  offsetY = 8; }
        }

        font.recs[fontIndex].x = (float)currentX;
        font.recs[fontIndex].y = 0;
        font.recs[fontIndex].width = (float)width;
        font.recs[fontIndex].height = (float)height;

        font.glyphs[fontIndex].value = letter;
        font.glyphs[fontIndex].offsetX = 0;
        font.glyphs[fontIndex].offsetY = offsetY;
        font.glyphs[fontIndex].advanceX = width + 2; 
        font.glyphs[fontIndex].image = (Image){ 0 };

        currentX += width + pngGap; 
    }

    return font;
}