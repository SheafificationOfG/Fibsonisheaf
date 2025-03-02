//
//  42 million, C#, 100 lines, out of the box libraries...
//
//  This algorithm uses the doubling algorithm with Karatsuba multiplication, precomputation, caching, threading and recursion
//

using System.Numerics;

namespace Fibonacci
{
    partial class Program
    {
        const int CUTOFF = 1 << 20;

        static BigInteger[] small = [0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181, 6765, 10946, 17711, 28657, 46368, 75025, 121393, 196418, 317811, 514229, 832040, 1346269, 2178309, 3524578, 5702887, 9227465, 14930352, 24157817, 39088169, 63245986, 102334155, 165580141, 267914296, 433494437, 701408733, 1134903170, 1836311903, 2971215073, 4807526976, 7778742049, 12586269025, 20365011074, 32951280099, 53316291173, 86267571272, 139583862445, 225851433717, 365435296162, 591286729879, 956722026041, 1548008755920, 2504730781961, 4052739537881, 6557470319842, 10610209857723, 17167680177565, 27777890035288, 44945570212853, 72723460248141, 117669030460994, 190392490709135, 308061521170129, 498454011879264, 806515533049393, 1304969544928657, 2111485077978050, 3416454622906707, 5527939700884757, 8944394323791464, 14472334024676221, 23416728348467685, 37889062373143906, 61305790721611591, 99194853094755497, 160500643816367088, 259695496911122585, 420196140727489673, 679891637638612258, 1100087778366101931, 1779979416004714189, 2880067194370816120, 4660046610375530309, 7540113804746346429, 12200160415121876738];

        static Dictionary<ulong, BigInteger> cache = new();

        static BigInteger Karatsuba(BigInteger x, BigInteger y)
        {
            int n = Math.Max(x.ToByteArray().Length * 8, y.ToByteArray().Length * 8);

            if (n <= CUTOFF) return x * y;

            n = (n + 1) >> 1;

            BigInteger b = x >> n;
            BigInteger a = x - (b << n);
            BigInteger d = y >> n;
            BigInteger c = y - (d << n);

            BigInteger ac = 0, bd = 0, abcd = 0;

            Task[] task =
            [
                Task.Run(() => { ac = Karatsuba(a, c); }),
                Task.Run(() => { bd = Karatsuba(b, d); }),
                Task.Run(() => { abcd = Karatsuba(a + b, c + d); }),
            ];
            Task.WaitAll(task);

            return ac + (abcd << (n + 1)) + (bd << (n << 1));
        }

        static BigInteger KaratsubaSquare(BigInteger x)
        {
            int n = x.ToByteArray().Length * 8;

            if (n <= CUTOFF) return x * x;

            n = (n + 1) >> 1;

            BigInteger b = x >> n;
            BigInteger a = x - (b << n);

            BigInteger ac = 0, bd = 0, abcd = 0;

            Task[] task =
            [
                Task.Run(() => { ac = KaratsubaSquare(a); }),
                Task.Run(() => { bd = KaratsubaSquare(b); }),
                Task.Run(() => { abcd = KaratsubaSquare(a + b); }),

            ];
            Task.WaitAll(task);

            return ac + (abcd << (n + 1)) + (bd << (n << 1));
        }

        static BigInteger Fibonacci(ulong n)
        {
            if (n < (ulong)small.Length)
            {
                return small[n];
            }
            else
            {
                if (cache.ContainsKey(n)) return cache[n];

                ulong k = n / 2;
                BigInteger Fk = Fibonacci(k), Fkp1 = Fibonacci(k + 1), result;

                if (n % 2 == 0)
                {
                    result = Karatsuba(Fk, 2 * Fkp1 - Fk);
                }
                else
                {
                    result = KaratsubaSquare(Fkp1) + KaratsubaSquare(Fk);
                }

                cache[n] = result;

                return result;
            }
        }
    }
}
