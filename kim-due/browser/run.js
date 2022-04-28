"use strict";

function welcomeMessage() {
	const bytes = Array.from(
		`\r\n\r\r\n\r\n` +
		`This is the KIM-1 terminal.\r\n\r\n` +
		`The original KIM-1 could be connected to either a teletype` +
		` (somthing like a huge noisy electric typewriter)` +
		` or a electronic terminal.` +
		`\r\n\r\n`
	).map((_, i, a) => ("" + _).charCodeAt(0));

	bytes.forEach(b => serialPrint(b));
}

document.addEventListener('DOMContentLoaded', () => {
	// In order to twiddle the SVG, it must be directly inlined.
	// 
	// For some reason, you can't twiddle SVGs that are loaded as an object
	// or an image.
	// So we inline our prototype and we just make the copies of it as
	// we need

	for (let i = 0; i < 6; i++) {
		const c = document.querySelector("#seg7").cloneNode(true);
		c.id = `display-${i}`;
		const ledbar = document.querySelector(`#ledbar-${i}`);
		ledbar.appendChild(c);
	}

}, false);

const ledTimer = [];

function setLed(pos, data, force) {

	const el = document.querySelector(`#display-${pos}`);

	if (!el) {
		return;
	}

	if (force) {
		ledTimer[pos] = null;
	} else {
		if (data === 0) {
			ledTimer[pos] = cpuInstructionCount;
			return;
		} else {
			ledTimer[pos] = null;
		}
	}

	const s = "gfedcba".split("").map((a, i) => {
		const seg = el.getElementById(a);
		seg.style.fill = (data & 1 << i) ? "red" : "#3f0020";
	});
}

let cpuInstructionCount = 0;
function runloop() {
	const ANTIFLICKER_DELAY = 2000;

	const NUMBER_OF_INSTRUCTIONS = 1000;
	try {
	_webloop(NUMBER_OF_INSTRUCTIONS);
	} catch (err) {
		const add = _debuggetaddress();
		console.log("Error: Last read address: " + add.toString(16));
		throw err;
	}

	cpuInstructionCount += NUMBER_OF_INSTRUCTIONS;

	for (let i = 0; i < ledTimer.length; i++) {
		let oldCount = ledTimer[i];
		if (oldCount !== null) {
			const instrunctionsSince = cpuInstructionCount - oldCount;
			if (instrunctionsSince > ANTIFLICKER_DELAY) {
				setLed(i, 0, true);
			}
		}
	}

	setTimeout(runloop, 1);
}

function wireupKeyboard() {
	const buttons = document.querySelectorAll("#button-box button");

	Array.from(buttons).forEach((b) => {
		b.addEventListener("click", (e) => {
			e.preventDefault();

			const ch = e.currentTarget.getAttribute("data-asc");
			const v = ch.charCodeAt(0) & 0x7F;
			_injectkey(ch);
		});
	});
}

Module['onRuntimeInitialized'] = onRuntimeInitialized;

function onRuntimeInitialized() {
	_websetup();
	wireupKeyboard();
	runloop();
	welcomeMessage();
}

const serialPrint = (() => {
	// Since the KIM-1 was designed to hook up to a teletype, a simple
	// "glass tty" is all we need. No real cursor handling is needed.

	const charsToSkip = [
		10 // Linefeed character we just ignore entirely
	];

	const rows = 25;
	const colums = 80;

	// Make sure our terminal display is at least this big.
	const spacer = new Array(colums).fill(" ").join("");
	const cursor = `<span class='cursor'>${String.fromCharCode(9608)}</span>`;

	const lines = [];
	lines[0] = "";

	return (codePoint) => {
		const ch = String.fromCodePoint(codePoint);

		if (charsToSkip.indexOf(codePoint) !== -1) {
			return;
		}

		const l = lines[lines.length - 1];
		const l_no_cursor = l.replace(cursor, "");

		if (codePoint == 13) {
			// Carriage return? Start a new line
			lines[lines.length - 1] = l_no_cursor;
			lines.push(cursor);
		} else {
			lines[lines.length - 1] = l_no_cursor + ch + cursor;
		}

		if (l_no_cursor.length === 80) {
			serialPrint(13);
		}

		while (lines.length > rows) {
			lines.shift();
		}

		lines[0] = spacer;

		document.querySelector("#serial").innerHTML = lines.join("\r");
	};
})();
