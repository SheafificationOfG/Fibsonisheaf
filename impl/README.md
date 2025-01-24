# Implementations

All `.c` files here implement backend for `fib_base.h` in the root directory, providing a definition for the main function

```c
struct number fibonacci(uint64_t);
```

Implementations of this function have the following expectations for the returned number `num`.
1. `num.bytes` points to heap-allocated memory, storing the `index`th Fibonacci number in *little-endian*; in particular, `*(uint8_t *)num.bytes` should be the least significant byte of the `index`th Fibonacci number.
1. `num.length` indicates the number of bytes in the block allocated in `num.bytes` dedicated to storing the `index`th Fibonacci number. Leading zeroes are permissible.
1. The responsibility is given to the caller to free the memory allocated in `num.bytes`.

## Debugging

Implementations may always be compared against the (relatively efficient) Python implementation in `scripts/fibonappy.py`, whose behaviour roughly matches that of `hex.c` when compiled.

Since the dumped hex is printed as plaintext in a single line, it may be helpful (for `diff`s, for instance) to pipe the output through `scripts/group_hex.py` to produce output that roughly imitates a `hexdump`:

```bash
./bin/$(algo).hex.out        $N | python3 scripts/group_hex.py -o $output_file
python3 -m scripts.fibonappy $N | python3 scripts/group_hex.py -o $python_file
```

> You can also run `scripts.fibonappy.$impl` to run a particular Fibonacci implementation in Python.

### Printing debug statements

`fib_base.h` provides a small suite of debugging functions:
```c
//// all debug info is dumped to stderr ////

/* print string (no newline) */
void debug(char const *str);
/* print hex repr of memory slice, with leading 0x */
void debugmem(void *addr, size_t nbytes);
/* pretty debug message */
void log(char const *format, /* fmt args */ ...);
/* pretty memdump */
void logmem(void *addr, size_t nbytes);
```
To enable this macro, build with `DEFINES="DEBUG"`.

```bash
make clean
make bin/$(algo).out OPTLEVEL="-Og" FLAGS="-g" DEFINES="DEBUG"
```

> Accordingly, other debug testing or functionality should be wrapped in an `#ifdef DEBUG`.
