#include <stdio.h>
#include <math.h>

int main() {
    float theta_spacing = 0.07;
    float phi_spacing = 0.02;

    float R1 = 1.0; // Radius of the circle
    float R2 = 2.0; // Radius of the torus
    float K2 = 5.0; // Distance from the viewer to the torus
    float screen_width = 1280.0; // Primary display width in pixels
    float K1 = (3*screen_width*K2)/(8*(R1+R2)); // Distance from the viewer to the screen
}