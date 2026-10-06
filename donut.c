#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define PI 3.14159265358979323846f

const float theta_spacing = 0.07f;
const float phi_spacing   = 0.02f;

const float R1 = 1.0f;
const float R2 = 2.0f;
const float K2 = 5.0f;


/*
 * Get the current terminal dimensions.
 *
 * cols = number of terminal columns
 * rows = number of terminal rows
 */
int get_terminal_size(int *cols, int *rows)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        return 0;
    }

    *cols = ws.ws_col;
    *rows = ws.ws_row;

    return 1;
}


/*
 * Render one frame of the rotating donut.
 */
void render_frame(float A, float B, int screen_width, int screen_height)
{
    /*
     * K1 controls the perspective projection.
     *
     * We want the maximum x-distance of the torus
     * (approximately R1 + R2) to occupy 3/8 of the
     * terminal width.
     */
    float K1 = screen_width * K2 * 3.0f
             / (8.0f * (R1 + R2));


    /*
     * Precompute sines and cosines of A and B.
     */
    float cosA = cosf(A);
    float sinA = sinf(A);

    float cosB = cosf(B);
    float sinB = sinf(B);


    /*
     * Allocate the output buffer and z-buffer.
     *
     * We use one-dimensional arrays:
     *
     * index = y * screen_width + x
     */
    char *output = malloc(
        screen_width * screen_height * sizeof(char)
    );

    float *zbuffer = malloc(
        screen_width * screen_height * sizeof(float)
    );


    if (output == NULL || zbuffer == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");

        free(output);
        free(zbuffer);

        return;
    }


    /*
     * Initialize the buffers.
     */
    for (int i = 0; i < screen_width * screen_height; i++) {
        output[i] = ' ';
        zbuffer[i] = 0.0f;
    }


    /*
     * theta goes around the cross-sectional circle
     * of the torus.
     */
    for (float theta = 0.0f;
         theta < 2.0f * PI;
         theta += theta_spacing)
    {
        float costheta = cosf(theta);
        float sintheta = sinf(theta);


        /*
         * phi goes around the center of revolution
         * of the torus.
         */
        for (float phi = 0.0f;
             phi < 2.0f * PI;
             phi += phi_spacing)
        {
            float cosphi = cosf(phi);
            float sinphi = sinf(phi);


            /*
             * Coordinates of the circle before
             * revolving it around the torus.
             */
            float circlex = R2 + R1 * costheta;
            float circley = R1 * sintheta;


            /*
             * Final 3D coordinates after rotation.
             */
            float x =
                circlex * (cosB * cosphi
                         + sinA * sinB * sinphi)
                - circley * cosA * sinB;

            float y =
                circlex * (sinB * cosphi
                         - sinA * cosB * sinphi)
                + circley * cosA * cosB;

            float z =
                K2
                + cosA * circlex * sinphi
                + circley * sinA;


            /*
             * 1 / z is used for perspective projection
             * and the z-buffer.
             */
            float ooz = 1.0f / z;


            /*
             * Project the 3D point onto the 2D terminal.
             *
             * y is negated because screen coordinates
             * increase downward.
             */
            int xp =
                (int)(screen_width / 2.0f
                      + K1 * ooz * x);

            int yp =
                (int)(screen_height / 2.0f
                      - K1 * ooz * y);


            /*
             * Calculate luminance.
             */
            float L =
                cosphi * costheta * sinB
                - cosA * costheta * sinphi
                - sinA * sintheta
                + cosB * (
                    cosA * sintheta
                    - costheta * sinA * sinphi
                );


            /*
             * Ignore points outside the terminal.
             *
             * This check must happen before accessing
             * output[yp * screen_width + xp].
             */
            if (xp < 0 || xp >= screen_width ||
                yp < 0 || yp >= screen_height)
            {
                continue;
            }


            /*
             * Only render surfaces facing the viewer.
             */
            if (L > 0.0f)
            {
                int index = yp * screen_width + xp;


                /*
                 * Check the z-buffer.
                 *
                 * Larger 1/z means the point is closer
                 * to the viewer.
                 */
                if (ooz > zbuffer[index])
                {
                    zbuffer[index] = ooz;


                    /*
                     * Convert luminance into a character
                     * index.
                     */
                    int luminance_index = (int)(L * 8.0f);


                    /*
                     * L can theoretically produce a value
                     * slightly outside our character range,
                     * so clamp it.
                     */
                    if (luminance_index < 0)
                        luminance_index = 0;

                    if (luminance_index > 11)
                        luminance_index = 11;


                    /*
                     * Dark -> bright characters.
                     */
                    const char *characters =
                        ".,-~:;=!*#$@";

                    output[index] =
                        characters[luminance_index];
                }
            }
        }
    }


    /*
     * Move the cursor to the top-left of the terminal.
     *
     * This allows us to overwrite the previous frame
     * instead of continuously scrolling the terminal.
     */
    printf("\x1b[H");


    /*
     * Print the frame.
     */
    for (int j = 0; j < screen_height; j++)
    {
        for (int i = 0; i < screen_width; i++)
        {
            putchar(output[j * screen_width + i]);
        }

        putchar('\n');
    }


    /*
     * Free the memory used by this frame.
     */
    free(output);
    free(zbuffer);
}


int main(void)
{
    int screen_width;
    int screen_height;


    /*
     * Get the terminal dimensions.
     */
    if (!get_terminal_size(&screen_width, &screen_height))
    {
        fprintf(stderr,
                "Could not determine terminal size.\n");

        return 1;
    }

    /*
     * Make sure the terminal is large enough.
     */
    if (screen_width < 20 || screen_height < 10)
    {
        fprintf(stderr,
                "Terminal is too small.\n");

        return 1;
    }


    /*
     * Rotation angles.
     */
    float A = 0.0f;
    float B = 0.0f;


    /*
     * Main animation loop.
     */
    while (1)
    {
        render_frame(
            A,
            B,
            screen_width,
            screen_height
        );


        /*
         * Change the rotation angles.
         */
        A += 0.04f;
        B += 0.02f;
    }


    return 0;
}