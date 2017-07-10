var server = require("./server");
var router = require("./route");
var requestHandlers = require("./requestHandlers");

var debug = false;

var handle = {}
handle["/"] = requestHandlers.sendInterface;
handle["/interface"] = requestHandlers.sendInterface;
handle["/images"] = requestHandlers.sendImages;
handle["/notes/G"] = requestHandlers.sendNotes;

server.start(router.route,handle,debug);