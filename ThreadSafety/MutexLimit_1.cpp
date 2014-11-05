#include<iostream>
#include"../Mutex.h"
#include<pthread.h>
#include<assert.h>
#include<boost/lexical_cast.hpp>

using namespace std;

//mutex不能保护析构过程
//

class Resource{
private:
	mutable MutexLock mutex_;
	int* buff_;
public:
	Resource(){
		buff_ = new int[1];
		buff_[0] = 0;
	}
	~Resource(){
		//这里不安全，如果一个destroyer线程调用进入这里，
		//同时另一个worker线程阻塞在Update的锁位置，
		//析构后发生未知情况：worker有可能永远锁住，或者进入临界区，或者其他不可预知情况
		MutexLockGuard lock(mutex_);
		delete[] buff_;
	}
	void Update(){
		MutexLockGuard lock(mutex_);
		++buff_[0];
	}
	int Fetch(){
		MutexLockGuard lock(mutex_);
		return buff_[0];
	}
};

Resource* rs = NULL;

void* work(void* var){
	while(rs){
		sleep(1);
		if(rs){
			rs->Update();
			cout<<(rs->Fetch())<<endl;
		}else{
			cout<<"Resource has been destroyed"<<endl;
		}
	}
	cout<<"worker exit"<<endl;
}

void* destroy(void* var){
	sleep(10);
	delete rs;  //调用析构
	rs = NULL;
	cout<<"Resource destroyed"<<endl;
}

int main(int argc, char* argv[]){

	rs = new Resource;
	cout<<"Resouce is ready : "<<rs->Fetch()<<endl;
	int n_worker = argc>1? boost::lexical_cast<int>(argv[1]):1;
	//int n_worker = 10;
	assert(n_worker>0);

	pthread_t* workers = new pthread_t[n_worker];
	for(int i=0; i<n_worker; i++){
		if(pthread_create(workers+i, NULL, work, NULL)){
			workers[i] = -1;
			cout<<"Failed to create worker "<<i<<endl;
		}else{
			cout<<"Worker "<<i<<" created : "<<workers[i]<<endl;
		}
	}
	for(int i=0; i<n_worker; i++){
		pthread_detach(workers[i]);
	}

	pthread_t destroyer;
	if(pthread_create(&destroyer, NULL, destroy, NULL)){
		cout<<"Failed to create destroyer "<<endl;
	}else{
		cout<<"Destroyer in : "<<destroyer<<endl;
		pthread_join(destroyer, NULL);
	}

	sleep(20);//尽可能让所有worker可以检测到rs已经为NULL，进而结束线程

	return 0;
}
