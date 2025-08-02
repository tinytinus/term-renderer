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
#include <math.h>

#define MAX_POINTS 100

typedef struct {
		float x, y;
} vec2;

typedef struct {
		vec2 points[MAX_POINTS];
		int point_count;
} shape;

int loadShapeCsv(const char* filename, shape* s) {
		FILE* file = fopen(filename, "r");
		if (!file) {
				perror("fopen");
				return 0;
		}

		char line[256];
		s->point_count = 0;

		while (fgets(line, sizeof(line), file)) {
				if (line[0] == '#' || line[0] == '\n') {
						continue;
				}

				float x, y;
				if (sscanf(line, "%f,%f", &x, &y) == 2) {
						if (s->point_count >= MAX_POINTS) {
								break;
						}
						s->points[s->point_count].x = x;
						s->points[s->point_count].y = y;
						s->point_count++;
				}
		}
		fclose(file);
		return 1;
}

vec2 rotatePoint(vec2 p, float angle) {
		float s = sinf(angle);
		float c = cosf(angle);
		
		vec2 output = {p.x * c - p.y * s, p.x * s + p.y * c};
		return output;
};

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

		if (!loadShapeCsv("./shape.csv", &triangle)) {
				fprintf(stderr, "Failed to load shape\n");
				return 1;
		}

		while (true) {
				clear();
				
				for (int i = 0; i < triangle.point_count; i++) {
						printf("%d:(%f, %f)", i, triangle.points[i].x, triangle.points[i].y);
				}
				printf("\n");

				float angle = 3.14159 / 4;	

				for (int j = 0; j < triangle.point_count; j++) {
						triangle.points[j] = rotatePoint(triangle.points[j], angle);
				}

				//refresh();
				usleep(500000);

				int ch = getch();
				if (ch == 'q') {
						break;
				}
		}

		endwin();
		return 0;
		
}
