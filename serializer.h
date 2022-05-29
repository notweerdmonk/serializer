/*
  MIT License
  
  Copyright (c) 2022 notweerdmonk
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <exception>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <array>

#define CAT(a, b)   a##b

#define SEQ_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...)   N

#define NARGS(...)  SEQ_N(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1)

#define FOR_EACH_1(F, a)       F(a)
#define FOR_EACH_2(F, a, ...)  F(a); FOR_EACH_1(F, __VA_ARGS__)
#define FOR_EACH_3(F, a, ...)  F(a); FOR_EACH_2(F, __VA_ARGS__)
#define FOR_EACH_4(F, a, ...)  F(a); FOR_EACH_3(F, __VA_ARGS__)
#define FOR_EACH_5(F, a, ...)  F(a); FOR_EACH_4(F, __VA_ARGS__)
#define FOR_EACH_6(F, a, ...)  F(a); FOR_EACH_5(F, __VA_ARGS__)
#define FOR_EACH_7(F, a, ...)  F(a); FOR_EACH_6(F, __VA_ARGS__)
#define FOR_EACH_8(F, a, ...)  F(a); FOR_EACH_7(F, __VA_ARGS__)

#define FOR_EACH_EXPAND(N, F, ...)  CAT(FOR_EACH_, N)(F, __VA_ARGS__)

#define FOR_EACH(F, ...)  FOR_EACH_EXPAND(NARGS(__VA_ARGS__), F, __VA_ARGS__)

#define WRITE_MANY(obj, ...)  FOR_EACH((obj).write, __VA_ARGS__)

#define READ_MANY(obj, ...)   FOR_EACH((obj).read, __VA_ARGS__)

#define SERIALIZE_BEGIN(obj) \
  void serialize(serializer& obj) { \
    try {

#define SERIALIZE_END \
    } catch(yas::serializer::serializer_error& e) { \
      std::cerr << e.what() << std::endl; \
    } \
  }

#define DESERIALIZE_BEGIN(obj) \
  void deserialize(serializer& obj) { \
    try {

#define DESERIALIZE_END \
    } catch(yas::serializer::serializer_error& e) { \
      std::cerr << e.what() << std::endl; \
    } \
  }


namespace yas {

  class serializer {

    std::basic_stringstream<uint8_t> buffer;
    std::mutex buffer_mutex;

    void _write(const uint8_t *data, std::size_t size) {
      /* set MSB */
      size = size | ((unsigned long)0x80 << 8 * (sizeof(size) - 1));
      buffer.write(reinterpret_cast<const uint8_t*>(&size), sizeof(size));

      size = size & ~((unsigned long)0x80 << 8 * (sizeof(size) - 1));
      buffer.write(data, size);
    }

    bool _check_magic(std::size_t& size) {

      return (size & ((unsigned long)0x80 << 8 * (sizeof(size) - 1))) &&
        (size = size & ~((unsigned long)0x80 << 8 * (sizeof(size) - 1)));
    }

  public:

    class serializer_error : public std::exception {

    public:
      enum err_type {
        range_err = 1,
        arr_range_err,
        data_err,
        size_err,
      };

      explicit serializer_error(const err_type& e) {
        error = e;
      }

      const char* what() const noexcept {

        if (error == range_err) {

          return "Size of type exceeds 127.";

        } else if (error == arr_range_err) {

          return "Size of array exceeds 9223372036854775807.";

        } else if (error == data_err) {

          return "Corrupt data in buffer.";

        } else if (error == size_err) {

          return "Size mismatch.";

        } else {
          return "Unhandled exception.";
        }
      }

    private:
      err_type error;

    };

    virtual ~serializer() = default;

    /* implement functions to serialize/deserialize class members individually */
    virtual void serialize() { }
    virtual void deserialize() { }

    template<typename T>
    void write(T t) {

      static_assert(std::is_trivially_copyable<T>::value,
          "Attempt to write object of non-trivially copyable type. Implement"
          "serialization for the class.");

      auto size = sizeof(T);
      if (size > 127) {

        throw serializer_error(serializer_error::range_err);
      }

      const std::lock_guard<std::mutex> lock(buffer_mutex);

      _write(reinterpret_cast<const uint8_t*>(&t), size);
    }

    template<typename T>
    void write(T *t, std::size_t n) {

      static_assert(std::is_trivially_copyable<T>::value,
          "Attempt to write array with elements of non-trivially copyable type."
          "Implement serialization for the class.");

      auto size = sizeof(T) * n;
      if (size > 0x7fffffffffffffff) {

        throw serializer_error(serializer_error::arr_range_err);
      }

      const std::lock_guard<std::mutex> lock(buffer_mutex);

      _write(reinterpret_cast<const uint8_t*>(t), size);
    }

    template<typename CharT>
    void write(std::basic_string<CharT>& s) {

      auto size = s.length() * sizeof(CharT);

      if (size > 0x7fffffffffffffff) {

        throw serializer_error(serializer_error::arr_range_err);
      }

      if (std::is_trivially_copyable<CharT>::value) {

        const std::lock_guard<std::mutex> lock(buffer_mutex);

        /* 
         * We cannot use opetator<< as it has no overload for std::basic_string.
         */
        _write(reinterpret_cast<const uint8_t*>(s.c_str()), size);

      } else {
        const std::lock_guard<std::mutex> lock(buffer_mutex);

        size = size | ((unsigned long)0x80 << 8 * (sizeof(size) - 1));
        buffer.write(reinterpret_cast<const uint8_t*>(&size), sizeof(size));

        for (auto c : s) {
          write(c);
        }
      }
    }

    template<typename T>
    void write(std::vector<T>& v) {

      auto size = v.size() * sizeof(T);

      if (size > 0x7fffffffffffffff) {

        throw serializer_error(serializer_error::arr_range_err);
      }

      if (std::is_trivially_copyable<T>::value) {

        const std::lock_guard<std::mutex> lock(buffer_mutex);

        _write(reinterpret_cast<const uint8_t*>(v.data()), size);

      } else {
        const std::lock_guard<std::mutex> lock(buffer_mutex);

        size = size | ((unsigned long)0x80 << 8 * (sizeof(size) - 1));
        buffer.write(reinterpret_cast<const uint8_t*>(&size), sizeof(size));

        for (auto u : v) {
          write(u);
        }
      }
    }

    template<typename K, typename T>
    void write(std::map<K, T>& m) {

      auto size = m.size();

      if (size > 0x7fffffffffffffff) {
        throw serializer_error(serializer_error::range_err);
      }

      const std::lock_guard<std::mutex> lock(buffer_mutex);

      size = size | ((unsigned long)0x80 << 8 * (sizeof(size) - 1));
      buffer.write(reinterpret_cast<const uint8_t*>(&size), sizeof(size));

      for (auto n : m) {

        write(reinterpret_cast<const uint8_t*>(&n.first), sizeof(K));
        write(reinterpret_cast<const uint8_t*>(&n.second), sizeof(T));
      }
    }

    template<typename T, std::size_t sz>
    void write(std::array<T, sz>& a) {

      auto size = sz * sizeof(T);

      if (size > 0x7fffffffffffffff) {

        throw serializer_error(serializer_error::arr_range_err);
      }

      if (std::is_trivially_copyable<T>::value) {

        const std::lock_guard<std::mutex> lock(buffer_mutex);

        _write(reinterpret_cast<const uint8_t*>(a.data()), size);

      } else {
        const std::lock_guard<std::mutex> lock(buffer_mutex);

        size = size | ((unsigned long)0x80 << 8 * (sizeof(size) - 1));
        buffer.write(reinterpret_cast<const uint8_t*>(&size), sizeof(size));

        for (auto e : a) {
          write(e);
        }
      }
    }

    template<typename T>
    void read(T *t) {

      std::size_t size;
      buffer.read(reinterpret_cast<uint8_t*>(&size), sizeof(size));

      if (_check_magic(size)) {

        if (size > 127) {

          throw serializer_error(serializer_error::data_err);

        } else if (size != sizeof(T)) {

          throw serializer_error(serializer_error::size_err);

        } else {

          const std::lock_guard<std::mutex> lock(buffer_mutex);

          buffer.read(reinterpret_cast<uint8_t*>(t), size);
        }
      }
    }

    template<typename T>
    void read(T t[], std::size_t n) {

      std::size_t size;
      buffer.read(reinterpret_cast<uint8_t*>(&size), sizeof(size));

      if (_check_magic(size)) {

        if (size > 0x7fffffffffffffff) {
          throw serializer_error(serializer_error::data_err);

        } else if (n * sizeof(T) < size) {
          throw serializer_error(serializer_error::size_err);

        } else {

          const std::lock_guard<std::mutex> lock(buffer_mutex);

          buffer.read(reinterpret_cast<uint8_t*>(t), size);
        }
      }
    }
    
    template<typename CharT>
    void read(std::basic_string<CharT>& s) {

      std::size_t size;
      buffer.read(reinterpret_cast<uint8_t*>(&size), sizeof(size));

      if (_check_magic(size)) {

        if (size > 0x7fffffffffffffff) {
          throw serializer_error(serializer_error::data_err);

        } else {

          if (std::is_trivially_copyable<CharT>::value) {

            const std::lock_guard<std::mutex> lock(buffer_mutex);

            CharT *str = new CharT[size / sizeof(CharT)];

            buffer.read(reinterpret_cast<uint8_t*>(str), size);
            s = str;

            delete[] str;

          } else {

            CharT t;
            for (int i = 0; i < size; i++) {

              read(&t);
              s.push_back(t);
            }
          }
        }
      }
    }

    template<typename T>
    void read(std::vector<T>& v) {

      std::size_t size;
      buffer.read(reinterpret_cast<uint8_t*>(&size), sizeof(size));

      if (_check_magic(size)) {

        if (size > 0x7fffffffffffffff) {
          throw serializer_error(serializer_error::data_err);

        } else {

          if (std::is_trivially_copyable<T>::value) {

            const std::lock_guard<std::mutex> lock(buffer_mutex);

            T *arr = new T[size / sizeof(T)];

            buffer.read(reinterpret_cast<uint8_t*>(arr), size);
            v.insert(v.begin(), arr, arr + (size / sizeof(T)));

            delete[] arr;

          } else {

            T t;
            for (int i = 0; i < size; i++) {
              
              read(&t);
              v.push_back(t);
            }
          }
        }
      }
    }

    template<typename K, typename T>
    void read(std::map<K, T>& m) {

      std::size_t size;
      buffer.read(reinterpret_cast<uint8_t*>(&size), sizeof(size));

      if (_check_magic(size)) {

        if (size > 0x7fffffffffffffff) {
          throw serializer_error(serializer_error::data_err);

        } else {

          K key;
          T value;

          for (int i = 0; i < size; i++) {

            read(reinterpret_cast<uint8_t*>(&key), sizeof(K));
            read(reinterpret_cast<uint8_t*>(&value), sizeof(T));

            m[key] = value;
          }
        }
      }
    }

    template<typename T, std::size_t sz>
    void read(std::array<T, sz>& a) {

      std::size_t size;
      buffer.read(reinterpret_cast<uint8_t*>(&size), sizeof(size));

      if (_check_magic(size)) {

        if (size > 0x7fffffffffffffff) {
          throw serializer_error(serializer_error::data_err);

        } else {

          if (std::is_trivially_copyable<T>::value) {

            const std::lock_guard<std::mutex> lock(buffer_mutex);

            T t;
            for (int i = 0; i < sz; i++) {

              buffer.read(reinterpret_cast<uint8_t*>(&t), sizeof(T));
              a[i] = t;
            }

          } else {

            T t;
            for (int i = 0; i < sz; i++) {
              
              read(&t);
              a[i] = t;
            }
          }
        }
      }
    }

    std::size_t seek() {

      std::size_t size = 0;

      if (_check_magic(size)) {

        buffer.seekg(-sizeof(size), std::ios_base::cur);
      }
      return size;
    }

    /* store serialized data to ostream */
    void store(std::ostream &out) {

      std::ios_base::iostate prev = out.exceptions();
      out.exceptions(std::ostream::badbit | std::ostream::failbit);

      out.write(reinterpret_cast<const char*>(buffer.str().c_str()), buffer.str().size());

      out.exceptions(prev);
    }

    /* retrieve serialzed data from istream */
    void retrieve(std::istream& in) {

      std::basic_string<char> data;

      try {
        std::ios_base::iostate prev = in.exceptions();
        in.exceptions(std::istream::badbit | std::istream::failbit);

        in.seekg(0, std::ios_base::end);
        data.resize(in.tellg());
        in.seekg(0, std::ios_base::beg);
        in.read(const_cast<char*>(data.c_str()), data.size());

        in.exceptions(prev);

      } catch (std::ios_base::failure &e) {

        std::streamsize idx = 0, count = 0;

        do {
          data.resize(idx + 32);
          in.read(const_cast<char*>(data.c_str() + idx), 32);

          count = in.gcount();
          idx += count;

        } while (count > 0 && count == 32);
        data.resize(idx);
      }

      buffer.write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
    }
  };
}

#endif /* __SERIALIZER_H__ */

