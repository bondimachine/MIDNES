#ifndef USE_TINYUSB_HOST
#error "Select USB Stack: Adafruit TinyUSB Host"
#endif

#include <Adafruit_TinyUSB.h>

#ifndef CFG_TUH_MIDI
#error "Midi host not enabled. Set #define CFG_TUH_MIDI 1 in ~/Library/Arduino15/packages/rp2040/hardware/rp2040/<version>/libraries/Adafruit_TinyUSB_Arduino/src/arduino/ports/rp2040/tusb_config_rp2040.h"
#endif


#define RING_BUFFER_SIZE 3072
uint8_t ring_buffer[RING_BUFFER_SIZE];
uint16_t ring_buffer_pos_write = 0;
uint16_t ring_buffer_pos_read = 0;


#define PIN_DATA  2
#define PIN_LATCH 3
#define PIN_CLOCK 4
#define PIN_MODE 14

bool midi_passthru_mode = true;

struct __attribute__((packed)) {
  uint8_t a: 1;
  uint8_t b: 1;
  uint8_t select: 1;
  uint8_t start: 1;
  uint8_t up: 1;
  uint8_t down: 1;
  uint8_t left: 1;
  uint8_t right: 1;
} joystick_data;

// USB Host object
Adafruit_USBH_Host USBHost;

// holding device descriptor
tusb_desc_device_t desc_device;

// holding the device address of the MIDI device
uint8_t dev_idx = TUSB_INDEX_INVALID_8;

volatile uint8_t next_byte;
volatile uint8_t next_clock;

void on_latch() {

  if (midi_passthru_mode) {
    if (ring_buffer_pos_read != ring_buffer_pos_write) {
      next_byte = ring_buffer[ring_buffer_pos_read];
      next_clock = 1;
      digitalWriteFast(PIN_DATA, !(next_byte & 1));
      ring_buffer_pos_read++;
      if (ring_buffer_pos_read == RING_BUFFER_SIZE) {
        ring_buffer_pos_read = 0;
      }
    } else {
      next_byte = 0;
      next_clock = 0;
    }
  } else {
    next_byte = *((uint8_t*)&joystick_data);
    digitalWriteFast(PIN_DATA, !(next_byte & 1));
    next_clock = 1;
  }

}

void on_clock() {
  if (next_clock < 8) {
    digitalWriteFast(PIN_DATA, !((next_byte >> next_clock) & 1));
    next_clock++;
  } else {
    digitalWriteFast(PIN_DATA, 1);
  }
}

void on_mode() {
  midi_passthru_mode = (digitalRead(PIN_MODE) == HIGH);
  if (midi_passthru_mode) {
      Serial1.println("MIDI Passthru mode");
  } else {
      Serial1.println("Joystick mode");
      *((uint8_t*)&joystick_data) = 0;
  }
}

// the setup function runs once when you press reset or power the board
void setup() {
  USBHost.begin(0); // 0 means use native RP2040 host

  Serial1.begin(115200);
  Serial1.println("NES MIDI adapter");

  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_LATCH, INPUT);
  pinMode(PIN_CLOCK, INPUT);
  pinMode(PIN_MODE, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_LATCH), on_latch, FALLING);  
  attachInterrupt(digitalPinToInterrupt(PIN_CLOCK), on_clock, RISING);  
  attachInterrupt(digitalPinToInterrupt(PIN_MODE), on_mode, CHANGE);  

  midi_passthru_mode = (digitalRead(PIN_MODE) == HIGH);

}

void loop() {
  USBHost.task();
}


void tuh_mount_cb (uint8_t daddr) {
  Serial1.printf("Device attached, address = %d\r\n", daddr);
}

void tuh_umount_cb(uint8_t daddr) {
  Serial1.printf("Device removed, address = %d\r\n", daddr);
}

//--------------------------------------------------------------------+
void tuh_midi_mount_cb(uint8_t idx, const tuh_midi_mount_cb_t* mount_cb_data) {
  Serial1.printf("MIDI Device Index = %u, MIDI device address = %u, %u IN cables, %u OUT cables\r\n", idx,
      mount_cb_data->daddr, mount_cb_data->rx_cable_count, mount_cb_data->tx_cable_count);

  if (dev_idx == TUSB_INDEX_INVALID_8) {
    // then no MIDI device is currently connected
    dev_idx = idx;
  } else {
    Serial1.printf("A different USB MIDI Device is already connected.\r\nOnly one device at a time is supported in this program\r\nDevice is disabled\r\n");
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_midi_umount_cb(uint8_t idx) {
  if (idx == dev_idx) {
    dev_idx = TUSB_INDEX_INVALID_8;
    Serial1.printf("MIDI Device Index = %u is unmounted\r\n", idx);
  } else {
    Serial1.printf("Unused MIDI Device Index  %u is unmounted\r\n", idx);
  }
}

void tuh_midi_rx_cb(uint8_t idx, uint32_t num_packets) {
  if (dev_idx == idx) {
    if (num_packets != 0) {
      uint8_t cable_num;
      while (1) {
        uint8_t buffer[20];
        uint32_t bytes_read = tuh_midi_stream_read(dev_idx, &cable_num, buffer, 20);
        if (bytes_read == 0)
          return;
        Serial1.printf("MIDI RX Cable #%u:", cable_num);
        for (uint32_t jdx = 0; jdx < bytes_read; jdx++) {
          Serial1.printf("%02x ", buffer[jdx]);
          if (midi_passthru_mode) {
            ring_buffer[ring_buffer_pos_write] = buffer[jdx];
            ring_buffer_pos_write++;
            if (ring_buffer_pos_write == RING_BUFFER_SIZE) {
              ring_buffer_pos_write = 0;
            }
          }
        }
        Serial1.printf("\r\n");
        if (!midi_passthru_mode) {
          int i = 0;
          while (i < bytes_read) {
            if (buffer[i] & 128) {
              if (buffer[i] == 0x90 || buffer[i] == 0x80) {
                // 0x90 note on
                // 0x80 note off
                bool note_on = buffer[i] == 0x90;
                uint8_t note = buffer[i+1] % 12; // in base 10, C1 = 24, C2 = 36, etc.
                switch (note) {
                  case 0: // C
                    joystick_data.left = note_on;
                    break;
                  case 2: // D
                    joystick_data.down = note_on;
                    break;
                  case 3: // D#
                    joystick_data.up = note_on;
                    break;
                  case 4: // E
                    joystick_data.right = note_on;  
                    break;
                  case 5: // F  
                    joystick_data.up = note_on;
                    break;
                  case 6: // F#
                    joystick_data.select = note_on;
                    break;
                  case 7: // G
                    joystick_data.a = note_on;
                    break;
                  case 8: // G#
                    joystick_data.start = note_on;
                    break;
                  case 9: // A
                    joystick_data.b = note_on;
                    break;
                  case 11: // B
                    joystick_data.a = note_on;
                    break;
                }
                i+= 2; // already read pitch and ignore velocity
              }
            }
            i++;
          }
        }
      }
    }
  }
}