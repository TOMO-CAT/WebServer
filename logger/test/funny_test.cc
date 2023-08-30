#include <iostream>

class Cat {
 public:
  Cat() {
    std::cout << "Cat constructor" << std::endl;
  }
  ~Cat() {
    std::cout << "Cat destructor" << std::endl;
  }

 public:
  void eat() {
    std::cout << "Cat is eating" << std::endl;
  }
};

// 堆上变量在程序退出时不会执行析构函数, 除非显式 delete
int main() {
  Cat* cat = new Cat();
  cat->eat();
  // delete cat;
}
