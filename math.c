// This is a C implementation of the classic ASCII donut animation by Andy Sloane.

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SCREEN_WIDTH  80
#define SCREEN_HEIGHT 24

static const float theta_spacing = 0.07f;
static const float phi_spacing   = 0.02f;

static const float R1 = 1.0f;
static const float R2 = 2.0f;
static const float K2 = 5.0f;

// K1 is based on screen size: the maximum distance occurs roughly at the
// edge of the torus, at R1 + R2, z = 0. We want that to be displaced
// 3/8ths of the screen size (3/4 of the way from center to the side).
//   screen_height*3/8 = K1*(R1+R2)/(K2+0)
// Note: the pseudocode derives K1 from screen_width, but terminal characters
// are about twice as tall as wide, so here K1 is derived from the height and
// x is stretched by ASPECT to compensate.
static const float K1 = SCREEN_HEIGHT * 5.0f * 3.0f / (8.0f * (1.0f + 2.0f));
#define ASPECT 2.0f

static void render_frame(float A, float B) {
    // precompute sines and cosines of A and B
    float cosA = cosf(A), sinA = sinf(A);
    float cosB = cosf(B), sinB = sinf(B);

    char  output[SCREEN_HEIGHT][SCREEN_WIDTH];
    float zbuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

    memset(output, ' ', sizeof(output));
    memset(zbuffer, 0, sizeof(zbuffer));  // 0 == infinitely far away

    const char *luminance_chars = ".,-~:;=!*#$@";

    // theta goes around the cross-sectional circle of the torus
    for (float theta = 0; theta < 2 * M_PI; theta += theta_spacing) {
        float costheta = cosf(theta), sintheta = sinf(theta);

        // phi goes around the center of revolution of the torus
        for (float phi = 0; phi < 2 * M_PI; phi += phi_spacing) {
            float cosphi = cosf(phi), sinphi = sinf(phi);

            // the x,y coordinate of the circle, before revolving
            float circlex = R2 + R1 * costheta;
            float circley = R1 * sintheta;

            // final 3D (x,y,z) coordinate after rotations
            float x = circlex * (cosB * cosphi + sinA * sinB * sinphi)
                      - circley * cosA * sinB;
            float y = circlex * (sinB * cosphi - sinA * cosB * sinphi)
                      + circley * cosA * cosB;
            float z = K2 + cosA * circlex * sinphi + circley * sinA;
            float ooz = 1.0f / z;  // "one over z"

            // x and y projection. y is negated because y goes up in 3D
            // space but down on 2D displays.
            int xp = (int)(SCREEN_WIDTH  / 2 + ASPECT * K1 * ooz * x);
            int yp = (int)(SCREEN_HEIGHT / 2 - K1 * ooz * y);

            // luminance, ranges from -sqrt(2) to +sqrt(2)
            float L = cosphi * costheta * sinB - cosA * costheta * sinphi
                      - sinA * sintheta
                      + cosB * (cosA * sintheta - costheta * sinA * sinphi);

            // L < 0 means the surface points away from us: skip it
            if (L > 0) {
                // bounds check (the pseudocode omits this)
                if (xp < 0 || xp >= SCREEN_WIDTH ||
                    yp < 0 || yp >= SCREEN_HEIGHT)
                    continue;

                // larger 1/z means closer to the viewer
                if (ooz > zbuffer[yp][xp]) {
                    zbuffer[yp][xp] = ooz;
                    int luminance_index = (int)(L * 8);  // 0..11
                    output[yp][xp] = luminance_chars[luminance_index];
                }
            }
        }
    }

    // dump output[] to the screen: move cursor to "home" first
    printf("\x1b[H");
    for (int j = 0; j < SCREEN_HEIGHT; j++) {
        for (int i = 0; i < SCREEN_WIDTH; i++) {
            putchar(output[j][i]);
        }
        putchar('\n');
    }
}

int main(void) {
    float A = 0, B = 0;

    printf("\x1b[2J");  // clear the screen once

    for (;;) {
        render_frame(A, B);
        A += 0.04f;
        B += 0.02f;
        usleep(30000);  // ~30 ms per frame
    }
    return 0;
}