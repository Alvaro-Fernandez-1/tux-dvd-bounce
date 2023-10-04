#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

uint32_t *framebuffer;
uint32_t tempBuf[1920*1080];
uint32_t tuxBuf[249 * 294]; // image
uint32_t colorChange = 0;
int maxXPos = 1920 - 249;
int maxYPos = 1080 - 294;
struct timespec tms;
unsigned long long start, end, elapsed;

uint32_t newColor();
uint32_t min(uint32_t a, uint32_t b);
void renderTux(int xPos, int yPos, uint32_t color); // position of top left pixel
void removeTux(int xPos, int yPos); // paints black over tux
void draw(); //draws tempBuf to framebuffer, using framebuffer directly makes it less smooth
unsigned long long getMicros();

int main() {
	int img = open("tux.bmp", O_RDONLY);
	lseek(img, 0x8a, SEEK_SET);
	size_t bufOffset = 0;
	int amount;
	while ((amount = read(img, tuxBuf+bufOffset, 249*294*4 - bufOffset)) > 0) {
		bufOffset += amount;
	}

	for (int i=0; i<1920*1080; i++) tempBuf[i] = 0;

	// initialize framebuffer device but only for 1920x1080 screens
	int fb = open("/dev/fb0", O_RDWR);
	framebuffer = mmap(NULL, 1920*1080 * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

	// initialize random position and direction
	srand(getMicros());
	int xPos = rand() % 1000 + 100;
	int yPos = rand() % 500 + 100;
	int xDir = 4 * (rand() % 2 ? 1 : -1);
	int yDir = 3 * (rand() % 2 ? 1 : -1);

	double frameTime = 1000000.0 / 60;
	while (1) {
		start = getMicros();
		renderTux(xPos, yPos, colorChange);
		draw();
		removeTux(xPos, yPos);
		end = getMicros();
		elapsed = end - start;
		
		xPos += xDir;
		yPos += yDir;
		if (xPos < 0) {
			xPos *= -1;
			xDir *= -1;
			colorChange = newColor();
		}
		if (yPos < 0) {
			yPos *= -1;
			yDir *= -1;
			colorChange = newColor();
		}
		if (xPos > maxXPos) {
			xPos = maxXPos - (xPos - maxXPos);
			xDir *= -1;
			colorChange = newColor();
		}
		if (yPos > maxYPos) {
			yPos = maxYPos - (yPos - maxYPos);
			yDir *= -1;
			colorChange = newColor();
		}
		if (frameTime > elapsed) {
			usleep(frameTime - elapsed);
		}

		printf("\n"); // for some reason if you don't print anything it doesn't refresh smoothly
	}
	
}

uint32_t newColor(){
	unsigned char change = 200;
	uint32_t red = rand() % change;
	uint32_t green = rand() % change;
	uint32_t blue = rand() % change;
	return red * 0x10000 + green * 0x100 + blue;
}

uint32_t min(uint32_t a, uint32_t b) {
	return a < b ? a : b;
}

void renderTux(int xPos, int yPos, uint32_t color) {
	for (int i=0; i<294; i++) {
		for (int j=0; j<249; j++) {
			uint32_t pix = tuxBuf[(293-i)*249 + j];
			pix = (pix & 0xff000000) +
				min((pix & 0x00ff0000) + (color & 0x00ff0000), 0x00ff0000) +
				min((pix & 0x0000ff00) + (color & 0x0000ff00), 0x0000ff00) +
				min((pix & 0x000000ff) + (color & 0x000000ff), 0x000000ff);
			tempBuf[(i + yPos)*1920 + j + xPos] = (pix & 0x00ffffff) * ((pix & 0xff000000) > 0);
		}
	}
}

void removeTux(int xPos, int yPos) {
	for (int i=0; i<294; i++) {
		for (int j=0; j<249; j++) {
			tempBuf[(i + yPos)*1920 + j + xPos] = 0;
		}
	}
}

void draw() {
	memcpy(framebuffer, tempBuf, 1920*1080*4);
}

unsigned long long getMicros() {
	clock_gettime(CLOCK_MONOTONIC_RAW, &tms);
	return tms.tv_sec * 1000000 + tms.tv_nsec/1000;
}
