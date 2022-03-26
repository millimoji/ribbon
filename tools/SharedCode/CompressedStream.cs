using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text;

namespace Ribbon.Shared
{
    class CompressedStreamWriter : IDisposable
    {
        GZipStream gZipStream;
        readonly byte[] terminator = System.Text.Encoding.Unicode.GetBytes("\n");

        public CompressedStreamWriter(string filePath)
        {
            this.gZipStream = new GZipStream(File.Create(filePath), CompressionLevel.Optimal, false);
        }

        ~CompressedStreamWriter()
        {
            this.Dispose();
        }

        public virtual void Dispose()
        {
            if (this.gZipStream != null)
            {
                this.gZipStream.Flush();
                this.gZipStream.Close();
                this.gZipStream.Dispose();
                this.gZipStream = null;
            }
            GC.SuppressFinalize(this);
        }

        public void WriteLine(string text)
        {
            if (text.Length > 0)
            {
                var data = System.Text.Encoding.Unicode.GetBytes(text);
                this.gZipStream.Write(data, 0, data.Length);
                this.gZipStream.Write(this.terminator, 0, this.terminator.Length);
            }
        }
    }

    class CompressedStreamReader : IDisposable
    {
        GZipStream gZipStream;

        public CompressedStreamReader(string filePath)
        {
            this.gZipStream = new GZipStream(File.OpenRead(filePath), CompressionMode.Decompress);
        }
        ~CompressedStreamReader()
        {
            this.Dispose();
        }

        public virtual void Dispose()
        {
            if (this.gZipStream != null)
            {
                this.gZipStream.Close();
                this.gZipStream.Dispose();
                this.gZipStream = null;
            }
            GC.SuppressFinalize(this);
        }
        public string ReadLine()
        {
            var byteList = new List<int>();
            for (; ; )
            {
                int byte1st = this.gZipStream.ReadByte();
                if (byte1st < 0)
                {
                    break;
                }
                int byte2nd = this.gZipStream.ReadByte();
                if (byte1st < 0)
                {
                    break;
                }
                if (byte1st == '\n' && byte2nd == 0)
                {
                    break;
                }
                byteList.Add(byte1st);
                byteList.Add(byte2nd);
            }
            var byteArray = byteList.Select(x => (byte)x).ToArray();
            return System.Text.Encoding.Unicode.GetString(byteArray);
        }
    }

}
