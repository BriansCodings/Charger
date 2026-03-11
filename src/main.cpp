/**
 * ! DO NOT USE THIS CODE WITHOUT UNDERSTANDING THE CIRCUIT AND SAFETY IMPLICATIONS !
 * ! THERE IS NO HARDWARE LEVELOVERVOLTAGE OR CURRENT PROTECTION IN THIS CODE. USE AT YOUR OWN RISK. !
 * TODO: ADD HARDWARE LEVEL OVERVOLTAGE PROTECTION
 * TODO: ADD CURRENT SENSING AND HARDWARE LEVEL OVERCURRENT PROTECTION
 * TODO: ADD TEMPERATURE SENSING AND OVERHEAT 
 * TODO: ADD A SCREEN FOR REAL-TIME STATUS DISPLAY AND CONFIGURATION
 * TODO: Make voltage thresholds, rest times and other charging parameters user-configurable (store in NVS/EEPROM and expose via serial/UI) instead of hardcoding
 * TODO: Add support for more cells in series by adding more relays and voltage sensing channels, and updating the logic accordingly.

  Simple battery charger for 2 cells in series using 2 relays and voltage sensing.
  Only one relay can be closed at a time to prevent shorting the battery.
  Each cell is charged until it reaches 4.2V, then the relay opens and rests for 5 seconds.
  If the voltage drops below 4.15V after rest, charging resumes.
  
  Relays are active-low (energized = open circuit) to fail safe in case of power loss.

 */



  

#include <Arduino.h> // Include the Arduino core library for basic functions
#include <esp_adc_cal.h> // Include ESP ADC calibration library for accurate ADC readings


// Define a Relay class to encapsulate relay control logic
class Relay {
    int pin; // The GPIO pin number connected to the relay
    bool activeLow; // Whether the relay is active-low (true) or active-high (false)
    const char* name; // Name of the relay for serial output
public:
    // Constructor: initializes pin, activeLow logic, and relay name
    Relay(int relayPin, bool isActiveLow = false, const char* relayName = "Relay") : pin(relayPin), activeLow(isActiveLow), name(relayName) {}
    // Sets up the relay pin as output and turns relay off initially
    void begin() {
        pinMode(pin, OUTPUT); // Set the relay pin as an output
        stopCharging(); // Energize relay (open circuit, NC is open) at startup
    }
    // Turns the relay ON (open circuit, NC is open)
    void stopCharging() {
        digitalWrite(pin, activeLow ? LOW : HIGH);
        Serial.print(name);
        Serial.println(" is ON (open)");
    }
    // Turns the relay OFF (close circuit, NC is closed, charging)
    void startCharging() {
        digitalWrite(pin, activeLow ? HIGH : LOW);
        Serial.print(name);
        Serial.println(" is OFF (closed, charging)");
    }
    bool isOn() const {
        return digitalRead(pin) == (activeLow ? HIGH : LOW);
    }
};

class vSense {
    int pin;
    float dividerRatio;
    const char* name;
    esp_adc_cal_characteristics_t adc_chars; // ADC calibration characteristics
public:
    vSense(int analogPin, float ratio, const char* sensorName = "vSense") : pin(analogPin), dividerRatio(ratio), name(sensorName) {}

    void begin() {
        pinMode(pin, INPUT);
        // Configure ADC width and attenuation
        analogReadResolution(12); // 12-bit resolution
        analogSetAttenuation(ADC_11db); // 11dB attenuation for max range
        // Characterize ADC using eFuse Vref if available
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc_chars);
    }

    float readVoltage() {
        int rawValue = analogRead(pin);
        uint32_t voltage = esp_adc_cal_raw_to_voltage(rawValue, &adc_chars); // Calibrated voltage in mV
        float scaledVoltage = (voltage / 1000.0) * dividerRatio; // Scale to battery voltage
        return scaledVoltage;
    }

    void printVoltage() {
        float voltage = readVoltage(); // Read the voltage
        Serial.print(name); // Print sensor name
        Serial.print(": ");
        Serial.print(voltage, 2); // Print voltage with 2 decimal places
        Serial.println(" V"); // Print units
    }
};

Relay relay1(35, true, "Relay 1"); // Create Relay 1 object on GPIO 7, active-low
Relay relay2(37, true, "Relay 2"); // Create Relay 2 object on GPIO 8, active-low

vSense v1(18, 2.7857, "Cell 1");
vSense v2(6, 2.7857, "Cell 2");

    
void setup() {
    Serial.begin(115200); // Initialize serial communication at 115200 baud
    relay1.begin(); // Initialize Relay 1
    relay2.begin(); // Initialize Relay 2
    v1.begin();
    v2.begin();

};

void loop() {
    static unsigned long lastUpdate = 0;
    static unsigned long stateStart = 0;
    static bool rest1 = false;
    static bool rest2 = false;
    static float restV1 = 0;
    static float restV2 = 0;
    const unsigned long restTime = 5000; // 5 seconds rest

    unsigned long now = millis();
    if (now - lastUpdate >= 350) {
        lastUpdate = now;

        float cell1 = v1.readVoltage();
        float cell2 = v2.readVoltage();

        // Only one relay can be ON at a time to prevent a short
        const float CHARGE_CUTOFF = 4.2;      // Stop charging at this voltage
        const float CHARGE_RESUME = 4.15;     // Resume charging if voltage drops below this after rest
        if (!rest1 && cell1 < CHARGE_CUTOFF && cell1 > 0.5) {
            relay1.startCharging(); // Close relay to start charging
            relay2.stopCharging();  // Keep relay2 open
            Serial.println("Relay 1 OFF (closed, charging) - Cell 1 charging");
            Serial.println(cell1);
            if (cell1 >= CHARGE_CUTOFF) {
                relay1.stopCharging(); // Open relay to stop charging
                rest1 = true;
                stateStart = now;
                Serial.println("Cell 1 done charging, entering rest period");
            }
        } else if (!rest2 && cell2 < CHARGE_CUTOFF && cell2 > 0.5 && !relay1.isOn()) {
            relay1.stopCharging();  // Open relay1
            relay2.startCharging(); // Close relay2 to start charging
            Serial.print("Relay 2 OFF (closed, charging) - Cell 2 charging: ");
            Serial.println(cell2);
            if (cell2 >= CHARGE_CUTOFF) {
                relay2.stopCharging(); // Open relay to stop charging
                rest2 = true;
                stateStart = now;
                Serial.println("Cell 2 done charging, entering rest period");
            }
        } else if (rest1) {
            relay1.stopCharging(); // Ensure relay is open during rest
            relay2.stopCharging();
            if (now - stateStart >= restTime) {
                restV1 = v1.readVoltage();
                Serial.print("Cell 1 rest voltage: "); Serial.println(restV1);
                if (restV1 < CHARGE_RESUME && restV1 > 0.5) {
                    rest1 = false; // Resume charging if needed
                    Serial.println("Resuming Cell 1 charging after rest");
                } else {
                    rest1 = false;
                    Serial.println("Cell 1 fully charged after rest");
                }
            }
        } else if (rest2) {
            relay1.stopCharging();
            relay2.stopCharging(); // Ensure both relays open during rest
            if (now - stateStart >= restTime) {
                restV2 = v2.readVoltage();
                Serial.print("Cell 2 rest voltage: "); Serial.println(restV2);
                if (restV2 < CHARGE_RESUME && restV2 > 0.5) {
                    rest2 = false; // Resume charging if needed
                    Serial.println("Resuming Cell 2 charging after rest");
                } else {
                    rest2 = false;
                    Serial.println("Cell 2 fully charged after rest");
                }
            }
        } else {
            relay1.stopCharging();  // Ensure both relays are open (not charging)
            relay2.stopCharging();
            Serial.println("Both relays stopCharging (open) - No cell needs charging or voltages out of range");
            Serial.print("Cell 1: "); Serial.println(cell1);
            Serial.print("Cell 2: "); Serial.println(cell2);
        }

        v1.printVoltage();
        v2.printVoltage();
    }
}