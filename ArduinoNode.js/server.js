var fs = require('fs')
,http = require('http'),
socketio = require('socket.io'),
url = require("url"), 
SerialPort = require('serialport');
var play = require('play');


var socketServer;
var serialPort;
var portName = '/dev/tty.usbmodem2851921'; //change this to your Arduino port
var sendData = "";

// handle contains locations to browse to (vote and poll); pathnames.
function startServer(route,handle,debug)
{
	// on request event
	function onRequest(request, response) {
	  // parse the requested url into pathname. pathname will be compared
	  // in route.js to handle (var content), if it matches the a page will 
	  // come up. Otherwise a 404 will be given. 
	  var pathname = url.parse(request.url).pathname; 
	  console.log("Request for " + pathname + " received");
	  var content = route(handle,pathname,response,request,debug);
	}
	

	var httpServer = http.createServer(onRequest).listen(1337, function(){
		console.log("Listening at: http://localhost:1337");
		console.log("Server is up");
	}); 

	// play with a callback]
	//frequencyHandler(pitchTest);
	//frequencyHandler(pitchTest2);

	serialListener(debug);
	initSocketIO(httpServer,debug);
}


var pitchTest = {
	'note': 'A#',
	'octave': '3'
};

var pitchTest2 = {
	'note': 'C',
	'octave': '3'
}

function frequencyHandler(pitch) {
	var filePath = 'notes/';

	filePath += pitch.note.replace('n','') + pitch.octave;

	filePath += '.wav';
	console.log(filePath);

	play.sound(filePath);
}

function processLogDataToSound(logData) {
	
	var logArr = logData.split('\n');

		console.log(logArr);

	for (var logFreq of logArr) {
		if (logFreq.length == 0 || logFreq.indexOf('\u0000\u0000') != -1) 
			continue;

		var tempObj = JSON.parse(logFreq);

		if (tempObj.octave && tempObj.octave <=6){
			frequencyHandler(tempObj);
		}
	}
}

function initSocketIO(httpServer,debug)
{
	socketServer = socketio.listen(httpServer);
	if(debug == false){
		socketServer.set('log level', 1); // socket IO debug off
	}
	socketServer.on('connection', function (socket) {
	console.log("user connected");
	socket.emit('onconnection', {pollOneValue:sendData});
	socketServer.on('update', function(data) {
	socket.emit('updateData',{pollOneValue:data});
	});
	socket.on('buttonval', function(data) {
		serialPort.write(data + 'E');
	});
	socket.on('sliderval', function(data) {
		serialPort.write(data + 'P');
	});
	
    });
}

// Listen to serial port
function serialListener(debug)
{
    var receivedData = "";
    serialPort = new SerialPort(portName, {
        baudrate: 9600,
        // defaults for Arduino serial communication
         dataBits: 8,
         parity: 'none',
         stopBits: 1,
         flowControl: false
    });
 
    serialPort.on("open", function () {
      console.log('open serial communication');
            // Listens to incoming data
        serialPort.on('data', function(data) {
             receivedData = data.toString();
        console.log(receivedData);
        processLogDataToSound(receivedData);
        // send the incoming data to browser with websockets.
    	socketServer.emit('update', receivedData);
      });  
    });  
}

exports.start = startServer;