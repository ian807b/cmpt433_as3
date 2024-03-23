// Handles communication with BBG.
// PORT 12345: For bidirectional communication between the server and BBG.
// PORT 12346: For unidirectional communication bewteen the server and BBG.
// This setup is to avoid having the server constantly polling.

'use strict';

var socketio = require('socket.io');
var io;
var dgram = require('dgram');
var udpServer = dgram.createSocket('udp4');
var fs = require('fs');
var path = '/proc/uptime';

exports.listen = function (server) {
	io = socketio.listen(server);
	io.set('log level 1');

	io.sockets.on('connection', function (socket) {
		handleCommand(socket);
		socket.on('ping', () => {
			socket.emit('running');
		});
		handleCommand(socket, 'sendstatus');
	});

	// Dedicated port when BBG sends status to the server
	var UDP_PORT = 12346;
	udpServer.on('message', function (message, remote) {
		console.log(`Received message from BBG: ${message}`);
		var status = message.toString().split(',');
		var volume = parseInt(status[0]);
		var tempo = parseInt(status[1]);
		var beat = parseInt(status[2]);
		io.sockets.emit('statusUpdate', { volume, tempo, beat });
	});

	udpServer.on('listening', function () {
		var address = udpServer.address();
		console.log(`UDP Server listening on ${address.address}:${address.port}`);
	});

	udpServer.bind(UDP_PORT);
};

function handleCommand(socket, command = '') {
	var PORT = 12345;
	var HOST = '192.168.7.2';

	if (command) {
		var buffer = new Buffer.from(command);
		var client = dgram.createSocket('udp4');
		client.send(buffer, 0, buffer.length, PORT, HOST, function (err, bytes) {
			if (err) throw err;
			console.log('UDP message sent to ' + HOST + ':' + PORT);
			client.close();
		});
	} else {
		socket.on('commandToRelay', function (data) {
			var commandToSend = command || data;
			console.log('Relayed command: ' + commandToSend);

			var buffer = new Buffer.from(commandToSend);

			var client = dgram.createSocket('udp4');
			client.send(buffer, 0, buffer.length, PORT, HOST, function (err, bytes) {
				if (err) throw err;
				console.log('UDP message sent to ' + HOST + ':' + PORT);
			});

			let bbgTimeout = setTimeout(() => {
				socket.emit('bbgError', 'BBG is not responding. Is the program running?');
			}, 1500);

			client.on('listening', function () {
				var address = client.address();
				console.log(
					'UDP Client: listening on ' + address.address + ':' + address.port
				);
			});

			client.on('message', function (message, remote) {
				console.log(
					'UDP Client: message Rx' +
					remote.address +
					':' +
					remote.port +
					' - ' +
					message
				);
				clearTimeout(bbgTimeout);

				var reply = message.toString('utf8');
				socket.emit('commandReply', reply);

				// x,y volume & tempo information from the BBG
				let msg = message.toString();
				if (msg === 'terminating') {
					console.log('BBG is terminating. Server shutting down...');
					shutdownServer();
					return;
				} else {
					let [volume, tempo, beat] = msg.split(',').map(Number);
					socket.emit('statusUpdate', { volume, tempo, beat });
				}

				client.close();
			});
			client.on('UDP Client: close', function () {
				console.log('closed');
			});
			client.on('UDP Client: error', function (err) {
				console.log('error: ', err);
			});
		});
	}
}

function shutdownServer() {
	udpServer.close(() => {
		console.log('UDP Server closed.');
	});
	process.exit(0);
}

function emitUptime() {
	fs.readFile(path, 'utf8', (err, data) => {
		if (err) {
			console.error(`Error reading ${path}: ${err}`);
			return;
		}
		let uptimeSeconds = parseFloat(data.split(' ')[0]);
		io.sockets.emit('uptime', { uptime: uptimeSeconds });
	});
}

setInterval(emitUptime, 1000);
