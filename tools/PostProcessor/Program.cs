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
            // test code
            var normalizer = new Shared.TextNormalizer();

            var src = "abc&nbsp;def&lt;ghi&#3001;jkl　 　 mnoーーーーーーpqr";

            var result = normalizer.NormalizeInput(src);

            Console.WriteLine(result);

#else
            var postProcessor = new PostProcessor();
            postProcessor.Run();
#endif
        }
    }
}

