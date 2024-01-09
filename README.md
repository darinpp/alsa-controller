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
On Fedora use `alsa-lib-devel`

Build
```
cmake . && make
```
Test
```
./alsa-controller -h
Usage:
  alsa-controller [flags]

Example:
  alsa-controller -s name="Speaker Playback Volume" -d iface=CARD,name="Speaker Digital Gain" -m 90

Flags:
  -s, --src string          Id of the source control. It will be monitors and its changes will trigger a change in the destination control.
                            (default: name="Speaker Playback Volume")

  -d, --dest string         Id of the destination control. It will be updated each time the source is updated.
                            (default: iface=CARD,name="Speaker Digital Gain")

  -m, --min_dest double     This will be used as a starting value for the destination control when the source is at 1.
                            If the source value is zero, the destination will always be zero too.
                            (default: 90)

  --src_hw string           Id of the source device.
                            (default: hw:1)

  --src_hw string           Id of the destination device.
                            (default: the --src_hw value)

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
numid=3,iface=CARD,name='Speaker Digital Gain'
  ; type=INTEGER,access=rw---R--,values=1,min=0,max=200,step=0
  : values=177
  | dBscale-min=-100.00dB,step=1.00dB,mute=0
```
The value of the monitored control will be scaled appropriatelly for the updated control. The maximum value of the source will always map to the maximum value of the destination. If the source value is zero the destination value will be set to zero too. If the destination control needs to start at a higher value then using `-m,--min_dest` will allow this.

So for example, running `alsa-controller -m 90` will map the source `0,1..87` into destinations `0,90..200`

# Run as a service
* copy to `/usr/loca/bin` or some other location
```
sudo cp cmake-build-release/alsa-controller /usr/local/bin/
```
* create the service file
```
cat /etc/systemd/system/alsa-controller.service 
[Unit]
Description=alsa-controller
After=syslog.target sound.target

[Service]
Type=simple
ExecStart=/usr/local/bin/alsa-controller

[Install]
WantedBy=multi-user.target
```
* enable and start the service
```
sudo systemctl daemon-reload
sudo systemctl enable alsa-controller.service
sudo systemctl start alsa-controller.service
sudo systemctl status alsa-controller.service 
● alsa-controller.service - alsa-controller
     Loaded: loaded (/etc/systemd/system/alsa-controller.service; enabled; preset: enabled)
     Active: active (running) since Sat 2023-10-28 21:20:16 PDT; 10min ago
   Main PID: 10256 (alsa-controller)
      Tasks: 1 (limit: 33342)
     Memory: 488.0K
        CPU: 121ms
     CGroup: /system.slice/alsa-controller.service
             └─10256 /usr/local/bin/alsa-controller
```
* check that the destination control is updated when the source control's value changes (volume up/down)
```
sudo journalctl -u alsa-controller -f
Oct 28 21:26:31 host alsa-controller[10256]: Event read L 81,R 81 (max 87) -> bass 192 (max 200)
Oct 28 21:26:31 host alsa-controller[10256]: Event read L 78,R 78 (max 87) -> bass 188 (max 200)
```
