#include<iostream>
#include<boost/shared_ptr.hpp>
#include<boost/enable_shared_from_this.hpp>


using namespace std;
using namespace boost;

class Shared0{
public:
	int data;
	void doIt();
};


void callBack0(shared_ptr<Shared0> p){
	cout<<(p->data)<<endl;
}
void Shared0::doIt(){
	callBack0(shared_ptr<Shared0>(this));//这样是错误的，会将同一个对象同时托管给多个shared_ptr，从而析构的时候会报错
}

class Shared1 : public enable_shared_from_this<Shared1>{
public:
	int data;
	void doIt();
};

void callBack1(shared_ptr<Shared1> p){
	cout<<(p->data)<<endl;
}

void Shared1::doIt(){
	//一旦使用了shared_ptr管理一个对象就要从一而终，不能即用shared_ptr又用raw指针，但shared_ptr析构时，raw指针可能还在被使用
	callBack1(shared_from_this());//为了能够在类内部将this指针传出，同时统一地使用shared_ptr管理。这就是enable_shared_from_this的作用
}


int main(){
	shared_ptr<Shared0> o0(new Shared0);
	o0->doIt();
  
	//使用shared_from_this，则只能使用堆对象，不能用栈对象
	shared_ptr<Shared1> o1(new Shared1);
	o1->doIt();

	return 0;
}
