//Serial_Comm_FootPedal


#ifndef SERIAL_COMM_FOOTPEDAL_H
#define SERIAL_COMM_FOOTPEDAL_H

#include <libserial/SerialPort.h>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>

using namespace LibSerial ;
using namespace std;


class USB_Comm{
	
private:
	 // Instantiate a SerialPort object.
    SerialPort serial_port;
    bool isOpen = false, isConnected_ = false;
    string inMsg,port;
    thread * thListen = nullptr;
    void (* parseFunct)(string);
	
	
public:
	USB_Comm(string port){
		inMsg = "";
		this->port = port;
		isOpen = false;
		while (!isOpen){
			//cout << "!!PUT TEENSY RETRY BACK IN!!" << endl;
			reconnect();
        }

	}
	
	~USB_Comm(){
		cout << "Killing USB Comms..." << endl;
		serial_port.Close();
		cout << "Closed serial port..." << endl;

	}
	
	void setParseFunc( void (*pF)(string)){
		parseFunct = pF;
		//listen for a new message in a new thread
		thListen = new thread(&USB_Comm::listenForMsg, this);
		thListen->detach();

        //once it connects wait for the ready signal
       /* while (!isConnected){
            sendMsg("init");
            usleep(500*1000);
        }*/
	}

    void setConnected(bool b){
        isConnected_ = b;
    }

    bool isConnected(){
        return isConnected_;
    }
	void listen(){
		if (thListen != nullptr){
			delete thListen;
		}
		//listen for a new message in a new thread
		thListen = new thread(&USB_Comm::listenForMsg, this);
		thListen->detach();
        //after listening now wait for the ready msg
        //once it connects wait for the ready signal
        /*while (!isConnected){
            sendMsg("init");
            cout << "Listen..." << endl;
            usleep(500*1000);
        }*/
	}

	bool reconnect(){
		
		try
			{
				// Open the Serial Port at the desired hardware port.
				serial_port.Open(port.c_str()) ;
				isOpen = true;
				
					// Set the baud rate of the serial port.
				serial_port.SetBaudRate(BaudRate::BAUD_115200) ;

				// Set the number of data bits.
				serial_port.SetCharacterSize(CharacterSize::CHAR_SIZE_8) ;

				// Turn off hardware flow control.
				serial_port.SetFlowControl(FlowControl::FLOW_CONTROL_NONE) ;

				// Disable parity.
				serial_port.SetParity(Parity::PARITY_NONE) ;
				
				// Set the number of stop bits.
				serial_port.SetStopBits(StopBits::STOP_BITS_1) ;
				listen();			
				return true;
				
			}
			catch (const OpenFailed&)
			{
                std::cerr << "NOT CONNECTED TO ARDUINO..trying on " << port << std::endl ;
				isOpen = false;
				usleep(200*2000);
				return false;
				
			} catch(LibSerial::NotOpen const &){
                std::cerr << "NOT CONNECTED TO ARDUINO..trying on " << port << std::endl ;
				isOpen = false;
				usleep(200*2000);
				return false;
			}
		
	}
	
	void parseUSBCommand(string cmd){
		cout << "Parse: " << cmd << endl;
	}
	void sendMsg(string cmd){
		string sen = "<" + cmd + ">";
		if (serial_port.IsOpen()){
			serial_port.Write(sen);
			cout << "Sent to FootPedal: " << sen << endl;
		} else {
			cout << "Serial Closed, Did Not Send: " << sen << endl;
		}
	}
	
	void close(){
		serial_port.Close();
	}
	
	bool isClosed(){
		//cout << "isClosed()" << endl;
	
		//checking for available data will crash if not connected
		try{
			serial_port.GetFlowControl();
			return false;
		} catch (std::runtime_error const&){
			cout << "No connection to USB" << endl;
			serial_port.Close();
			return true;
			//cout << "NOT CONNECTED TO TEENSY" << endl;
		} catch (LibSerial::NotOpen const&){
			return true;
		}
	}
	void listenForMsg(){
		try{
		while(true && serial_port.IsOpen()){
			
			// Wait for data to be available at the serial port.
			while(serial_port.IsOpen() && !serial_port.IsDataAvailable()) 
			{
				usleep(1000) ;
				
			}
			size_t ms_timeout = 1000 ;

			// Char variable to store data coming from the serial port.
			char data_byte ;

			// Read one byte from the serial port and print it to the terminal.
			try
			{
				// Read a single byte of data from the serial port.
				serial_port.ReadByte(data_byte, ms_timeout) ;
				//cout << data_byte;
				if ( data_byte == '<'){
					inMsg = "";
				}else if ( data_byte == '>'){
					//parseUSBCommand(inMsg);
                    /*cout << inMsg << endl;
                    if (inMsg == "ready"){
                        isConnected = true;
                    } else {*/
                        parseFunct(inMsg);
                   // }
				} else {
					inMsg += data_byte;
				}
				// Show the user what is being read from the serial port.
				//std::cout << data_byte << std::flush ;
			}
			catch (const ReadTimeout&)
			{
				std::cerr << "\nThe ReadByte() call has timed out." << std::endl ;
			}
			
		}
		} catch (LibSerial::NotOpen const &){
			cout << "USB disconnected while listening" << endl;
			
		}
	}
	
};



#endif
