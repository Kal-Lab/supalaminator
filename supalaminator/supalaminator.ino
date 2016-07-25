// this example is public domain. enjoy!
// www.ladyada.net/learn/sensors/thermocouple

#include <LiquidCrystal.h>
#include <max6675.h>
#include <Encoder.h>

/* Rotary Encoder */
#define ENC_A_PIN 2
#define ENC_B_PIN 3
#define ENC_SW_PIN A1

/* Relay */
#define RELAY_PIN A0

/* THERMAL */
#define THERM_DO 4
#define THERM_CS 5
#define THERM_CLK  6

/* LCD */
#define LCD_RS_PIN 12
#define LCD_EN_PIN 11
#define LCD_D4_PIN  10
#define LCD_D5_PIN  7
#define LCD_D6_PIN  9
#define LCD_D7_PIN  8


#define THRESHOLD      1
#define MAX_TEMP       160
#define MIN_TEMP       100

#define DEFAULT_TEMP   120

byte heat_char[8] = {
  0b00000,
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00100
};

byte heat_invert_char[8] = {
  0b11111,
  0b11011,
  0b10001,
  0b01010,
  0b11011,
  0b11011,
  0b11011,
  0b11011
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


#define HEAT_CHAR 0
#define HEAT_INVERT_CHAR 1
#define DEGREE_CHAR 2

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

float ref_temp = DEFAULT_TEMP;

unsigned long last_millis = 0;
unsigned int heat_state = HEATING_OFF;

long last_enc_pos;

static void 
lcd_draw_ref_temp()
{
  lcd.setCursor(0,0);

  lcd.print("Ref ");
  lcd.print(ref_temp);
  lcd.write((uint8_t)DEGREE_CHAR);
  lcd.print(" ");

  Serial.print("New reference temp: ");
  Serial.println(ref_temp);
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting thermal control");

  lcd.begin(16, 2);
  lcd.createChar(HEAT_CHAR, heat_char);
  lcd.createChar(HEAT_INVERT_CHAR, heat_invert_char);
  lcd.createChar(DEGREE_CHAR, degree_char);

  lcd_draw_ref_temp();
  
  pinMode(ENC_SW_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  /* Disable relay */
  digitalWrite(RELAY_PIN, 1);
  last_enc_pos = enc.read();

  // wait for MAX chip to stabilize
  delay(500);
}


unsigned long heat_char_update = 0;
int heat_char_idx = HEAT_CHAR;

static void
draw_lcd(float cur_temp)
{
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
      if (heat_char_idx == HEAT_CHAR)
        heat_char_idx = HEAT_INVERT_CHAR;
      else 
        heat_char_idx = HEAT_CHAR;
     lcd.write((uint8_t) heat_char_idx);
     heat_char_update = millis();
    }
  }
  last_millis = millis();
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
set_ref_temp()
{
  int dir = get_enc_dir();
  if (dir) {
    ref_temp += dir;
    if (ref_temp > MAX_TEMP)
      ref_temp = MAX_TEMP;
    if (ref_temp < MIN_TEMP)
      ref_temp = MIN_TEMP;

    lcd_draw_ref_temp();
  }
}

unsigned long last_heating = 0;

static void
manage_heating(float temp)
{
    switch(heat_state) {
    case HEATING_ON:
      if (temp >= (ref_temp + THRESHOLD)) {
        heat_state = HEATING_OFF;
        digitalWrite(RELAY_PIN, 1);
        Serial.println("Heating Off !");
      }
      break;
    case HEATING_OFF:
      if (temp <= ref_temp) {
        heat_state = HEATING_ON;
        digitalWrite(RELAY_PIN, 0);
        Serial.println("Heating On !");
      }
      break;
  }
}

unsigned long temp_milli = 0;

void loop()
{
  if (millis() - temp_milli > 300) {
	  float temp = thermocouple.readCelsius();
	  draw_lcd(temp);
	  manage_heating(temp);
    temp_milli = millis();
  }
  
  set_ref_temp();
}
