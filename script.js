const brown = "#5c4a3d";
const light = "#ebe5d8";
const lightBrown = "#d2b791";
const lightGreen = "#cdcc98";
const lightOrange = "#f1bb74";
const lighter = "#f3f0eb";
const newGreen = "#c2c070";
const orange = "#de8f54";

window.onload = function () {
    const container = document.getElementById('svgContainer');
    fetch('assets/jaggedCircles/0c0u/Vector.svg')
        .then(response => response.text())
        .then(data => {
            container.innerHTML = data;
            initAnimation();
        })
        .catch(error => console.error('Error loading the SVG:', error));

    function initAnimation() {
        const svgElement = document.querySelector('#svgContainer svg');
        const paths = svgElement.getElementsByTagName('path');
        let step;
        Array.from(paths).forEach(path => {
            const dot = document.createElementNS("http://www.w3.org/2000/svg", "circle");
            dot.setAttribute("r", "30");
            dot.setAttribute("fill", orange);
            svgElement.appendChild(dot);

            const pathLength = path.getTotalLength();
            let progress = 0;
            step = randomSpeed(); // Adjust step size for speed control

            function animateDot() {
                requestAnimationFrame(animateDot);
                const point = path.getPointAtLength(progress % pathLength);
                dot.setAttribute('cx', point.x);
                dot.setAttribute('cy', point.y);
                progress += step;
            }

            animateDot();
        });
    }

    function randomSpeed() {
        return Math.random() * 2 + 2;
    }
};
