#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"

#define GRID_SIZE   10 
#define MINIMAP_SIZE 3 

#define FPS          60
#define WIDTH        1280 * 1.5
#define HEIGHT       720 * 1.5 
#define RADIUS       10.0f
#define EPS          1e-6
#define MAX_DIST     10
#define FOV          480.0f


#define grid_at(grid, i, j) grid.items[(int)i*grid.cols+(int)j] 

typedef struct {
    int rows;
    int cols;
    int *items;

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
    grid->items = malloc(sizeof(int)*rows*cols);
    
    if (grid->items == NULL) {
        return -1;
    }

    memset(grid->items, 0, sizeof(int)*rows*cols);

    return 0;
}

// TODO: Stop enforcing grid to be square
void draw_grid(Grid grid)
{
    int cell_width = WIDTH / GRID_SIZE / MINIMAP_SIZE;
    int cell_height = HEIGHT / GRID_SIZE / MINIMAP_SIZE;
       
    DrawLineV((Vector2){.x = 0, .y = 1}, (Vector2){.x = WIDTH/MINIMAP_SIZE, .y = 1}, RAYWHITE);
    DrawLineV((Vector2){.x = 1, .y = 0}, (Vector2){.x = 1, .y = HEIGHT/MINIMAP_SIZE}, RAYWHITE);
    DrawLineV((Vector2){.x = WIDTH/MINIMAP_SIZE, .y = 0}, (Vector2){.x = WIDTH/MINIMAP_SIZE, .y = HEIGHT/MINIMAP_SIZE}, RAYWHITE);
    DrawLineV((Vector2){.x = 0, .y = HEIGHT/MINIMAP_SIZE}, (Vector2){.x = WIDTH/MINIMAP_SIZE, .y = HEIGHT/MINIMAP_SIZE}, RAYWHITE);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++){
            if (grid_at(grid, i, j) != 0) {
                DrawRectangleV((Vector2){j*cell_width, i*cell_height}, (Vector2){cell_width, cell_height}, RAYWHITE);
            }
        }

        DrawLineV((Vector2){.x = i*cell_width, .y = 0}, (Vector2){.x = i*cell_width, .y = HEIGHT/MINIMAP_SIZE},  RAYWHITE);
        DrawLineV((Vector2){.x = 0, .y = i*cell_height}, (Vector2){.x = WIDTH/MINIMAP_SIZE, .y = i*cell_height}, RAYWHITE);
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
    return (Vector2){ .x = point.x * WIDTH / GRID_SIZE / MINIMAP_SIZE, .y = point.y * HEIGHT / GRID_SIZE / MINIMAP_SIZE };
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
    double m = 0.0f;
    if (p1.x != p2.x) m = (p2.y - p1.y) / (p2.x - p1.x);
    
    double n = p2.y - m * p2.x;

    return (Vector2){.x = m, .y = n};
}

// TODO: Fix edge case where some rays don't collide properly
bool check_collision(Vector2 p, Grid grid)
{
     if (p.x < GRID_SIZE && p.y < GRID_SIZE && p.x > 0 && p.y > 0){
        if (grid_at(grid, (int)floorf(p.y), (int)floorf(p.x)) != 0) return 1;
     }
    return 0;

}

double snap(double x, double dx)
{
    if (dx > 0) return ceil(x + EPS);
    if (dx < 0) return floorf(x - EPS);
    return x;
}

Vector2 step_ray(Vector2 p1, Vector2 p2)
{   
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    Vector2 p3 = p2;
    if (dx != 0)
    {
        double m = dy / dx;
        double n = p2.y - m*p2.x;

        double x3 = snap(p2.x, dx);
        double y3 = m * x3 + n;
        p3 = (Vector2){x3, y3};

        if (m != 0)
        {
            y3 = snap(p2.y, dy);
            x3 = (y3-n)/m;
            Vector2 p3y = {x3, y3};
            if (Vector2DistanceSqr(p2, p3y) <= Vector2DistanceSqr(p2, p3)) p3 = p3y;
        }

    }
    else {
        double y3 = snap(p2.y, dy);
        double x3 = p2.x;
        p3 = (Vector2){x3, y3};
    }

    return p3;
}

float sign_of(double x)
{
    if (x < 0) return -1;
    if (x > 0) return 1;
    return 0;
}

Vector2 cast_ray(Vector2 p1, Vector2 p2, Grid g)
{
    Vector2 start = p1;
   

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    Vector2 eps = {.x = sign_of(dx) * EPS, .y = sign_of(dy) * EPS};
    while(Vector2DistanceSqr(start, p1) < MAX_DIST*MAX_DIST)
    {
        Vector2 next = step_ray(start, p2);
        next = Vector2Add(next, eps);//Very important for collision checking apparently.
        start = p2;
        p2 = next;

        if (check_collision(next, g)) 
        {   
            return next;
        }
    }
    return start;
}



Vector2 get_fov_right(Vector2 dir)
{
    double cos_45 = cos(PI/4.0f);
    double x = cos_45 * dir.x - cos_45 * dir.y;
    double y = cos_45 * dir.x + cos_45 * dir.y;
    return (Vector2) {x, y};
}


Vector2 get_fov_left(Vector2 dir)
{
    double cos_45 = cos(PI/4.0f);
    double x = cos_45 * dir.x + cos_45 * dir.y;
    double y = cos_45 * -1 * dir.x + cos_45 * dir.y;
    return (Vector2) {x, y};
}


void draw_minimap(Grid g, Player player)
{
    draw_grid(g);
    for (double i = 0.0f; i <= FOV; i++) {
        double l_x = Lerp(player.fov_left.x, player.fov_right.x, i/FOV);
        double l_y = Lerp(player.fov_left.y, player.fov_right.y, i/FOV);
        
        Vector2 lerp_dir = (Vector2){l_x, l_y};
        cast_ray(player.pos, lerp_dir, g);
        draw_line(player.fov_left, player.fov_right);

    }
    draw_point(player.pos, BLUE);
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
    grid_at(g, 2, 4) = 2;
    grid_at(g, 5, 4) = 2;

    for (int i = 0; i < g.rows; i++){ 
        for (int j = 0; j < g.cols; j++) {
            printf("%d ", grid_at(g, i, j));
        }
        printf("\n");
    }


    InitWindow(WIDTH, HEIGHT, "Hello Raylib");
    SetTargetFPS(FPS); 

    double cos_30 = cos(PI/6.0f);
    double sin_30 = sin(PI/6.0f);

    Player player = {0};
    player.pos = (Vector2){2.830127, 7.830127};  
    player.dir = (Vector2){0.1, -0.1};


    while(!WindowShouldClose()) 
    {
        //NOTE: If things break again it might be a good idea to bring this back
   //     if (player.dir.x == 0 || player.dir.y == 0) 
   //     {
   //         player.dir.x += EPS;
   //         player.dir.y += EPS;
   //     }

        switch(GetKeyPressed()) {
            case KEY_W:
                player.pos = Vector2Add(player.pos, Vector2Scale(player.dir, 10));
                break;

            case KEY_S:
                player.pos = Vector2Subtract(player.pos, Vector2Scale(player.dir, 10));
                break;

            case KEY_D: {
                double x = player.dir.x;
                player.dir.x = player.dir.x * cos_30 - player.dir.y * sin_30;
                player.dir.y = x * sin_30 + player.dir.y * cos_30;
                break;
            }

            case KEY_A: {
                double x = player.dir.x;
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
            for (double i = 0.0f; i <= FOV; i++)
            {
                double l_x = Lerp(player.fov_left.x, player.fov_right.x, i/FOV);
                double l_y = Lerp(player.fov_left.y, player.fov_right.y, i/FOV);
                Vector2 lerp_point= (Vector2){l_x, l_y};

                Vector2 coll_point = cast_ray(player.pos, lerp_point, g);
                double dist_to_player = Vector2Distance(player.pos, coll_point);

                Vector2 camera_line = get_line_eq(player.fov_left, player.fov_right);
                double dist_to_camera = fabs(camera_line.x*coll_point.x - coll_point.y + camera_line.y) / sqrtf(camera_line.x*camera_line.x + 1);

                if (floorf(dist_to_player) < MAX_DIST) {
                    Color color = RAYWHITE;
                    if (grid_at(g, floorf(coll_point.y), floorf(coll_point.x)) == 1) color = RED;
                    if (grid_at(g, floorf(coll_point.y), floorf(coll_point.x)) == 2) color = BLUE;

                    double width = WIDTH / FOV;
                    double height = HEIGHT / dist_to_camera;

                    Vector2 position = {.x = width * i, .y = HEIGHT / 2 - height / 2};
                    Vector2 size = {width, height};
                    DrawRectangleV(position, size, color);
                }
            }
            draw_minimap(g, player);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
