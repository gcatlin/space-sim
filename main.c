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

// https://en.wikipedia.org/wiki/Astronomical_system_of_units
// https://en.wikipedia.org/wiki/Standard_gravitational_parameter
// https://arxiv.org/pdf/1510.07674.pdf
static const float G      = 6.67408e-11; // Gravitational constant (m³ kg⁻¹ s⁻²)
static const float GMsun  = 1.3271244e+20; // standard gravitational parameter (m³ s⁻²)
static const float Msun   = 1.98550e+30; // mass (kg)
static const float Mearth = 5.97200e+24; // mass (kg)
static const float Rsun   = 6.95700e+08; // radius (m)
static const float Rearth = 6.37810e+06; // radius (m)
static const float Searth = 2.97860e+04; // orbital speed (m s⁻¹)
static const float Dearth = 1.49600e+11; // distance to sun (m)

static const float simulation_time_step = 1.0f/240.0f; // 4.16 ms
static const float time_multiplier = 2e6;
static const float zoom = 2.5e-9;

static const int label_font_size = 10;
static const Color label_font_color = WHITE;
static const Color label_shadow_color = BLACK;
static const int label_shadow_offset = 1;

static body_t sun;
static body_t mercury;
static body_t venus;
static body_t earth;
static body_t mars;
static body_t jupiter;
static body_t saturn;
static body_t uranus;
static body_t neptune;
static body_t *planets[8] = {
    &mercury, &venus, &earth, &mars,
    &jupiter, &saturn, &uranus, &neptune
};

static inline v2 v2_add(v2 v, v2 u) { return (v2){ v.x + u.x, v.y + u.y }; }
static inline v2 v2_div(v2 v, v2 u) { return (v2){ v.x / u.x, v.y / u.y }; }
static inline v2 v2_mul(v2 v, v2 u) { return (v2){ v.x * u.x, v.y * u.y }; }
static inline v2 v2_sub(v2 v, v2 u) { return (v2){ v.x - u.x, v.y - u.y }; }
static inline v2 v2_scale(v2 v, float s) { return (v2){ v.x * s, v.y * s }; }
static inline float v2_distance(v2 v, v2 u) { return sqrtf((v.x-u.x)*(v.x-u.x) + (v.y-u.y)*(v.y-u.y)); }
static inline float v2_magnitude(v2 v) { return sqrtf(v.x*v.x + v.y*v.y); }

static void simulate_planet(body_t *planet, float dt)
{
    // Calculate force
    float dx = sun.position.x - planet->position.x;
    float dy = sun.position.y - planet->position.y;
    float dist = v2_distance(sun.position, planet->position);
    float invDist = 1.0f / (dist + SOFTENING);
    float invDist3 = invDist * invDist * invDist;
    float F = GMsun * invDist3;
    float Fx = F * dx;
    float Fy = F * dy;

    // Apply force
    planet->velocity.x += dt * Fx;
    planet->velocity.y += dt * Fy;
    planet->position.x += dt * planet->velocity.x;
    planet->position.y += dt * planet->velocity.y;
}

static void simulate(float dt)
{
    for (int i = 0; i < 4; i++) {
	simulate_planet(planets[i], dt);
    }
}

static void DrawCross(v2 center, float radius, Color color)
{
    v2 c = center; float r = radius;
    DrawLine(c.x - r, c.y,     c.x + r, c.y,     color);
    DrawLine(c.x,     c.y - r, c.x,     c.y + r, color);
}

static void DrawBodyLabelText(body_t *body, char *name, v2 pos)
{
    const float dist = v2_distance(sun.position, body->position) / 1000;
    const float speed = v2_magnitude(body->velocity) / 1000;
    sprintf(tmpstr, "%s \n distance: %.1f km \n speed: %.1f km/s", name, dist, speed);

    const float x_offset = 0.0f;
    const float y_offset = 20.0f;
    const int lfs = label_font_size; const int lso = label_shadow_offset;
    const Color lfc = label_font_color; const Color lsc = label_shadow_color;
    DrawText(tmpstr, pos.x + x_offset + lso, pos.y + y_offset + lso, lfs, lsc);
    DrawText(tmpstr, pos.x + x_offset,       pos.y + y_offset,       lfs, lfc);
}

static void DrawBodyLabel(body_t *body, char *name, v2 pos, float box_size, Color box_color)
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
    const float distance = v2_distance(parent_pos, child_pos);
    DrawCircleLines(parent_pos.x, parent_pos.y, distance, color);
}

static void init_bodies(void)
{
    sun     = (body_t){{0, 0},      {0, 0},       Msun,   1.0f/Msun,   Rsun};
    mercury = (body_t){{0, Dearth/3}, {Searth, 0}, Mearth, 1.0f/Mearth, Rearth};
    venus   = (body_t){{Dearth/2, 0}, {0, -Searth}, Mearth, 1.0f/Mearth, Rearth};
    earth   = (body_t){{-Dearth, 0}, {0, -Searth}, Mearth, 1.0f/Mearth, Rearth};
    mars    = (body_t){{0, -Dearth*1.2}, {Searth, 0}, Mearth, 1.0f/Mearth, Rearth};
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

    init_bodies();

    // Main game loop
    bool paused = false;
    while (!WindowShouldClose()) {
        // Handle input
        if (IsKeyPressed(KEY_SPACE)) { paused = !paused; }
        if (IsKeyPressed(KEY_R)) { init_bodies(); }

        // Update
        if (!paused) {
            simulate(simulation_time_step * time_multiplier);
            simulate(simulation_time_step * time_multiplier);
            simulate(simulation_time_step * time_multiplier);
            simulate(simulation_time_step * time_multiplier);
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
            v2 sun_pos = v2_add(center, v2_scale(sun.position, zoom));
	    v2 planet_pos[8];
	    for (int i = 0; i < 4; i++) {
		planet_pos[i] = v2_add(center, v2_scale(planets[i]->position, zoom));
	    }

            // Draw bodies
            DrawCircleV(sun_pos,   sun.radius   * zoom, YELLOW);
	    for (int i = 0; i < 4; i++) {
		DrawCircleV(planet_pos[i], planets[i]->radius * zoom, BLUE);
	    }

            // Draw extra stuff
            DrawBodyLabel(&sun, "Sun", sun_pos, 60.0f, DARKGREEN);
	    for (int i = 0; i < 4; i++) {
		char name[1024];
		sprintf(name, "Planet %d", i);
		DrawBodyLabel(planets[i], name, planet_pos[i], 15.0f, DARKGREEN);
		// DrawBodyOrbit(sun_pos, planet_pos[i], DARKGREEN);
	    }

            // DrawBodyLabelText(earth, "Earth", (v2){20, screenHeight-80});
            // draw velocity vector(s)?
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
