# based on F(2n) = F(n) * (2*F(n+1) - F(n))
#          F(2n+1) = F(n+1)**2 + F(n)**2
def fibonacci(n: int) -> int:
    def fib_pair(k: int) -> tuple[int, int]:
        """returns F(k), F(k+1)"""
        if k <= 1:
            return (k, 1)
        fh, fh1 = fib_pair(k >> 1)
        fk = fh * ((fh1 << 1) - fh)
        fk1 = fh1**2 + fh**2
        if k & 1:
            fk, fk1 = fk1, fk + fk1
        return fk, fk1

    return fib_pair(n)[0]


if __name__ == "__main__":
    from . import argparser, main

    args = argparser().parse_args()
    main(args.fname, args.n, fibonacci)
