#ifndef Mutex_H
#define Mutex_H

#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/syscall.h>

namespace{
	pid_t gettid(){
		return static_cast<pid_t>(::syscall(SYS_gettid));
	}
}

class MutexLock {
private:
	pthread_mutex_t mutex_;
	pid_t holder_;

	MutexLock(const MutexLock&);
	MutexLock& operator = (const MutexLock);

public:
	MutexLock():holder_(0){
		pthread_mutex_init(&mutex_, NULL);
	}
	~MutexLock(){
		pthread_mutex_destroy(&mutex_);
	}
	void Lock(){
		pthread_mutex_lock(&mutex_);
		holder_ = ::gettid();
	}
	void Unlock(){
		holder_ = 0;
		pthread_mutex_unlock(&mutex_);
	}
};

class MutexLockGuard{
private:
	MutexLock& mutex_;
public:
	explicit MutexLockGuard(MutexLock& mutex):mutex_(mutex){
		mutex_.Lock();
	}
	~MutexLockGuard(){
		mutex_.Unlock();
	}
};

#endif
