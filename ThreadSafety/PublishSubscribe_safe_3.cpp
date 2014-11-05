#ifndef PUBLISHSUBSCRIBE
#define PUBLISHSUBSCRIBE

#include<iostream>
#include<vector>
#include<algorithm>
#include<pthread.h>
#include<boost/lexical_cast.hpp>
#include<boost/enable_shared_from_this.hpp>
#include<boost/weak_ptr.hpp>
#include<boost/shared_ptr.hpp>
#include"../Mutex.h"

using namespace std;
/*
 * 1.使用shared_ptr解决了mutex不能控制析构的问题（仅限于此）
 * 2.Publisher的vector存储的是weak_ptr而不是shared_ptr，是因为存在于vector中的shared_ptr可能延长了Subscriber的生命期，
 * 直到vector随Publisher消亡或者从中注销后，subscriber才有可能消亡，有时候这不是预期的
 * 3.Publisher理论上也应该用shared_ptr管理，但是一般它会是一个单例，生存期比subscribers长
 * 4.Publisher的register_和notifyAll争用锁，而notifyAll又回调了subscriber的update，这会产生很多问题：
 * notifyAll时间不可预期，可能会非常长，从而register_要等待很久
 * subscriber的update险象环生：
 * 如果其再调用一次publisher的争用锁函数（比如register_），且锁不是可重入的，就会死锁
 * 即使是可重入的锁，在notifyAll的迭代过程中，修改容器vector会使得程序失效，产生非预期的效果
 *
 * 可重入的锁在本身设计上就有冲突，锁是为了防止资源被意外修改，而可重入的锁允许同一个人从多处去修改，这其实多少等价与多人修改
 * */

class Publisher;

class Subscriber : public boost::enable_shared_from_this<Subscriber> {
protected:
	Publisher* publisher_;
public:
	virtual ~Subscriber();
	virtual void update() = 0;
	virtual void subscribe(Publisher* p);
};

class Publisher{
protected:
	vector< boost::weak_ptr<Subscriber> > subscribers_;
	mutable MutexLock mutex_;
public:
	virtual ~Publisher(){};

	virtual void register_(boost::weak_ptr<Subscriber> s){
		MutexLockGuard lock(mutex_);
		subscribers_.push_back(s);
	}
	/* 不需要了
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
	*/

	virtual void notifyAll(){
		MutexLockGuard lock(mutex_);
		vector< boost::weak_ptr<Subscriber> >::iterator it = subscribers_.begin();
		while( it!=subscribers_.end() ){
			boost::shared_ptr<Subscriber> sber(it->lock());
			if(sber){
				cout<<"notify "<< endl;
				sber ->  update();
				it++;
			}else{
				cout<<"erase "<<endl;
				it = subscribers_.erase(it);
			}
		}
	}
};

Subscriber::~Subscriber(){
	//if(publisher_)
		//publisher_ -> unregister_(this); 
	//这里Publisher的生存期可能已经结束，即使加判断是不是NULL也没有用：
	//在多线程下在判断的时候publisher还存在，进入分支后线程切换，恰好此时publisher消亡，
	//于是这里会出现不可预知的情况
}
void Subscriber::subscribe(Publisher* p){
	if(p){
		p -> register_(shared_from_this());
		publisher_ = p;
	}else
		cout<<"subscriber() NULL publisher"<<endl;
}
/////////////////////////////////////////////////////////////////
class Worker : public Subscriber {
	virtual void update(){
		cout<<"Worker::update() ";
		cout<<this<<endl;
	}
};
//////////////////////////////////////////////////////////////////
void* run(void* var){
	Publisher* pber = (Publisher*)var;
	//cout<<"run() var:"<< pber <<endl;
	boost::shared_ptr<Worker> worker(new Worker);
	worker->subscribe(pber);
	srand((int)(gettid()));
	int t = rand()%4+1;
	cout<<"sleep "<<t<<endl;
	sleep(t);//每一个worker存在4秒钟左右，从而构建出一部分存在一部分消亡的状态
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

	sleep(2);//特意等待2秒钟

	pber.notifyAll();

	return 0;
}
#endif
