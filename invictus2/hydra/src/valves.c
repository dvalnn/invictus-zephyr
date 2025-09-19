#include "valves.h"

// Sets the servo angle in degrees [0, 270]
void valve_pwm_set_angle(float angle) {
    // Clamp angle to valid range
    if (angle < 0) angle = 0;
    if (angle > 270) angle = 270;

    // Servo spec:
    // 0º  ->  500 µs
    // 270º -> 2500 µs
    const int minPulse = 500;   // microseconds
    const int maxPulse = 2500;  // microseconds
    const int angleRange = 270;

    // Map angle to pulse width
    int pulseWidth = minPulse + (int)((angle / angleRange) * (maxPulse - minPulse));

    // Send PWM signal using your function
    pwm_set_duty_cycle(5, 20000, pulseWidth); // Channel 5, 20ms period, pulse width calculated above
}