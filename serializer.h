#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <exception>
#include <mutex>

namespace yas {

  class serializer {

    std::basic_stringstream<uint8_t> buffer;
    std::mutex buffer_mutex;

  public:

    class serializer_error : public std::exception {

    public:
      enum err_type {
        range_err = 1,
        range_err_arr,
        data_err,
        size_err,
        non_trivial_err
      };

      explicit serializer_error(const err_type& e) {
        error = e;
      }

      const char* what() const noexcept {

        if (error == range_err) {
          return "size of type exceeds 127";

        } else if (error == range_err_arr) {
          return "storage size of array exceeds 9223372036854775807";

        } else if (error == data_err) {
          return "corrupt data in buffer";

        } else if (error == size_err) {
          return "size mismatch";

        } else if (error == non_trivial_err) {
          return "writing non-trivial object";
          
        } else {
          return "unhandled exception";
        }
      }

    private:
      err_type error;

    };

    /* implement functions to serialize/deserialize class members individually */
    virtual void serialize() { }
    virtual void deserialize() { }

    template<typename T>
    void write(T t) {

      if (std::is_trivially_copyable<T>::value) {

        std::size_t size = sizeof(T);
        if (size > 127) {
          throw serializer_error(serializer_error::range_err);
        }
        const std::lock_guard<std::mutex> lock(buffer_mutex);

        buffer.write(reinterpret_cast<const uint8_t*>(&size), sizeof(size));
        buffer.write(reinterpret_cast<const uint8_t*>(&t), size);

      } else {
        throw serializer_error(serializer_error::non_trivial_err);
      }
    }

    template<typename T>
    void write(T *t, std::size_t n) {

      if (std::is_trivially_copyable<T>::value) {

        std::size_t size = sizeof(T) * n;
        if (size > 0x7fffffffffffffff) {
          throw serializer_error(serializer_error::range_err_arr);
        }

        const std::lock_guard<std::mutex> lock(buffer_mutex);

        /* set MSB */
        size = size | ((unsigned long)0x80 << 8 * (sizeof(size) - 1));
        buffer.write(reinterpret_cast<const uint8_t*>(&size), sizeof(size));

        size = size & ~((unsigned long)0x80 << 8 * (sizeof(size) - 1));
        buffer.write(reinterpret_cast<const uint8_t*>(t), size);

      } else {
        throw serializer_error(serializer_error::non_trivial_err);
      }
    }

    template<typename T>
    void read(T *t) {

      std::size_t size;
      buffer.read(reinterpret_cast<uint8_t*>(&size), sizeof(size));

      if (size > 127) {
        throw serializer_error(serializer_error::data_err);

      } else if (size != sizeof(T)) {
        throw serializer_error(serializer_error::size_err);

      } else {

        const std::lock_guard<std::mutex> lock(buffer_mutex);

        buffer.read(reinterpret_cast<uint8_t*>(t), size);
      }
    }

    template<typename T>
    void read(T t[], std::size_t n) {

      std::size_t size;
      buffer.read(reinterpret_cast<uint8_t*>(&size), sizeof(size));

      if (size & ((unsigned long)0x80 << 8 * (sizeof(size) - 1))) {

        size = size & ~((unsigned long)0x80 << 8 * (sizeof(size) - 1));

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
    
    std::size_t seek() {

      std::size_t size;
      buffer.read(reinterpret_cast<uint8_t*>(&size), sizeof(size));

      if (size & ((unsigned long)0x80 << 8 * (sizeof(size) - 1))) {
        size = size & ~((unsigned long)0x80 << 8 * (sizeof(size) - 1));
      }

      buffer.seekg(-sizeof(size), std::ios_base::cur);

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

