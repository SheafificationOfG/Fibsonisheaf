import typing


def skip_leading_zeroes(srcs: typing.Iterable[str]):
    def gen():
        leading_zero = True
        for src in srcs:
            for c in src:
                if c == "0" and leading_zero:
                    continue
                leading_zero = False
                yield c

    return "".join(gen())


def group(fp: typing.TextIO, src: str, *, chunk_width: int, num_chunks: int):
    char_counter = -len(src) % chunk_width
    chunk_counter = (-len(src) // chunk_width) % num_chunks

    padding = " ".join("0" * chunk_width for _ in range(chunk_counter))
    if padding:
        padding += " "
    padding += "0" * char_counter
    fp.write(padding)

    for c in src:
        fp.write(c)
        char_counter += 1
        if char_counter == chunk_width:
            char_counter = 0
            chunk_counter += 1
            if chunk_counter == num_chunks:
                chunk_counter = 0
                fp.write("\n")
            else:
                fp.write(" ")


if __name__ == "__main__":

    import argparse
    import sys

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-i",
        "--input",
        metavar="FILE",
        help="Input source. If not provided, input is read from stdin.",
    )
    parser.add_argument(
        "-o",
        "--output",
        metavar="FILE",
        help="Output. If not provided, output is written to stdout.",
    )
    parser.add_argument(
        "--chunk",
        type=int,
        default=4,
        help="Number of characters to group in a single chunk.",
    )
    parser.add_argument(
        "--nchunks",
        type=int,
        default=8,
        help="Number of chunks to display in a single line.",
    )

    args = parser.parse_args()

    source_file = open(args.input) if args.input is not None else sys.stdin
    output_file = open(args.output, "w") if args.output is not None else sys.stdout

    source = skip_leading_zeroes(map(str.strip, source_file))
    if args.input is not None:
        source_file.close()

    group(output_file, source, chunk_width=args.chunk, num_chunks=args.nchunks)

    if args.output is not None:
        output_file.close()
