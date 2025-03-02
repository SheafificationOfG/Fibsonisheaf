//
//  15 million per sec, C#, 40 lines, out of the box libraries
//
//  This algorithm uses the doubling algorithm with precomputation, caching, threading and recursion
//
//  This would be a good place to _start_ optimisation
//

using System.Numerics;

namespace Fibonacci
{
    partial class Program
    {
        static BigInteger Fibonacci(ulong n)
        {
            if (n < (ulong)small.Length)
            {
                return small[n];
            }
            else
            {
                lock (cache)
                {
                    if (cache.ContainsKey(n)) return cache[n];
                }

                ulong k = n / 2;
                BigInteger Fk = 0, Fkp1 = 0, result;

                Task[] task =
                [
                    Task.Run(() => { Fk = Fibonacci(k); }),
                    Task.Run(() => { Fkp1 = Fibonacci(k + 1); }),
                ];
                Task.WaitAll(task);

                if (n % 2 == 0)
                {
                    result = Fk * (2 * Fkp1 - Fk);
                }
                else
                {
                    result = Fkp1 * Fkp1 + Fk * Fk;
                }
                
                lock(cache)
                {
                    cache[n] = result;
                }

                return result;
            }
        }
    }
}
