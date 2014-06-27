#include <stdio.h>
#include <string.h>
struct Pixel {
	int x;
	int y;
};

struct SuperPixel {
	int x;
	int y;
	struct Pixel z;
};

struct Pixel gpixel;
struct SuperPixel gsuper;

void print_pixel(struct Pixel* p, unsigned size) {
	while (size--) {
		printf("-------------------------\n");
		printf("%s: pixel->x = %d\n", __func__, p->x);
		printf("%s: pixel->y = %d\n", __func__, p->y);
		p++;
	}
}

int sum_gpixel() {
	return gpixel.x + gpixel.y;
}


int sum_super_pixel() {
	return gsuper.x + gsuper.y + gsuper.z.x + gsuper.z.y;
}

int sum_pixel_struct(struct Pixel* p) {
	return p->x + p->y;
}

int sum_super_pixel_struct(struct SuperPixel* s) {
	return s->x + s->y + s->z.x + s->z.y;
}

void testing_malloc() {
	struct Pixel localpixel;
	memset(&localpixel,0, sizeof(struct Pixel));
}
