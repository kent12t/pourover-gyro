// Variables to store device motion data
let x, y, z, alpha, beta, gamma;

// Initialize an array to store the last N readings
const windowSize = 10; // Size of the moving average window
let xReadings = new Array(windowSize).fill(0);
let yReadings = new Array(windowSize).fill(0);
// Similar for z, alpha, beta, gamma

function setup() {
    createCanvas(windowWidth, windowHeight);
}

function draw() {
    background(255, 0, 0);
    // Ensure x and y are numbers before using them
    if (typeof x === 'number' && typeof y === 'number') {
        ellipse(map(x, -10, 10, 0, windowWidth), map(y, -10, 10, 0, windowHeight), 50, 50);
    }
}

if (window.DeviceMotionEvent) {
    window.addEventListener('devicemotion', (event) => {
        // Update global variables based on accelerationIncludingGravity
        let rawX = event.accelerationIncludingGravity.x;
        let rawY = event.accelerationIncludingGravity.y;
        // let rawZ = event.accelerationIncludingGravity.z; // Assuming you will use it later

        // Gyroscope data
        // let rawAlpha = event.rotationRate.alpha; // Assuming you will use it later
        // let rawBeta = event.rotationRate.beta; // Assuming you will use it later
        // let rawGamma = event.rotationRate.gamma; // Fixed typo

        // Update the moving average arrays
        x = updateMovingAverage(rawX, xReadings);
        y = updateMovingAverage(rawY, yReadings);
    });
} else {
    console.log("DeviceMotionEvent is not supported by your device.");
}

function updateMovingAverage(newVal, arr) {
    arr.unshift(newVal);
    arr.pop();
    return arr.reduce((a, b) => a + b, 0) / arr.length;
}
