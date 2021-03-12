""" A small program to collect audio recordings sent over serial during inference

This Python 3 program receives a series of PCM audio samples over serial and writes it to a wav file. The following arguments need to be provided:
`--label` This is the dataset label for the audio being collected. The files are named and saved into different subdirectories based on this label.
`--dev` This is the path to the serial device for the Arduino board.
"""
import json
import os, time, hmac, hashlib
import requests
import serial
import argparse
import errno
import wave
import audioop
import struct
from datetime import datetime
from scipy.io.wavfile import write

# Parses command line arguments
parser = argparse.ArgumentParser()
parser.add_argument('--label', type=str, help='label for data', required=True)
parser.add_argument('--dev', type=str, help='serial device', required=True)

args = parser.parse_args()

# These byte patterns are used to denote the start and end of a transmission
start_marker = bytearray(b'\x00\xff\x00\xff\x00\xff\x00\xff')
end_marker = bytearray(b'\x00\xff\x00\xff\xff\x00\xff\x00')
last_eight = bytearray(8)

# These values should be adjusted to match the sample length and sample rate used in the Arduino program
sample_sec = 5
sample_hz = 16000

samples_required = sample_sec * sample_hz
bytes_required = samples_required * 2
samples_recorded = 0
bytes_recorded = 0
recording = False

ser = serial.Serial(args.dev,115200) # setups the serial port

ser.flush()
samples = bytearray()
while bytes_recorded < bytes_required:
    ser_bytes = ser.read(1)     # read in a single byte

    if (recording):                             # if recording has started
        samples.append(ser_bytes[0])            # add the received bytes to the sample     
        bytes_recorded=bytes_recorded + 1       # increase count

    last_eight = last_eight[1:8] + ser_bytes    # create a rolling buffer of the last 8 bytes received
    if (last_eight == start_marker):            # if this is the start marker
        recording = True                        # start recording
        print("Start Transfer")
    if (last_eight == end_marker):              # if an end marker was received, stop recording
        recording = False
        samples = samples[:-8]                  # the last 8 bytes were the end marker, so remove them from the samples
        bytes_recorded=bytes_recorded - 8
        print("Stop Transfer")

ser.close()

print("Bytes Recorded: {}".format(bytes_recorded))


mydir = "./audio-capture/" + args.label     # Directory structure where the wav file will be saved
try:
    os.makedirs(mydir)
except OSError as e:
    if e.errno != errno.EEXIST:
        raise  # This was not a "directory exist" error..
filename = "{}/{}_{}.wav".format(mydir,args.label,datetime.now().strftime('%Y-%m-%d-%H-%M-%S'))
 
output = wave.open(filename, 'wb')
output.setnchannels(1)                          # the audio is mono, so there is a single channel
output.setsampwidth(2)                          # it is 16 bit audio
output.setframerate(sample_hz) 
output.writeframes(samples)


