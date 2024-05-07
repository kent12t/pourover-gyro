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



window.onload = () => {

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

