#include <math.h>    // sqrtf
#include <stdio.h>   // sprintf
#include <stdbool.h> // bool

#include <raylib.h>  // v2.5.0

#define BACKGROUND (Color){ 5, 5, 5, 255 }
#define SOFTENING 1e-9f

typedef Vector2 v2;

typedef struct {
    v2 position;
    v2 velocity;
    float mass;
    float _mass; // inverse mass
    float radius;
} body_t;

static char tmpstr[1024];

// https://arxiv.org/pdf/1510.07674.pdf
// static const m3_kgs2 Gearth    = { 6.67408e-11 }; //
static const float G      = 6.6730e-11; // Gravitational constant (m³ kg⁻¹ s⁻²)
static const float Msun   = 1.9855e+30;  // mass (kg)
static const float Mearth = 5.9720e+24;  // mass (kg)
static const float Rsun   = 6.9570e+08;   // radius (m)
static const float Rearth = 6.3781e+06;   // radius (m)
static const float Searth = 2.9786e+04;   // orbital speed (m s⁻¹)
static const float Dearth = 1.4960e+11;   // distance to sun (m)

static const float time_multiplier = 2e6;
static const float zoom = 2.5e-9;
static const float time_step = 1.0f/240.0f; // 4.16 ms

static body_t sun;
static body_t earth;

static void simulate(float dt)
{
    // Calculate force
    float dx = sun.position.x - earth.position.x;
    float dy = sun.position.y - earth.position.y;
    float distSqr = dx*dx + dy*dy + SOFTENING;
    float invDist = 1.0f / sqrtf(distSqr);
    float invDist3 = invDist * invDist * invDist;
    float F = G * sun.mass * invDist3;
    float Fx = F * dx;
    float Fy = F * dy;

    // Apply force
    earth.velocity.x += dt * Fx;
    earth.velocity.y += dt * Fy;
    earth.position.x += dt * earth.velocity.x;
    earth.position.y += dt * earth.velocity.y;
}

static inline v2 v2_add(v2 v, v2 u) { return (v2){ v.x + u.x, v.y + u.y }; }
static inline v2 v2_div(v2 v, v2 u) { return (v2){ v.x / u.x, v.y / u.y }; }
static inline v2 v2_mul(v2 v, v2 u) { return (v2){ v.x * u.x, v.y * u.y }; }
static inline v2 v2_sub(v2 v, v2 u) { return (v2){ v.x - u.x, v.y - u.y }; }
static inline v2 v2_scale(v2 v, float s) { return (v2){ v.x * s, v.y * s }; }

static void DrawCross(v2 center, float radius, Color color)
{
    v2 c = center; float r = radius;
    DrawLine(c.x - r, c.y,     c.x + r, c.y,     color);
    DrawLine(c.x,     c.y - r, c.x,     c.y + r, color);
}

static void DrawBodyLabelText(body_t body, char *name, v2 pos)
{
    sprintf(tmpstr, "%s \n pos: %.1f, %.1f (km) \n vel: %.1f, %.1f (km/s)", name,
            body.position.x / 1000, body.position.y / 1000,
            body.velocity.x / 1000, body.velocity.y / 1000);
    const float x_offset = 0;
    const float y_offset = 20.0f;
    const int font_size = 11;
    const Color shadow_color = BLACK;
    const Color font_color = WHITE;
    DrawText(tmpstr, pos.x + x_offset + 1, pos.y + y_offset + 1, font_size, shadow_color);
    DrawText(tmpstr, pos.x + x_offset,     pos.y + y_offset,     font_size, font_color);
}

static void DrawBodyLabel(body_t body, char *name, v2 pos, float box_size, Color box_color)
{
    // Box and cross shapes
    float half = box_size/2.0f;
    DrawRectangleLines(pos.x - half, pos.y - half, box_size, box_size, box_color);
    DrawCross(pos, box_size / 8.0f, box_color);

    // Text
    DrawBodyLabelText(body, name, pos);
}

static void DrawBodyOrbit(v2 parent_pos, v2 child_pos, Color color)
{
    float dx = parent_pos.x - child_pos.x;
    float dy = parent_pos.y - child_pos.y;
    float dist = sqrtf(dx*dx + dy*dy + SOFTENING);
    DrawCircleLines(parent_pos.x, parent_pos.y, dist, color);
}

static void reset_bodies(void)
{
    sun   = (body_t){{0, 0},      {0, 0},       Msun,   1.0f/Msun,   Rsun};
    earth = (body_t){{Dearth, 0}, {0, -Searth}, Mearth, 1.0f/Mearth, Rearth};
}

int main(void)
{
    // Initialization
    const char *title = "Space Sim";
    int screenWidth = 1280;
    int screenHeight = 800;

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, title);
    SetTargetFPS(60);

    reset_bodies();

    // Main game loop
    bool paused = false;
    while (!WindowShouldClose()) {
        // Handle input
        if (IsKeyPressed(KEY_SPACE)) { paused = !paused; }
        if (IsKeyPressed(KEY_R)) { reset_bodies(); }

        // Update
        if (!paused) {
            simulate(time_step * time_multiplier);
            simulate(time_step * time_multiplier);
            simulate(time_step * time_multiplier);
            simulate(time_step * time_multiplier);
        }

        // Render
        BeginDrawing();
        {
            ClearBackground(BACKGROUND);
            DrawFPS(10, 10);

            // Scale and Translate
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
            v2 center = { screenWidth/2, screenHeight/2 };
            v2 sun_pos   = v2_add(center, v2_scale(sun.position, zoom));
            v2 earth_pos = v2_add(center, v2_scale(earth.position, zoom));

            // Draw bodies
            DrawCircleV(sun_pos,   sun.radius   * zoom, YELLOW);
            DrawCircleV(earth_pos, earth.radius * zoom, BLUE);

            // Draw extra stuff
            DrawBodyLabel(sun,   "Sun",   sun_pos,   60.0f, DARKGREEN);
            DrawBodyLabel(earth, "Earth", earth_pos, 15.0f, DARKGREEN);
            DrawBodyOrbit(sun_pos, earth_pos, DARKGREEN);

            DrawBodyLabelText(earth, "Earth", (v2){20, screenHeight-80});
            // draw velocity vector(s)?
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
