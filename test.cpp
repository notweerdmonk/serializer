#include <serializer.h>
#include <cstring>
#include <cassert>
#include <iostream>
#include <string>
#include <fstream>

using namespace yas;

/* class that inherits serializer class */
class demo : public serializer {
public:
  int n;
  double f;
  std::string s;
  char mem[2048];

  demo() : n(0), s(""), f(0.0), mem{0,} {}

  demo(int _n, std::string _s, double _f, char _mem[]) {

    n = _n;
    s = _s;
    f = _f;

    memcpy(mem, _mem, 2048);
  }

  void serialize() {

    try {
      WRITE_MANY(*this, n, f, s);

      write(mem, 2048);

    } catch(serializer::serializer_error& e) {

      std::cerr << e.what() << std::endl;
    }
  }

  void deserialize() {

    try {
      READ_MANY(*this, &n, &f, s);

      read(mem, 2048);

    } catch(serializer::serializer_error& e) {

      std::cerr << e.what() << std::endl;
    }
  }
};

/* class that uses macros to generate serialize/deserialize functions */
class demo2 {
public:
  int n;
  double f;
  std::string s;
  char mem[2048];

  demo2() : n(0), s(""), f(0.0), mem{0,} {}

  demo2(int _n, std::string _s, double _f, char _mem[]) {

    n = _n;
    s = _s;
    f = _f;

    memcpy(mem, _mem, 2048);
  }

  SERIALIZE_BEGIN(obj)
    WRITE_MANY(obj, n, f, s);

    obj.write(mem, 2048);
  SERIALIZE_END

  DESERIALIZE_BEGIN(obj)
    READ_MANY(obj, &n, &f, s);

    obj.read(mem, 2048);
  DESERIALIZE_END
};

int main() {

  /*
   * GIVEN C dataypes
   * WHEN data is serialized and deserialzied
   * THEN check if data is not corruped
   */

  unsigned int n;
  double f;
  char *str = new char[8];
  std::string s_in{"c++ string"};
  std::string s_out;
  int arr_in[] = { 127, 255, 65535 };
  int arr_out[4];

  serializer s;

  try {

    s.write(0xffffffff);
    s.write(3.14159);
    s.write("string", 7);
    s.write(arr_in, 3);
    s.write(s_in);

  } catch(serializer::serializer_error& e) {

    std::cerr << e.what() << std::endl;
  }
  try {

    s.read(&n);
    s.read(&f);
    s.read(str, 7);
    s.read(arr_out, 3);
    s.read(s_out);

  } catch(serializer::serializer_error& e) {

    std::cerr << e.what() << std::endl;
  }

  assert(("int check failed", n == 0xffffffff));

  assert(("char[] string check failed", !strcmp(str, "string")));

  assert(("double check failed", f == 3.14159));

  assert(("int[] check failed", !memcmp(arr_out, arr_in, 3)));

  delete str;

  /***************************************************************************/

  /*
   * GIVEN object of class which inherits serializer class
   * WHEN members are serialized, stored to a file, retrieved and deserialzied
   * THEN check if data is not corruped
   */

  char mem[2048];
  memset(mem, 'A', 2048);

  demo obj1(0xffff, "string", 3.14159, mem);
  obj1.serialize();

  std::ofstream out("data", std::ios::binary);

  obj1.store(out);
  out.close();

  demo obj2;

#if 0
  std::istream in(std::cin.rdbuf());
#endif
  std::ifstream in("data", std::ios::binary);

  obj2.retrieve(in);
  obj2.deserialize();

  assert(("int member check failed", obj2.n == 0xffff));

  assert(("std::string member check failed", obj2.s == "string"));

  assert(("double member check failed", obj2.f == 3.14159));

  assert(("char[] member check failed", !memcmp(obj2.mem, mem, 2048)));

  /***************************************************************************/

  /*
   * GIVEN object of class which contains generated code
   * WHEN members are serialized and deserialzied
   * THEN check if data is not corruped
   */

  serializer *sptr = new serializer();

  demo2 obj3(0xffff, "string", 3.14159, mem);
  obj3.serialize(*sptr);

  demo2 obj4;
  obj4.deserialize(*sptr);

  delete sptr;

  assert(("int member check failed", obj4.n == 0xffff));

  assert(("std::string member check failed", obj4.s == "string"));

  assert(("double member check failed", obj4.f == 3.14159));

  assert(("char[] member check failed", !memcmp(obj4.mem, mem, 2048)));

  /***************************************************************************/

  std::cout << "All checks have passed\n";

  return 0;
}
