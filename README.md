# alsa-controller
Synchronize ALSA controls

# About
alsa-controller will copy the changes in value of one ALSA control to another.

# Build

Install dependencies.
On Ubuntu:
```
sudo apt install cmake libasound2-dev
```
Build
```
cmake .
```
Test
```
./cmake-build-release/alsa-controller -h
Usage:
  alsa-controller <src ctrl name> <dest ctrl name> <min dest volume>

Example:
  alsa-controller "Speaker Playback Volume" "Speaker Digital Gain" 90
```

# Usage
If ran without any parameters, it will monitor the changes in `Speaker Playback Volume` and will keep `Speaker Digital Gain` synchronized.
To check the names of all available controls:
```
amixer contents
```
To check individual ones:
```
amixer cget numid=7
numid=7,iface=MIXER,name='Speaker Playback Volume'
  ; type=INTEGER,access=rw---R--,values=2,min=0,max=87,step=0
  : values=69,69
  | dBscale-min=-65.25dB,step=0.75dB,mute=0
```
```
amixer cget numid=3
numid=3,iface=MIXER,name='Speaker Digital Gain'
  ; type=INTEGER,access=rw---R--,values=1,min=0,max=200,step=0
  : values=177
  | dBscale-min=-100.00dB,step=1.00dB,mute=0
```
The value of the monitored control will be scaled appropriatelly for the updated control. The maximum value of the source will always map to the maximum value of the destination. If the source value is zero the destination value will be set to zero too. If the destination control needs to start at a higher value then the third argument will allow this.

So for example, running `alsa-controller "Speaker Playback Volume" "Speaker Digital Gain" 90` will map the source `0,1..87` into destinations `0,90..200`

# TODO
* Make it a systemd service
