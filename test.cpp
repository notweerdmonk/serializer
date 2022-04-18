#include <serializer.h>
#include <cstring>
#include <cassert>
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

      std::size_t size = seek();
      char* buffer = new char[size];
      read(buffer, size);
      buffer[size] = '\0';
      s = buffer; 

      read(&f);
      read(str, 2048);

    } catch(serializer::serializer_error e) {

      std::cerr << e.what() << std::endl;
    }
  }
};

int main() {

  bool check = true;

  /*
   * GIVEN C dataypes
   * WHEN data is serialzed, stored to a file, retrieved and deserialzied
   * THEN check if data is not corruped
   */

  unsigned int n;
  char *str = new char[8];
  double f;
  int arr_in[] = { 127, 255, 65535 };
  int arr_out[4];

  serializer s;

  try {

    s.write(0xffffffff);
    s.write("string", 7);
    s.write(3.14159);
    s.write(arr_in, 3);

  } catch(serializer::serializer_error e) {

    std::cerr << e.what() << std::endl;
  }
  try {

    s.read(&n);
    s.read(str, 7);
    s.read(&f);
    s.read(arr_out, 3);

  } catch(serializer::serializer_error e) {

    std::cerr << e.what() << std::endl;
  }

  assert(("int check failed", n == 0xffffffff));

  assert(("char[] string check failed", !strcmp(str, "string")));

  assert(("double check failed", f == 3.14159));

  assert(("int[] check failed", !memcmp(arr_out, arr_in, 3)));

  delete str;

  /*
   * GIVEN object
   * WHEN members are serialzed, stored to a file, retrieved and deserialzied
   * THEN check if data is not corruped
   */

  char mem_in[2048];
  memset(mem_in, 'A', 2048);

  demo obj1(0xffff, "string", 3.14159, mem_in);
  obj1.serialize();

  std::ofstream out("data", std::ios::binary);

  obj1.store(out);
  out.close();

  demo obj2;

  //std::istream in(std::cin.rdbuf());
  std::ifstream in("data", std::ios::binary);

  obj2.retrieve(in);
  obj2.deserialize();

  check = true;

  assert(("int member check failed", obj2.n == 0xffff));

  assert(("std::string member check failed", obj2.s == "string"));

  assert(("double member check failed", obj2.f == 3.14159));

  char mem_out[2048];
  memset(mem_out, 'A', 2048);

  assert(("char[] member check failed", !memcmp(mem_out, mem_in, 2048)));

  std::cout << "All checks have passed\n";

  return 0;
}
