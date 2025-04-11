if __name__ == "__main__":
    from . import argparser, main
    from .fast_double import fibonacci

    args = argparser().parse_args()
    main(args.fname, args.n, fibonacci)
