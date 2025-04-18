/*
 *	SMF to SEQ
 *
 *	Convert Standard MIDI file to event array
 *	Output file as sequence.h: to be compiled with ino files
 *
 *	This program uses part of jasmid to decode and parse .mid files
 *	Please refer to jasmid/LICENSE for the jasmid license
 *
 *	Run this file with node.js
 *	$ node smf2seq.js [SMF filename]
 *
 *	2016 by ilufang
 */

Midifile = require("./jasmid/midifile.js");
fs = require("fs");

// # Read and parse SMF
var filename = process.argv[2] ? process.argv[2] : "song.mid";
var midi_blob = fs.readFileSync(filename);
var t = "";
for (var i = 0; i < midi_blob.length; i++) {
  t += String.fromCharCode(midi_blob[i] & 255);
}
try {
  midi = Midifile(t);
} catch (e) {
  console.error("Could not parse midi.");
  process.exit(1);
}

if (midi.header.formatType != 1) {
  console.error(
    "MIDI Type not supported. Expect 1, got " + midi.header.formatType
  );
  process.exit(1);
}

// fs.writeFileSync("midi.json",JSON.stringify(midi,null,'\t'));
// process.exit();

// # Process events

var ptr = []; // iteration pointer per track
seq = []; // merged result

// convert delta time to accumulative time
for (var t in midi.tracks) {
  ptr.push(0); // Irrelevant to the calculation, just initialize ptr

  var tick = 0;
  var width = 0,
    maxwidth = 0;
  for (var i in midi.tracks[t]) {
    tick += midi.tracks[t][i].deltaTime;
    midi.tracks[t][i].time = tick;
    midi.tracks[t][i].track = parseInt(t); // Still need to keep track of the track
  }
}

// Merge the tracks
var time = 0;
while (true) {
  // 'Pop' front
  var minTrack = 0;
  for (var t in midi.tracks) {
    if (!midi.tracks[t][ptr[t]]) {
      if (minTrack == t) {
        // In case a prior tracks are shorter, assign the default minTrack to a latter one
        minTrack++;
      }
      continue;
    }
    if (
      midi.tracks[t][ptr[t]].time < midi.tracks[minTrack][ptr[minTrack]].time
    ) {
      minTrack = t;
    }
  }
  if (minTrack == midi.tracks.length) {
    // No tracks have events left
    break;
  }
  seq.push(midi.tracks[minTrack][ptr[minTrack]]);
  ptr[minTrack]++;
}

// Scan for consecutive notes
/*
time = 0;
var notes_off = {};
for (var i = 0; i < seq.length; i++) {
	if (seq[i].time != time) {
		time = seq[i].time;
		notes_off = {};
	}
	if (seq[i].type=="channel") {
		if (seq[i].subtype=="noteOff") {
			notes_off[seq[i].noteNumber] = i;
		} else if (seq[i].subtype=="noteOn") {
			if (notes_off[seq[i].noteNumber]) {
				seq[notes_off[seq[i].noteNumber]].time -= 20;
			}
		}
	}
}
*/

// Regenerate deltatime
var tempo = 0;

var notes = [],
  note_params = [];

for (var i = 0; i < seq.length; i++) {
  if (seq[i + 1]) {
    seq[i].deltaTime = seq[i + 1].time - seq[i].time;
  } else {
    seq[i].deltatime = 0;
  }
  prevTime = seq[i].time;

  if (seq[i].type == "meta" && seq[i].subtype == "setTempo") {
    tempo =
      (1.0 * seq[i].microsecondsPerBeat) / 1000 / midi.header.ticksPerBeat;
  } else if (seq[i].type == "channel") {
    if (seq[i].subtype == "noteOff") {
      seq[i].velocity = 0;
    }
    if (seq[i].subtype == "noteOn" || seq[i].subtype == "noteOff") {
      seq[i].velocity *= 128 / 96;
      if (seq[i].velocity > 127) seq[i].velocity = 127;
      notes.push(seq[i].noteNumber);
      note_params.push((seq[i].deltaTime << 4) + (seq[i].velocity >> 3));
    }
  }
}

fs.writeFileSync("midi.json", JSON.stringify(seq, null, "\t"));

// # Generate sin table
var file = "// MIDI events\n// Generated by smf2seq.js\n";

var sine_sample = [],
  sine_sample_size = 256;
for (var i = 0; i < sine_sample_size; i++) {
  sine_sample.push(
    128 + Math.round(128 * Math.sin((2 * Math.PI * i) / sine_sample_size))
  );
}

file += "const unsigned char sine[] = {" + sine_sample.join(",") + "};\n";
file += "#define TEMPO " + tempo + "\n";
file += "#define SONG_LEN " + notes.length + "\n";
file += "PROGMEM const unsigned char notes[] = {" + notes.join(",") + "};\n";
file += "PROGMEM const int params[] = {" + note_params.join(",") + "};\n";

console.log("Using memory: " + notes.length * 3);
console.log("Total memory: 32256");

// # Write to file
fs.writeFileSync("sequence.h", file);
