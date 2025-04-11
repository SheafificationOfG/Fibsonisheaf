import typing

# field_ext implementation in Python


def main(fname: typing.Optional[str], n: int, fibonacci: typing.Callable[[int], int]):
    import sys
    import time

    start_time = time.time()
    fib = fibonacci(n)
    end_time = time.time()

    if fname is None:
        fp = sys.stdout
    else:
        fp = open(fname, "w")

    fibhex = f"{fib:x}"
    print(f"# Runtime: {end_time-start_time:.9f}s", file=sys.stderr)
    print(f"# Size:    {(len(fibhex)+1)>>1} B", file=sys.stderr)
    fp.write(fibhex)

    if fname is None:
        fp.write("\n")
    else:
        fp.close()


def argparser():
    import argparse

    parser = argparse.ArgumentParser()

    parser.add_argument("n", metavar="INDEX", type=int, help="desired Fibonacci index")
    parser.add_argument(
        "fname",
        metavar="FILE",
        type=str,
        nargs="?",
        help="output file to store result (or stdout if not provided)",
    )

    return parser
