def fibonacci(n: int) -> int:
    a = 0
    b = 1
    for _ in range(n):
        a, b = b, a + b
    return a


if __name__ == "__main__":
    from . import argparser, main

    args = argparser().parse_args()
    main(args.fname, args.n, fibonacci)
