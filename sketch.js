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
        const dt = (now - lastUpdateTime) / 1000; // Time in seconds
        lastUpdateTime = now;

        // Assuming accelerationIncludingGravity is what you intend to use
        const accelX = kfX.filter(event.accelerationIncludingGravity.x);
        const accelY = kfY.filter(event.accelerationIncludingGravity.y);

        console.log('Accel:', accelX, accelY);

        // Implement a simple check for 'rest' state to reset velocities
        if (Math.abs(accelX) < 0.1 && Math.abs(accelY) < 0.1) {
            velocityX = 0;
            velocityY = 0;
        } else {
            velocityX += accelX * dt;
            velocityY += accelY * dt;
        }

        posX += velocityX * dt;
        posY += velocityY * dt;

        posX = constrain(posX, 0, width);
        posY = constrain(posY, 0, height);

        console.log('Pos:', posX, posY);
    });
} else {
    console.log("DeviceMotionEvent is not supported by your device.");
}
