const http = require("http");
const fs = require("fs");
const https = require("https");
const express = require("express");
const httpProxy = require("http-proxy");

const privateKey = fs.readFileSync("private.key", "utf8");
const certificate = fs.readFileSync("cert.pem", "utf8");
const credentials = { key: privateKey, cert: certificate };

const app = express();
const proxy = httpProxy.createProxyServer();

app.use((req, res) => {
  proxy.web(req, res, { target: "http://192.168.116.39" });
});

const httpsServer = https.createServer(credentials, app);

httpsServer.listen(443, () => {
  console.log("HTTPS Server running on port 443");
});
