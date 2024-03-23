'use strict';

var socket = io.connect();

$(document).ready(function () {
	// Drum beat selection
	var beatButtons = document.querySelectorAll('.beat-select button');
	beatButtons.forEach(function (button) {
		button.addEventListener('click', function () {
			var beatMode = this.getAttribute('id');
			socket.emit('commandToRelay', beatMode);
		});
	});

	// Volume and Tempo controls
	$('#volume-up').click(function () {
		socket.emit('commandToRelay', 'volumeup');
	});

	$('#volume-down').click(function () {
		socket.emit('commandToRelay', 'volumedown');
	});

	$('#tempo-up').click(function () {
		socket.emit('commandToRelay', 'tempoup');
	});

	$('#tempo-down').click(function () {
		socket.emit('commandToRelay', 'tempodown');
	});

	// Display the BBG status sent via UDP
	var lastVolume = parseInt($('#volume').val());
	var lastTempo = parseInt($('#tempo').val());
	let bbgError = false;

	socket.on('statusUpdate', function (status) {
		$('#volume').val(status.volume);
		$('#tempo').val(status.tempo);

		if (
			(status.volume === 100 || status.volume === 0) &&
			status.volume !== lastVolume
		) {
			setTimeout(() => alert('Volume Range [0, 100]'), 10);
		}

		if (
			(status.tempo === 300 || status.tempo === 40) &&
			status.tempo !== lastTempo
		) {
			setTimeout(() => alert('Tempo Range [40, 300]'), 10);
		}

		lastVolume = status.volume;
		lastTempo = status.tempo;

		var currentBeatText = 'None';
		switch (status.beat) {
			case 0:
				currentBeatText = 'None';
				break;
			case 1:
				currentBeatText = 'Rock #1';
				break;
			case 2:
				currentBeatText = 'Rock #2';
				break;
			case 3:
				currentBeatText = 'Rock #3';
				break;
			case 4:
				currentBeatText = 'Rock #4';
				break;
		}
		document.getElementById('current-beat').innerText = currentBeatText;

		if (bbgError) {
			closeBBGErrorBox();
		}
	});

	socket.on('uptime', function (parsedSeconds) {
		let uptime = parsedSeconds.uptime;
		let hours = Math.floor(uptime / 3600);
		let minutes = Math.floor((uptime % 3600) / 60);
		let seconds = Math.floor(uptime % 60);
		let formattedUptime = `${hours}h ${minutes}m ${seconds}s`;
		document.getElementById('uptime').innerText = formattedUptime;
	});

	// Error handling
	socket.on('bbgError', function (errorMessage) {
		document.getElementById('error-title').innerText = 'BeagleBone Error';
		document.getElementById('error-message').innerText = errorMessage;
		var errorBox = document.getElementById('error-box');
		errorBox.classList.remove('error-hidden');
		errorBox.classList.add('error-visible');
		bbgError = true;
	});

	function closeBBGErrorBox() {
		var errorBox = document.getElementById('error-box');
		errorBox.classList.add('error-hidden');
		errorBox.classList.remove('error-visible');
		bbgError = false;
	}

	document.getElementById('error-close').addEventListener('click', function () {
		closeBBGErrorBox();
	});

	function displayServerError() {
		document.getElementById('error-title').innerText = 'Server Error';
		document.getElementById('error-message').innerText =
			'Server error detected. Is the server running?';
		var errorBox = document.getElementById('error-box');
		errorBox.classList.remove('error-hidden');
		errorBox.classList.add('error-visible');
	}

	function hideServerError() {
		if (!bbgError) {
			var errorBox = document.getElementById('error-box');
			errorBox.classList.add('error-hidden');
			errorBox.classList.remove('error-visible');
		}
	}

	function pingServer() {
		let serverTimeout = setTimeout(displayServerError, 1000);
		socket.emit('ping');
		socket.on('running', () => {
			clearTimeout(serverTimeout);
			hideServerError();
		});
	}

	setInterval(pingServer, 1000);

	// Play individual sounds
	document.getElementById('play-hihat').addEventListener('click', function () {
		socket.emit('commandToRelay', 'hihat');
	});
	document.getElementById('play-snare').addEventListener('click', function () {
		socket.emit('commandToRelay', 'snare');
	});
	document.getElementById('play-base').addEventListener('click', function () {
		socket.emit('commandToRelay', 'base');
	});

	// Terminate program
	$('#terminate-button').click(function () {
		socket.emit('commandToRelay', 'terminate');
	});
});
