def fibonacci(n: int) -> int:
    if n <= 1:
        return n

    # (a, b) ~~ a + b\sqrt{5}
    sa, sb = (1, 1)
    a, b = (1, 1)

    n -= 1
    while n:
        if n & 1:
            a, b = (
                (a * sa + 5 * b * sb) >> 1,
                (a * sb + b * sa) >> 1,
            )
        sa, sb = (
            (sa**2 + 5 * sb**2) >> 1,
            sa * sb,
        )
        n >>= 1
    return b


if __name__ == "__main__":
    from . import argparser, main

    args = argparser().parse_args()
    main(args.fname, args.n, fibonacci)
