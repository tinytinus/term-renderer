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
		float x, y;
} vec2;

typedef struct {
		int origin_index;
		int end_index;
} edge;

typedef struct {
		vec2 points[MAX_POINTS];
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

				float x, y;
				int origin, end;
				if (sscanf(line, "%f,%f,%d,%d", &x, &y, &origin, &end) == 4) {
						if (s->point_count >= MAX_POINTS) {
								break;
						}
						s->points[s->point_count].x = x;
						s->points[s->point_count].y = y;

						s->edges[s->edge_count].origin_index = origin;
						s->edges[s->edge_count].end_index = end;

						s->point_count++;
						s->edge_count++;
				}
		}
		fclose(file);
		return 1;
}

vec2 rotatePoint(vec2 p, float angle) {
		float s = sinf(angle);
		float c = cosf(angle);
		
		vec2 output = {p.x * c + p.y * s, p.x * s - p.y * c};
		return output;
};

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

vec2 worldToScreen(vec2 p, int screen_width, int screen_height, float scale ) {
		vec2 output = { 
		.x = screen_width / 2 + p.x * scale,
		.y = screen_height / 2 - p.y * scale
		};

		return output;
}; 

int main() {
		initscr();
		noecho();
		curs_set(FALSE);
		timeout(0);


		shape triangle;
		shape rotated_triangle;

		if (!loadShapeCsv("./shape.csv", &triangle)) {
				fprintf(stderr, "Failed to load shape\n");
				return 1;
		}

		float angle = 0;

		while (1) {
				clear();

				angle += 0.1f;

				for (int j = 0; j < triangle.point_count; j++) {
						rotated_triangle.points[j] = rotatePoint(triangle.points[j], angle);
				}
				rotated_triangle.point_count = triangle.point_count;

				for (int j = 0; j < triangle.edge_count; j++) {
						rotated_triangle.edges[j] = triangle.edges[j];
				}
				rotated_triangle.edge_count = triangle.edge_count;

				for (int i = 0; i < rotated_triangle.edge_count; i++) {
						int origin_index = rotated_triangle.edges[i].origin_index;
						int end_index = rotated_triangle.edges[i].end_index;

						if (origin_index >= rotated_triangle.point_count || end_index >= rotated_triangle.point_count) {
								continue;
						}

						vec2 start_screen = worldToScreen(rotated_triangle.points[origin_index], COLS, LINES, 10.0f);
						vec2 end_screen = worldToScreen(rotated_triangle.points[end_index], COLS, LINES, 10.0f);

						drawLine((int)start_screen.x, (int)start_screen.y, (int)end_screen.x, (int)end_screen.y, '*');
				}
				
				for (int i = 0; i < rotated_triangle.point_count; i++) {
						vec2 screen_pos = worldToScreen(rotated_triangle.points[i], COLS, LINES, 10.0f);
						if (screen_pos.x >= 0 && screen_pos.x < COLS && screen_pos.y >= 0 && screen_pos.y < LINES) {
								mvaddch((int)screen_pos.y, (int)screen_pos.x, '*');
						}
				}	

				refresh();
				usleep(50000);

				int ch = getch();
				if (ch == 'q') {
						break;
				}
		}

		endwin();
		return 0;
		
}
