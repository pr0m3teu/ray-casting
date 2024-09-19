#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "raylib/raylib.h"
#include "raylib/raymath.h"

#define GRID_SIZE  8
#define FPS        60
#define WIDTH      800
#define HEIGHT     800
#define RADIUS     10.0f
#define EPS        1e-5
#define MAX_DIST   10
#define FOV        180.0f


#define grid_at(grid, i, j) grid.items[i*grid.cols+j] 

typedef struct {
    int rows;
    int cols;
    bool *items;

} Grid;

typedef struct {
    Vector2 pos;
    Vector2 dir;
    Vector2 fov_left;
    Vector2 fov_right;
} Player;


int make_grid(Grid* grid, int rows, int cols) {
    grid->rows = rows;
    grid->cols = cols;
    grid->items = malloc(sizeof(bool)*rows*cols);
    
    if (grid->items == NULL) {
        return -1;
    }

    memset(grid->items, 0, sizeof(bool)*rows*cols);

    return 0;
}

// TODO: Stop enforcing grid to be square
void draw_grid(Grid grid)
{
    int cell_size = WIDTH / GRID_SIZE;
       
    DrawLineV((Vector2){.x = 0, .y = 1}, (Vector2){.x = WIDTH, .y = 1}, RAYWHITE);
    DrawLineV((Vector2){.x = 1, .y = 0}, (Vector2){.x = 1, .y = HEIGHT}, RAYWHITE);
    DrawLineV((Vector2){.x = WIDTH, .y = 0}, (Vector2){.x = WIDTH, .y = HEIGHT}, RAYWHITE);
    DrawLineV((Vector2){.x = 0, .y = HEIGHT}, (Vector2){.x = WIDTH, .y = HEIGHT}, RAYWHITE);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++){
            if (grid_at(grid, i, j) != 0) {
                DrawRectangleV((Vector2){j*cell_size, i*cell_size}, (Vector2){cell_size, cell_size}, RAYWHITE);
            }
        }

        DrawLineV((Vector2){.x = i*cell_size, .y = 0}, (Vector2){.x = i*cell_size, .y = HEIGHT},  RAYWHITE);
        DrawLineV((Vector2){.x = 0, .y = i*cell_size}, (Vector2){.x = WIDTH, .y = i*cell_size}, RAYWHITE);
    }

}

// TODO: Stop using global constants
// TODO: Refactor this functions / Completly get rid of them
Vector2 world_to_grid(Vector2 point)
{
    return (Vector2){ .x = point.x / WIDTH * GRID_SIZE, .y = point.y / HEIGHT * GRID_SIZE };
}

Vector2 grid_to_world(Vector2 point)
{
    return (Vector2){ .x = point.x * WIDTH / GRID_SIZE, .y = point.y * HEIGHT / GRID_SIZE };
}

void draw_line(Vector2 p1, Vector2 p2)
{
    Vector2 p1w = grid_to_world(p1);
    Vector2 p2w = grid_to_world(p2);
    DrawLineV(p1w, p2w, RED);
}

void draw_point(Vector2 p, Color color)
{
    Vector2 pw = grid_to_world(p);
    DrawCircleV(pw, RADIUS, color);
}
// TODO: Refactor this functions / Completly get rid of them


Vector2 get_line_eq(Vector2 p1, Vector2 p2)
{
    float m = 0.0f;
    if (p1.x != p2.x) m = (p2.y - p1.y) / (p2.x - p1.x);
    
    float n = p2.y - m * p2.x;

    return (Vector2){.x = m, .y = n};
}

// TODO: Fix edge case where some rays don't collide properly
bool check_collision(Vector2 p, Grid grid)
{
     if (p.x < GRID_SIZE && p.y < GRID_SIZE && p.x > 0 && p.y > 0){
        if (grid_at(grid, (int)floorf(p.y), (int)p.x) != 0) return 1;
     }
    return 0;

}


// TODO: Refactor the way collisions are computed
Vector2 step_ray(Vector2 p1, Vector2 p2)
{   
    Vector2 line_eq = get_line_eq(p1, p2);
    float m = line_eq.x;
    float n = line_eq.y;
    
    Vector2 dx = {0};
    Vector2 dy = {0};

    // Find closest X axis collision
    if (m) {
        if (p2.y > p1.y) dx = (Vector2){.x = (ceil(p2.y) - n) / m, .y = ceil(p2.y)};
        else dx = (Vector2){.x = (floorf(p2.y) - n) / m, .y = floorf(p2.y)};
    } else dx = (Vector2){.x = 0, .y = n};

    // Find Y axis collision
    if (p2.x > p1.x) dy = (Vector2){.x = ceil(p2.x), .y = m * ceil(p2.x) + n};
    else if (p2.x < p1.x) dy = (Vector2){.x = floorf(p2.x), .y = m * floorf(p2.x) + n};
    else {
        if (p2.y > p1.y) dy = (Vector2){.x = p2.x, .y = ceil(p2.y)};
        else dy = (Vector2){.x = p2.x, .y = floorf(p2.x)};
    }
   
    if (Vector2DistanceSqr(p2, dx) < Vector2DistanceSqr(p2, dy)) return dx;
    return dy;

}

Vector2 cast_ray(Vector2 pos, Vector2 dir, Grid g)
{
    Vector2 start = pos;

    Vector2 eps = {.x = (dir.x / fabsf(dir.x)) * EPS, .y = (dir.y / fabsf(dir.y)) * EPS};
    while(Vector2DistanceSqr(start, pos) < MAX_DIST*MAX_DIST)
    {
        Vector2 next = step_ray(start, Vector2Add(start, dir));
        draw_line(start, next);
        next = Vector2Add(next, eps);//Very important for collision checking apparently.

        if (check_collision(next, g)) 
        {   
            draw_point(next, RED);
            start = next;
            break;
        }
        start = next;
    }
    return start;
}



Vector2 get_fov_right(Vector2 dir)
{
    float cos_45 = cos(PI/4.0f);
    float x = cos_45 * dir.x - cos_45 * dir.y;
    float y = cos_45 * dir.x + cos_45 * dir.y;
    return (Vector2) {x, y};
}


Vector2 get_fov_left(Vector2 dir)
{
    float cos_45 = cos(PI/4.0f);
    float x = cos_45 * dir.x + cos_45 * dir.y;
    float y = cos_45 * -1 * dir.x + cos_45 * dir.y;
    return (Vector2) {x, y};
}


int main(void)
{
    Grid g = {0};
    if(make_grid(&g, GRID_SIZE, GRID_SIZE) != 0)
    {
        printf("[ERROR] Could not allocate memory for grid!\n");
        exit(1);
    }
    grid_at(g, 2, 2) = 1;
    grid_at(g, 3, 2) = 1;
    grid_at(g, 1, 2) = 1;
    grid_at(g, 1, 3) = 1;
    grid_at(g, 1, 4) = 1;
    grid_at(g, 2, 4) = 1;

    for (int i = 0; i < g.rows; i++){ 
        for (int j = 0; j < g.cols; j++) {
            printf("%d ", grid_at(g, i, j));
        }
        printf("\n");
    }


    InitWindow(WIDTH, HEIGHT, "Hello Raylib");
    SetTargetFPS(FPS); 

    float cos_30 = cos(PI/6.0f);
    float sin_30 = sin(PI/6.0f);

    Player player = {0};
    player.pos = (Vector2){4, 5.6};  
    player.dir = (Vector2){-0.2, 0.2};


    while(!WindowShouldClose()) 
    {
        // TODO: Fix bug where player.dir cannot be zero anymore
        if (player.dir.x == 0) player.dir.x += EPS;
        if (player.dir.y == 0) player.dir.y += EPS;

        switch(GetKeyPressed()) {
            case KEY_W:
                player.pos = Vector2Add(player.pos, Vector2Scale(player.dir, 2));
                break;

            case KEY_S:
                player.pos = Vector2Subtract(player.pos, Vector2Scale(player.dir, 2));
                break;

            case KEY_D: {
                float x = player.dir.x;
                player.dir.x = player.dir.x * cos_30 - player.dir.y * sin_30;
                player.dir.y = x * sin_30 + player.dir.y * cos_30;
                break;
            }

            case KEY_A: {
                float x = player.dir.x;
                player.dir.x = player.dir.x * cos_30 + player.dir.y * sin_30;
                player.dir.y = x * -1 * sin_30 + player.dir.y * cos_30;
                break;
            }
        }
        player.fov_right = Vector2Add(player.pos, get_fov_right(player.dir));
        player.fov_left = Vector2Add(player.pos, get_fov_left(player.dir));

        // TODO: Refactor this where possible
        BeginDrawing();
            ClearBackground(BLACK);
            draw_grid(g);
            for (float i = 0.0f; i <= FOV; i++) {
                float l_x = Lerp(player.fov_left.x, player.fov_right.x, i/FOV);
                float l_y = Lerp(player.fov_left.y, player.fov_right.y, i/FOV);
                Vector2 lerp_dir = Vector2Scale(Vector2Subtract((Vector2){l_x, l_y}, player.pos), 0.005f);
                if (lerp_dir.x == 0 || lerp_dir.y == 0)
                {   
                    lerp_dir.x += EPS;
                    lerp_dir.y += EPS;
                }
                Vector2 coll_point = cast_ray(player.pos, lerp_dir, g);
                draw_point(coll_point, RED);

            }
            draw_line(player.fov_left, player.fov_right);
            draw_point(player.pos, BLUE);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
