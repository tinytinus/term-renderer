/*
 -- A terminal ncurses renderer -- 

Goals:
- render 2-dimensional shapes spinning around
- use the best shaped charchtr instead of just plain 1
- csv rendering 
- color

*/

#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#define MAX_POINTS 100

typedef struct {
    float x, y, z;
} vec3;

typedef struct {
	float x, y;
} vec2 ;

typedef struct {
		int origin_index;
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

		fgets(line, sizeof(line), file);

		while (fgets(line, sizeof(line), file)) {
				if (line[0] == '#' || line[0] == '\n') {
						continue;
				}

				float x, y, z;
				int origin, end;
				if (sscanf(line, "%f,%f,%f,%d,%d", &x, &y, &z,&origin, &end) == 5) {
						if (s->point_count >= MAX_POINTS) {
								break;
						}
						s->points[s->point_count].x = x;
						s->points[s->point_count].y = y;
						s->points[s->point_count].z = z;

						s->edges[s->edge_count].origin_index = origin;
						s->edges[s->edge_count].end_index = end;

						s->point_count++;
						s->edge_count++;
				}
		}
		fclose(file);
		return 1;
}

vec3 getShapeCenter3D(shape* s) {
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

float autoScale(shape* s, float output) {
		float max_x = -999, min_x = 999, max_y = -999, min_y = 999;
		for (int i = 0; i < s->point_count; i++) {
    			if (s->points[i].x > max_x) max_x = s->points[i].x;
    			if (s->points[i].x < min_x) min_x = s->points[i].x;
    			if (s->points[i].y > max_y) max_y = s->points[i].y;
    			if (s->points[i].y < min_y) min_y = s->points[i].y;
		}

		float shape_width = max_x - min_x;
		float shape_height = max_y - min_y;
		float auto_scale = (shape_width < shape_height) ? (COLS * 0.5f) / shape_width : (LINES * 0.5f) / shape_height;

		output = auto_scale;
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
	float scale = autoScale(&triangle, scale);
	

    while (1) {
        clear();
        
        angle += 0.1f;          
        
        rotated_triangle = triangle;
        vec3 center3d = getShapeCenter3D(&triangle);
        
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
            int origin_index = rotated_triangle.edges[i].origin_index;
            int end_index = rotated_triangle.edges[i].end_index;
            
            if (origin_index >= rotated_triangle.point_count || end_index >= rotated_triangle.point_count) {
                continue;
            }
            
            vec2 start_2d = project3Dto2D(rotated_triangle.points[origin_index], 5.0f);
            vec2 end_2d = project3Dto2D(rotated_triangle.points[end_index], 5.0f);
            
            vec2 start_screen = worldToScreen(start_2d, COLS, LINES, scale);
            vec2 end_screen = worldToScreen(end_2d, COLS, LINES, scale);
            
            drawLine((int)start_screen.x, (int)start_screen.y, 
                     (int)end_screen.x, (int)end_screen.y, '*');
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
