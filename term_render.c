/*
 -- A terminal ncurses renderer -- 

Goals:
- render 3-dimensional shapes spinning around
- use the best shaped charchtr instead of just plain 1
- csv rendering 
- color

*/

#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MAX_POINTS 100

typedef struct {
    float x, y, z;
} vec3;

typedef struct {
	float x, y;
} vec2 ;

typedef struct {
		int start_index;
		int end_index;
} edge;

typedef struct {
		vec3 points[MAX_POINTS];
		int point_count;
		edge edges[MAX_POINTS * 2];
		int edge_count;
} shape;

int loadShapeCsv(const char* filename, shape* s) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return 0;
    }

    char line[256];
    s->point_count = 0;
    s->edge_count = 0;
    int reading_points = 0;
    int reading_edges = 0;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        
        if (strncmp(line, "POINTS", 6) == 0) {
            reading_points = 1;
            reading_edges = 0;
            continue;
        }
        if (strncmp(line, "EDGES", 5) == 0) {
            reading_points = 0;
            reading_edges = 1;
            continue;
        }

        if (reading_points) {
            float x, y, z;

			if (s->point_count >= MAX_POINTS) break;

            if (sscanf(line, "%f,%f,%f", &x, &y, &z) == 3) {
                s->points[s->point_count++] = (vec3){x, y, z};
            }
        }
        else if (reading_edges) {
            int start, end;

			if (s->edge_count >= MAX_POINTS * 2) break;

            if (sscanf(line, "%d,%d", &start, &end) == 2) {
                s->edges[s->edge_count++] = (edge){start, end};
            }
        }
    }
    fclose(file);
    return 1;
}

vec3 get3DShapeCenter(shape* s) {
		vec3 center = {0,0,0};
		for (int i = 0; i < s->point_count; i++) {
			center.x += s->points[i].x;
			center.y += s->points[i].y;
			center.z += s->points[i].z;
		}

		center.x /= s->point_count;
		center.y /= s->point_count;
		center.z /= s->point_count;

		return center;
}

vec3 rotatePointAxis(vec3 p,  float angle) {
		float s = sinf(angle);
		float c = cosf(angle);

		vec3 output = {p.x * c + p.z * s, p.y, -p.x * s + p.z * c};
		return output;
}

vec2 project3Dto2D (vec3 p, float distance) {
	float factor = distance / (distance + p.z);
	vec2 output = {p.x * factor, p.y * factor};
   	return output;	
}

vec2 worldToScreen(vec2 p, int screen_width, int screen_height, float scale) {
    vec2 output = { 
        .x = screen_width / 2 + p.x * scale,
        .y = screen_height / 2 - p.y * scale
    };
    return output;
}

void drawLine(int x0, int y0, int x1, int y1, char ch) {
		int dx = abs(x1 - x0);
		int dy = abs(y1 - y0);
		int sx = (x0 < x1) ? 1 : -1;
		int sy = (y0 < y1) ? 1 : -1;
		int err = dx - dy;

		while (1) {
				if (x0 >= 0 && x0 < COLS && y0 >= 0 && y0 < LINES) {
						mvaddch(y0, x0, ch);
				}

				if (x0 == x1 && y0 == y1) {
						break;
				}

				int e2 = 2 * err;
				if (e2 > -dy) {err -= dy; x0 += sx;}
				if (e2 < dx) {err += dx; y0 += sy;}
		}
}

char getChar(float z) {
    if (z < 0.1f) return '#';
    else if (z < 0.5f) return '*';
    else if (z < 1.0f) return '+';
    else return '.';
}

int main() {
    initscr();
    noecho();
    curs_set(FALSE);
    timeout(0);

    shape triangle;
    shape rotated_triangle;

    if (!loadShapeCsv("./shape.csv", &triangle)) {
        endwin();
        fprintf(stderr, "Failed to load shape\n");
        return 1;
    }

    float angle = 0;
	vec3 center3d = get3DShapeCenter(&triangle);

    while (1) {
        clear();
        
        angle += 0.1f;
		float scale = 50.0f;
		
        
        rotated_triangle = triangle;
        
        for (int j = 0; j < triangle.point_count; j++) {
            vec3 translated = {
                triangle.points[j].x - center3d.x,
                triangle.points[j].y - center3d.y, 
                triangle.points[j].z - center3d.z
            };
            
            vec3 rotated = rotatePointAxis(translated, angle);
            
            rotated_triangle.points[j] = (vec3){
                rotated.x + center3d.x,
                rotated.y + center3d.y,
                rotated.z + center3d.z
            };
        }

        for (int i = 0; i < rotated_triangle.edge_count; i++) {
            int start_index = rotated_triangle.edges[i].start_index;
            int end_index = rotated_triangle.edges[i].end_index;
            
            if (start_index >= rotated_triangle.point_count || end_index >= rotated_triangle.point_count) {
                continue;
            }
            
            vec2 start_2d = project3Dto2D(rotated_triangle.points[start_index], 5.0f);
            vec2 end_2d = project3Dto2D(rotated_triangle.points[end_index], 5.0f);
            
            vec2 start_screen = worldToScreen(start_2d, COLS, LINES, scale);
            vec2 end_screen = worldToScreen(end_2d, COLS, LINES, scale);
           
			float avg_z = (rotated_triangle.points[start_index].z + rotated_triangle.points[end_index].z) / 2.0f;
			char line_char = getChar(avg_z);

            drawLine((int)start_screen.x, (int)start_screen.y, (int)end_screen.x, (int)end_screen.y, line_char);
        }

        refresh();
        usleep(50000);  // ~20 FPS

        int ch = getch();
        if (ch == 'q') {
            break;
        }
    }

    endwin();
    return 0;
}
