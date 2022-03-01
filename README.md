# Sony Shooting 

Simple app based on the Sony Camera RDK to write datetime on the camera and then start and interval shooting. The app creates a log file with the UNIX timestamp when the trigger is sent

## Building

In the build folder there is already a compiled version for a 32bit ARMv8 (Raspberry Pi). If the app needs to be recompiled just go in the build folder and run 

```
cmake --build . 
```

## Running

To run the code just run from the build folder

```
.\shooting fps Nphoto
```

where:

- *fps* is the number of frame per seconds
- *Nphoto* is the number of photos to be taken (if -1 it just continues)

## Suggestions

To keep the shooting as fast as possible, set the camera to save local file and to manual focus. However, the max fps needs to be tested and is strongly dependent on the SD Card choosen and the file format
