// Variables to store device motion data
let x, y, z;
let alpha, beta, gamma;

function setup() {
    createCanvas(windowWidth, windowHeight);
}

function draw() {
    background(220);
    // Map device acceleration to canvas coordinates
    // This is a basic mapping; you might need to adjust the scaling

    if (x && y) {
        ellipse(map(x, -10, 10, 0, width), map(y, -10, 10, 0, height), 50, 50);
    }
}

if (window.DeviceMotionEvent) {
    window.addEventListener('devicemotion', (event) => {
        // Update global variables based on accelerationIncludingGravity
        // These values might need calibration for smoother movement

        // Accelerometer data
        const x = event.accelerationIncludingGravity.x;
        const y = event.accelerationIncludingGravity.y;
        const z = event.accelerationIncludingGravity.z;

        // Gyroscope data
        const alpha = event.rotationRate.alpha; // Rotation around the z-axis
        const beta = event.rotationRate.beta;  // Rotation around the x-axis
        const gamma = event.rotationRate.gamma; // Rotation around the y-axis

        console.log(x, y, z, alpha, beta, gamma);
    });
} else {
    console.log("DeviceMotionEvent is not supported by your device.");
}