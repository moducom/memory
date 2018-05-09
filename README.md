# memory
Various C++ memory helpers for embedded environments

# ipc-ish helpers

For moving bulk data from one place to another

|                     | No-copy   | Copy
| ------------------- |:---------:| ---------
| **System managed**  | netbuf    | stream
| **Caller managed**  | pipeline  | stream

# adapted from old util.embedded:

* layer 1: barest-metal, tends to be templates; buffer pointers avoided, expect inline buffers
* layer 2: low-level. buffer pointer + template size field or inline buffer + variable size field
* layer 3: low-level. buffer pointers and size fields used
* layer 4: mid-level. buffers inline (pointers avoided). virtual functions allowed
  * layer 4 is currently experimental and subject to change definition.
* layer 5: mid-level. buffer pointers used. virtual functions allowed

## library.json

Assists platform.io tooling