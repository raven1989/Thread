#include<iostream>
#include"Mutex.h"

using namespace std;

void foo(MutexLock& m){
	MutexLockGuard lock(m);
	cout<<"Locked"<<endl;
}

int main(){
	MutexLock mutex;
	foo(mutex);
	return 0;
}
