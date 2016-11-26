//Sockets For ESP8266
var WebSocketServer = require('websocket').server;
var http = require('http');
const Sequelize = require('sequelize');
var crypto = require('crypto');

var dbcon = false;
const dbconnect = new Sequelize("IOT Connect","root","bhushans");
const dbusers = dbconnect.define("users",{
									username: {
										type: Sequelize.STRING,
										unique: true,
										allowNull: false
									},
									password:{
										type: Sequelize.STRING,
										allowNull: false
									},
									email:{
										type: Sequelize.STRING,
										unique: true,
										allowNull: false
									},
									address1:Sequelize.TEXT,
									address2:Sequelize.TEXT,
									city:Sequelize.STRING,
									zip:Sequelize.INTEGER,
									confirm:Sequelize.BOOLEAN
								});
dbconnect.sync().then(function(){
		dbcon = true;
	});


var server = http.createServer(function(request, response) {
    console.log((new Date()) + ' Received request for ' + request.url);
    response.writeHead(404);
    response.end();
});
server.listen(8085, function() {
    console.log((new Date()) + ' ESP8266 Server is listening on port 8085');
});

wsServer = new WebSocketServer({
    httpServer: server,
    autoAcceptConnections: false
});

function originIsAllowed(origin) {
  // put logic here to detect whether the specified origin is allowed.
  return true;
}
var conlist = [];
wsServer.on('request', function(request) {
    if (!originIsAllowed(request.origin)) {
      // Make sure we only accept requests from an allowed origin
      request.reject();
      console.log((new Date()) + ' Connection from origin ' + request.origin + ' rejected.');
      return;
    }
    var connection = request.accept('', request.origin);
    conlist.push(connection);
	io.sockets.emit('alert',{msg: "Device Connected"});
	for(var i=0; i<conlist.length;i++)
		if(conlist[i]!==null)
			conlist[i].sendUTF(JSON.stringify({action:'STATUS'}));
    connection.on('message', function(message) {
        if (message.type === 'utf8') {
			if(message.utf8Data!== "")
				console.log(message.utf8Data);
				io.sockets.emit('device_status',{msg: message.utf8Data});
        }
        else if (message.type === 'binary') {
			if(message.binaryData!== "" || message.binaryData!== "\n")
				console.log(message.binaryData);
				io.sockets.emit('device_status',{msg: message.binaryData});
        }
	});
	connection.on('close', function(reasonCode, description) {
		io.sockets.emit('alert',{msg: "Device disconnected"});
		conlist.splice(conlist.indexOf(connection), 1);
	});
});



//Sockets For WEB


var express = require('express');
var app = express();
var server_web = require('http').createServer(app);
var io = require('socket.io').listen(server_web);
users = [];
connections = [];

server_web.listen(process.env.PORT || 8081);
console.log('Server running....');

app.use(express.static('public'));



app.get('/',function(req,res){
	res.sendFile(__dirname + '/index.html')
});

io.sockets.on('connection', function(socket){
	connections.push(socket);
	console.log('Connected: %s sockets connected',connections.length);
	
	//Disconnect
	socket.on('disconnect', function(data){
		users.splice(users.indexOf(socket.username), 1);
		connections.splice(connections.indexOf(socket), 1);
		console.log('Disconnected: %s sockets connected', connections.length);	
	});
	
	//Send Action
	socket.on('send action', function(data){
		console.log(data);
		for(var i=0; i<conlist.length;i++)
			if(conlist[i]!==null)
				conlist[i].sendUTF(JSON.stringify(data));
	});
	socket.on('login', function(data){
	});
	socket.on('signup', function(data){
		if(data["username"] !== '' && data["password"] !== '' && data["email"] !== '')
		{
			if(data["username"].length >= 6)
			{
				if(data["password"].length >= 6)
				{
					if(validateEmail(data["email"]))
					{
						if(dbcon)
						{
							dbconnect.sync().then(function(){
								return dbusers.create({
									username : data["username"],
									password : crypto.createHash('md5').update(data["password"]).digest("hex"),
									email : data["email"],
									address1 : data["address1"],
									address2 : data["address2"],
									city : data["city"],
									zip : data["zip"],
									confirm : false
								}).then(function(){
									io.sockets.emit('alert',{msg: "Thankyou for signup, User Account created.... "});
								});
							}).catch(function(error){
								io.sockets.emit('alert',{msg: error});
							});
						}
					}
				}
			}
		}
		
	});
})
function validateEmail(email) 
{
    var re = /^(([^<>()\[\]\.,;:\s@\"]+(\.[^<>()\[\]\.,;:\s@\"]+)*)|(\".+\"))@(([^<>()[\]\.,;:\s@\"]+\.)+[^<>()[\]\.,;:\s@\"]{2,})$/i;
    return re.test(email);
}