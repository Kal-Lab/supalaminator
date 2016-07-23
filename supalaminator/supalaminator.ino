// this example is public domain. enjoy!
// www.ladyada.net/learn/sensors/thermocouple

#include <LiquidCrystal.h>
#include <max6675.h>
#include <Encoder.h>

/* Rotary Encoder */
#define ENC_A_PIN 2
#define ENC_B_PIN 3
#define ENC_SW_PIN A0

/* Relay */
#define RELAY_PIN A1

/* THERMAL */
#define THERM_DO 4
#define THERM_CS 5
#define THERM_CLK  6

/* LCD */
#define LCD_RS_PIN 12
#define LCD_EN_PIN 11
#define LCD_D4_PIN  7
#define LCD_D5_PIN  8
#define LCD_D6_PIN  9
#define LCD_D7_PIN  10

/* Values defines */
#define MIN_TEMP   50
#define MAX_TEMP   150

#define THRESHOLD  5

#define DEFAULT_MIN_TEMP   100
#define DEFAULT_MAX_TEMP   110

byte up_char[8] = {
  0b00000,
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00100
};

byte up_invert_char[8] = {
  0b11111,
  0b11011,
  0b10001,
  0b01010,
  0b11011,
  0b11011,
  0b11011,
  0b11011
};

byte down_char[8] = {
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100
};

byte degree_char[8] = {
  0b00000,
  0b11000,
  0b11000,
  0b00011,
  0b00100,
  0b00100,
  0b00100,
  0b00011
};

byte bottom_char[8] = {
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100,
  0b11111
};

byte bottom_invert_char[8] = {
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b01010,
  0b10001,
  0b11011,
  0b00000
};

byte top_char[8] = {
  0b11111,
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00100
};

byte top_invert_char[8] = {
  0b00000,
  0b11011,
  0b10001,
  0b01010,
  0b11011,
  0b11011,
  0b11011,
  0b11011
};

#define UP_CHAR 0
#define UP_INVERT_CHAR 1
#define DOWN_CHAR 2
#define DEGREE_CHAR 3
#define BOTTOM_CHAR 4
#define BOTTOM_INVERT_CHAR 5
#define TOP_CHAR 6
#define TOP_INVERT_CHAR 7

#define STATE_SET_MIN_TEMP 0
#define STATE_SET_MAX_TEMP 1
#define STATE_IDLE 2
#define STATE_COUNT 3

#define HEATING_ON 0
#define HEATING_OFF 1

#define LCD_UPDATE  200
#define CHAR_UPDATE  1000

MAX6675 thermocouple(THERM_CLK, THERM_CS, THERM_DO);
Encoder enc(ENC_A_PIN, ENC_B_PIN);
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(LCD_RS_PIN, LCD_EN_PIN, LCD_D4_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);

float min_temp = DEFAULT_MIN_TEMP;
float max_temp = DEFAULT_MAX_TEMP;

unsigned long last_millis = 0;
unsigned int conf_state = STATE_IDLE;
unsigned int heat_state = HEATING_OFF;

long last_enc_pos;

void 
lcd_draw_min_temp()
{
  lcd.setCursor(0,0);
  if (conf_state == STATE_SET_MIN_TEMP)
    lcd.write((uint8_t)BOTTOM_INVERT_CHAR);
  else
    lcd.write((uint8_t)BOTTOM_CHAR);
  lcd.print(" ");
  lcd.print(min_temp);
  lcd.write((uint8_t)DEGREE_CHAR);
  lcd.print(" ");

  Serial.print("New min temp: ");
  Serial.println(min_temp);
}

void 
lcd_draw_max_temp()
{
  lcd.setCursor(0,0);
  if (conf_state == STATE_SET_MAX_TEMP)
    lcd.write((uint8_t)TOP_INVERT_CHAR);
  else
    lcd.write((uint8_t)TOP_CHAR);
  lcd.print(" ");
  lcd.print(max_temp);
  lcd.write((uint8_t)DEGREE_CHAR);
  lcd.print(" ");
  Serial.print("New max temp: ");
  Serial.println(max_temp);
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting thermal control");

  lcd.begin(16, 2);
  lcd.createChar(UP_CHAR, up_char);
  lcd.createChar(UP_INVERT_CHAR, up_invert_char);
  lcd.createChar(DOWN_CHAR, down_char);
  lcd.createChar(DEGREE_CHAR, degree_char);
  lcd.createChar(BOTTOM_CHAR, bottom_char);
  lcd.createChar(BOTTOM_INVERT_CHAR, bottom_invert_char);
  lcd.createChar(TOP_CHAR, top_char);
  lcd.createChar(TOP_INVERT_CHAR, top_invert_char);
  lcd_draw_min_temp();
  lcd_draw_max_temp();
  
  pinMode(ENC_SW_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  /* Disable relay */
  digitalWrite(RELAY_PIN, 1);
  last_enc_pos = enc.read();

  // wait for MAX chip to stabilize
  delay(500);
}


unsigned long heat_char_update = 0;
int heat_char_idx = UP_CHAR;

static void
draw_lcd(float cur_temp)
{
  if (millis() - last_millis > LCD_UPDATE) {
   
    /* Current temp */
    lcd.setCursor(1,0);
    lcd.print("Temp: ");
    lcd.print(cur_temp);
    lcd.write((uint8_t)DEGREE_CHAR);
    lcd.print(" ");

    Serial.print("Temp:");
    Serial.println(cur_temp);

    if (heat_state == HEATING_ON) {
      if (millis() - heat_char_update > CHAR_UPDATE)  {
        if (heat_char_idx == UP_CHAR)
          heat_char_idx = UP_INVERT_CHAR;
        else 
          heat_char_idx = UP_CHAR;

        lcd.write((uint8_t) heat_char_idx);

        heat_char_update = millis();
      }
    } else {
        lcd.write((uint8_t)DOWN_CHAR);
    }
    last_millis = millis();
  }
}


int get_enc_dir()
{
  long new_pos = enc.read();
  int dir = 0;

  if (new_pos < last_enc_pos)
    dir = 1;
  else if (new_pos > last_enc_pos)
    dir = -1;

  last_enc_pos = new_pos;
  delay(50);
  return dir;
}


static void
set_min_temp()
{
  int dir = get_enc_dir();
  if (dir) {
    min_temp += dir;
    if (min_temp > MAX_TEMP)
      min_temp = (MAX_TEMP - THRESHOLD);
    if (min_temp < MIN_TEMP)
      min_temp = MIN_TEMP;
    lcd_draw_min_temp();
  }
}

static void
set_max_temp()
{
  int dir = get_enc_dir();
  if (dir) {
    max_temp += dir;
    if (max_temp < (min_temp + THRESHOLD))
      max_temp = min_temp + THRESHOLD;
    lcd_draw_max_temp();
  }
}

static void
manage_heating(float temp)
{
    switch(heat_state) {
    case HEATING_ON:
      if (temp >= (max_temp - THRESHOLD)) {
        heat_state = HEATING_OFF;
        digitalWrite(RELAY_PIN, 1);
        Serial.println("Heating Off !");
      }
      break;
    case HEATING_OFF:
      if (temp <= min_temp) {
        heat_state = HEATING_ON;
        digitalWrite(RELAY_PIN, 0);
        Serial.println("Heating On !");
      }
      break;
  }
}

void loop() {
  float temp = thermocouple.readCelsius();
  delay(200);
  draw_lcd(temp);
 
  if (!digitalRead(ENC_SW_PIN)) {
    delay(300);
    conf_state++;
    if (conf_state == STATE_COUNT)
      conf_state = 0;
    /* Reset enc state */
    last_enc_pos = enc.read();

    if (conf_state == STATE_SET_MAX_TEMP) {
      if (min_temp > max_temp) {
        max_temp = min_temp + THRESHOLD;
      }
    }

    Serial.print("State changed: ");
    Serial.println(conf_state);
  }

  switch (conf_state) {
    case STATE_SET_MIN_TEMP:  set_min_temp(); break;
    case STATE_SET_MAX_TEMP:  set_max_temp(); break;
    default: break;
  }

  manage_heating(temp);

}
