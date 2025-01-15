# umgr-hands
The alternative to AIR sensor using webcam. It's inspired by chuni-hands.
It's for users who the ac slider is owned and works well but the air tower is not by some issue.

## Requirements
- Webcam
- OpenCV2
- com0com

## Usage
```
+- umgr-hands -+                             +-- UMIGURI --+
|  UmgrIo Emu  | <-------[ com0com ]-------> | UmgrIO port |
+--------------+ COM11                COM??  +-------------+
      |                           (your choice)
  [ Webcam ]
      detects your arms
```
1. Install OpenCV2.
2. Build with Visual Studio.
3. Install com0com and setup one loopback.
   - The number of com port used by umgr-hands is configured in `AppImpl.cpp`. (default is COM11)
4. Setup your webcam.
5. Connect as UmgrIO to UMIGURI via the loopback COM port.
6. Launch the built umgr-hands.
7. Press <kbd>w</kbd> to capture an initial frame.
   - Your arms are detected from the difference of frames.
8. If you want to kill umgr-hands, press <kbd>q</kbd>

> [!NOTE]
> You can adjust detection range by changing `rangeTop`, `rangeBottom`, `left`, `right`, `top` and `bottom` in `src/AppImpl.cpp` function at `AppImpl.cpp`. (because this project is incomplete)

## License
MIT License
