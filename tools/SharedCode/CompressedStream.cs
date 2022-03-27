using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace Ribbon.Shared
{
    class CompressedStreamWriter : IDisposable
    {
        GZipStream gZipStream;
        string filePath;
        readonly byte[] terminator = System.Text.Encoding.Unicode.GetBytes("\n");
        readonly string writingSuffix = ".writing";

        public CompressedStreamWriter(string filePath)
        {
            this.filePath = filePath;
            this.gZipStream = new GZipStream(File.Create(filePath + this.writingSuffix), CompressionLevel.Optimal, false);
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

            File.Move(this.filePath + this.writingSuffix, this.filePath);
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

    class CompressedFileSet
    {
        static readonly Regex isTxtGzSuffix = new Regex(@"\.txt\.gz$");

        static IEnumerable<string> GetSortedFile()
        {
            var files = Directory.EnumerateFiles(Constants.workingFolder).ToList();
            var matchFiles = files.Where(x => isTxtGzSuffix.IsMatch(x)).ToList();
            var sorted = matchFiles.OrderByDescending(x => File.GetLastWriteTime(x)).ToList();
            return sorted;
            /*
            return Directory.EnumerateFiles(Constants.workingFolder)
                .Where(x => isTxtGzSuffix.IsMatch(x))
                .OrderByDescending(x => File.GetLastWriteTime(Constants.workingFolder + x));
            */
        }

        public static IEnumerable<string> getNewerFiles(int fileCount)
        {
            var files = GetSortedFile();
            var taken = files.Take(fileCount).ToList();
            return taken;
            /*
            return GetSortedFile()
                .Take(fileCount)
                .Select(x => (Constants.workingFolder + x));
            */
        }
    }

}
