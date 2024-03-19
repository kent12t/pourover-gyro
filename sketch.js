// Kalman Filters for x and y accelerations
const kfX = new KalmanFilter();
const kfY = new KalmanFilter();

let velocityX = 0;
let velocityY = 0;
let posX = 0;
let posY = 0;

let lastUpdateTime = Date.now();

function setup() {
    createCanvas(windowWidth, windowHeight);
    // Assuming the starting position is the center of the canvas
    posX = width / 2;
    posY = height / 2;
}

function draw() {
    background(255);
    // Visualize the estimated position
    ellipse(posX, posY, 50, 50);
}

if (window.DeviceMotionEvent) {
    window.addEventListener('devicemotion', (event) => {
        const now = Date.now();
        const dt = (now - lastUpdateTime) / 1000; // Convert milliseconds to seconds
        lastUpdateTime = now;

        // Get smoothed acceleration
        const accelX = kfX.filter(event.accelerationIncludingGravity.x);
        const accelY = kfY.filter(event.accelerationIncludingGravity.y);

        // Update velocity by integrating acceleration
        velocityX += accelX * dt;
        velocityY += accelY * dt;

        // Update position by integrating velocity
        posX += velocityX * dt;
        posY += velocityY * dt;

        // Optional: Adjust position based on some constraints, e.g., canvas size
        posX = constrain(posX, 0, width);
        posY = constrain(posY, 0, height);
    });
} else {
    console.log("DeviceMotionEvent is not supported by your device.");
}
