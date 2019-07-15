#include <math.h>    // sqrtf
#include <stdio.h>   // sprintf
#include <stdbool.h> // bool

#include <raylib.h>  // v2.5.0

#include "ringbuf.c"

#define SOFTENING 1e-9f

typedef Vector2 v2;

typedef enum {
    Star   = 0,
    Planet = 1,
    Moon   = 2,
} body_type_t;

typedef struct {
    int         id;
    int         primary_id;
    body_type_t type;
    v2          position; // (m)
    v2          velocity; // (m s⁻²)
    float       mass;     // (kg)
    float       _mass;    // inverse mass (kg⁻¹)
    float       radius;   // (m)
    float       distance; // distance from primary (m)
    float       speed;    // orbital speed (m s⁻²)
    float       GM;       // standard gravitational parameter (m³ s⁻²)
    char        name[8];
    Color       color;
} body_t;

static char tmpstr[1024];
static const Color background         = { 5, 5, 5, 255 };
static const int   label_font_size    = 10;
static const Color label_font_color   = WHITE;
static const Color label_shadow_color = BLACK;

// https://en.wikipedia.org/wiki/Standard_gravitational_parameter
// https://en.wikipedia.org/wiki/List_of_gravitationally_rounded_objects_of_the_Solar_System
// https://en.wikipedia.org/wiki/Moon
// https://en.wikipedia.org/wiki/Moons_of_Jupiter
static const int num_bodies = 10;
static body_t bodies[num_bodies] = {
    //id p type pos vel  mass (kg)   1/mass (kg⁻¹)   radius (m)   distance (m)   speed      GM (m³ s⁻²)   name       color
    {0, -1, 0, {0}, {0}, 1.9855e+30, 5.03651473e-31, 6.955700e+8, 0,             0,         1.327124e+20, "Sun",     YELLOW   },

    {1,  0, 1, {0}, {0}, 3.3020e+23, 3.02846760e-24, 2.439640e+6, 5.7909175e+10, 4.7870e+4, 2.203200e+13, "Mercury", GRAY     },
    {2,  0, 1, {0}, {0}, 4.8690e+24, 2.05380982e-25, 6.051590e+6, 1.0820893e+11, 3.5020e+4, 3.248590e+14, "Venus",   GREEN    },
    {3,  0, 1, {0}, {0}, 5.9720e+24, 1.67448091e-25, 6.378100e+6, 1.4959789e+11, 2.9786e+4, 3.986004e+14, "Earth",   BLUE     },
    {4,  0, 1, {0}, {0}, 6.4191e+23, 1.55785079e-24, 3.397000e+6, 2.2793664e+11, 2.4077e+4, 4.282837e+13, "Mars",    RED      },
    {5,  0, 1, {0}, {0}, 1.8987e+27, 5.26676147e-28, 7.149268e+7, 7.7841201e+11, 1.3070e+4, 1.266865e+17, "Jupiter", ORANGE   },
    {6,  0, 1, {0}, {0}, 5.6851e+26, 1.75898401e-27, 6.026714e+7, 1.4267254e+12, 9.6900e+3, 3.793119e+16, "Saturn",  BEIGE    },
    {7,  0, 1, {0}, {0}, 8.6849e+25, 1.15142374e-26, 2.555725e+7, 2.8709722e+12, 6.8100e+3, 5.793939e+15, "Uranus",  SKYBLUE  },
    {8,  0, 1, {0}, {0}, 1.0244e+26, 9.76181179e-27, 2.476636e+7, 4.4982529e+12, 5.4300e+3, 6.836529e+15, "Neptune", DARKBLUE },

    {9,  3, 2, {0}, {0}, 7.3472e+22, 1.36097508e-23, 1.737100e+3, 3.8443990e+08, 1.0220e+3, 4.904866e+12, "Moon",    GRAY     },
    // Deimos
    // Phobos
    // Io
    // Europa
    // Ganymede
    // Callisto
};
static const body_t *sun     = &bodies[0];
static const body_t *planets = &bodies[1];
static const int num_planets = 8;

static const float box_sizes[3] = {
    [Star]   = 60.0f,
    [Planet] = 30.0f,
    [Moon]   = 15.0f,
};

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define clamp(x, min, max) (((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x))
#define countof(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

static inline v2 v2_add(v2 v, v2 u) { return (v2){ v.x + u.x, v.y + u.y }; }
// static inline v2 v2_div(v2 v, v2 u) { return (v2){ v.x / u.x, v.y / u.y }; }
// static inline v2 v2_mul(v2 v, v2 u) { return (v2){ v.x * u.x, v.y * u.y }; }
static inline v2 v2_sub(v2 v, v2 u) { return (v2){ v.x - u.x, v.y - u.y }; }
static inline v2 v2_scale(v2 v, float s) { return (v2){ v.x * s, v.y * s }; }
static inline float v2_distance(v2 v, v2 u) { return sqrtf((v.x-u.x)*(v.x-u.x) + (v.y-u.y)*(v.y-u.y)); }
static inline float v2_magnitude(v2 v) { return sqrtf(v.x*v.x + v.y*v.y); }

static void init_bodies(void)
{
    for (int i = 0; i < num_bodies; i++) {
        body_t *b = &bodies[i];
        float xpos = b->distance;
        if (b->primary_id) {
            xpos += bodies[b->primary_id].distance;
        }
        b->position.x = xpos;
        b->position.y = 0;
        b->velocity.x = 0;
        b->velocity.y = -b->speed;
    }
}

static v2 calculate_force(const body_t *restrict primary, const body_t *restrict body)
{
    float dist = v2_distance(primary->position, body->position);
    float invDist = 1.0f / (dist + SOFTENING);
    float invDist3 = invDist * invDist * invDist;
    float F = primary->GM * invDist3;
    return v2_scale(v2_sub(primary->position, body->position), F);
}

static void simulate_body(body_t *body, float dt)
{
    // TODO account for sibling bodies?
    v2 F = {0, 0};
    int primary_id = body->primary_id;
    while (primary_id != -1) {
        const body_t *primary = &bodies[primary_id];
        F = v2_add(F, calculate_force(primary, body));
        primary_id = -1;
        // primary_id = primary->primary_id;
    }
    body->velocity.x += dt * F.x;
    body->velocity.y += dt * F.y;
    body->position.x += dt * body->velocity.x;
    body->position.y += dt * body->velocity.y;
}

static void simulate(float dt)
{
    for (int i = 0; i < num_bodies; i++) {
        simulate_body(&bodies[i], dt);
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
    const float dist = v2_distance(sun->position, body->position) / 1000;
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

    // Flags
    bool paused = true;
    bool draw_fps = true;
    bool draw_bar = true;
    bool draw_labels = true;
    bool draw_bodies = true;

    const float simulation_time_step = 1.0f/240.0f; // 4.16 ms
    const float time_multiplier = 2e6;
    float zoom = 1.5e-9;
    v2 pan = {0, 0};
    // int selected_body = 0;

    // TODO fill with chart-width zeroes
    float *frame_times = {0}; rbuf_init(frame_times, 256);
    float *mouse_wheel_moves = {0}; rbuf_init(mouse_wheel_moves, 256);

    // Main game loop
    while (!WindowShouldClose()) {
        // Handle input
        if (IsKeyPressed(KEY_SPACE))      { paused       = !paused; }
        if (IsKeyDown(KEY_B))             { draw_bar     = !draw_bar; }
        if (IsKeyDown(KEY_F))             { draw_fps     = !draw_fps; }
        if (IsKeyDown(KEY_L))             { draw_labels  = !draw_labels; }
        if (IsKeyDown(KEY_R))             { init_bodies(); }
        if (IsKeyDown(KEY_X))             { zoom *= 1.25f; }
        if (IsKeyDown(KEY_Z))             { zoom *= 0.80f; }
        if (IsKeyDown(KEY_UP))            { pan.y += 20.0f; }
        if (IsKeyDown(KEY_DOWN))          { pan.y -= 20.0f; }
        if (IsKeyDown(KEY_LEFT))          { pan.x += 20.0f; }
        if (IsKeyDown(KEY_RIGHT))         { pan.x -= 20.0f; }
        // if (IsKeyDown(KEY_LEFT_BRACKET))  { selected_body = (num_bodies + 1) % num_bodies; }
        // if (IsKeyDown(KEY_RIGHT_BRACKET)) { selected_body = (num_bodies - 1) % num_bodies; }

        int mouse_wheel_move = GetMouseWheelMove();
        if (mouse_wheel_move) {
            // zoom = clamp(zoom + mouse_wheel_move, 1e-12, 1);
        }

        // Update
        const v2 center = { screenWidth/2, screenHeight/2 };
        if (paused) {
            DrawTextWithShadow("Paused", center.x-100, center.y-100, 60, WHITE, BLACK);
        } else {
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

            // Bodies
            if (draw_bodies) {
                v2 positions[num_bodies];
                // Get positions of bodies first because they'll be used for moon orbits later
                for (int i = 0; i < num_bodies; i++) {
                    const v2 scaled = v2_scale(bodies[i].position, zoom);
                    positions[i] = v2_add(v2_add(center, pan), scaled);
                }
                for (int i = 0; i < num_bodies; i++) {
                    body_t b = bodies[i];
                    const v2 pos = positions[i];
                    DrawCircleV(pos, b.radius * zoom, b.color);
                    if (draw_labels) {
                        DrawBodyLabel(&b, pos, box_sizes[b.type], DARKGREEN);
                        if (b.primary_id != -1) {
                            DrawBodyOrbit(positions[b.primary_id], pos, b.color);
                        }
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
                DrawCircle(0, posY, max(sun->radius / height_scale, 10.0f), Fade(sun->color, 0.25f));
                for (int i = 0; i < num_planets; i++) {
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
