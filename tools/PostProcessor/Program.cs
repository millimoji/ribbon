using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Ribbon.PostProcessor
{
    class Program
    {
        static void Main(string[] args)
        {
            var postProcessor = new PostProcessor();
            postProcessor.Run();
        }
    }
}

