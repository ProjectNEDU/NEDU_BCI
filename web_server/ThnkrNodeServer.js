var http = require('http');
var ffi = require("node-ffi");
var lib = ffi.Library("libgetWavesTest", { "getData": [ 'string', [ ] ] });

http.createServer(function(req, res) {

	res.writeHead(200, { 
		'Content-Type': 'text/plain',
		'Access-Control-Allow-Origin': '*'
	});
	
	res.end(lib.getData());

}).listen(8080, '192.168.0.18');

console.log('NodeJS server is running @ http://192.168.0.18:8080/');
