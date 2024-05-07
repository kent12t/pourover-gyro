window.onload = function () {
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
};