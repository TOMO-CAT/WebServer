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

int main() {
  MySingleton::getInstance().printMessage();  // 使用单例对象
  return 0;
}
