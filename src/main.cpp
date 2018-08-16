#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

using namespace std;

#include "record.hpp"






Record record;

void signal_callback(int sig){
	record.release();
	exit(0);
}


int main(){
	signal(SIGINT, signal_callback);
	while ( true ){
		record.execute();
	}
	return 0;
}
