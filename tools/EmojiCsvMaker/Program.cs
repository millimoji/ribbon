using System;
using System.IO;
using System.Text.RegularExpressions;

namespace EmojiCsvMaker
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Hello World!");

            var emojiConv = new EmojiCsvMaker(
                "..\\..\\..\\..\\emoji\\",
                "..\\..\\..\\..\\script\\");

            emojiConv.DoConvert(
                new string[] { "emoji-sequences.txt", "emoji-zwj-sequences.txt" },
                "all-emoji.csv");
        }
    }

    class EmojiCsvMaker
    {
        static string emojiPos = ",5,5,8196,記号,絵文字,*,*,*,*,*,";
        static string emojiPosUnq = ",5,5,9196,記号,絵文字,*,*,*,*,*,";
        static string emojiReading = ",エモジ,エモジ";

        string sourceDir;
        string outputDir;

        Regex isCommentLine = new Regex(@"^\s*#");
        Regex isHexString = new Regex(@"^[0-9A-Fa-z]+$");
        Regex isVariationSelector = new Regex(@"^[\uFE00-\uFE0F]$");

        public EmojiCsvMaker(string sourceDir, string outputDir)
        {
            this.sourceDir = sourceDir;
            this.outputDir = outputDir;
        }

        public void DoConvert(string[] inputFilenames, string outputFilename)
        {
            using (StreamWriter outputStream = new StreamWriter(this.outputDir + outputFilename))
            {
                foreach (var fileName in inputFilenames)
                {
                    this.Convert(fileName, outputStream);
                }
            }
        }

        public void Convert(string fileName, StreamWriter outputStream)
        {
            using (StreamReader fileStream = new StreamReader(this.sourceDir + fileName))
            {
                string line;
                while ((line = fileStream.ReadLine()) != null)
                {
                    if (line.Length == 0)
                    {
                        continue;
                    }
                    if (this.isCommentLine.IsMatch(line))
                    {
                        continue;
                    }
                    var fields = line.Split(';');
                    if (fields.Length == 0)
                    {
                        continue;
                    }
                    var codeList = fields[0].Split(' ', StringSplitOptions.RemoveEmptyEntries);
                    if (codeList.Length == 0)
                    {
                        continue;
                    }
                    Tuple<int, int> topCodeRange;
                    if (codeList[0].Contains(".."))
                    {
                        var codeRange = codeList[0].Split('.', StringSplitOptions.RemoveEmptyEntries);
                        if (codeRange.Length != 2 || !this.isHexString.IsMatch(codeRange[0]) || !this.isHexString.IsMatch(codeRange[1]))
                        {
                            continue;
                        }
                        topCodeRange = new Tuple<int, int>(
                            int.Parse(codeRange[0], System.Globalization.NumberStyles.HexNumber),
                            int.Parse(codeRange[1], System.Globalization.NumberStyles.HexNumber));
                    }
                    else
                    {
                        if (!this.isHexString.IsMatch(codeList[0]))
                        {
                            continue;
                        }
                        var topCharCode = int.Parse(codeList[0], System.Globalization.NumberStyles.HexNumber);
                        topCodeRange = new Tuple<int, int>(topCharCode, topCharCode);
                    }
                    for (int i = topCodeRange.Item1; i <= topCodeRange.Item2; ++i)
                    {
                        var resultText = char.ConvertFromUtf32(i);

                        for (int j = 1; j < codeList.Length; j++)
                        {
                            var utf32Code = int.Parse(codeList[j], System.Globalization.NumberStyles.HexNumber);
                            resultText += char.ConvertFromUtf32(utf32Code);
                        }

                        var outputLine = resultText + emojiPos + resultText + emojiReading;

                        outputStream.WriteLine(outputLine);

                        if (codeList.Length == 2)
                        {
                            var secondCharCode = int.Parse(codeList[1], System.Globalization.NumberStyles.HexNumber);
                            var secondCharText = char.ConvertFromUtf32(secondCharCode);

                            if (this.isVariationSelector.IsMatch(secondCharText))
                            {
                                var firstChar = resultText = char.ConvertFromUtf32(i);
                                var noVsOutput = firstChar + emojiPosUnq + firstChar + emojiReading;
                                outputStream.WriteLine(noVsOutput);

                                // old variation
                                noVsOutput = firstChar + "\uFE0E" + emojiPosUnq + firstChar + "\uFE0E" + emojiReading;
                                outputStream.WriteLine(noVsOutput);
                            }

                        }
                    }
                }
            }
        }
    }
}
