#ifdef TFT_ENABLE
#ifndef __TFT_H__
#define __TFT_H__

#include <SPI.h>
#include <TFT_eSPI.h>

#include "shared.h"

#define LINE_HEIGHT 20
#define LINE_WIDTH TFT_WIDTH
#define MAX_LINES 10

// TFT class
class TFT {
public:
  void init() {
    tft.init();
    tft.setRotation(0);
    setFontSize(2);
    lastPktDraw = 0;
    background = TFT_BLACK;
    textColor = TFT_WHITE;
    myLastPacketsPerSecond = 0;
    myLastFailedPerSecond = 0;
    tft.fillScreen(background);
    tft.setTextColor(textColor, background);
  }

  void setFontSize(char size) {
    tft.setTextSize(size);
    fontSize = size;
  }

  bool tooFast(unsigned long* lastDrawRequest) {
    unsigned long ms = millis();
    if (ms > 250 && *lastDrawRequest < ms - 250) {
      *lastDrawRequest = ms;
      return false;
    } else {
      return true;
    }
  }

  void zeroPrefix(char* buf, short input, char zero) {
    short offset = 0;
    for (int i = 1; i < zero+1; ++i) {
      if (input < pow(10, i)) {
        buf[offset] = '0';
        offset+=1;
      }
    }
    sprintf(buf+offset, "%d", input);
  }

  short getLineIndex(short line) {
    return line*LINE_HEIGHT+10;
  }

  void renderControlState(ControlPacket& packet) {
    if (tooFast(&lastCtlDraw)) {
      return;
    }
    char pkt[64];
    short y = this->getLineIndex(9);
    short len = 0;
    sprintf(pkt, "R%d", map(packet.channels[0],1000,2000,0,9));
    setGoodColor(packet.channels[0], 1550, 1450);
    len += write(len, y, pkt);
    len += getCharSize();
    sprintf(pkt, "P%d", map(packet.channels[1],1000,2000,0,9));
    setGoodColor(packet.channels[1], 1550, 1450);
    len += write(len, y, pkt);
    len += getCharSize();
    sprintf(pkt, "Y%d", map(packet.channels[3],1000,2000,0,9));
    setGoodColor(packet.channels[3], 1550, 1450);
    len += write(len, y, pkt);
    len += getCharSize();
    sprintf(pkt, "T%d", map(packet.channels[2],1000,2000,0,9));
    setGoodColor(packet.channels[2], 1750, 1250);
    len += write(len, y, pkt);
    len += getCharSize();

    len = 0;
    y = this->getLineIndex(8);
    if (packet.digitalChannels[MAX_DIGITAL_BYTES-1] & 0x1) {
      sprintf(pkt, "PRE.");
      tft.setTextColor(TFT_GREEN, background);
      len += write(len, y, pkt);
      len += getCharSize();
    } else {
      sprintf(pkt, "    ");
      len += write(len, y, pkt);
      len += getCharSize();
    }
    if (packet.digitalChannels[MAX_DIGITAL_BYTES-1] & 0x2) {
      sprintf(pkt, " ARMED");
      tft.setTextColor(TFT_GREEN, background);
      len += write(len-1, y, pkt);
      len += getCharSize();
    } else {
      sprintf(pkt, "      ");
      len += write(len, y, pkt);
    }
  }

  void updatePackets() {
    if (tooFast(&lastPktDraw)) {
      return;
    }
    char pkt[64];
    short len = 0;
    short y = this->getLineIndex(MAX_LINES);
    // pretty much always needs another render
    zeroPrefix(pkt, packetsPerSecond, 2);
    tft.setTextColor(TFT_SILVER, background);
    len += write(len, y, pkt);
    len += getCharSize();

    // cheapen screen write timers by skipping a little execution
    if (lastPacketsPerSecond != myLastPacketsPerSecond) {
      zeroPrefix(pkt, lastPacketsPerSecond, 2);
      setGoodColor(lastPacketsPerSecond, 250, 100);
      len += write(len, y, pkt);
      len += getCharSize();
      myLastPacketsPerSecond = lastPacketsPerSecond;
    } else {
      len *= 2; // pretend we wrote another pktspsec
    }
    if (packetsFailedPerSecond != myLastFailedPerSecond) {
      setBadColor(packetsFailedPerSecond, 5, 10);
      zeroPrefix(pkt, packetsFailedPerSecond, 2);
      len += write(len, y, pkt);
      myLastFailedPerSecond = packetsFailedPerSecond;
    }
  }

  void setGoodColor(short value, short mid, short high) {
    if (value < high) {
      tft.setTextColor(TFT_RED, background);
    } else if (value < mid) {
      tft.setTextColor(TFT_YELLOW, background);
    } else {
      tft.setTextColor(TFT_GREEN, background);
    }
  }
  
  void setBadColor(short value, short mid, short high) {
    if (value > high) {
      tft.setTextColor(TFT_RED, background);
    } else if (value > mid) {
      tft.setTextColor(TFT_YELLOW, background);
    } else {
      tft.setTextColor(TFT_SILVER, background);
    }
  }

  char getCharSize() {
     if (fontSize == 1) {
      return 10;
     } else if (fontSize == 2) {
      return 12;
     }
     return 1;
  }
  
  long write(int x, int y, const char* str) {
     tft.setCursor(1+x, 1+y, 1);
     tft.print(str);
     return strlen(str)*getCharSize();
  }

  void addColumn() {
    
  }

  void addRow(const char* rowText) {
    
  }

  void updateRow(short column, short row) {
    
  }

  void debugWrite(const char* str) {
    
  }

  void cube(int x, int y, int height, int width) {
    // draw two horizontal lines, one from x to 2x along y, one from x2y and y2x2
      tft.drawFastHLine(x, y, width, TFT_WHITE);
      tft.drawFastHLine(x, y+height, width, TFT_WHITE);
    // draw two vertical lines, one from xy to x2y and xy2 x2y2 
      tft.drawFastVLine(x, y, height, TFT_WHITE);
      tft.drawFastVLine(x+width, y, height, TFT_WHITE);
  }
private:
  TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
  char fontSize;
  int background;
  int textColor;
  short myLastPacketsPerSecond;
  short myLastFailedPerSecond;
  unsigned long lastPktDraw;
  unsigned long lastCtlDraw;
};


#endif // __TFT_H__
#else
class TFT {
  void init() {}
}
#endif // TFT_ENABLE
