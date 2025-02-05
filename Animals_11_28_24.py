#!/usr/bin/env python3

# -*- coding: utf-8 -*-
# Created on Wed Sep 21 10:07:13 2022 by Ariana Anderson
# Last updated: 5/31/2024
# Purpose: Backend script to transcribe and analyze spoken feedback (animal naming task) obtained via the Chorus app.
# Functionality includes: audio preprocessing, speech-to-text conversion, animal word counting, and duplicate removal.
# Dependencies: tkinter for GUI, pydub for audio processing, speech recognition, pandas for data handling, etc.

# Libraries and Modules Required
import speech_recognition as sr  # For speech recognition (speech-to-text)
import os  # For file operations
import re  # For regular expressions
import csv  # For reading/writing CSV files
import pandas as pd  # For data manipulation
import numpy as np  # For array and numerical operations
import sys  # For handling command-line arguments and system operations
from pydub import AudioSegment  # For audio processing
from pydub.silence import split_on_silence  # For detecting and splitting audio on silence

# Function to count animal names and their occurrences in transcribed speech
def countAnimals(myString):
    """Count occurrences of animal names in a transcribed string."""
    
    # Load size adjectives (e.g., giant, small) from file
    file = open("./SizeAdjectives.csv", "r", encoding='utf-8-sig')
    size_words = list(csv.reader(file, delimiter=","))
    file.close()
    
    search_list = size_words
    arr0 = [0] * len(search_list)  # Initialize counter array
    myString = myString.lower()  # Convert the speech text to lowercase
    
    # Remove size words (to avoid counting animals with size adjectives separately)
    for x in range(len(search_list)):
        size_name = re.sub(r'[^A-Za-z0-9 ]+', '', str(search_list[x])).lower()
        arr0[x] = myString.count(size_name)  # Count occurrences of size words
        if arr0[x] > 0:
            myString = myString.replace(size_name, '')  # Remove size adjectives from the text
    
    # Load animal names from a CSV and search for them in the speech
    file = open("./animals_Sorted.csv", "r", encoding='utf-8-sig')
    data = list(csv.reader(file, delimiter=","))
    file.close()
    
    search_list = data
    arr0 = [0] * len(search_list)
    myString = " " + myString.lower() + " "  # Add spaces to avoid partial matches
    
    # Count animal occurrences
    for x in range(len(search_list)):
        animal_name = " " + re.sub(r'[^A-Za-z0-9 ]+', '', str(search_list[x])).lower() + " "
        arr0[x] = myString.count(animal_name)
        if arr0[x] > 0:
            myString = myString.replace(animal_name.strip(), '')  # Remove found animal name from text
    
    # Return a DataFrame with animals and their counts
    df = pd.DataFrame({'Animals': search_list, 'Count': arr0})
    words_used = df.loc[df['Count'] > 0]
    
    return words_used

# Function to handle plural forms of animal names and avoid duplicate counting
def countPlurals(myDF_fromCountAnimals):
    """Handle plural forms of animal names and check for duplicate counting."""
    
    words_used = myDF_fromCountAnimals
    word = words_used['Animals'].tolist()
    words = words_used['Animals'].values.tolist()
    
    # Generate plural forms of animal names (handle various pluralization rules)
    plural = []
    for i in range(0, len(words)):
        modifiedWord = ""
        word_i = re.sub(r'[^A-Za-z0-9 ]+', '', str(words[i])).lower()
        
        if word_i.endswith("y"):
            modifiedWord = word_i[0:len(word_i) - 1] + "ies"
        elif word_i.endswith("f"):
            modifiedWord = word_i[0:len(word_i) - 1] + "ves"
        elif word_i.endswith("s") or word_i.endswith("x") or word_i.endswith("z") or word_i.endswith("ch") or word_i.endswith("sh"):
            modifiedWord = word_i + "es"
        elif word_i.endswith("us"):
            modifiedWord = word_i[0:len(word_i) - 2] + "i"
        else:
            modifiedWord = word_i + "s"
        plural.append(modifiedWord)
    
    # Add some common irregular plurals (e.g., mice, geese)
    plural += ["mice", "geese", "oxen", "monkeys", "turkeys"]
    
    # Count overlaps between singular and plural forms
    animals_set_loss = 0
    for plural_word in plural:
        if word.count(plural_word) > 0:
            animals_set_loss += 1
    
    return animals_set_loss

# Audio Preprocessing and Recognition Setup
filename = "./AnimalnamingBob1.m4a"  # Audio file to process
print("I want you to bring to mind as many animals as you can think of in about a minute. Ready? Go!")

# Main program execution starts here
if __name__ == '__main__':
    # Check if additional arguments are passed for batch processing or output directory
    n = len(sys.argv)
    print("Total arguments passed:", n)
    
    if n == 3:
        argument1 = sys.argv[1]
        argument2 = sys.argv[2]
        filename = argument1
        outputdir = argument2
        print("Arguments:", argument1, argument2)
        print("Filename:", filename)
        print("Output Directory:", outputdir)

# File handling (get file name and extension)
split_tup = os.path.splitext(filename)
file_name = re.sub(r'[^\w]', '', split_tup[0])
file_extension = split_tup[-3:]
print("File Name:", file_name)
print("File Extension:", file_extension)

# Check if the file is in wav format, if not, convert it
if file_extension == "wav":
    print("File is in wav format! No conversion necessary.")
else:
    print("Converting input file to .wav format. Only the first 60 seconds of speech will be used.")
    ffmpeg_command = f"/usr/local/bin/ffmpeg -y -i {filename} -acodec pcm_s16le -ac 1 -ar 16000 {file_name}.wav"
    os.popen(ffmpeg_command).read()

# Trim the audio file to 60 seconds
ffmpeg_command = f"/usr/local/bin/ffmpeg -i {file_name}.wav -ss 00:00:00 -to 60 -c copy -y {file_name}_final.wav"
os.popen(ffmpeg_command).read()

# Recognize speech from the audio file using Google Speech API
sample_audio = sr.AudioFile(f"{file_name}_final.wav")
r = sr.Recognizer()
r.pause_threshold = 5  # Increase pause threshold for better accuracy
r.energy_threshold = 50  # Adjust energy threshold for background noise

with sample_audio as audio_file:
    r.adjust_for_ambient_noise(audio_file)
    audio_content = r.record(audio_file, duration=60)  # Limit to 60 seconds of audio

# Recognize speech and return all alternatives
myStringWhole = r.recognize_google(audio_content, show_all=True)
print("Whole Unparsed Audio File:")
print(myStringWhole)

# Further processing of audio chunks and recognition
sound = AudioSegment.from_wav(f"{file_name}_final.wav")
audio_chunks = split_on_silence(sound, min_silence_len=800, silence_thresh=-40)

# Recombine audio chunks if needed, ensure target length is met
output_chunks = [audio_chunks[0]]
for chunk in audio_chunks[1:]:
    if len(output_chunks[-1]) < 30000:  # Target chunk length of 30 seconds
        output_chunks[-1] += chunk
    else:
        output_chunks.append(chunk)

# Export the chunks and process each one
myStringUnpadded = []
myStringPadded = []
df = pd.DataFrame()

for i, chunk in enumerate(output_chunks):
    output_file = f"chunk{i}.wav"
    chunk.export(output_file, format="wav")
    
    # Use speech recognition on each chunk and analyze animal words
    sample_audio = sr.AudioFile(output_file)
    with sample_audio as audio_file:
        r.adjust_for_ambient_noise(audio_file)
        audio_content = r.record(audio_file)
    myString2 = r.recognize_google(audio_content, show_all=True)
    
    # Count the animals mentioned in the transcription
    words_used = countAnimals(myString2['alternative'][0]['transcript'])
    myStringUnpadded.append(myString2['alternative'][0]['transcript'])

# Output results and analysis
print("This is the string for unpadded audio:")
print(myStringUnpadded)
