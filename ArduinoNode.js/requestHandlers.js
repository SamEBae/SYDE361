// functions that will be executed when 
// typeoff handle[pathname] === a function in requestHandlers.
// the handle and function are discribed in index.js

var fs = require('fs'),
server = require('./server');
var pageHTML = "/pages/line.html";

function sendInterface(response) {
  console.log("Request handler 'interface' was called.");
  response.writeHead(200, {"Content-Type": "text/html"});

  var html = fs.readFileSync(__dirname + pageHTML);
  //var html = fs.readFileSync(__dirname + "/test.html");
  response.end(html);
}

function sendImages(response) {

	// response.writeHead(200, {})
	console.log(response);
	asd;
	response.end()
}

function sendNotes(response){
	//var html = fs.readFileSync(__dirname + "/notes/G");

}
exports.sendInterface = sendInterface;
exports.sendNotes = sendNotes;