﻿using System;
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
            var result = nc.TryParseNumber("６万１０００１０００");
            Console.WriteLine($"{result.Item2}");

            result = nc.TryParseNumber("１５０００億");
            Console.WriteLine($"{result.Item2}");
#else
            var postProcessor = new PostProcessor();
            postProcessor.Run();
#endif
        }
    }
}

