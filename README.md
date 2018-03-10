# memory
Various C++ memory helpers for embedded environments

# ipc-ish helpers

For moving bulk data from one place to another

|                     | No-copy   | Copy
| ------------------- |:---------:| ---------
| **System managed**  | netbuf    | stream
| **Caller managed**  | pipeline  | stream