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
            console.warn("========= message", e.data, "=========");
        }, false);

        source.addEventListener('imu_data', function (e) {  // Listening for 'imu_data'
            var data = e.data;
            var arr = data.split(',');

            // imuData[16] = { ax, ay, az, gx, gy, gz, mx, my, mz, q0, q1, q2, q3, roll, pitch, heading }

            console.log(arr);
        }, false);
    }
};