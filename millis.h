

#ifndef MILLIS_H
#define MILLIS_H


#include <chrono>
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

double getMillis(){
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

double getSeconds(){
    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}



#endif
