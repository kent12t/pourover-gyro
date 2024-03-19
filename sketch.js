// Variables to store device motion data
let x = null;
let y = null;
let z = null;
let alpha, beta, gamma;
let bg;


function setup() {
    createCanvas(windowWidth, windowHeight);

}

function draw() {
    // Map device acceleration to canvas coordinates
    // This is a basic mapping; you might need to adjust the scaling
    bg = color(255, 0, 0);

    background(bg);
    if (x != 0 && y != 0) {
        bg = color(0, 255, 0);
        ellipse(map(x, -10, 10, 0, windowWidth), map(y, -10, 10, 0, windowHeight), 50, 50);
    }
}

if (window.DeviceMotionEvent) {
    window.addEventListener('devicemotion', (event) => {
        // Update global variables based on accelerationIncludingGravity
        // These values might need calibration for smoother movement

        // Accelerometer data
        x = event.accelerationIncludingGravity.x;
        y = event.accelerationIncludingGravity.y;
        z = event.accelerationIncludingGravity.z;

        // Gyroscope data
        alpha = event.rotationRate.alpha; // Rotation around the z-axis
        beta = event.rotationRate.beta;  // Rotation around the x-axis
        gamma = event.rotationRate.gamma; // Rotation around the y-axis

        console.log(x, y, z, alpha, beta, gamma);
    });
} else {
    console.log("DeviceMotionEvent is not supported by your device.");
}
