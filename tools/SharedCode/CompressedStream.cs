using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Text;

namespace Ribbon.Shared
{
    class CompressedFiles
    {

    }

    class CompressedStreamWriter
    {
        GZipStream gZipStream;
        

        public void Open(string filePath)
        {
            this.gZipStream = new GZipStream(File.Create(filePath), CompressionLevel.Optimal, false);
        }
        public void Close()
        {
            this.gZipStream.Flush();
            this.gZipStream.Close();
            this.gZipStream.Dispose();
        }
        public void WriteLine(string text)
        {

        }
    }

    class CompressedStreamReader
    {
        public bool Open(string filePath)
        {

        }
        public void Close()
        {

        }
        public void WriteLine(string text)
        {

        }
    }


}
