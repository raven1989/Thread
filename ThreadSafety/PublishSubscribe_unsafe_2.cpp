#ifndef PUBLISHSUBSCRIBE
#define PUBLISHSUBSCRIBE

#include<iostream>
#include<vector>
#include<algorithm>
#include<pthread.h>
#include<boost/lexical_cast.hpp>
#include"../Mutex.h"


using namespace std;

class Publisher;

class Subscriber{
protected:
	Publisher* publisher_;
public:
	virtual ~Subscriber();
	virtual void update() = 0;
	virtual void subscribe(Publisher* p);
};

class Publisher{
protected:
	vector<Subscriber*> subscribers_;
	mutable MutexLock mutex_;
public:
	virtual ~Publisher(){};

	virtual void register_(Subscriber* s){
		if(s){
			MutexLockGuard lock(mutex_);
			if(s)
				subscribers_.push_back(s);
		}
	}
	virtual void unregister_(Subscriber* s){
		if(s){
			MutexLockGuard lock(mutex_);
			if(s){
				vector<Subscriber*>::iterator it = find(subscribers_.begin(), subscribers_.end(), s);
				if( it != subscribers_.end()){
					swap(*it, subscribers_.back());
					subscribers_.pop_back();
				}
			}
		}
	}

	virtual void notifyAll(){
		for(int i=0; i<subscribers_.size(); i++){
			Subscriber* item = subscribers_[i];
			if(item){
				cout<<"notify "<<item<<endl;
				item -> update();
				//这里item指向的subscriber可能已经消亡，参看下面~Subscribe()中的解释
			}
			else
				cout<<"NULL subscriber"<<endl;
		}
	}
};

Subscriber::~Subscriber(){
	if(publisher_)
		publisher_ -> unregister_(this); 
	//这里Publisher的生存期可能已经结束，即使加判断是不是NULL也没有用：
	//在多线程下在判断的时候publisher还存在，进入分支后线程切换，恰好此时publisher消亡，
	//于是这里会出现不可预知的情况
}
void Subscriber::subscribe(Publisher* p){
	if(p){
		p -> register_(this);
		publisher_ = p;
	}else
		cout<<"subscriber() NULL publisher"<<endl;
}
/////////////////////////////////////////////////////////////////
class Worker : public Subscriber {
	virtual void update(){
		cout<<"Worker::update() ";
		cout<<hex<<this<<endl;
	}
};
//////////////////////////////////////////////////////////////////
void* run(void* var){
	Publisher* pber = (Publisher*)var;
	//cout<<"run() var:"<< pber <<endl;
	Worker worker;
	worker.subscribe(pber);
	sleep(10);//特意让worker存在10秒钟，好让publisher不至于一个都notify不到,一旦取消就会看到Publisher一个都通知不到
}

int main(int argc, char* argv[]){

	Publisher pber;
	void* pvar = (void*)(&pber);
	//cout<<"main() pvar:"<<pvar<<endl;

	int n_worker = argc>1? boost::lexical_cast<int>(argv[1]):1;
	pthread_t* workers = new pthread_t[n_worker];
	for(int i=0; i<n_worker; i++){
		if(pthread_create(workers+i, NULL, run, pvar)){
			workers[i] = -1;
			cout<<"Failed to create worker "<<i<<endl;
		}else{
			cout<<"Worker "<<i<<" created : "<<workers[i]<<endl;
		}
	}
	for(int i=0; i<n_worker; i++){
		pthread_detach(workers[i]);
		//pthread_join(workers[i], NULL);
	}

	//sleep(5);//特意等待5秒钟，好让worker线程去注册

	pber.notifyAll();

	return 0;
}
#endif
