## MIDI

Standard MIDI files can be imported with the desktop version of FamiStudio. 

Notes, time signature and tempo changes will be imported. Only blank instruments, named after their GM instrument, will be created. Users should not expect the imported song to sound anything like the original. 

Moreover, since the NES can only play one note at a time for a given channel, any kind of polyphony is not supported. Users are responsible for properly processing their MIDI files before importing them. That being said, this can be a handy feature to bring notes from an old project into FamiStudio.

![](images/ImportMIDI.png#center)

Available options:

* **Expansion**: Allows selecting the expansion audio to set for the project. This will add extra sound channels.
* **Polyphony behavior**: Although polyphony is not supported, if it were to happen, there are 2 ways FamiStudio can resolve it:
	* **Favor most recent note**: If a new note is triggered while one was still playing, the old note will be stopped and the new note will start.
	* **Favor currently playing note**: If a new note is triggered while one was still playing, the old note will keep playing and the new note will be ignored.
* **Measures per pattern**: Having a single measure per pattern tends to create too many patterns. FamiStudio can try to pack multiple measures in a single pattern. Note that tempo/time signature changes will always cause a new pattern to start.
* **Import velocity as volume**: Will convert velocity values to volume effects. 
* **Create PAL project**: Will create a PAL project and do all BPMs calculations using PAL speeds.

At the bottom of the dialog is the channel mapping table. For each of the NES channels, you can specific which MIDI data should be put in that channel. Double-clicking on a NES channel will bring up more options. 

![](images/ImportMIDIChannel.png#center)

There are 3 MIDI sources from which MIDI data can be read from:

* **Channel**: Data from the specified MIDI channel will be assigned to the NES channel. This is basically a 1:1 mapping between NES an channel and a MIDI channel.
* **Track**: If the MIDI file has proper tracks defined, data from the that track can be assigned to the NES channel.
* **None**: Will not read any data in this channel. The NES channel will be left blank.

The MIDI channel 10 is special an specific keys can be selected to filter specific drum sounds.