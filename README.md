# Fibsonisheaf

Sheafification of [Fibsonicci](https://github.com/SheafificationOfG/Fibsonicci), a response to the responses.

The goal of all of these algorithms is to (hopefully efficiently) compute $`F_n`$, defined by

```math
F_n = \begin{cases}
    F_{n-1} + F_{n-2}, & n \geq2 \\
    1, & n = 1 \\
    0, & n = 0
\end{cases}
```

## Videos

#### Python laid waste to my C++!
[![Python laid waste to my C++!](https://img.youtube.com/vi/02rh1NjJLI4/0.jpg)](https://youtu.be/02rh1NjJLI4)
#### I'm not sorry for switching to C
[![I'm not sorry for switching to C](https://img.youtube.com/vi/LXm6ygZ3h7A/0.jpg)](https://youtu.be/LXm6ygZ3h7A)
#### What does BLAZINGLY FAST even mean??
[![What does BLAZINGLY FAST even mean??](https://img.youtube.com/vi/u24AjKinHNU/0.jpg)](https://youtu.be/u24AjKinHNU)

## Usage

After cloning the repo, make sure to run

```bash
make init
```

to initialise the output folders.
This step is necessary before trying to build anything else.

### Computing one Fibonacci number

To compute a particular Fibonacci number (using one of the [supported implementations](#algorithms)), use

```bash
# Generate binary
make bin/$(algo).hex.out

# Usage:
./bin/$(algo).hex.out $(fibonacci_index) $(output_file)
# If output_file is not provided, output is directed to stdout
```

Due to laziness, I have not implemented any intelligent conversion from hexadecimal to decimal.
If you don't jive with hex, you can convert the hex output with `scripts/hex2dec.py`.

```bash
./bin/$(algo).hex.out $(fibonacci_index) | python3 scripts/hex2dec.py $(output_file)
# If output_file is not provided, output is directed to stdout
```

By default, `hex2dec` prints out at most 32 significant digits.
To print *all* digits, pass `-n0` or `--ndigits=0` as an argument.

### Configuring builds

The current algorithms have defaults that make the most sense on my hardware, but you can override some settings from `make`.

```bash
# CC: C compiler
# PY: Python interpreter
# DEFINES: C macro definitions
#   (e.g., DIGIT_BIT: number of bits in a single "digit"
#    of the large integer representation)
# FLAGS: C compiler flags
# AUTOHEADER_FLAGS: Flags passed to the header autogenerator
make CC=gcc-14 PY=python3.14 DEFINES="DEBUG DIGIT_BIT=$(bits)" FLAGS="-g" AUTOHEADER_FLAGS="--verbose" bin/$(whatever).out
```

There are, of course, other flags.

### Plotting performance

> [!WARNING]
> There is nothing scientific about how these algorithms are benchmarked.
> I promise no consistency between what you get, and what I present in videos.

To construct data for plotting, run

```bash
# To generate data for a specific algorithm
make data/$(algo).dat
# Or, to generate all data
make all-data
```

To plot the data, a prerequisite is a configuration JSON file, having the following form.

```json
{
    "Name of algorithm (for legend)": {
        "path": "data/$(algo).dat",
        "colour": "$(some_matplotlib_colour)",
    },
}
```

> [!NOTE]
> Paths to `*.dat` files are relative to the config JSON you define.

> [!TIP]
> You can also plot against data files generated from the [OG Fibsonicci project](https://github.com/SheafificationOfG/Fibsonicci).
> The `anim` script is able to parse either format.

You should then be able to run

```bash
python3 scripts/anim.py $(config).json
```

If you provide `--output=foobar`, then the plot animation will be saved to `foobar.mp4`, and the final plot to `foobar.png`.

> [!IMPORTANT]
> The `anim` script requires `matplotlib`, and `ffmpeg` (for saving `.mp4`s).

### Issues

- The default compiler is `clang-18`, because I like the taste of iron (and for other reasons), but this may not work on ARM64; see issue #1.
  The build should be fine with `gcc`, though.

# Algorithms

The project includes the following implementations.

| Algorithm | Source (`impl/`) | Runtime | Video |
|:---------:|:----------------:|:-------:|:-----:|
| [Naive](#naive) | `naive.c` | $`\Omega(\exp(n))`$ | - |
| ["Linear"](#linear) | `linear.c` | $`O(n^2)`$ | [[1]](#python-laid-waste-to-my-c) |
| [Fast exponentiation](#fast-exponentiation) | `fastexp{,2d}.c` | $`O(n^2)`$ | [[2]](#im-not-sorry-for-switching-to-c) |
| [Fast squaring](#fast-squaring) | `fastsquaring.c` | $`O(n^2)`$ | [[2]](#im-not-sorry-for-switching-to-c) |
| [NTT](#number-theoretic-transform) | `ntt{,t}.c` | $`O(n\log n)`$ [^1] | [[3]](#what-does-blazingly-fast-even-mean) |

## Naive

Always need to have the nonrecursive "by definition" implementation present, for completeness:

```py
def naive(n):
    if n <= 1:
        return n
    return fib(n-1) + fib(n-2)
```

## "Linear"

The bottom-up iterative implementation, which involves $`O(n)`$ iterations to compute $`F_n`$.

```py
def linear(n):
    a, b = 1, 0
    for _ in range(n):
        a, b = a+b, a
    return b
```

> [!TIP]
> Although the algorithm is called "linear" (referring to the number of iterations), the algorithm is $\Theta(n^2)$ overall.

## Fast exponentiation

Based on the following identity:

```math
\begin{bmatrix}
    0 & 1 \\ 1 & 1
\end{bmatrix}^n =
\begin{bmatrix}
    F_{n-1} & F_n \\ F_n & F_{n+1}
\end{bmatrix}
```
we compute $`F_n`$ using the $`O(\log n)`$ fast exponentiation algorithm.

Since all matrices involved are symmetric, we can represent these matrices as triples, with the following "multiplication":

```math
\begin{bmatrix}
    a \\ b \\ c
\end{bmatrix}
\boxtimes
\begin{bmatrix}
    a' \\ b' \\ c'
\end{bmatrix}
:=
\begin{bmatrix}
    aa' + bb' \\
    ab' + bc' \\
    bb' + cc'
\end{bmatrix}
```

The multiplication of integers is achieved with the simple $`O(n^2)`$ grade-school algorithm.
However, we add some simple modifications to take advantage of the nature of our products.

This "multiplication" is performed in three fused steps, as suggested by the expansion:

```math
\begin{bmatrix}
    a \\ b \\ c
\end{bmatrix}
\boxtimes
\begin{bmatrix}
    a' \\ b' \\ c'
\end{bmatrix}
=
a
\begin{bmatrix}
    a' \\ b' \\ ~
\end{bmatrix}
+
bb'
\begin{bmatrix}
    1 \\ ~ \\ 1
\end{bmatrix}
+
c'
\begin{bmatrix}
    ~ \\ b \\ c
\end{bmatrix}
```

### Reducing the dimension

In the [fast exponentiation](#fast-exponentiation) algorithm, the $`2\times2`$ matrices were encoded as triples to reduce redundancy, but notice that the relevant triples $`\langle a, b, c\rangle`$ still carry a bit of redundancy: we always have $`c = a + b`$.
Therefore, we can reduce our memory footprint further by working instead with the pairs $`\langle F_{n-1}, F_n\rangle`$.

Now, the "multiplication" becomes

```math
\begin{bmatrix}
    a \\ b
\end{bmatrix}
\boxtimes
\begin{bmatrix}
    a' \\ b'
\end{bmatrix}
:=
\begin{bmatrix}
    aa' + bb' \\
    ab' + ba' + bb'
\end{bmatrix}
```

which we perform in three fused steps as well:

```math
\begin{bmatrix}
    a \\ b
\end{bmatrix}
\boxtimes
\begin{bmatrix}
    a' \\ b'
\end{bmatrix}
=
a
\begin{bmatrix}
    a' \\ b'
\end{bmatrix}
+
bb'
\begin{bmatrix}
    1 \\ 1
\end{bmatrix}
+
ab'
\begin{bmatrix}
    ~ \\ 1
\end{bmatrix}
```

## Fast squaring

This is a "bottom-up" variant of fast exponentiation, which processes the bits of the index top-down.
As a result, the transitions for set bits are much simpler (following only a single transition step of the Fibonacci sequence), and the main computation is iterative squaring:

```math
\begin{bmatrix}
    a \\ b
\end{bmatrix}
^{\boxtimes2}
=
\begin{bmatrix}
    a^2 + b^2 \\ 2ab + b^2
\end{bmatrix}
```

We perform this squaring operation in two fused steps:

```math
\begin{bmatrix}
    a \\ b
\end{bmatrix}
^{\boxtimes2}
=
b^2
\begin{bmatrix}
    1 \\ 1
\end{bmatrix}
+
a
\begin{bmatrix}
    a \\ 2b
\end{bmatrix}
```

## Number-theoretic transform

I've embarrassed myself enough with Karatsuba's multiplication algorithm before, so let's just fast-fourierward to an $`O(n\log n)`$ multiplication algorithm[^1].

[^1]: It's not actually $`O(n\log n)`$, unless you like incorrect algorithms as $`n\to\infty`$.

The idea is pretty convoluted :smirk:.
We will represent our large integers in base $`B = 2^b`$ (by default, $`B = 2^{32}`$).

Suppose we are multiplying two large integers, written out as $`N`$ -digit integers:

```math
\begin{align*}
    x &= \sum_{0\leq i < N} x_iB^i \\
    y &= \sum_{0\leq j < N} y_jB^j
\end{align*}
```

Then, observe that their product $`z := x\cdot y`$ is a $`\leq2N`$ -digit integer whose "improper digits" are the result of convolving the digits of $`x`$ with the digits of $`y`$:

```math
z_m = \left(\sum_{i+j=m}x_iy_j\right)B^m
```
Note that these "digits" don't necessarily fit in the interval $`[0, B-1]`$.

Through a Fourier transform, we can convert these convolutions into digit-wise products in the frequency domain.
With $`\omega_{2N}`$ a primitive $`2N`$ -th root of unity, let

```math
\hat x_k := \sum_{0\leq i<2N}x_i\omega_{2N}^{-ki}
```
and similarly for $`\hat y_k`$ and $`\hat z_k`$.
Then, $`\hat z_k = \hat x_k\cdot\hat y_k`$, and we can recover $`z_m`$ with an inverse Fourier transform:

```math
2Nz_m = \sum_{0\leq k<2N}\hat z_k\omega_{2N}^{+km}
```

Normally, the idea is to embed the digits into the complex numbers, and then take $`\omega_{2N} := \exp(-\frac{2\pi i}{2N})`$, but I'm not in the mood to roll out my own arbitrary-precision floats.
Therefore, we work modulo a fixed prime $`p`$ such that $`2N`$ divides $`(p-1)`$.

<!-- objdump -Mintel -d --visualize-jumps --no-show-raw-insn --no-addresses bin.out -->
<!-- `x86asm` gives syntax highlighting in GitHub md (but requires Intel notation) -->
