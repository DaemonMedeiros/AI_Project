/* ============================================================
 * combatants.c - Procedural combatant sprites
 * ============================================================ */
#include "../include/combatants.h"
#include "raymath.h"
#include <math.h>
#include <string.h>

static Vector2 Flip(Vector2 local, bool facingRight)
{
    if (!facingRight) local.x = -local.x;
    return local;
}

/* ---------- Player Knight ---------- */
void CombatantDrawKnight(Vector2 pos, bool facingRight, CombatantPose pose)
{
    float bob   = sinf(pose.time * 3.0f) * 1.5f;
    float lunge = pose.poseT * 22.0f;

    Vector2 base = { pos.x + (facingRight ? lunge : -lunge), pos.y + bob };
    float px = base.x, py = base.y;

    Color armor   = { 180, 190, 205, 255 };
    Color armorHi = { 220, 228, 240, 255 };
    Color armorLo = { 105, 115, 135, 255 };
    Color cape    = { 90, 20, 30, 255 };
    Color skin    = { 225, 185, 150, 255 };
    Color leather = { 90, 60, 40, 255 };
    Color blade   = { 230, 235, 245, 255 };
    Color shield  = { 60, 90, 150, 255 };
    Color gold    = { 220, 180, 70, 255 };

    Vector2 capeTop = Flip((Vector2){ -10, -30 }, facingRight);
    Vector2 capeBot = Flip((Vector2){ -16,  22 }, facingRight);
    DrawTriangle(
        (Vector2){ px + capeTop.x, py + capeTop.y },
        (Vector2){ px + capeBot.x, py + capeBot.y },
        (Vector2){ px + (facingRight ? 4.0f : -4.0f), py + 22.0f },
        cape);

    for (int s = -1; s <= 1; s += 2)
    {
        Vector2 hip  = Flip((Vector2){ 0,        8 }, facingRight);
        Vector2 knee = Flip((Vector2){ 4 * s,   22 }, facingRight);
        Vector2 foot = Flip((Vector2){ 8 * s,   34 }, facingRight);
        DrawLineEx((Vector2){ px + hip.x,  py + hip.y  },
                   (Vector2){ px + knee.x, py + knee.y }, 8.0f, armorLo);
        DrawLineEx((Vector2){ px + knee.x, py + knee.y },
                   (Vector2){ px + foot.x, py + foot.y }, 8.0f, armor);
        DrawCircleV((Vector2){ px + foot.x, py + foot.y }, 5.0f, leather);
    }

    Rectangle chest = { px - 13, py - 14, 26, 26 };
    DrawRectangleRec(chest, armor);
    DrawRectangle((int)chest.x, (int)chest.y, (int)chest.width, 4, armorHi);
    DrawRectangle((int)chest.x, (int)(chest.y + chest.height - 4), (int)chest.width, 4, armorLo);
    DrawCircle((int)px, (int)(py - 2), 5, gold);

    DrawRectangle((int)px - 13, (int)py + 8, 26, 5, leather);
    DrawRectangle((int)px - 3,  (int)py + 8,  6, 5, gold);

    for (int s = -1; s <= 1; s += 2)
    {
        Vector2 shoulder = Flip((Vector2){ 15 * s, -12 }, facingRight);
        DrawCircleV((Vector2){ px + shoulder.x, py + shoulder.y }, 8.0f, armor);
        DrawCircleLines((int)(px + shoulder.x), (int)(py + shoulder.y), 8.0f, armorLo);
    }

    {
        Vector2 shoulder = Flip((Vector2){ -14, -8 }, facingRight);
        float sx = px + shoulder.x, sy = py + shoulder.y;
        DrawCircleV((Vector2){ sx, sy }, 14.0f, shield);
        DrawCircleLines((int)sx, (int)sy, 14.0f, armorLo);
        DrawCircleV((Vector2){ sx, sy }, 4.0f, gold);
        DrawLineEx((Vector2){ sx - 12, sy }, (Vector2){ sx + 12, sy }, 2.0f, gold);
        DrawLineEx((Vector2){ sx, sy - 12 }, (Vector2){ sx, sy + 12 }, 2.0f, gold);
    }

    DrawCircle((int)px, (int)(py - 26), 11, skin);
    DrawCircle((int)px, (int)(py - 30), 12, armor);
    DrawCircle((int)px - 3, (int)(py - 33), 6, armorHi);
    DrawRectangle((int)px - 9, (int)py - 26, 18, 3, (Color){ 30, 30, 40, 255 });
    DrawRectangle((int)px - 12, (int)py - 24, 3, 8, armorLo);
    DrawRectangle((int)px +  9, (int)py - 24, 3, 8, armorLo);

    {
        Vector2 shoulder = Flip((Vector2){ 16, -10 }, facingRight);
        float sx = px + shoulder.x, sy = py + shoulder.y;
        DrawCircleV((Vector2){ sx, sy }, 7.0f, armor);

        float reach = 14.0f + pose.poseT * 16.0f;
        Vector2 hand = Flip((Vector2){ reach + 12.0f, -6 }, facingRight);
        float hx = px + hand.x, hy = py + hand.y;
        DrawLineEx((Vector2){ sx, sy }, (Vector2){ hx, hy }, 6.0f, armor);

        DrawLineEx((Vector2){ hx, hy },
                   (Vector2){ hx + (facingRight ? 4.0f : -4.0f), hy - 10.0f },
                   3.0f, leather);
        DrawLineEx((Vector2){ hx - 6.0f, hy - 12.0f },
                   (Vector2){ hx + 6.0f, hy - 12.0f },
                   3.0f, gold);

        Vector2 bladeTip = Flip((Vector2){ reach + 26.0f, -32.0f }, facingRight);
        DrawLineEx((Vector2){ hx + (facingRight ? 4.0f : -4.0f), hy - 14.0f },
                   (Vector2){ px + bladeTip.x, py + bladeTip.y },
                   3.0f, blade);
        DrawLineEx((Vector2){ hx + (facingRight ? 4.0f : -4.0f), hy - 15.0f },
                   (Vector2){ px + bladeTip.x, py + bladeTip.y - 1.0f },
                   1.0f, WHITE);
    }
}

/* ---------- Slime ---------- */
void CombatantDrawSlime(Vector2 pos, bool facingRight, CombatantPose pose)
{
    float squash = 1.0f + sinf(pose.time * 4.0f) * 0.08f;
    float width = 44.0f / squash;
    float height = 30.0f * squash;

    Color body   = {  80, 200, 110, 235 };
    Color bodyHi = { 160, 240, 180, 200 };
    Color bodyLo = {  40, 130,  70, 255 };
    Color core   = {  60, 160,  90, 255 };

    DrawEllipse((int)pos.x, (int)(pos.y + height * 0.5f + 4), width * 0.6f, 6.0f, (Color){ 0, 0, 0, 80 });
    DrawEllipse((int)pos.x, (int)(pos.y + 2), width * 0.5f, height * 0.5f, body);
    DrawEllipse((int)(pos.x - width * 0.15f), (int)(pos.y - 4), width * 0.22f, height * 0.28f, bodyHi);
    DrawEllipse((int)pos.x, (int)(pos.y + height * 0.42f), width * 0.52f, 4.0f, bodyLo);
    DrawCircle((int)pos.x, (int)(pos.y + 4), 6.0f, core);

    float eyeOffset = facingRight ? 6.0f : -6.0f;
    DrawCircle((int)(pos.x + eyeOffset - 4), (int)(pos.y - 4), 4.0f, WHITE);
    DrawCircle((int)(pos.x + eyeOffset + 4), (int)(pos.y - 4), 4.0f, WHITE);
    DrawCircle((int)(pos.x + eyeOffset - 4 + (facingRight ? 1 : -1)), (int)(pos.y - 3), 2.0f, BLACK);
    DrawCircle((int)(pos.x + eyeOffset + 4 + (facingRight ? 1 : -1)), (int)(pos.y - 3), 2.0f, BLACK);

    DrawCircle((int)(pos.x - width * 0.2f), (int)(pos.y - height * 0.25f), 3.0f,
               (Color){ 255, 255, 255, 180 });
}

/* ---------- Goblin ---------- */
void CombatantDrawGoblin(Vector2 pos, bool facingRight, CombatantPose pose)
{
    float bob = sinf(pose.time * 5.0f) * 1.5f;
    float lunge = pose.poseT * 18.0f;

    float px = pos.x + (facingRight ? lunge : -lunge);
    float py = pos.y + bob;

    Color skin   = { 110, 170,  90, 255 };
    Color skinLo = {  70, 120,  60, 255 };
    Color rag    = { 130,  90,  50, 255 };
    Color ragLo  = {  90,  60,  30, 255 };
    Color eye    = { 240, 220,  60, 255 };
    Color tooth  = { 240, 240, 220, 255 };
    Color dagger = { 200, 205, 215, 255 };

    DrawEllipse((int)pos.x, (int)(pos.y + 42), 22.0f, 6.0f, (Color){ 0, 0, 0, 90 });

    DrawLineEx((Vector2){ px - 5, py + 12 }, (Vector2){ px - 7, py + 34 }, 7.0f, skinLo);
    DrawLineEx((Vector2){ px + 5, py + 12 }, (Vector2){ px + 7, py + 34 }, 7.0f, skinLo);
    DrawCircle((int)(px - 7), (int)(py + 36), 5.0f, (Color){ 60, 40, 30, 255 });
    DrawCircle((int)(px + 7), (int)(py + 36), 5.0f, (Color){ 60, 40, 30, 255 });

    DrawRectangle((int)px - 11, (int)py + 8, 22, 12, rag);
    DrawRectangle((int)px - 11, (int)py + 16, 22, 4, ragLo);

    Rectangle torso = { px - 12, py - 10, 24, 22 };
    DrawRectangleRec(torso, skin);
    DrawRectangle((int)torso.x, (int)(torso.y + torso.height - 6), (int)torso.width, 6, skinLo);

    DrawLineEx((Vector2){ px - 10, py - 8 }, (Vector2){ px + 10, py + 6 }, 2.0f, ragLo);

    Vector2 shoulderL = { px - 14, py - 8 };
    Vector2 shoulderR = { px + 14, py - 8 };
    DrawLineEx(shoulderL, (Vector2){ px - 20, py + 4 }, 6.0f, skin);
    DrawCircle((int)(px - 20), (int)(py + 4), 4.0f, skinLo);

    float reach = 20.0f + pose.poseT * 14.0f;
    Vector2 hand = { px + reach, py + 2 };
    DrawLineEx(shoulderR, hand, 6.0f, skin);
    DrawCircle((int)hand.x, (int)hand.y, 4.0f, skinLo);

    DrawLineEx(hand, (Vector2){ hand.x + 14, hand.y - 8 }, 3.0f, dagger);
    DrawLineEx((Vector2){ hand.x + 2, hand.y - 2 },
               (Vector2){ hand.x + 4, hand.y + 2 }, 4.0f, ragLo);

    DrawCircle((int)px, (int)(py - 22), 12, skin);
    DrawCircle((int)px - 4, (int)(py - 26), 6, skin);

    DrawTriangle((Vector2){ px - 11, py - 26 }, (Vector2){ px - 20, py - 34 }, (Vector2){ px - 10, py - 20 }, skin);
    DrawTriangle((Vector2){ px + 11, py - 26 }, (Vector2){ px + 20, py - 34 }, (Vector2){ px + 10, py - 20 }, skin);

    DrawCircle((int)px - 4, (int)(py - 22), 3.0f, eye);
    DrawCircle((int)px + 4, (int)(py - 22), 3.0f, eye);
    DrawCircle((int)px - 4 + (facingRight ? 1 : -1), (int)(py - 22), 1.2f, BLACK);
    DrawCircle((int)px + 4 + (facingRight ? 1 : -1), (int)(py - 22), 1.2f, BLACK);

    DrawLineEx((Vector2){ px - 5, py - 14 }, (Vector2){ px + 5, py - 14 }, 1.5f, skinLo);
    DrawTriangle((Vector2){ px - 3, py - 14 }, (Vector2){ px - 1, py - 14 }, (Vector2){ px - 2, py - 10 }, tooth);
    DrawTriangle((Vector2){ px + 1, py - 14 }, (Vector2){ px + 3, py - 14 }, (Vector2){ px + 2, py - 10 }, tooth);

    DrawCircle((int)px, (int)(py - 18), 2.5f, skinLo);
}

/* ---------- Wolf ---------- */
void CombatantDrawWolf(Vector2 pos, bool facingRight, CombatantPose pose)
{
    float bob   = sinf(pose.time * 6.0f) * 1.0f;
    float lunge = pose.poseT * 24.0f;

    float px = pos.x + (facingRight ? lunge : -lunge);
    float py = pos.y + bob;

    Color fur    = { 130, 135, 145, 255 };
    Color furHi  = { 175, 180, 190, 255 };
    Color furLo  = {  80,  85,  95, 255 };
    Color belly  = { 170, 170, 175, 255 };
    Color nose   = {  30,  30,  35, 255 };
    Color eye    = { 230, 200,  70, 255 };
    Color claw   = {  40,  40,  45, 255 };
    Color tongue = { 210,  90,  90, 255 };

    DrawEllipse((int)pos.x, (int)(pos.y + 26), 34.0f, 7.0f, (Color){ 0, 0, 0, 90 });

    float legSpacing = 18.0f;
    for (int i = 0; i < 4; i++)
    {
        float lx = px + (i < 2 ? -legSpacing : legSpacing * 0.8f);
        float ly = py + 10;
        if (i == 1) lx += 8;
        if (i == 3) lx += 8;
        DrawLineEx((Vector2){ lx, ly },
                   (Vector2){ lx + (facingRight ? 1.5f : -1.5f), ly + 18 }, 6.0f, furLo);
        DrawRectangle((int)lx - 3, (int)(ly + 16), 7, 6, furLo);
        DrawCircle((int)lx, (int)(ly + 22), 1.5f, claw);
    }

    DrawEllipse((int)px, (int)(py - 2), 26.0f, 14.0f, fur);
    DrawEllipse((int)(px - 4), (int)(py + 4), 20.0f, 6.0f, belly);

    for (int i = 0; i < 6; i++)
    {
        float furX = px - 18.0f + i * 6.0f;
        DrawLineEx((Vector2){ furX, py - 12 }, (Vector2){ furX + 1, py - 4 }, 1.0f, furHi);
    }

    DrawLineEx((Vector2){ px - 24, py - 4 }, (Vector2){ px - 40, py - 16 }, 6.0f, furLo);
    DrawCircle((int)(px - 40), (int)(py - 16), 5.0f, fur);

    Vector2 neck = { px + 18, py - 12 };
    DrawLineEx((Vector2){ px + 14, py - 6 }, neck, 10.0f, fur);

    DrawCircle((int)(neck.x + 8), (int)(neck.y - 2), 11.0f, fur);
    DrawTriangle(
        (Vector2){ neck.x + 10, neck.y - 10 },
        (Vector2){ neck.x + 26, neck.y - 2 },
        (Vector2){ neck.x + 10, neck.y + 8 }, fur);
    DrawCircle((int)(neck.x + 24), (int)(neck.y - 2), 3.5f, nose);

    DrawTriangle(
        (Vector2){ neck.x + 2, neck.y - 12 },
        (Vector2){ neck.x - 2, neck.y - 24 },
        (Vector2){ neck.x + 8, neck.y - 12 }, furLo);
    DrawTriangle(
        (Vector2){ neck.x + 6, neck.y - 12 },
        (Vector2){ neck.x + 6, neck.y - 22 },
        (Vector2){ neck.x + 12, neck.y - 12 }, furLo);
    DrawTriangle(
        (Vector2){ neck.x + 3, neck.y - 13 },
        (Vector2){ neck.x + 1, neck.y - 20 },
        (Vector2){ neck.x + 7, neck.y - 13 }, fur);

    DrawCircle((int)(neck.x + 6), (int)(neck.y - 4), 2.5f, eye);
    DrawCircle((int)(neck.x + 7), (int)(neck.y - 4), 1.0f, BLACK);

    if (pose.poseT > 0.2f)
    {
        DrawTriangle(
            (Vector2){ neck.x + 14, neck.y - 1 },
            (Vector2){ neck.x + 24, neck.y + 2 },
            (Vector2){ neck.x + 14, neck.y + 6 },
            (Color){ 60, 20, 20, 255 });
        DrawTriangle(
            (Vector2){ neck.x + 15, neck.y + 2 },
            (Vector2){ neck.x + 22, neck.y + 3 },
            (Vector2){ neck.x + 15, neck.y + 6 },
            tongue);
        DrawTriangle(
            (Vector2){ neck.x + 16, neck.y - 1 },
            (Vector2){ neck.x + 17, neck.y - 1 },
            (Vector2){ neck.x + 16, neck.y + 3 }, WHITE);
    }
}

/* ---------- Orc ---------- */
void CombatantDrawOrc(Vector2 pos, bool facingRight, CombatantPose pose)
{
    float bob   = sinf(pose.time * 4.0f) * 1.5f;
    float lunge = pose.poseT * 20.0f;

    float px = pos.x + (facingRight ? lunge : -lunge);
    float py = pos.y + bob;

    Color skin    = {  80, 140,  70, 255 };
    Color skinHi  = { 120, 180, 100, 255 };
    Color skinLo  = {  50,  95,  45, 255 };
    Color horn    = { 220, 210, 190, 255 };
    Color hornLo  = { 160, 150, 130, 255 };
    Color armor   = { 110,  70,  40, 255 };
    Color armorLo = {  70,  45,  25, 255 };
    Color metal   = { 130, 130, 135, 255 };
    Color eye     = { 220,  60,  40, 255 };
    Color tusk    = { 240, 235, 210, 255 };

    DrawEllipse((int)pos.x, (int)(pos.y + 48), 26.0f, 7.0f, (Color){ 0, 0, 0, 90 });

    DrawLineEx((Vector2){ px - 8, py + 14 }, (Vector2){ px - 12, py + 40 }, 11.0f, skinLo);
    DrawLineEx((Vector2){ px + 8, py + 14 }, (Vector2){ px + 12, py + 40 }, 11.0f, skinLo);
    DrawRectangle((int)px - 20, (int)py + 38, 14, 8, armor);
    DrawRectangle((int)px +  6, (int)py + 38, 14, 8, armor);

    DrawRectangle((int)px - 15, (int)py + 8, 30, 14, armor);
    DrawRectangle((int)px - 15, (int)py + 18, 30, 4, armorLo);

    Rectangle torso = { px - 18, py - 14, 36, 26 };
    DrawRectangleRec(torso, skin);
    DrawRectangle((int)torso.x, (int)torso.y, (int)torso.width, 4, skinHi);
    DrawRectangle((int)torso.x, (int)(torso.y + torso.height - 6), (int)torso.width, 6, skinLo);

    DrawCircle((int)px - 7, (int)(py - 4), 6.0f, skinHi);
    DrawCircle((int)px + 7, (int)(py - 4), 6.0f, skinHi);
    DrawLineEx((Vector2){ px, py - 10 }, (Vector2){ px, py + 6 }, 1.5f, skinLo);

    DrawRectangle((int)px - 18, (int)py + 6, 36, 5, armorLo);

    Vector2 shoulderL = { px - 22, py - 12 };
    Vector2 shoulderR = { px + 22, py - 12 };
    DrawCircleV(shoulderL, 9.0f, skinHi);
    DrawCircleV(shoulderR, 9.0f, skinHi);

    DrawLineEx(shoulderL, (Vector2){ px - 30, py + 4 }, 9.0f, skin);
    DrawCircle((int)(px - 30), (int)(py + 4), 5.5f, skinLo);

    float reach = 26.0f + pose.poseT * 18.0f;
    Vector2 hand = { px + reach, py - 2 };
    DrawLineEx(shoulderR, hand, 10.0f, skin);
    DrawCircle((int)hand.x, (int)hand.y, 6.0f, skinLo);

    DrawLineEx(hand, (Vector2){ hand.x + 8, hand.y - 10 }, 4.0f, armorLo);
    DrawTriangle(
        (Vector2){ hand.x + 6,  hand.y - 12 },
        (Vector2){ hand.x + 26, hand.y - 26 },
        (Vector2){ hand.x + 22, hand.y - 4 },  metal);
    DrawTriangle(
        (Vector2){ hand.x + 6,  hand.y - 12 },
        (Vector2){ hand.x + 22, hand.y - 4 },
        (Vector2){ hand.x + 8,  hand.y - 2 },  (Color){ 90, 90, 95, 255 });

    DrawCircle((int)px, (int)(py - 28), 15, skin);
    DrawRectangle((int)px - 12, (int)py - 24, 24, 12, skinLo);

    for (int s = -1; s <= 1; s += 2)
    {
        Vector2 hornBase = { px + s * 11, py - 34 };
        Vector2 hornMid  = { px + s * 20, py - 46 };
        Vector2 hornTip  = { px + s * 16, py - 60 };
        DrawLineEx(hornBase, hornMid, 7.0f, horn);
        DrawLineEx(hornMid,  hornTip, 5.0f, horn);
        DrawCircleV(hornTip, 3.0f, horn);
        DrawLineEx(hornBase, hornMid, 2.0f, hornLo);
    }

    DrawCircle((int)px - 5, (int)(py - 28), 3.0f, eye);
    DrawCircle((int)px + 5, (int)(py - 28), 3.0f, eye);
    DrawCircle((int)px - 5 + (facingRight ? 1 : -1), (int)(py - 28), 1.0f, BLACK);
    DrawCircle((int)px + 5 + (facingRight ? 1 : -1), (int)(py - 28), 1.0f, BLACK);
    DrawLineEx((Vector2){ px - 9, py - 33 }, (Vector2){ px - 2, py - 31 }, 2.0f, skinLo);
    DrawLineEx((Vector2){ px + 9, py - 33 }, (Vector2){ px + 2, py - 31 }, 2.0f, skinLo);

    DrawCircle((int)px, (int)(py - 22), 3.0f, skinLo);

    DrawLineEx((Vector2){ px - 7, py - 16 }, (Vector2){ px + 7, py - 16 },
               2.0f, (Color){ 40, 20, 20, 255 });
    DrawTriangle(
        (Vector2){ px - 6, py - 16 },
        (Vector2){ px - 3, py - 16 },
        (Vector2){ px - 5, py - 10 }, tusk);
    DrawTriangle(
        (Vector2){ px + 3, py - 16 },
        (Vector2){ px + 6, py - 16 },
        (Vector2){ px + 5, py - 10 }, tusk);

    DrawLineEx((Vector2){ px - 12, py - 36 }, (Vector2){ px - 7, py - 22 },
               1.5f, (Color){ 140, 60, 60, 255 });
}

/* ---------- Dispatcher ---------- */
void CombatantDrawEnemy(const char *name, Vector2 pos, bool facingRight, CombatantPose pose)
{
    if (!name) return;
    if      (strcmp(name, "Slime")  == 0) CombatantDrawSlime (pos, facingRight, pose);
    else if (strcmp(name, "Goblin") == 0) CombatantDrawGoblin(pos, facingRight, pose);
    else if (strcmp(name, "Wolf")   == 0) CombatantDrawWolf  (pos, facingRight, pose);
    else if (strcmp(name, "Orc")    == 0) CombatantDrawOrc   (pos, facingRight, pose);
    else                                  CombatantDrawSlime (pos, facingRight, pose);
}