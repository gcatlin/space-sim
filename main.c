#include <math.h>    // sqrtf
#include <stdio.h>   // sprintf
#include <stdbool.h> // bool

#include <raylib.h>  // v2.5.0

#include "ringbuf.c"

#define SOFTENING 1e-9f

typedef Vector2 v2;

typedef struct {
    int id;
    v2 position;
    v2 velocity;
    float mass;
    float _mass; // inverse mass
    float radius;
    float distance;
    float GM; // gravitational parameter (m³ s⁻²)
    char name[8];
    int parent_id;
    Color color;
} body_t;

static char tmpstr[1024];

// https://en.wikipedia.org/wiki/Astronomical_system_of_units
// https://en.wikipedia.org/wiki/Standard_gravitational_parameter
// static const float G     = 6.67408e-11;   // Gravitational constant (m³ kg⁻¹ s⁻²)
// static const float GMsun = ; // standard gravitational parameter

static const Color background = { 5, 5, 5, 255 };
static const int   label_font_size     = 10;
static const Color label_font_color    = WHITE;
static const Color label_shadow_color  = BLACK;

// https://en.wikipedia.org/wiki/List_of_gravitationally_rounded_objects_of_the_Solar_System
// https://en.wikipedia.org/wiki/Standard_gravitational_parameter
static body_t sun = { 0, {0}, {0}, 1.98550e+30, 5.03651473e-31, 6.95700e+08, 1.3271244e+20, 1.3271244e+20, "Sun", 0, YELLOW };
static body_t planets[8] = {
    //id pos vel  mass (kg)   1/mass (kg⁻¹)   radius (m)   distance (m)   GM (m³ s⁻²)  name       parent color
    {0, {0}, {0}, 3.3020e+23, 3.02846760e-24, 2.439640e+6, 5.7909175e+10, 0,               "Mercury", 0,     GRAY     },
    {1, {0}, {0}, 4.8690e+24, 2.05380982e-25, 6.051590e+6, 1.0820893e+11, 0,               "Venus",   0,     GREEN    },
    {2, {0}, {0}, 5.9720e+24, 1.67448091e-25, 6.378100e+6, 1.4959789e+11, 3.986004418e+14, "Earth",   0,     BLUE     },
    {3, {0}, {0}, 6.4191e+23, 1.55785079e-24, 3.397000e+6, 2.2793664e+11, 0,               "Mars",    0,     RED      },
    {4, {0}, {0}, 1.8987e+27, 5.26676147e-28, 7.149268e+7, 7.7841201e+11, 0,               "Jupiter", 0,     ORANGE   },
    {5, {0}, {0}, 5.6851e+26, 1.75898401e-27, 6.026714e+7, 1.4267254e+12, 0,               "Saturn",  0,     BEIGE    },
    {6, {0}, {0}, 8.6849e+25, 1.15142374e-26, 2.555725e+7, 2.8709722e+12, 0,               "Uranus",  0,     SKYBLUE  },
    {7, {0}, {0}, 1.0244e+26, 9.76181179e-27, 2.476636e+7, 4.4982529e+12, 0,               "Neptune", 0,     DARKBLUE },
};

static body_t moons[1] = {
    { 0, {0}, {0}, 7.3472e+22, 1.36097508e-23, 1, 3.84402e+8, 4.9048695e+12, "Moon", 2, GRAY },
};


#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define clamp(x, min, max) (((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x))
#define countof(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

static inline v2 v2_add(v2 v, v2 u) { return (v2){ v.x + u.x, v.y + u.y }; }
// static inline v2 v2_div(v2 v, v2 u) { return (v2){ v.x / u.x, v.y / u.y }; }
// static inline v2 v2_mul(v2 v, v2 u) { return (v2){ v.x * u.x, v.y * u.y }; }
// static inline v2 v2_sub(v2 v, v2 u) { return (v2){ v.x - u.x, v.y - u.y }; }
static inline v2 v2_scale(v2 v, float s) { return (v2){ v.x * s, v.y * s }; }
static inline float v2_distance(v2 v, v2 u) { return sqrtf((v.x-u.x)*(v.x-u.x) + (v.y-u.y)*(v.y-u.y)); }
static inline float v2_magnitude(v2 v) { return sqrtf(v.x*v.x + v.y*v.y); }

static void init_bodies(void)
{
    for (int i = 0; i < 8; i++) {
        planets[i].position.x = planets[i].distance;
        planets[i].position.y = 0;
        planets[i].velocity.x = 0;
    }
    planets[0].velocity.y = -4.7870e+4;
    planets[1].velocity.y = -3.5020e+4;
    planets[2].velocity.y = -2.9786e+4;
    planets[3].velocity.y = -2.4077e+4;
    planets[4].velocity.y = -1.3070e+4;
    planets[5].velocity.y = -9.6900e+3;
    planets[6].velocity.y = -6.8100e+3;
    planets[7].velocity.y = -5.4300e+3;

    moons[0].position.x = moons[0].distance;
    moons[0].position.y = moons[0].distance;
    moons[0].velocity.x = 0;
    moons[0].velocity.y = -1.022e+3;

}

static v2 calculate_force(const body_t *restrict parent, const body_t *restrict child)
{
    // Calculate force
    float dx = parent->position.x - child->position.x;
    float dy = parent->position.y - child->position.y;
    float dist = v2_distance(parent->position, child->position);
    float invDist = 1.0f / (dist + SOFTENING);
    float invDist3 = invDist * invDist * invDist;
    float F = parent->GM * invDist3;
    return (v2){ F*dx, F*dy};
}

static void simulate_planet(const body_t *restrict star, body_t *restrict planet, float dt)
{

    v2 F = calculate_force(star, planet);
    planet->velocity.x += dt * F.x;
    planet->velocity.y += dt * F.y;
    planet->position.x += dt * planet->velocity.x;
    planet->position.y += dt * planet->velocity.y;
}

static void simulate_moon(const body_t *restrict star, body_t *restrict moon, float dt)
{
    // TODO account for sibling moons
    // v2 Fstar = calculate_force(star, moon);
    v2 Fstar = {0, 0};
    v2 Fplanet = calculate_force(&planets[moon->parent_id], moon);
    moon->velocity.x += dt * (Fstar.x + Fplanet.x);
    moon->velocity.y += dt * (Fstar.y + Fplanet.y);
    moon->position.x += dt * moon->velocity.x;
    moon->position.y += dt * moon->velocity.y;
    // treat the moon like a planet (influence of sun)
    // then get impact of the parent planet
}

static void simulate(float dt)
{
    for (int i = 0; i < 8; i++) {
        simulate_planet(&sun, &planets[i], dt);
    }
    for (int i = 0; i < 1; i++) {
        simulate_moon(&sun, &moons[i], dt);
    }
}

static void DrawCross(v2 center, float radius, Color color)
{
    DrawLine(center.x - radius, center.y, center.x + radius, center.y, color);
    DrawLine(center.x, center.y - radius, center.x, center.y + radius, color);
}

static void DrawTextWithShadow(const char *text, int posX, int posY, int font_size, Color fg, Color bg)
{
    DrawText(text, posX + 1, posY + 1, font_size, bg);
    DrawText(text, posX, posY, font_size, fg);
}

static void DrawBodyLabelText(body_t *body, char *name, v2 pos)
{
    const float dist = v2_distance(sun.position, body->position) / 1000;
    const float speed = v2_magnitude(body->velocity) / 1000;
    sprintf(tmpstr, "%s \n distance: %.1f km \n speed: %.1f km/s", name, dist, speed);

    const float x_offset = 0.0f;
    const float y_offset = 20.0f;
    DrawTextWithShadow(
            tmpstr, pos.x + x_offset, pos.y + y_offset,
            label_font_size, label_font_color, label_shadow_color);
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
    // Tests
    rbuf_test();

    // Initialization
    const char *title = "Space Sim";
    int screenWidth = 1280;
    int screenHeight = 800;

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, title);
    SetTargetFPS(60);

    init_bodies();

    bool paused = false;
    bool draw_fps = true;
    bool draw_bar = true;
    bool draw_labels = true;
    bool draw_planets = true;
    bool draw_moons = true;

    const float simulation_time_step = 1.0f/240.0f; // 4.16 ms
    const float time_multiplier = 2e6;
    float zoom = 1.5e-10;

    // TODO fill with chart-width zeroes
    float *frame_times = {0}; rbuf_init(frame_times, 256); 
    float *mouse_wheel_moves = {0}; rbuf_init(mouse_wheel_moves, 256);

    // Main game loop
    while (!WindowShouldClose()) {
        // Handle input
        if (IsKeyPressed(KEY_SPACE)) { paused = !paused; }
        if (IsKeyPressed(KEY_B)) { draw_bar = !draw_bar; }
        if (IsKeyPressed(KEY_F)) { draw_fps = !draw_fps; }
        if (IsKeyPressed(KEY_L)) { draw_labels = !draw_labels; }
        if (IsKeyPressed(KEY_M)) { draw_moons = !draw_moons; }
        if (IsKeyPressed(KEY_P)) { draw_planets = !draw_planets; }
        if (IsKeyPressed(KEY_R)) { init_bodies(); }

        if (IsKeyPressed(KEY_X)) { zoom *= 1.5f; }
        if (IsKeyPressed(KEY_Z)) { zoom /= 1.5f; }

        int mouse_wheel_move = GetMouseWheelMove();
        if (mouse_wheel_move) {
            // zoom = clamp(zoom + mouse_wheel_move, 1e-12, 1);
        }

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
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();

            ClearBackground(background);

            // Scale and Translate
            const v2 center = { screenWidth/2, screenHeight/2 };
            const v2 sun_pos = v2_add(center, v2_scale(sun.position, zoom));
            v2 planet_pos[8];
            for (int i = 0; i < 8; i++) {
                planet_pos[i] = v2_add(center, v2_scale(planets[i].position, zoom));
            }
            v2 moon_pos[1];
            for (int i = 0; i < 1; i++) {
                moon_pos[i] = v2_add(center, v2_scale(moons[i].position, zoom));
            }

            // Sun
            DrawCircleV(sun_pos, sun.radius * zoom, sun.color);
            if (draw_labels) {
                DrawBodyLabel(&sun, sun_pos, 60.0f, DARKGREEN);
            }

            // Planets
            if (draw_planets) {
                for (int i = 0; i < 8; i++) {
                    DrawCircleV(planet_pos[i], planets[i].radius * zoom, planets[i].color);
                }
                if (draw_labels) {
                    for (int i = 0; i < 8; i++) {
                        DrawBodyLabel(&planets[i], planet_pos[i], 15.0f, DARKGREEN);
                        DrawBodyOrbit(sun_pos, planet_pos[i], planets[i].color);
                    }
                }
            }

            // Moons
            if (draw_moons) {
                for (int i = 0; i < 1; i++) {
                    DrawCircleV(moon_pos[i], planets[i].radius * zoom, planets[i].color);
                }
                if (draw_labels) {
                    for (int i = 0; i < 1; i++) {
                        DrawBodyLabel(&moons[i], moon_pos[i], 15.0f, DARKBLUE);
                        DrawBodyOrbit(planet_pos[moons[i].parent_id], moon_pos[i], moons[i].color);
                    }
                }
            }

            // Solar system bar
            if (draw_bar) {
                const Color bar_color = { 10, 10, 10, 255 };
                const float bar_height = 40.0f;
                DrawRectangle(0, screenHeight - bar_height, screenWidth, bar_height, bar_color);
                const body_t jupiter = planets[4];
                const body_t neptune = planets[7];
                const float width_scale = neptune.distance / screenWidth * 1.01f;
                const float height_scale = (jupiter.radius * 2.0f / bar_height) * 1.2f;
                const float posY = screenHeight - (bar_height / 2.0f);
                DrawCircle(0, posY, max(sun.radius / height_scale, 10.0f), Fade(sun.color, 0.25f));
                for (int i = 0; i < 8; i++) {
                    const int posX = planets[i].distance / width_scale;
                    const int radius = planets[i].radius / height_scale;
                    DrawCircle(posX, posY, max(radius, 2.0f), planets[i].color);
                }
            }

            sprintf(tmpstr, "Zoom: %.1e \n Mouse wheel: %d", zoom, mouse_wheel_move);
            DrawTextWithShadow(tmpstr, 10, screenHeight - 100, 10, WHITE, BLACK);

            // FPS
            // TODO create a chart_t struct and call DrawChart(chart, data);
            // TODO improve padding of text labels (axis,
            // TODO improve constant stuff: 16ms, 20ms scale
            rbuf_push(frame_times, GetFrameTime()*1000.0f);
            if (draw_fps) {
                DrawFPS(10, 10);
                const Color fps_color = Fade(LIME, 0.75f);
                const Color fps_color_slow = Fade(RED, 0.75f);
                const Color fps_label_color = Fade(WHITE, 0.75f);
                const int fps_width = 152;
                const int fps_height = 42;
                const float fps_x_scale = 1.0f;
                const float fps_y_scale = ((float)(fps_height - 2)/20.0f);
                const v2 fps_pos = {100, 10};
                const int fps_xpos = fps_pos.x + fps_width - 1;
                const int fps_ypos = fps_pos.y + fps_height - 1;

                // Target line
                const int fps_target_height = fps_ypos - round(16.666666667f*fps_y_scale);
                DrawLine(fps_pos.x + 1, fps_target_height, fps_pos.x + fps_width - 1, fps_target_height, MAGENTA);

                // Draw data left to right
                for (int i = 1, max = min(fps_width-2, rbuf_len(frame_times)); i <= max; i++) {
                    const int x = fps_xpos - round((float)i*fps_x_scale);
                    const int y = fps_ypos;
                    const float frame_time = rbuf_get(frame_times, -i);
                    const int height = clamp(round(frame_time*fps_y_scale), 0, fps_height - 2);
                    const Color color = (frame_time > 16.67f ? fps_color_slow : fps_color);
                    DrawLine(x, y, x, y - height, color);
                }

                // Border
                DrawRectangleLines(fps_pos.x, fps_pos.y, fps_width, fps_height, LIME);

                // Labels
                DrawText("0 ms", fps_xpos + 4, fps_pos.y + fps_height - 5, 8, fps_label_color);
                DrawText("20 ms", fps_xpos + 4, fps_pos.y - 5, 8, fps_label_color);
            }

            // DrawBodyLabelText(earth, "Earth", (v2){20, screenHeight-80});
            // draw velocity vector(s)?
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
