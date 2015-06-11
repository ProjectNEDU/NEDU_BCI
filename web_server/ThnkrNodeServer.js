function runServer() {
	var http = require('http');
	jxcore.store.shared.set("jsonData", "{\"attention\":\"0\",\"meditation\":\"0\",\"delta\":\"0\",\"theta\":\"0\",\"low_alpha\":\"0\",\"high_alpha\":\"0\",\"low_beta\":\"0\",\"high_beta\":\"0\",\"low_gamma\":\"0\",\"mid_gamma\":\"0\"}");
	
	http.createServer(function(req, res) {

		res.writeHead(200, { 
			'Content-Type': 'text/plain',
			'Access-Control-Allow-Origin': '*'
		});

		res.end(jxcore.store.shared.get("jsonData"));

	}).listen(8080, '192.168.0.18');

	console.log('NodeJS server is running @ http://192.168.0.18:8080/');
}

function getData() {
	var ffi = require("node-ffi");
	var lib = ffi.Library("libThnkrEegDecoder", { "getThnkrDataJSON": [ 'string', [ ] ] });
	var json = "";
	
	while(1) {
		setTimeout(function(){}, 700);
		json = lib.getThnkrDataJSON();
		
		if(json != "") {
			jxcore.store.shared.set("jsonData", json);
			json = "";
		}
	}
}

jxcore.tasks.runOnThread(0, runServer);
jxcore.tasks.runOnThread(1, getData);