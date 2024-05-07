const brown = "#5c4a3d";
const light = "#ebe5d8";
const lightBrown = "#d2b791";
const lightGreen = "#cdcc98";
const lightOrange = "#f1bb74";
const lighter = "#f3f0eb";
const newGreen = "#c2c070";
const orange = "#de8f54";

let currentSVGIndex = 0;

const svgFolders = ['assets/jaggedCircles/0c0u', 'assets/jaggedCircles/0c1u', 'assets/jaggedCircles/1c0u', 'assets/jaggedCircles/1c1u', 'assets/jaggedCircles/1c2u'];

const container = document.getElementById('svgContainer');
let svgArray = [];

var imuData = [];

window.onload = () => {

    if (!!window.EventSource) {
        var source = new EventSource('http://192.168.116.39:80/events'); // Add withCredentials if using HTTP auth

        source.addEventListener('open', function (e) {
            console.log("Events Connected");
        }, false);

        source.addEventListener('error', function (e) {
            if (e.target.readyState != EventSource.OPEN) {
                console.error("Events Disconnected");
            }
        }, false);

        source.addEventListener('message', function (e) {
            console.info("========= message", e.data, "=========");
        }, false);

        source.addEventListener('imu_data', function (e) {  // Listening for 'imu_data'
            // Parse the JSON data received from the server
            var data = e.data;
            // var data = JSON.parse(e.data);

            // imuData[16] = { ax, ay, az, gx, gy, gz, mx, my, mz, q0, q1, q2, q3, roll, pitch, heading }

            // processData(e.data);

            // data is expected to be an array of objects
            // data.forEach(function (entry) {
            //     console.log("Timestamp: " + entry.timestamp);
            //     console.log("Accelerometer X: " + entry.ax);
            //     console.log("Accelerometer Y: " + entry.ay);
            //     console.log("Accelerometer Z: " + entry.az);
            //     console.log("Gyroscope X: " + entry.gx);
            //     console.log("Gyroscope Y: " + entry.gy);
            //     console.log("Gyroscope Z: " + entry.gz);
            //     console.log("Magnetometer X: " + entry.mx);
            //     console.log("Magnetometer Y: " + entry.my);
            //     console.log("Magnetometer Z: " + entry.mz);
            //     console.log("Quaternion q0: " + entry.q0);
            //     console.log("Quaternion q1: " + entry.q1);
            //     console.log("Quaternion q2: " + entry.q2);
            //     console.log("Quaternion q3: " + entry.q3);
            //     console.log("Roll: " + entry.roll);
            //     console.log("Pitch: " + entry.pitch);
            //     console.log("Heading: " + entry.heading);
            // });


            console.log(data);
        }, false);
    }

    for (let i = 0; i < svgFolders.length; i++) {
        // dynamically fetch folders into the svg arrays
        for (let j = 0; j < 10; j++) {
            fetch(svgFolders[i] + `/Vector-${j}.svg`)
                .then(response => response.text())
                .then(data => {
                    svgArray.push(data);

                    if (i == svgFolders.length - 1 && j == 9) {
                        // console.log('All SVGs loaded', svgArray.length);
                        // console.log(container.innerHTML);
                        loadNextSVG();
                    }
                })
                .catch(error => console.error('Error loading the SVG:', error));

        }
    }

    let currentSVGIndex = 0;

    function loadNextSVG() {
        currentSVGIndex = returnRandInt(0, svgArray.length - 1);
        initAnimation(currentSVGIndex);
    }

    function initAnimation(num) {
        // const container = document.getElementById('svgContainer');
        container.innerHTML = svgArray[num];

        const svgElement = container.querySelector('svg');
        const paths = svgElement.getElementsByTagName('path');

        Array.from(paths).forEach(path => {
            const dot = document.createElementNS("http://www.w3.org/2000/svg", "circle");
            dot.setAttribute("r", "30");
            dot.setAttribute("fill", "orange");
            svgElement.appendChild(dot);

            animatePath(dot, path);
        });
    }

    function animatePath(dot, path) {
        const pathLength = path.getTotalLength();
        let progress = 0;

        function animateDot() {
            if (progress < pathLength) {
                requestAnimationFrame(animateDot);
                const point = path.getPointAtLength(progress);
                dot.setAttribute('cx', point.x);
                dot.setAttribute('cy', point.y);
                progress += 1;
            } else {
                loadNextSVG();
            }
        }

        animateDot();
    }

}

function returnRandInt(min, max) {
    return Math.floor(Math.random() * (max - min + 1) + min);
}

function processData(data) {
    try {
        var parsedData = JSON.parse(data);

        // Validate data consistency
        var minLength = Infinity;
        for (let key in parsedData) {
            if (Array.isArray(parsedData[key])) {
                minLength = Math.min(minLength, parsedData[key].length);
            }
        }

        // Truncate data to the shortest array length found
        for (let key in parsedData) {
            if (Array.isArray(parsedData[key])) {
                parsedData[key] = parsedData[key].slice(0, minLength);
            }
        }

        // Update imuData
        imuData[16] = parsedData;
    } catch (error) {
        console.error("Error parsing JSON data: ", error);
        // Optional: Handle the error by trying to clean and re-parse data
        // cleanAndRetryParse(data);
    }
}

// Example of a function to clean data before retrying parse
function cleanAndRetryParse(data) {
    var cleanedData = data.replace(/[^,\[\]{}\d.\-+eE]/g, ''); // Regex to remove unwanted characters
    processData(cleanedData);
}