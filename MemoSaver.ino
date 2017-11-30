/*
  MemoSaver - Storage for files in the Text app on the Tandy 100/102
 */
#include <SD.h>

// Hard drive activity light on pin 7
#define HDD_IND 7
#define SEND_BUTTON A4

#define NO_FILE 75535

// Display debug output
//#define DEBUG_PRINT(x) Serial.print(x)
//#define DEBUG_PRINT1(x,y) Serial.print(x,y)
//#define DEBUG_PRINTLN(x) Serial.println(x)
//#define DEBUG_PRINTLN1(x,y) Serial.println(x,y)

// Do not display debug output
#define DEBUG_PRINT(x) 
#define DEBUG_PRINT1(x,y)
#define DEBUG_PRINTLN(x) 
#define DEBUG_PRINTLN1(x,y) 

char data;
unsigned long timeout;
char pathname[255];
char buf[255];
int path_loc;
char command;
bool debounce_button;

File selected_file;
int selected_file_open;

int state;
#define initial_state 0
#define start_preamble 1
#define got_preamble 2
#define getting_command 3
#define process_command 4
#define process_save 5

int save_result_value;
#define save_result_unknown 0
#define save_result_good 1
#define save_result_fail 2

int del_result_value;
#define del_result_unknown 0
#define del_result_good 1
#define del_result_fail 2


int result_state;
#define no_result 0
#define save_result 1
#define load_result 2
#define list_result 3
#define del_result 4

int count;

void setup() {
  // Open serial communications for debugging
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // Set the hard drive light pin
  pinMode(HDD_IND,OUTPUT);
  
  // Set up boot button
  pinMode(SEND_BUTTON,INPUT_PULLUP);
  
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (53 on the Mega) must be left as an output or the SD library 
  // functions will not work. 
  // Note that we connect to this pin through the SPI header.  The SD
  // shield doesn't stretch to pin 53 on the Mega.
  pinMode(53, OUTPUT);
  
  Serial.print("Initializing SD card...");
  if (!SD.begin(8))
  {
    Serial.println("Card failed, or not present");
  } 
  else {
    Serial.println("card initialized.");
  }
  
  // set the data rate for the SoftwareSerial port
  Serial1.begin(1200);
  timeout = 0;
  state = initial_state;
  result_state = no_result;
  save_result_value = save_result_unknown;
  del_result_value = del_result_unknown;
  debounce_button = false;
  
  // Clear the trash out  
  while(Serial1.available()) {
      Serial1.read();
  }
}

void loop() {
  
  // If there's data available
  if (Serial1.available()) {
    timeout = 0;
    data = Serial1.read();
    display(data);
    count++;

    if (data != 0) {

      switch(state) {
        case initial_state: // We started getting data
          if (data=='#') {
            state = start_preamble;
          }
          break;
  
        case start_preamble:
          if (data=='#') {
            state = getting_command;
            path_loc = 0;
          }
          break;
          
        case getting_command:
          if (data == 13) { // cr/lf
            pathname[path_loc] = 0; // null terminate the path name
            data = Serial1.read(); // We need to eat up the lf
            state = process_command;
          }
          else {
            pathname[path_loc] = data;
            path_loc++;
          }
          break;

        case process_save:
          if (selected_file.write(data) != 1) {
            selected_file.close();
            selected_file_open = 0;
            state = initial_state;
            save_result_value = save_result_fail;
            result_state = save_result;
            DEBUG_PRINTLN("Save Error");
          }

          if (data == 26) { // End of data
            selected_file.close();
            selected_file_open = 0;
            state = initial_state;
            save_result_value = save_result_good;
            result_state = save_result;
            DEBUG_PRINTLN("File saved");
          }
          break;
      }
    } 
  } else {
    // We haven't seen data in a long time, timeout.
    timeout++;
    if (timeout > 100000 && state != initial_state) {
      //DEBUG_PRINTLN("Timeout!");
      timeout =0;
      state = initial_state;

      DEBUG_PRINT("Count");
      DEBUG_PRINTLN1(count,DEC);
      count = 0;
      
      if (selected_file_open) {
        selected_file.close();
        selected_file_open = 0;
      }
    }
  }

  int i;
  char *filename;

  if (state == process_command) {
    DEBUG_PRINTLN("");
    DEBUG_PRINT("Processing command:");
    
    switch(toupper(pathname[0])) {
      case 'S': // Save
        DEBUG_PRINTLN("Save");
        for(i=0; pathname[i] != ' ' && pathname[i] != 0; i++); // Go to the first space
        for(;pathname[i] == ' '; i++); // Eat up any spaces

        filename = &(pathname[i]);
        DEBUG_PRINTLN(filename); // print out the file name

        if (SD.exists(filename)) {
          SD.remove(filename);
        }
        
        selected_file = SD.open(filename,FILE_WRITE);

        // If the file opened
        if (selected_file) {
          selected_file_open=1;
          state = process_save;
        }
        else {
          DEBUG_PRINT("Unable to open file for saving:");
          DEBUG_PRINTLN("");
          selected_file_open=0;
          state = initial_state;
          save_result_value = save_result_fail;
          result_state = save_result;
        }
        
        break;
        
      case 'L': // Load/List
        switch(toupper(pathname[1])) {
          case 'I':
            DEBUG_PRINTLN("List");
            result_state = list_result;
            state = initial_state;
            break;
        
          case 'O':
            DEBUG_PRINTLN("Load");
            result_state = load_result;
            state = initial_state;
            break;
        }
        break;
        
      case 'D': // Delete
        DEBUG_PRINTLN("Delete");
        result_state = del_result;

        for(i=0; pathname[i] != ' ' && pathname[i] != 0; i++); // Go to the first space
        for(;pathname[i] == ' '; i++); // Eat up any spaces
        filename = &(pathname[i]);
        DEBUG_PRINT(filename);
        
        if (SD.exists(filename)) {
          DEBUG_PRINT(" exists ");
          SD.remove(filename);
          DEBUG_PRINTLN(" removed");
          del_result_value = del_result_good;
        } else {
          DEBUG_PRINT(filename);
          DEBUG_PRINTLN(" does not exist");
          del_result_value = del_result_fail;
        }

        state = initial_state;
        break;

      default:
        DEBUG_PRINTLN("Unknown");        
        state = initial_state;
    }

  }

  if (digitalRead(SEND_BUTTON) != HIGH) {
    if (!debounce_button) {
      send_result();  
      debounce_button = true;
    }
  } else {
    debounce_button = false;
  }

}

void send_result() {
  char *filename;
  int i;

  DEBUG_PRINTLN("Send button pressed");
  switch(result_state) {
    case no_result:
      send_data("No results\x1a");
      break;
      
    case save_result:
      switch(save_result_value) {
        case save_result_good:
          send_data("File was saved\x1a");
          break;
          
        case save_result_fail:
          send_data("File was not saved\x1a");
          break;        
      }
      break;
      
    case load_result:
      for(i=0; pathname[i] != ' ' && pathname[i] != 0; i++); // Go to the first space
      for(;pathname[i] == ' '; i++); // Eat up any spaces
      
      filename = &(pathname[i]);
      DEBUG_PRINTLN(filename); // print out the file name

      selected_file = SD.open(filename,FILE_READ);

      int length;
      char last_char;
      
      // If the file opened
      if (selected_file) {
        length = selected_file.read(buf, 128);
        if (length == 0) {
          send_data("File is empty\x1a");
        } else {
          while(length > 0) {
            send_data_length(buf,length);
            DEBUG_PRINTLN("Sending");
            last_char = buf[length-1];
            length = selected_file.read(buf, 128);
          }
        }

        // If the last character sent was NOT an ^Z, send a ^Z
        if (last_char != 26) {
          Serial1.write(26);
        }
        
        selected_file.close();
      }
      else {
        send_data("Unable to open file\x1a");
      }

      selected_file_open=0;
      state = initial_state;
      result_state = no_result;
      
      break;
      
    case list_result:
      send_file_list();
      break;

    case del_result:
      switch(del_result_value) {
        case del_result_good:
          send_data("File deleted\x1a");
          break;
          
        case del_result_fail:
          send_data("File not deleted.  Exists?\x1a");
          break;
        
        default:
          send_data("No delete command given\x1a");
          break;
      }
      break;

    default:
      send_data("Result not defined\x1a");
  }
}

void send_file_list() {
  // Open the directory
  selected_file = SD.open("/");
  selected_file_open = 1;
  selected_file.rewindDirectory();

  // Get the first file
  File entry =  selected_file.openNextFile();
  
  // If there was no file, return a no file
  if (! entry) {
    send_data("No files");
    selected_file.close();
    selected_file_open = 0;
  }

  // While we have directory entries
  while(entry) {
    send_data(entry.name());
    send_data(" ");
    Serial1.println(entry.size(),DEC);
    entry.close();

    entry =  selected_file.openNextFile();
  }

  send_data("\x1a");
  selected_file.close();
  selected_file_open = 0;
}

void send_data_length(char data[],int length) {
  int x;
  
  for(int i=0; i < length; i++) {
    Serial1.write(data[i]);
  }
}

void send_data(char data[]) {
  int x;
  
  for(int i=0; data[i] != 0; i++) {
    Serial1.write(data[i]);
  }
}

void display(char c) {
    if (c > 31 && c < 127) {
      DEBUG_PRINT((char)c);
    }
    else {
      DEBUG_PRINT("x");
      DEBUG_PRINT1(c,HEX);
    }

    if (c == 10) {
      DEBUG_PRINTLN("");
    } else {
      DEBUG_PRINT(" ");
    }

    
}

