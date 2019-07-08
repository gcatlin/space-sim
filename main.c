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
    char name[8];
    Color color;
} body_t;

static char tmpstr[1024];

// https://en.wikipedia.org/wiki/Astronomical_system_of_units
// https://en.wikipedia.org/wiki/Standard_gravitational_parameter
static const float G     = 6.67408e-11;   // Gravitational constant (m³ kg⁻¹ s⁻²)
static const float GMsun = 1.3271244e+20; // standard gravitational parameter (m³ s⁻²)

static const float simulation_time_step = 1.0f/240.0f; // 4.16 ms
static const float time_multiplier = 2e6;
static const float zoom = 1.5e-10;

static const int   label_font_size     = 10;
static const Color label_font_color    = WHITE;
static const Color label_shadow_color  = BLACK;
static const int   label_shadow_offset = 1;

// https://en.wikipedia.org/wiki/List_of_gravitationally_rounded_objects_of_the_Solar_System
static body_t sun = { {0}, {0}, 1.98550e+30, 5.03651473e-31, 6.95700e+08, "Sun", YELLOW };
static body_t planets[8] = {
    //pos  vel  mass (kg)   1/mass (kg⁻¹)   radius (m)   name       color
    { {0}, {0}, 3.3020e+23, 3.02846760e-24, 2.439640e+6, "Mercury", GRAY     },
    { {0}, {0}, 4.8690e+24, 2.05380982e-25, 6.051590e+6, "Venus",   GREEN    },
    { {0}, {0}, 5.9720e+24, 1.67448091e-25, 6.378100e+6, "Earth",   BLUE     },
    { {0}, {0}, 6.4191e+23, 1.55785079e-24, 3.397000e+6, "Mars",    RED      },
    { {0}, {0}, 1.8987e+27, 5.26676147e-28, 7.149268e+7, "Jupiter", ORANGE   },
    { {0}, {0}, 5.6851e+26, 1.75898401e-27, 6.026714e+7, "Saturn",  BEIGE    },
    { {0}, {0}, 8.6849e+25, 1.15142374e-26, 2.555725e+7, "Uranus",  SKYBLUE  },
    { {0}, {0}, 1.0244e+26, 9.76181179e-27, 2.476636e+7, "Neptune", DARKBLUE },
};


static inline v2 v2_add(v2 v, v2 u) { return (v2){ v.x + u.x, v.y + u.y }; }
static inline v2 v2_div(v2 v, v2 u) { return (v2){ v.x / u.x, v.y / u.y }; }
static inline v2 v2_mul(v2 v, v2 u) { return (v2){ v.x * u.x, v.y * u.y }; }
static inline v2 v2_sub(v2 v, v2 u) { return (v2){ v.x - u.x, v.y - u.y }; }
static inline v2 v2_scale(v2 v, float s) { return (v2){ v.x * s, v.y * s }; }
static inline float v2_distance(v2 v, v2 u) { return sqrtf((v.x-u.x)*(v.x-u.x) + (v.y-u.y)*(v.y-u.y)); }
static inline float v2_magnitude(v2 v) { return sqrtf(v.x*v.x + v.y*v.y); }

static void init_bodies(void)
{
    for (int i = 0; i < 8; i++) {
	planets[i].position.y = 0;
	planets[i].velocity.x = 0;
    }
    planets[0].position.x = 5.7909175e+10; planets[0].velocity.y = -4.7870e+4;
    planets[1].position.x = 1.0820893e+11; planets[1].velocity.y = -3.5020e+4;
    planets[2].position.x = 1.4959789e+11; planets[2].velocity.y = -2.9786e+4;
    planets[3].position.x = 2.2793664e+11; planets[3].velocity.y = -2.4077e+4;
    planets[4].position.x = 7.7841201e+11; planets[4].velocity.y = -1.3070e+4;
    planets[5].position.x = 1.4267254e+12; planets[5].velocity.y = -9.6900e+3;
    planets[6].position.x = 2.8709722e+12; planets[6].velocity.y = -6.8100e+3;
    planets[7].position.x = 4.4982529e+12; planets[7].velocity.y = -5.4300e+3;
}

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
    for (int i = 0; i < 8; i++) {
	simulate_planet(&planets[i], dt);
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

static void DrawBodyLabel(body_t *body, v2 pos, float box_size, Color box_color)
{
    // Box and cross shapes
    float half = box_size/2.0f;
    DrawRectangleLines(pos.x - half, pos.y - half, box_size, box_size, box_color);
    DrawCross(pos, box_size / 8.0f, box_color);

    // Text
    DrawBodyLabelText(body, body->name, pos);
}

static void DrawBodyOrbit(v2 parent_pos, v2 child_pos, Color color)
{
    const float distance = v2_distance(parent_pos, child_pos);
    DrawCircleLines(parent_pos.x, parent_pos.y, distance, color);
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
	    for (int i = 0; i < 8; i++) {
		planet_pos[i] = v2_add(center, v2_scale(planets[i].position, zoom));
	    }

	    // Draw bodies
	    DrawCircleV(sun_pos, sun.radius * zoom, sun.color);
	    for (int i = 0; i < 8; i++) {
		DrawCircleV(planet_pos[i], planets[i].radius * zoom, planets[i].color);
	    }

	    // Draw extra stuff
	    DrawBodyLabel(&sun, sun_pos, 60.0f, DARKGREEN);
	    for (int i = 0; i < 8; i++) {
		DrawBodyLabel(&planets[i], planet_pos[i], 15.0f, DARKGREEN);
		DrawBodyOrbit(sun_pos, planet_pos[i], planets[i].color);
	    }

	    // DrawBodyLabelText(earth, "Earth", (v2){20, screenHeight-80});
	    // draw velocity vector(s)?
	}
	EndDrawing();
    }

    CloseWindow();

    return 0;
}
