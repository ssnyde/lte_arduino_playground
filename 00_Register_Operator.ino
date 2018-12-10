/*
  Register your LTE Shield/SIM combo on a mobile network operator
  By: Jim Lindblom
  SparkFun Electronics
  Date: November 19, 2018
  License: This code is public domain but you buy me a beer if you use this 
  and we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14997

  This example demonstrates how to initialize your Cat M1/NB-IoT shield, and
  connect it to a mobile network operator (Verizon, AT&T, T-Mobile, etc.).

  Before beginning, you may need to adjust the mobile network operator (MNO)
  setting on line 45. See comments above that line to help select either
  Verizon, T-Mobile, AT&T or others.

  You may also need to set an APN on line 51 -- e.g. "hologram"
  
  Hardware Connections:
  Attach the SparkFun LTE Cat M1/NB-IoT Shield to your Arduino
  Power the shield with your Arduino -- ensure the PWR_SEL switch is in
    the "ARDUINO" position.
*/

//Click here to get the library: http://librarymanager/All#SparkFun_LTE_Shield_Arduino_Library
#include <SparkFun_LTE_Shield_Arduino_Library.h>

// We need to pass a Serial or SoftwareSerial object to the LTE Shield 
// library. Below creates a SoftwareSerial object on the standard LTE
// Shield RX/TX pins:
// Note: if you're using an Arduino board with a dedicated hardware
// serial port, comment out the line below. (Also see note in setup.)
// SoftwareSerial lteSerial(8, 9);

// Create a LTE_Shield object to be used throughout the sketch:
LTE_Shield lte;

// Network operator can be set to either:
// MNO_SW_DEFAULT -- DEFAULT
// MNO_ATT -- AT&T 
// MNO_VERIZON -- Verizon
// MNO_TELSTRA -- Telstra
// MNO_TMO -- T-Mobile
const mobile_network_operator_t MOBILE_NETWORK_OPERATOR = MNO_SW_DEFAULT;
const String MOBILE_NETWORK_STRINGS[] = {"Default", "SIM_ICCD", "AT&T", "VERIZON", 
  "TELSTRA", "T-Mobile", "CT"};

// APN -- Access Point Name. Gateway between GPRS MNO
// and another computer network. E.g. "hologram
const String APN = "hologram";

// This defines the size of the ops struct array. Be careful making
// this much bigger than ~5 on an Arduino Uno. To narrow the operator
// list, set MOBILE_NETWORK_OPERATOR to AT&T, Verizeon etc. instead
// of MNO_SW_DEFAULT.
#define MAX_OPERATORS 5

#define DEBUG_PASSTHROUGH_ENABLED

void setup() {
  int opsAvailable;
  struct operator_stats ops[MAX_OPERATORS];
  String currentOperator = "";
  bool newConnection = true;

  SerialUSB.begin(9600);

  SerialUSB.println(F("Initializing the LTE Shield..."));
  SerialUSB.println(F("...this may take ~25 seconds if the shield is off."));
  SerialUSB.println(F("...it may take ~5 seconds if it just turned on."));
  // Call lte.begin and pass it your Serial/SoftwareSerial object to 
  // communicate with the LTE Shield.
  // Note: If you're using an Arduino with a dedicated hardware serial
  // poert, you may instead slide "Serial" into this begin call.
  if ( lte.begin(Serial1, 9600) ) {
    SerialUSB.println(F("LTE Shield connected!\r\n"));
  }

  // First check to see if we're already connected to an operator:
  if (lte.getOperator(&currentOperator) == LTE_SHIELD_SUCCESS) {
    SerialUSB.print(F("Already connected to: "));
    SerialUSB.println(currentOperator);
    // If already connected provide the option to type y to connect to new operator
    SerialUSB.println(F("Press y to connect to a new operator, or any other key to continue.\r\n"));
    while (!SerialUSB.available()) ;
    if (SerialUSB.read() != 'y') {
      newConnection = false;
    }
  }

  if (newConnection) {
    // Set MNO to either Verizon, T-Mobile, AT&T, Telstra, etc.
    // This will narrow the operator options during our scan later
    SerialUSB.println(F("Setting mobile-network operator"));
    if (lte.setNetwork(MOBILE_NETWORK_OPERATOR)) {
      SerialUSB.print(F("Set mobile network operator to "));
      SerialUSB.println(MOBILE_NETWORK_STRINGS[MOBILE_NETWORK_OPERATOR] + "\r\n");
    } else {
      SerialUSB.println(F("Error setting MNO. Try cycling power to the shield/Arduino."));
      while (1) ;
    }
    
    // Set the APN -- Access Point Name -- e.g. "hologram"
    SerialUSB.println(F("Setting APN..."));
    if (lte.setAPN(APN) == LTE_SHIELD_SUCCESS) {
      SerialUSB.println(F("APN successfully set.\r\n"));
    } else {
      SerialUSB.println(F("Error setting APN. Try cycling power to the shield/Arduino."));
      while (1) ;
    }

    // Wait for user to press button before initiating network scan.
    SerialUSB.println(F("Press any key scan for networks.."));
    serialWait();

    SerialUSB.println(F("Scanning for operators...this may take up to 3 minutes\r\n"));
    // lte.getOperators takes in a operator_stats struct pointer and max number of
    // structs to scan for, then fills up those objects with operator names and numbers
    opsAvailable = lte.getOperators(ops, MAX_OPERATORS); // This will block for up to 3 minutes

    if (opsAvailable > 0) {
      // Pretty-print operators we found:
      SerialUSB.println("Found " + String(opsAvailable) + " operators:");
      printOperators(ops, opsAvailable);

      // Wait until the user presses a key to initiate an operator connection
      SerialUSB.println("Press 1-" + String(opsAvailable) + " to select an operator.");
      char c = 0;
      bool selected = false;
      while (!selected) {
        while (!SerialUSB.available()) ;
        c = SerialUSB.read();
        int selection = c - '0';
        if ((selection >= 1) && (selection <= opsAvailable)) {
          selected = true;
          SerialUSB.println("Connecting to option " + String(selection));
          if (lte.registerOperator(ops[selection - 1]) == LTE_SHIELD_SUCCESS) {
            SerialUSB.println("Network " + ops[selection - 1].longOp + " registered\r\n");
          } else {
            SerialUSB.println(F("Error connecting to operator. Reset and try again, or try another network."));
          }
        }
      }
    } else {
      SerialUSB.println(F("Did not find an operator. Double-check SIM and antenna, reset and try again, or try another network."));
      while (1) ;
    }
  }

  // At the very end print connection information
  printInfo();
}

void loop() {
  // Loop won't do much besides provide a debugging interface.
  // Pass serial data from Arduino to shield and vice-versa
#ifdef DEBUG_PASSTHROUGH_ENABLED
  if (SerialUSB.available()) {
    Serial1.write((char) SerialUSB.read());
  }
  if (Serial1.available()) {
    SerialUSB.write((char) Serial1.read());
  }
#endif
}

void printInfo(void) {
  String currentApn = "";
  IPAddress ip(0, 0, 0, 0);
  String currentOperator = "";

  SerialUSB.println(F("Connection info:"));
  // APN Connection info: APN name and IP
  if (lte.getAPN(&currentApn, &ip) == LTE_SHIELD_SUCCESS) {
    SerialUSB.println("APN: " + String(currentApn));
    SerialUSB.print("IP: ");
    SerialUSB.println(ip);
  }

  // Operator name or number
  if (lte.getOperator(&currentOperator) == LTE_SHIELD_SUCCESS) {
    SerialUSB.print(F("Operator: "));
    SerialUSB.println(currentOperator);
  }

  // Received signal strength
  SerialUSB.println("RSSI: " + String(lte.rssi()));
  SerialUSB.println();
}

void printOperators(struct operator_stats * ops, int operatorsAvailable) {
  for (int i = 0; i < operatorsAvailable; i++) {
    SerialUSB.print(String(i + 1) + ": ");
    SerialUSB.print(ops[i].longOp + " (" + String(ops[i].numOp) + ") - ");
    switch (ops[i].stat) {
    case 0:
      SerialUSB.println(F("UNKNOWN"));
      break;
    case 1:
      SerialUSB.println(F("AVAILABLE"));
      break;
    case 2:
      SerialUSB.println(F("CURRENT"));
      break;
    case 3:
      SerialUSB.println(F("FORBIDDEN"));
      break;
    }
  }
  SerialUSB.println();
}

void serialWait() {
  while (!SerialUSB.available()) ;
  while (SerialUSB.available()) SerialUSB.read();
}
