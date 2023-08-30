#include <iostream>

class MySingleton {
 public:
  static MySingleton& getInstance() {
    static MySingleton instance;
    return instance;
  }

  void printMessage() {
    std::cout << "This is a singleton!" << std::endl;
  }

 private:
  MySingleton() {
  }
  ~MySingleton() {
    std::cout << "析构函数" << std::endl;
  }
};

class Cat {
 public:
  Cat() {
    std::cout << "Cat构造函数" << std::endl;
  }
  ~Cat() {
    std::cout << "Cat析构函数" << std::endl;
  }

 public:
  void eat() {
    std::cout << "Cat is eating" << std::endl;
  }
};

int main() {
  Cat* cat = new Cat();
  cat->eat();
  return 0;
}
