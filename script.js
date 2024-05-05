const conn = new WebSocket('ws://192.168.116.39:80');

conn.binaryType = 'arraybuffer'; // Set binary data type to ArrayBuffer

conn.onopen = function () {
    console.log("Connected to the server.");
    document.getElementById('status').innerText = "Connected";
};

conn.onerror = function (error) {
    console.error('WebSocket Error ', error);
    document.getElementById('status').innerText = "Connection Error";
};

conn.onmessage = function (e) {
    var buffer = e.data;
    var view = new DataView(buffer);
    var floats = [];
    for (var i = 0; i < buffer.byteLength; i += 4) {
        floats.push(view.getFloat32(i, true)); // Assumes little-endian
    }
    console.log(floats);
};

conn.onclose = function () {
    console.log('WebSocket connection closed', event.reason, event.code);
    document.getElementById('status').innerText = "Disconnected";
};
