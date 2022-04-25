Yet another serializer
======================

Serialize data types, arrays, POD and classes and store or retrieve to and from
ostream and istream objects respectively.

Caveats
- Don't serialize pointers because memory locations can change.
- Disobeys strict aliasing rule with workaround that is using uint8_t* for
  aliasing data.
