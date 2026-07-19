## Tempo modes

FamiStudio supports two tempo modes : **FamiStudio** and **FamiTracker**. 

The tempo mode will affect how the tempo of you songs is calculated, how much control you have over the notes of your song and how your song plays on PAL systems. Note that changing the tempo mode when you have songs created is possible, but not recommended, the conversion is still quite crude at the moment.

### FamiStudio Tempo Mode

FamiStudio tempo modes gives full control over every frame (1/60th of a second on NTSC, 1/50th on PAL). It is the default mode. In this mode you will see the individual frames in the piano roll and will have more precise control on where each note starts/end. 
For example, a C-D-E-F scale where each note is stopped for 1 frame between each note will look like this using FamiStudio tempo. The dashed lines separate individual frames, so you can leave and empty frame 1 frame before the new note starts. Very intuitive and visual.

![](images/TempoExampleFamiStudio.png#center) 

FamiStudio tempo mode let's you simply choose a BPM value for the song (or an individual pattern) and will automatically choose the appropriate number of frames to make each notes. Some BPMs will require the use of a *groove* which is an uneven sequence of frames. 

For example, at 142 BPM (in NTSC), FamiStudio will know to use a 7-6-6 groove, which mean that the first note will be 7 frames long, then followed by two notes of 6 frames, and the whole thing will repeat until there is a tempo change in the song. But in order to keep the piano roll nice and even, FamiStudio will only only display the minimum values of the groove, 6 in our example. This mean that out of 19 frames in the groove, you only have control over 18. In other words, every 3rd note, FamiStudio will inject an empty frame for which you do not have any control. Effects, instrument envelopes & arpeggios will still advance on these empty frames, but otherwise no new note will be processed. You can tell FamiStudio *where* to inject this empty frame, by changing the **Groove Padding Mode** (Beginning, Middle or End).

On of the limitation of FamiStudio tempo mode is that it will limit your ability to suddenly changes tempo in the middle of a pattern. When using FamiStudio tempo, you can only change the BPM at the start of a new pattern.

### FamiTracker Tempo Mode

FamiTracker tempo has a limited visual granularity and relies on effects (delayed notes/cuts) to get frame-level precision. It uses the speed/tempo paradigm. Please check out the [FamiTracker documentation](http://famitracker.com/wiki/index.php?title=Fxx) for a detailed explanation on how the playback speed is affected. If you import a FamiTracker Text or FTM file, the project will be in this tempo mode. 

Same example, but using FamiTracker tempo. Here we do not have the individual frames so we need to use a "delayed cut" effect to achieve the same result. The delayed cut tells the sound engine to insert a stop note after 9 frames have elapsed, achieving the exact same result. That being said, one might argue that it is not very visual and feels like using a Tracker. Moreover, this would not always work correctly on PAL.

![](images/TempoExampleFamiTracker.png#center) 

### Which one to use?

You should use FamiStudio tempo mode if:

* You want to be able to visually control the position of every note at a frame-level precision.
* You are not planning to do smooth tempo changes during the song and you are OK with changing the tempo only at the start of a new pattern.

You should use FamiTracker tempo mode if:

* You are OK with using effects tracks (delayed notes, cuts) to finely tune the start/end of each notes.
* You need compatibility with FamiTracker.
* You want to have smooth tempo changes during the song, especially in the middle of a pattern.