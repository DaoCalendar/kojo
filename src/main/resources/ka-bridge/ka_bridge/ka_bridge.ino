/*
 * Copyright (C) 2014-2018 Lalit Pant <pant.lalit@gmail.com>
 *
 * The contents of this file are subject to the GNU General Public License
 * Version 3 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.gnu.org/copyleft/gpl.html
 */

#define IN_PACK_MAX_SIZE (10) // ns, proc, and eight more bytes for args
#define OUT_PACK_HDR_SIZE (3) // ret val type, ns, and proc

// #define DEBUG

#ifdef DEBUG
 #define debugLog(x)  log(x)
#else
 #define debugLog(x)
#endif

byte incoming_packet[IN_PACK_MAX_SIZE]; 
byte outgoing_packet_hdr[OUT_PACK_HDR_SIZE]; 
int counter;
int state;
int packetSize;

// Include libs here
#include <Servo.h>
#include <SoftwareSerial.h>

Servo servo;

#define softSerial_RX 10
#define softSerial_TX 11
SoftwareSerial *softSerial = NULL;

// For Ultrasonic sensor
byte triggerPin, echoPin;

void setup() {
  Serial.begin(115200);
  delay(10);
  // log("Board (re)starting.");
  counter = 0;
  state = 1;
}

void loop() {
}

void serialEvent() {
  while (Serial.available()) {
    switch (state) {
      case 1: // new packet
        packetSize = Serial.read();
        state = 2;
        break;
      case 2: // reading packet 
        byte input = Serial.read();
        incoming_packet[counter++] = input;
        if (counter == packetSize) {
          counter = 0;
          dispatchProc();
          counter = 0;
          packetSize = 0;
          state = 1;
        } 
        break;
    }
  }
}

int len(const char str[]) {
  int len = 0;
  char c = str[len] ;
  while (c != 0) {
    len++;
    c = str[len];
  }
  return len;
}

void log(String msg) {
  returnString(0, -1, msg);
}

byte readByte() {
  return incoming_packet[counter++];
}

unsigned int readInt() {
  byte lo = readByte();
  byte hi = readByte();
  unsigned int retVal = hi << 8 | lo;
//  String msg = String("Read Int: ") + retVal;
//  log(msg);
  return retVal;
}

char* readString() {
  int len = readInt();
  char* buf = new char[len+1];
  for (int i=0; i < len; i++){
    buf[i] = readByte();
  }
  buf[len] = 0;
  return buf;
}

void writeByte(byte b) {
  int written = Serial.write(b);
  while (written != 1) {
    written = Serial.write(b);
  }
}

void writeInt(unsigned int i) {
  writeByte(i & 0x00FF);
  writeByte(i >> 8);
}

void writeLong(unsigned long l) {
  writeByte(l & 0x000000FF);
  writeByte((l >> 8) & 0x0000FF);
  writeByte((l >> 16) & 0x00FF);
  writeByte(l >> 24);
}

void writeArray(byte arr[], int len) {
  int written = Serial.write(arr, len);
  while (written != len) {
    written += Serial.write(arr + written, len - written);
  }
}

void writeHeader(byte retType, byte ns, byte proc) {
  outgoing_packet_hdr[0] = retType;
  outgoing_packet_hdr[1] = ns;
  outgoing_packet_hdr[2] = proc;
  writeArray(outgoing_packet_hdr, OUT_PACK_HDR_SIZE);
}

void returnByte(byte ns, byte proc, byte byteRet) {
  writeByte(OUT_PACK_HDR_SIZE + 1); // packet size
  writeHeader(1, ns, proc);
  writeByte(byteRet);
}

void returnInt(byte ns, byte proc, unsigned int intRet) {
  writeByte(OUT_PACK_HDR_SIZE + 2); // packet size
  writeHeader(2, ns, proc);
  writeInt(intRet);
}

void returnLong(byte ns, byte proc, unsigned long longRet) {
  writeByte(OUT_PACK_HDR_SIZE + 4); // packet size
  writeHeader(4, ns, proc);
  writeLong(longRet);
}

void returnString(byte ns, byte proc, String msg) {
  int len = msg.length();
  writeByte(OUT_PACK_HDR_SIZE + len);
  writeHeader(3, ns, proc);
  byte buf[len+1];
  msg.getBytes(buf, len+1);
  writeArray(buf, len);
}

void dispatchProc() {
  int intRet;
  int writeSize;
  byte byteRet;
  byte b1, b2;
  unsigned int i1, i2;
  byte ns = readByte();
  byte proc = readByte();
  switch (ns) {
    case 0: // meta
      switch (proc) {
        case 1: // kojo ping
          log(String("Board Ready"));
          returnInt(0, 1, 0xF0F0);
          break;
      }
      break;
    case 1: // inbuilt
      switch (proc) {
        case 1: // pinMode
          b1 = readByte();
          b2 = readByte();
          debugLog(String("pinMode(") + b1 + ", " + b2 + ")");
          pinMode(b1, b2);
          break;
        case 2: // digitalWrite
          b1 = readByte();
          b2 = readByte();
          debugLog(String("digitalWrite(") + b1 + ", " + b2 + ")");
          digitalWrite(b1, b2);
          break;
        case 3: // digitalRead
          b1 = readByte();
          debugLog(String("digitalRead(") + b1 + ")");
          returnByte(1, 3, digitalRead(b1));
          break;
        case 4: // analogRead
          b1 = readByte();
          debugLog(String("analogRead(") + b1 + ")");
          returnInt(1, 4, analogRead(b1));
          break;
        case 5: // tone
          b1 = readByte();
          i1 = readInt();
          tone(b1, i1);
          break;
        case 6: // noTone
          noTone(readByte());
          break;
        case 7: // analogWrite
          b1 = readByte();
          b2 = readByte();
          debugLog(String("analogWrite(") + b1 + ", " + b2 + ")");
          analogWrite(b1, b2);
          break;
        case 8: // tone
          b1 = readByte();
          i1 = readInt();
          i2 = readInt();
          tone(b1, i1, i2);
          break;
      }
      break;
    case 2: // servo lib
      switch (proc) {
        case 1: // attach
          servo.attach(readByte());
          break;
        case 2: // write
          servo.write(readByte());
          break;
      }
      break;
    case 3: // soft serial lib
      switch (proc) {
        case 1: // begin
          if (softSerial == NULL) {
            softSerial = new SoftwareSerial(softSerial_RX, softSerial_TX);
            debugLog("Created softSerial instance");
          }
          i1 = readInt();
          debugLog(String("softSerial.begin(") + i1 + ")");
          softSerial->begin(i1);
          break;
        case 2: // available
          returnInt(3, 2, softSerial->available());
          // debugLog("softSerial.available()");
          break;
        case 3: // read
          i1 = softSerial->read();
          debugLog(String("softSerial.read()") + " = " + i1);
          returnInt(3, 3, i1);
          break;
        case 4: // println
          char* str = readString();
          debugLog(String("softSerial.println(") + str + ")");
          returnInt(3, 4, softSerial->println(str));
          delete str;
          break;
      }
      break;
    case 4: // Ultrasonic sensor
      switch (proc) {
        case 1: // init
          triggerPin = readByte();
          echoPin = readByte();
          debugLog(String("Ultrasonic.init(") + triggerPin + ", " + echoPin + ")");
          pinMode(triggerPin, OUTPUT);
          pinMode(echoPin, INPUT);
          break;
        case 2: // pingMicroSecs
          debugLog("Ultrasonic.pingMicroSecs()");
          digitalWrite(triggerPin, LOW);
          delayMicroseconds(2);
          digitalWrite(triggerPin, HIGH);
          delayMicroseconds(10);
          digitalWrite(triggerPin, LOW);
          returnLong(4, 2, pulseIn(echoPin, HIGH));
          break;
      }
      break;
  }
}

