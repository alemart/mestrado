var express = require("express"),
tuio = require("tuio"),
fs = require("fs"),

app = express.createServer({
    key: fs.readFileSync((__dirname + '/../../../sshcert/server.key')),
    cert: fs.readFileSync((__dirname + '/../../../sshcert/server.crt'))
});

app.use(express["static"](__dirname + "/../../../"));
app.listen(5000);

tuio.init({
	oscPort: 3333,
	oscHost: "0.0.0.0",
	socketPort: app
});
