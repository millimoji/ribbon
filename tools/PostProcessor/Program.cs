using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Ribbon.PostProcessor
{
    class Program
    {
        static void Main(string[] args)
        {
#if false
            var nc = new Shared.NumberConverter();
            var result = nc.TryParseNumber("１２万１０千");
            Console.WriteLine($"{result.Item2}");

            result = nc.TryParseNumber("壱万弐千参百四拾五");
            Console.WriteLine($"{result.Item2}");

            result = nc.TryParseNumber("二〇二一");
            Console.WriteLine($"{result.Item2}");

            result = nc.TryParseNumber("１万２万");
            Console.WriteLine($"{result.Item2}");

            result = nc.TryParseNumber("１２３百");
            Console.WriteLine($"{result.Item2}");

            result = nc.TryParseNumber("１２３万");
            Console.WriteLine($"{result.Item2}");

            result = nc.TryParseNumber("１２３億");
            Console.WriteLine($"{result.Item2}");
#else
            var postProcessor = new PostProcessor();
            postProcessor.Run();
#endif
        }
    }
}

