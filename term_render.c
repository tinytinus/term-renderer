/*
 -- A terminal ncurses renderer -- 

Goals:
- render 3-dimensional shapes spinning around
- use the best shaped charchtr instead of just plain 1
- csv rendering 
- color
- edit mode

*/

#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

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
	int color_pair;
} edge;

typedef struct {
	vec3 points[MAX_POINTS];
	int point_count;
	edge edges[MAX_POINTS * 2];
	int edge_count;
} shape;

float z_buffer[200][200];

void initZBuffer() {
	int max_lines = LINES < 200 ? LINES : 200;
	int max_cols = COLS < 200 ? COLS : 200;
	
	for (int y = 0; y < max_lines; y++) {
		for (int x = 0; x < max_cols; x++) {
			z_buffer[y][x] = -100.0f;
		}
	}
}

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
            int start, end, color;

			if (s->edge_count >= MAX_POINTS * 2) break;

            if (sscanf(line, "%d,%d,%d", &start, &end, &color) == 3) {
                s->edges[s->edge_count++] = (edge){start, end, color};
            }
			else if (sscanf(line, "%d,%d", &start, &end) == 2) {  // Fixed: removed third %d
				s->edges[s->edge_count++] = (edge){start, end, 1};
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

void drawLine(int x0, int y0, int x1, int y1, float z0, float z1, char ch, int color_pair) {
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	int total_steps = dx > dy ? dx : dy;
	int current_step = 0;

	while (1) {
		if (x0 >= 0 && x0 < COLS && y0 >= 0 && y0 < LINES && x0 < 200 && y0 < 200) {
			float t = total_steps > 0 ? (float)current_step / total_steps : 0;
			float current_z = z0 + t * (z1 - z0);

			if (current_z > z_buffer[y0][x0]) {
				z_buffer[y0][x0] = current_z; 
				attron(COLOR_PAIR(color_pair));
				mvaddch(y0, x0, ch);
				attroff(COLOR_PAIR(color_pair));
			}
		}

		if (x0 == x1 && y0 == y1) {
			break;
		}

		int e2 = 2 * err;
		if (e2 > -dy) {err -= dy; x0 += sx;}
		if (e2 < dx) {err += dx; y0 += sy;}
		current_step++;
	}
}

char getChar(float z) {
    if (z < 0.1f) return '#';
    else if (z < 0.5f) return '*';
    else if (z < 1.0f) return '+';
    else return '.';
}

float autoScale(shape* s, int screen_width, int screen_height, float distance) {
	float soft_limit = 0.7f;
   	float max_screen_x = 0, max_screen_y = 0;  	

	for (int i = 0; i < s->point_count; i++) {
		vec2 projected = project3Dto2D(s->points[i], distance);
		max_screen_x = fmaxf(max_screen_x, fabsf(projected.x));
		max_screen_y = fmaxf(max_screen_y, fabsf(projected.y));
	}

	float scale_x = (screen_width * soft_limit / 2) / max_screen_x;
	float scale_y = (screen_height * soft_limit / 2) / max_screen_y;

	return fminf(scale_x, scale_y);
}

int main(int argc, char *argv[]) {

	bool edit_mode = false;
	int current_point = -1;
	
	char* filename = "shape.csv";	
	int opt;

	while ((opt = getopt(argc, argv, "f:")!= -1)) {
		switch (opt) {
			case 'f':
				filename = optarg;
			case '?':
				fprintf(stderr, "Usage: %s [-d] [-f filename]\n", argv[0]);
				exit(1);
		}
	}

    initscr();
    noecho();
    curs_set(FALSE);
    timeout(0);
	start_color();

	init_pair(1, COLOR_WHITE, COLOR_BLACK); 
	init_pair(2, COLOR_RED, COLOR_BLACK);
	init_pair(3, COLOR_CYAN, COLOR_BLACK);
	init_pair(4, COLOR_YELLOW, COLOR_BLACK);
	init_pair(5, COLOR_GREEN, COLOR_BLACK);
	init_pair(6, COLOR_BLUE, COLOR_BLACK);
	init_pair(7, COLOR_MAGENTA, COLOR_BLACK);

    shape object;
    shape temp_object;

    if (!loadShapeCsv(filename, &object)) {
        endwin();
        fprintf(stderr, "Failed to load shape\n");
        return 1;
    }

    float angle = 0;
	float scale = 0;
	vec3 center3d = get3DShapeCenter(&object);

    while (1) {
        clear();
		initZBuffer();
        
        angle += 0.1f;
        
        temp_object = object;
        
		scale = autoScale(&temp_object, COLS, LINES, 5.0f);
		
		for (int j = 0; j < object.point_count; j++) {
			vec3 translated = {
				object.points[j].x - center3d.x,
				object.points[j].y - center3d.y, 
				object.points[j].z - center3d.z
			};
			
			vec3 temp = rotatePointAxis(translated, angle);
			
			temp_object.points[j] = (vec3){
				temp.x + center3d.x,
				temp.y + center3d.y,
				temp.z + center3d.z
			};
		}

        for (int i = 0; i < temp_object.edge_count; i++) {
            int start_index = temp_object.edges[i].start_index;
            int end_index = temp_object.edges[i].end_index;
            
            if (start_index >= temp_object.point_count || end_index >= temp_object.point_count) {
                continue;
            }

			vec3 start_3d = temp_object.points[start_index];
			vec3 end_3d = temp_object.points[end_index];
            
            vec2 start_2d = project3Dto2D(start_3d, 5.0f);
            vec2 end_2d = project3Dto2D(end_3d, 5.0f);
            
            vec2 start_screen = worldToScreen(start_2d, COLS, LINES, scale);
            vec2 end_screen = worldToScreen(end_2d, COLS, LINES, scale);
           
			float avg_z = (temp_object.points[start_index].z + temp_object.points[end_index].z) / 2.0f;
			char line_char = getChar(avg_z);

            drawLine((int)start_screen.x, (int)start_screen.y, (int)end_screen.x, (int)end_screen.y,
					 start_3d.z, end_3d.z, line_char, temp_object.edges[i].color_pair);
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
