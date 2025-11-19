#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

// WLAN credentials
const char* ssid = "Name";
const char* password = "Password";

// Helligkeit (0.0 - 1.0)
const float BRIGHTNESS = 0.5;

// Game constants
#define GRID_W (10)
#define GRID_H (20)
#define STRAND_LENGTH (GRID_W*GRID_H)
#define LED_DATA_PIN (13)
#define BACKWARDS (0)
#define PIECE_W (4)
#define PIECE_H (4) 
#define NUM_PIECE_TYPES (7)
#define DROP_MINIMUM (25)
#define DROP_ACCELERATION (20)
#define INITIAL_MOVE_DELAY (50)
#define INITIAL_DROP_DELAY (300)
#define INITIAL_DRAW_DELAY (30)
#define MAX_TOP_SCORES 5

// Webserver and WebSocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Piece definitions (unchanged from original code)
const char piece_I[] = {
  0,0,0,0, 1,1,1,1, 0,0,0,0, 0,0,0,0,
  0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0,
  0,0,0,0, 0,0,0,0, 1,1,1,1, 0,0,0,0,
  0,1,0,0, 0,1,0,0, 0,1,0,0, 0,1,0,0
};

const char piece_L[] = {
  0,0,1,0, 1,1,1,0, 0,0,0,0, 0,0,0,0,
  0,1,0,0, 0,1,0,0, 0,1,1,0, 0,0,0,0,
  0,0,0,0, 1,1,1,0, 1,0,0,0, 0,0,0,0,
  1,1,0,0, 0,1,0,0, 0,1,0,0, 0,0,0,0
};

const char piece_J[] = {
  1,0,0,0, 1,1,1,0, 0,0,0,0, 0,0,0,0,
  0,1,1,0, 0,1,0,0, 0,1,0,0, 0,0,0,0,
  0,0,0,0, 1,1,1,0, 0,0,1,0, 0,0,0,0,
  0,1,0,0, 0,1,0,0, 1,1,0,0, 0,0,0,0
};

const char piece_T[] = {
  0,1,0,0, 1,1,1,0, 0,0,0,0, 0,0,0,0,
  0,1,0,0, 0,1,1,0, 0,1,0,0, 0,0,0,0,
  0,0,0,0, 1,1,1,0, 0,1,0,0, 0,0,0,0,
  0,1,0,0, 1,1,0,0, 0,1,0,0, 0,0,0,0
};

const char piece_S[] = {
  0,1,1,0, 1,1,0,0, 0,0,0,0, 0,0,0,0,
  0,1,0,0, 0,1,1,0, 0,0,1,0, 0,0,0,0,
  0,0,0,0, 0,1,1,0, 1,1,0,0, 0,0,0,0,
  1,0,0,0, 1,1,0,0, 0,1,0,0, 0,0,0,0
};

const char piece_Z[] = {
  1,1,0,0, 0,1,1,0, 0,0,0,0, 0,0,0,0,
  0,0,1,0, 0,1,1,0, 0,1,0,0, 0,0,0,0,
  0,0,0,0, 1,1,0,0, 0,1,1,0, 0,0,0,0,
  0,1,0,0, 1,1,0,0, 1,0,0,0, 0,0,0,0
};

const char piece_O[] = {
  1,1,0,0, 1,1,0,0, 0,0,0,0, 0,0,0,0,
  1,1,0,0, 1,1,0,0, 0,0,0,0, 0,0,0,0,
  1,1,0,0, 1,1,0,0, 0,0,0,0, 0,0,0,0,
  1,1,0,0, 1,1,0,0, 0,0,0,0, 0,0,0,0
};

const char *pieces[NUM_PIECE_TYPES] = {
  piece_S, piece_Z, piece_L, piece_J, piece_O, piece_T, piece_I
};

const long piece_colors[NUM_PIECE_TYPES] = {
  0x009900, 0xFF0000, 0xFF8000, 0x000044, 0xFFFF00, 0x00FFFF, 0xFF00FF
};

// Score entry structure
struct ScoreEntry {
    char initials[4];
    int score;
};

// Global variables
bool game_started = false;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRAND_LENGTH, LED_DATA_PIN, NEO_RGB + NEO_KHZ800);
ScoreEntry topScores[MAX_TOP_SCORES];
int piece_id;
int piece_rotation;
int piece_x;
int piece_y;
char piece_sequence[NUM_PIECE_TYPES];
char sequence_i = NUM_PIECE_TYPES;
long last_move;
long move_delay = INITIAL_MOVE_DELAY;
long last_drop;
long drop_delay = INITIAL_DROP_DELAY;
long last_draw;
long draw_delay = INITIAL_DRAW_DELAY;
long grid[GRID_W*GRID_H];
bool command_left = false;
bool command_right = false;
bool command_drop = false;
bool command_pause = false;
bool command_rotateleft = false;
bool command_rotateright = false;
int score = 0;
int top_score = 0;

// Function prototypes
void draw_letters(const char* text, int start_x, int start_y, long color);
void scroll_text(const char* text, int start_y, long color, int delay_ms);
void p(int x, int y, long color);
void draw_grid();
void choose_new_piece();
void erase_piece_from_grid();
void add_piece_to_grid();
void delete_row(int y);
void fall_faster();
void remove_full_rows();
void try_to_rotate_pieceright();
void try_to_rotate_pieceleft();
int piece_can_fit(int px, int py, int pr);
int piece_off_edge(int px, int py, int pr);
int piece_hits_rubble(int px, int py, int pr);
void draw_restart();
void all_white();
void game_over();
void game_over_loop_leds();
void try_to_drop_piece();
void try_to_drop_faster();
void react_to_player();
int game_is_over();
void draw_pause();

// Helligkeits-Funktion
long applyBrightness(long color, float brightness) {
    if (brightness <= 0.0f) return 0;
    if (brightness >= 1.0f) return color;
    
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    r = (uint8_t)(r * brightness);
    g = (uint8_t)(g * brightness);
    b = (uint8_t)(b * brightness);
    
    return ((long)r << 16) | ((long)g << 8) | b;
}

String cleanMessage(String msg) {
    String cleaned = "";
    for (size_t i = 0; i < msg.length(); i++) {
        char c = msg[i];
        if (isPrintable(c)) {
            cleaned += c;
        }
    }
    return cleaned;
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        // Empfangene Daten in String umwandeln und bereinigen
        String message = String((char*)data).substring(0, len);
        message.trim();
        message.toLowerCase();
        message = cleanMessage(message);  // Störzeichen entfernen

        Serial.println("Received (cleaned): " + message);

        if (message == "drop") {
            Serial.println("Befehl: Drop");
            command_drop = true;
        } else if (message == "pause") {
            Serial.println("Befehl: Pause");
            if (!game_started) {
                game_started = true;
                command_pause = false;
            } else {
                command_pause = !command_pause;
            }
        } else if (message == "rotateleft") {
            Serial.println("Befehl: Drehen nach links");
            command_rotateleft = true;
        } else if (message == "rotateright") {
            Serial.println("Befehl: Drehen nach rechts");
            command_rotateright = true;
        } else if (message == "left") {
            Serial.println("Befehl: Links");
            command_left = true;
        } else if (message == "right") {
            Serial.println("Befehl: Rechts");
            command_right = true;
        } else if (message.startsWith("initials:")) {
            String initials = message.substring(9);
            if(initials.length() == 3) {
                // Update the most recent high score entry
                for(int i = 0; i < MAX_TOP_SCORES; i++) {
                    if(topScores[i].score == score && topScores[i].initials[0] == ' ') {
                        strncpy(topScores[i].initials, initials.c_str(), 3);
                        topScores[i].initials[3] = '\0';
                        saveTopScores();
                        sendGameData();
                        break;
                    }
                }
            }
        } else {
            Serial.println("Unbekannter Befehl: " + message);
        }
    }
}

void sendGameData() {
    String json = "{\"score\":" + String(score) + "}";
    ws.textAll(json);
}

void saveTopScores() {
    EEPROM.put(0, topScores);
    EEPROM.commit();
}

void loadTopScores() {
    EEPROM.get(0, topScores);
    top_score = topScores[0].score;
}

void setup() {
    Serial.begin(115200);
    Serial.println("Tetris game starting...");
    Serial.print("Initial score: ");
    Serial.println(score);
    Serial.print("Helligkeit: ");
    Serial.println(BRIGHTNESS);
    
    EEPROM.begin(sizeof(ScoreEntry) * MAX_TOP_SCORES);
    loadTopScores();
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Tetris ESP32</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin: 0; background: #fff; }
    h2 { margin-top: 20px; }
    .circle-container {
        position: relative;
        width: 300px;
        height: 300px;
        border-radius: 50%;
        background-color: #ddd;
        display: flex;
        justify-content: center;
        align-items: center;
        clip-path: circle(90%);
        margin: 40px auto;
    }
    .button {
        position: absolute;
        width: 50%;
        height: 50%;
        background-color: #3498db;
        color: white;
        font-size: 18px;
        font-weight: bold;
        border: none;
        cursor: pointer;
        transition: 0.3s;
        display: flex;
        justify-content: center;
        align-items: center;
    }
    .button:hover { opacity: 0.8; }
    .left { top: 25%; left: 0; clip-path: polygon(0% 50%, 50% 0%, 100% 50%, 50% 100%); background-color: #3498db; }
    .right { top: 25%; right: 0; clip-path: polygon(0% 50%, 50% 0%, 100% 50%, 50% 100%); background-color: #e74c3c; }
    .drop { bottom: 0; left: 25%; clip-path: polygon(0% 50%, 50% 0%, 100% 50%, 50% 100%); background-color: #f39c12; }
    .pause { top: 0; left: 25%; clip-path: polygon(0% 50%, 50% 0%, 100% 50%, 50% 100%); background-color: #9b59b6; }
    .rotate-left { top: 0%; left: -10%; width: 60px; height: 60px; background-color: #2ecc71; clip-path: polygon(0% 50%, 100% 0%, 100% 100%); }
    .rotate-right { top: 0%; right: -10%; width: 60px; height: 60px; background-color: #2ecc71; clip-path: polygon(0% 0%, 100% 50%, 0% 100%); }
  </style>
</head>
<body>
  <h2>Score: <span id="currentScore">0</span></h2>

  <div class="circle-container">
    <button class="button left" onclick="sendCommand('left')">←</button>
    <button class="button right" onclick="sendCommand('right')">→</button>
    <button class="button drop" onclick="sendCommand('drop')">↓</button>
    <button class="button pause" onclick="sendCommand('pause')">⏸</button>
    <button class="button rotate-left" onclick="sendCommand('rotateleft')"></button>
    <button class="button rotate-right" onclick="sendCommand('rotateright')"></button>
  </div>

  <script>
    let socket;
    let keyInterval = null;

    function connectWebSocket() {
        socket = new WebSocket("ws://" + window.location.hostname + "/ws");
        socket.onopen = () => socket.send("ping");
        socket.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                document.getElementById('currentScore').textContent = data.score;
            } catch (e) {
                console.error("Parse error:", e);
            }
        };
        socket.onclose = () => setTimeout(connectWebSocket, 1000);
        socket.onerror = (e) => { console.error("WebSocket error:", e); socket.close(); };
    }
    connectWebSocket();

    function sendCommand(cmd) {
        if (socket && socket.readyState === WebSocket.OPEN) {
            socket.send(cmd);
        }
    }

    const repeatableKeys = {
        'a': 'left',
        'd': 'right',
        's': 'drop',
        'q': 'rotateleft',
        'e': 'rotateright'
    };

    let currentCommand = null;

    document.addEventListener('keydown', function(event) {
        const key = event.key.toLowerCase();
        if (repeatableKeys[key] && !keyInterval) {
            currentCommand = repeatableKeys[key];
            sendCommand(currentCommand);
            keyInterval = setInterval(() => sendCommand(currentCommand), 120);
        } else if (key === 'w') {
            sendCommand('pause');
        }
    });

    document.addEventListener('keyup', function(event) {
        if (keyInterval) {
            clearInterval(keyInterval);
            keyInterval = null;
            currentCommand = null;
        }
    });
  </script>
</body>
</html>
)rawliteral");

    });

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.begin();

    strip.begin();
    strip.show();

    for(int i=0; i<GRID_W*GRID_H; ++i) {
        grid[i] = 0;
    }

    randomSeed(analogRead(2) + analogRead(3));
    choose_new_piece();

    last_draw = last_drop = last_move = millis();
    score = 0;
    sendGameData();
}

void loop() {
    ws.cleanupClients();
    yield();

    long t = millis();

    if(game_started && !command_pause) {
        if(t - last_move > move_delay ) {
            last_move = t;
            react_to_player();
        }

        if(t - last_drop > drop_delay ) {
            last_drop = t;
            try_to_drop_piece();
        }

        if(t - last_draw > draw_delay ) {
            last_draw = t;
            draw_grid();
        }
    } else {
        draw_pause();
        delay(1);
    }
}

void react_to_player() {
    erase_piece_from_grid();
    
    if(command_rotateright) {
        try_to_rotate_pieceright();
        command_rotateright = false;
    }
    if(command_rotateleft) {
        try_to_rotate_pieceleft();
        command_rotateleft = false;
    }
    
    if(command_left) {
        if(piece_can_fit(piece_x-1,piece_y,piece_rotation)) {
            piece_x--;
        }
        command_left = false;
    }
    if(command_right) {
        if(piece_can_fit(piece_x+1,piece_y,piece_rotation)) {
            piece_x++;
        }
        command_right = false;
    }
    
    add_piece_to_grid();
    
    if(command_drop) {
        try_to_drop_faster();
        command_drop = false;
    }
}

void p(int x, int y, long color) {
  // Helligkeit anwenden
  long adjustedColor = applyBrightness(color, BRIGHTNESS);
  
  int a = (GRID_H - 1 - y) * GRID_W;
  if ((y % 2) == BACKWARDS) {
    a += x;
  } else {
    a += GRID_W - 1 - x;
  }
  a %= STRAND_LENGTH;
  strip.setPixelColor(a, adjustedColor);
}

void draw_grid() {
  for (int y = 0; y < GRID_H; ++y) {
    for (int x = 0; x < GRID_W; ++x) {
      if (grid[y * GRID_W + x] != 0) {
        p(x, y, grid[y * GRID_W + x]);
      } else {
        p(x, y, 0);
      }
    }
  }
  strip.show();
}

void choose_new_piece() {
  if (sequence_i >= NUM_PIECE_TYPES) {
    for (int i = 0; i < NUM_PIECE_TYPES; ++i) {
      int j, k;
      do {
        j = rand() % NUM_PIECE_TYPES;
        for (k = 0; k < i; ++k) {
          if (piece_sequence[k] == j) break;
        }
      } while (k < i);
      piece_sequence[i] = j;
    }
    sequence_i = 0;
  }

  piece_id = piece_sequence[sequence_i++];
  piece_y = -4;
  piece_x = 3;
  piece_rotation = 0;
}

void erase_piece_from_grid() {
  const char *piece = pieces[piece_id] + (piece_rotation * PIECE_H * PIECE_W);

  for (int y = 0; y < PIECE_H; ++y) {
    for (int x = 0; x < PIECE_W; ++x) {
      int nx = piece_x + x;
      int ny = piece_y + y;
      if (ny < 0 || ny >= GRID_H) continue;
      if (nx < 0 || nx >= GRID_W) continue;
      if (piece[y * PIECE_W + x] == 1) {
        grid[ny * GRID_W + nx] = 0;
      }
    }
  }
}

void delete_row(int y) {
    for(; y > 0; --y) {
        for(int x = 0; x < GRID_W; ++x) {
            grid[y*GRID_W+x] = grid[(y-1)*GRID_W+x];
        }
    }
    for(int x = 0; x < GRID_W; ++x) {
        grid[x] = 0;
    }
}

void fall_faster() {
  if (drop_delay > DROP_MINIMUM) drop_delay -= DROP_ACCELERATION;
}

void remove_full_rows() {
    int lines_cleared = 0;

    for (int y = 0; y < GRID_H; ++y) {
        int full = 1;
        for (int x = 0; x < GRID_W; ++x) {
            if (grid[y * GRID_W + x] == 0) {
                full = 0;
                break;
            }
        }
        if (full) {
            delete_row(y);
            lines_cleared++;
            y--; // Nachrückende Zeile erneut prüfen
            fall_faster();
        }
    }

    if (lines_cleared > 0) {
        int points[] = {0, 40, 100, 300, 1200};
        int awarded = lines_cleared > 4 ? 1200 : points[lines_cleared];
        score += awarded;
        Serial.print("Lines cleared: ");
        Serial.print(lines_cleared);
        Serial.print(" → Score +");
        Serial.println(awarded);
        if (score > top_score) top_score = score;
        sendGameData();
    }
}

void try_to_rotate_pieceright() {
    int new_pr = (piece_rotation + 1) % 4;
    if (piece_can_fit(piece_x, piece_y, new_pr)) {
        piece_rotation = new_pr;
    } else {
        if (piece_can_fit(piece_x - 1, piece_y, new_pr)) {
            piece_x = piece_x - 1;
            piece_rotation = new_pr;
        } else if (piece_can_fit(piece_x + 1, piece_y, new_pr)) {
            piece_x = piece_x + 1;
            piece_rotation = new_pr;
        }
    }
}

void try_to_rotate_pieceleft() {
    int new_pr = (piece_rotation + 3) % 4;
    if (piece_can_fit(piece_x, piece_y, new_pr)) {
        piece_rotation = new_pr;
    } else {
        if (piece_can_fit(piece_x - 1, piece_y, new_pr)) {
            piece_x = piece_x - 1;
            piece_rotation = new_pr;
        } else if (piece_can_fit(piece_x + 1, piece_y, new_pr)) {
            piece_x = piece_x + 1;
            piece_rotation = new_pr;
        }
    }
}

int piece_can_fit(int px, int py, int pr) {
    return !piece_off_edge(px, py, pr) && !piece_hits_rubble(px, py, pr);
}

int piece_off_edge(int px, int py, int pr) {
    const char *piece = pieces[piece_id] + (pr * PIECE_H * PIECE_W);

    for (int y = 0; y < PIECE_H; ++y) {
        for (int x = 0; x < PIECE_W; ++x) {
            int nx = px + x;
            if (piece[y * PIECE_W + x] > 0) {
                if (nx < 0) return 1;
                if (nx >= GRID_W) return 1;
            }
        }
    }
    return 0;
}

int piece_hits_rubble(int px, int py, int pr) {
    const char *piece = pieces[piece_id] + (pr * PIECE_H * PIECE_W);

    for (int y = 0; y < PIECE_H; ++y) {
        int ny = py + y;
        if (ny < 0) continue;
        for (int x = 0; x < PIECE_W; ++x) {
            int nx = px + x;
            if (piece[y * PIECE_W + x] > 0) {
                if (ny >= GRID_H) return 1;
                if (grid[ny * GRID_W + nx] != 0) return 1;
            }
        }
    }
    return 0;
}

void add_piece_to_grid() {
    const char *piece = pieces[piece_id] + (piece_rotation * PIECE_H * PIECE_W);

    for(int y = 0; y < PIECE_H; ++y) {
        for(int x = 0; x < PIECE_W; ++x) {
            int nx = piece_x + x;
            int ny = piece_y + y;
            if(ny < 0 || ny >= GRID_H) continue;
            if(nx < 0 || nx >= GRID_W) continue;
            if(piece[y * PIECE_W + x] == 1) {
                grid[ny * GRID_W + nx] = piece_colors[piece_id];
            }
        }
    }
}

void try_to_drop_piece() {
    erase_piece_from_grid();
    if (piece_can_fit(piece_x, piece_y + 1, piece_rotation)) {
        piece_y++;
        add_piece_to_grid();
    } else {
        add_piece_to_grid();
        remove_full_rows();
        if (game_is_over() == 1) {
            game_over();
        }
        choose_new_piece();
    }
}

void try_to_drop_faster() {
    try_to_drop_piece();
}

int game_is_over() {
    const char *piece = pieces[piece_id] + (piece_rotation * PIECE_H * PIECE_W);

    for (int y = 0; y < PIECE_H; ++y) {
        for (int x = 0; x < PIECE_W; ++x) {
            int ny = piece_y + y;
            if (piece[y * PIECE_W + x] > 0 && ny < 0) {
                return 1;
            }
        }
    }
    return 0;
}

void game_over() {
    Serial.println("GAME OVER");
    Serial.print("Final score: ");
    Serial.println(score);
    
    scroll_text("GAME OVER", 7, 0xFF0000, 100);
    draw_letters("R", 2, 7, 0x999999);

    bool newHighScore = false;
    for(int i = 0; i < MAX_TOP_SCORES; i++) {
        if(score > topScores[i].score) {
            for(int j = MAX_TOP_SCORES-1; j > i; j--) {
                topScores[j] = topScores[j-1];
            }
            topScores[i].score = score;
            memset(topScores[i].initials, ' ', 3);
            topScores[i].initials[3] = '\0';
            newHighScore = true;
            saveTopScores();
            break;
        }
    }

    if(score > top_score) {
        top_score = score;
    }

    sendGameData();

    game_started = false;
    while (!game_started) {
        ws.cleanupClients();
        delay(100);
    }

    score = 0;
    all_white();
    
    for(int i = 0; i < GRID_W * GRID_H; ++i) {
        grid[i] = 0;
    }

    choose_new_piece();
    move_delay = INITIAL_MOVE_DELAY;
    drop_delay = INITIAL_DROP_DELAY;
    last_draw = last_drop = last_move = millis();
    sendGameData();
}

void all_white() {
    long whiteColor = applyBrightness(0x787878, BRIGHTNESS);
    for (int i = 0; i < STRAND_LENGTH; i++) {
        strip.setPixelColor(i, whiteColor);
        strip.show();
        delay(3);
    }
}

void draw_pause() {
    draw_letters("P", 2, 7, 0x999999);
}

void draw_letters(const char* text, int start_x, int start_y, long color) {
    for (int c = 0; text[c] != '\0'; ++c) {
        char letter = text[c];
        const char* bitmap[5];

        if (letter == 'H') {
            bitmap[0] = "10001";
            bitmap[1] = "10001";
            bitmap[2] = "11111";
            bitmap[3] = "10001";
            bitmap[4] = "10001";
        } else if (letter == 'I') {
            bitmap[0] = "01110";
            bitmap[1] = "00100";
            bitmap[2] = "00100";
            bitmap[3] = "00100";
            bitmap[4] = "01110";
        } else if (letter == 'P') {
            bitmap[0] = "11110";
            bitmap[1] = "10001";
            bitmap[2] = "11110";
            bitmap[3] = "10000";
            bitmap[4] = "10000";
        } else if (letter == 'R') {
            bitmap[0] = "11110";
            bitmap[1] = "10001";
            bitmap[2] = "11110";
            bitmap[3] = "10010";
            bitmap[4] = "10001";
        } else {
            continue;
        }

        for (int y = 0; y < 5; ++y) {
            for (int x = 0; x < 5; ++x) {
                if (bitmap[y][x] == '1') {
                    p(start_x + x + c * 6, start_y + y, color);
                }
            }
        }
    }
    strip.show();
}

void scroll_text(const char* text, int start_y, long color, int delay_ms) {
    const char* bitmaps[26][5] = {
        {"00100", "01010", "10001", "11111", "10001"}, // A
        {"11110", "10001", "11110", "10001", "11110"}, // B
        {"01110", "10001", "10000", "10001", "01110"}, // C
        {"11100", "10010", "10001", "10010", "11100"}, // D
        {"11111", "10000", "11110", "10000", "11111"}, // E
        {"11111", "10000", "11110", "10000", "10000"}, // F
        {"01110", "10000", "10111", "10001", "01111"}, // G
        {"10001", "10001", "11111", "10001", "10001"}, // H
        {"01110", "00100", "00100", "00100", "01110"}, // I
        {"00001", "00001", "00001", "10001", "01110"}, // J
        {"10001", "10010", "11100", "10010", "10001"}, // K
        {"10000", "10000", "10000", "10000", "11111"}, // L
        {"10001", "11011", "10101", "10001", "10001"}, // M
        {"10001", "11001", "10101", "10011", "10001"}, // N
        {"01110", "10001", "10001", "10001", "01110"}, // O
        {"11110", "10001", "11110", "10000", "10000"}, // P
        {"01110", "10001", "10001", "10011", "01111"}, // Q
        {"11110", "10001", "11110", "10010", "10001"}, // R
        {"01111", "10000", "01110", "00001", "11110"}, // S
        {"11111", "00100", "00100", "00100", "00100"}, // T
        {"10001", "10001", "10001", "10001", "01110"}, // U
        {"10001", "10001", "10001", "01010", "00100"}, // V
        {"10001", "10001", "10101", "11011", "10001"}, // W
        {"10001", "01010", "00100", "01010", "10001"}, // X
        {"10001", "01010", "00100", "00100", "00100"}, // Y
        {"11111", "00010", "00100", "01000", "11111"}, // Z
    };

    int spacing = 1;
    int letter_width = 5;
    int total_length = 0;
    for (int i = 0; text[i] != '\0'; ++i) {
        total_length += letter_width + spacing;
    }

    for (int scroll = 0; scroll < total_length + 10; ++scroll) {
        for (int i = 0; i < STRAND_LENGTH; i++) {
            strip.setPixelColor(i, 0);
        }

        int offset_x = 10 - scroll;
        for (int c = 0; text[c] != '\0'; ++c) {
            char ch = text[c];
            if (ch >= 'A' && ch <= 'Z') {
                const char** bm = bitmaps[ch - 'A'];
                for (int y = 0; y < 5; ++y) {
                    for (int x = 0; x < 5; ++x) {
                        if (bm[y][x] == '1') {
                            int px = offset_x + c * (letter_width + spacing) + x;
                            int py = start_y + y;
                            if (px >= 0 && px < GRID_W && py >= 0 && py < GRID_H) {
                                p(px, py, color);
                            }
                        }
                    }
                }
            }
        }

        strip.show();
        delay(delay_ms);
    }
}
