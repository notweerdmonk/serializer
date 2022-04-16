#include <serializer.h>
#include <cstring>
#include <iostream>
#include <string>
#include <fstream>

class demo : public serializer {
public:
  int n;
  std::string s;
  double f;
  char str[2048];

  demo() : n(0), s(""), f(0.0), str{0,} {}

  demo(int _n, std::string _s, double _f, char _str[]) {
    n = _n;
    s = _s;
    f = _f;
    memcpy(str, _str, 2048);
  }

  void serialize() {
    try {
      write(n);
      write(s.c_str(), s.size());
      write(f);
      write(str, 2048);
    } catch(serializer::serializer_error e) {
      std::cerr << e.what() << std::endl;
    }
  }

  void deserialize() {
    try {
      read(&n);
      s.resize(seek() + 1);
      read(const_cast<char*>(s.c_str()), s.size());
      read(&f);
      read(str, 2048);
    } catch(serializer::serializer_error e) {
      std::cerr << e.what() << std::endl;
    }
  }
};

int main() {
  serializer s;
  double f;
  unsigned int n;
  char *str = new char[8];

  int arr_in[] = { 127, 255, 65535 };
  int arr_out[4];

  try {
    s.write(0xffffffff);
    s.write("string", 6);
    s.write(arr_in, 3);
    s.write(3.14159);
  } catch(serializer::serializer_error e) {
    std::cerr << e.what() << std::endl;
  }
  try {
    s.read(&n);
    s.read(str, 8);
    s.read(arr_out, 4);
    s.read(&f);
  } catch(serializer::serializer_error e) {
    std::cerr << e.what() << std::endl;
  }
  std::cout << n << ", " << str << ", ";
  for (int n: arr_out) std::cout << n;
  std::cout << ", " << f << std::endl;
  delete str;

  char str2[2048];
  memset(str2, 'A', 2048);
  demo obj1(0xffff, "string", 3.14159, str2);
  obj1.serialize();
  std::ofstream out("data", std::ios::binary);
  //std::ostream out(std::cout.rdbuf());
  obj1.store(out);
  out.close();

  demo obj2;
  //std::istream in(std::cin.rdbuf());
  std::ifstream in("data", std::ios::binary);
  obj2.retrieve(in);
  obj2.deserialize();
  std::cout << "obj2: " << obj2.n << ", " << obj2.s << ", " << obj2.f << ", ";
  char str3[2048] = { 'A', };
  memset(str3, 'A', 2048);
  std::cout << "memcmp result: " << memcmp(str2, str3, 2048);
  std::cout << std::endl;

  return 0;
}
