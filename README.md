Yet another serializer
======================

Serialize data types, arrays, POD and classes and store or retrieve to and from
ostream and istream objects respectively.

Caveats
- User should not serialize pointers because memory locations can change.
- Disobeys strict aliasing rule. Read from and write to internal buffer requires
  casting the address of data to uint8_t*. This is acceptable as unsigned char*
  aliases other types.
