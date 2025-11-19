Arduino Tetris
============================
A complete Tetris game for ESP32 with 10x20 LED matrix and web interface control.

Features
--------
- Classic Tetris gameplay on LED matrix
- Web-based control interface
- Real-time score display
- High score tracking with EEPROM storage
- Adjustable brightness control
- Game pause/resume functionality
- Mobile-friendly web interface

Hardware Requirements
---------------------
- ESP32 development board
- WS2812B LED strip (NeoPixel) - 200 LEDs (10x20 matrix)
- 5V power supply for LED strip (adequate amperage)
- WiFi network access

Wiring
------
- LED Data Pin: GPIO13
- LED VCC: 5V (with sufficient power supply)
- LED GND: GND

Installation
------------
1. Set up Arduino IDE:
   - Install ESP32 board support (EEPROM.h included)
   - Install required libraries:
     * AsyncTCP (I used: https://github.com/dvarrel/AsyncTCP)
     * ESPAsyncWebServer (I used: https://github.com/dvarrel/ESPAsyncWebSrv)
     * Adafruit NeoPixel (I used: https://github.com/adafruit/Adafruit_NeoPixel)


2. Configure the code:
   - Enter WiFi credentials in `ssid` and `password` variables
   - Adjust brightness in `BRIGHTNESS` constant (0.0 - 1.0)

3. Upload to ESP32

Usage
-----
1. Start ESP32
2. Open Serial Monitor (115200 baud)
3. Copy IP address from Serial Monitor output
4. In web browser: http://[IP-ADDRESS]
5. Control via web interface or keyboard

Controls
--------
Web Interface:
- Left/Right arrows: Move piece
- Down arrow: Drop faster
- Rotate buttons: Rotate piece
- Pause button: Pause/resume game

Keyboard:
- A/D: Left/Right movement
- S: Drop faster
- Q/E: Rotate left/right
- W: Pause/resume

Game Features
-------------
- Score system: 40, 100, 300, 1200 points for 1-4 lines
- Increasing difficulty (fall speed acceleration)
- Game over detection with restart
- High score tracking (top 5 scores)
- Initials entry for high scores

Technical Details
-----------------
- Matrix: 10x20 grid
- 7 Tetris pieces with distinct colors
- Piece rotation and collision detection
- Row clearing and line scoring
- EEPROM storage for high scores
- Async web server for responsive control

Troubleshooting
---------------
- Ensure adequate power supply for LEDs
- Check WiFi credentials
- Verify correct GPIO pin connection
- Monitor Serial Monitor for debug information
- Ensure devices are on same WiFi network

License
-------
Open source - feel free to modify and distribute
