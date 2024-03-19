// Initialize Kalman Filters for x and y directions
var kfx = new KalmanFilter();
var kfy = new KalmanFilter();

let x = window.innerWidth / 2;
let y = window.innerHeight / 2;

function setup() {
    createCanvas(windowWidth, windowHeight);
}

function draw() {
    background(220);
    ellipse(x, y, 50, 50); // Use x, y for ellipse position
}

if (window.DeviceMotionEvent) {
    window.addEventListener('devicemotion', (event) => {
        let rawX = event.accelerationIncludingGravity.x;
        let rawY = event.accelerationIncludingGravity.y;

        // Apply Kalman filter to the raw x and y values
        let filteredX = kfx.filter(rawX);
        let filteredY = kfy.filter(rawY);

        // Update x and y for visualization
        // This part needs to be adapted based on how you wish to map accelerometer data to screen coordinates
        // For demonstration, we'll simply map the filtered values directly, assuming a scaling factor for visualization
        x += filteredX * 10; // Adjust scaling factor as necessary
        y += filteredY * 10;

        // Keep ellipse within canvas bounds
        x = constrain(x, 0, width);
        y = constrain(y, 0, height);
    });
} else {
    console.log("DeviceMotionEvent is not supported by your device.");
}