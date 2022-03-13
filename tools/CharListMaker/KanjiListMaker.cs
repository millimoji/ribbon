using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Ribbon.CharListMaker
{
    internal class KanjiListMaker
    {
        string srcDir;
        string outDir;

        public KanjiListMaker(string srcDir, string outDir)
        {
            this.srcDir = srcDir;
            this.outDir = outDir;
        }

        public void DoConvert(string srcFile, string outFile)
        {
            using (StreamWriter outputStream = new StreamWriter(this.outDir + outFile))
            {
                this.Convert(srcFile, outputStream);
            }
        }

        Regex isComment = new Regex(@"^\w*;");
        Regex isNumber = new Regex(@"^[0-9]+$");
        Regex isKatakana = new Regex(@"^[ァ-ヶー]+$");
        Regex isHiragana = new Regex(@"^[ぁ-ゖー]+$");
        string dictFields = ",5,5,8196,記号,漢字,*,*,*,*,*,";

        public void Convert(string fileName, StreamWriter outputStream)
        {
            using (StreamReader fileStream = new StreamReader(this.srcDir + fileName))
            {
                string line;
                while ((line = fileStream.ReadLine()) != null)
                {
                    if (line.Length == 0)
                    {
                        continue;
                    }
                    if (this.isComment.IsMatch(line))
                    {
                        continue;
                    }

                    var fields = line.Split('\t');
                    if (fields.Length == 0)
                    {
                        continue;
                    }
                    if (this.isNumber.IsMatch(fields[0]))
                    {
                        // normal kanji has line number at 1st field.
                        // deleted kanji does not have line number.
                        fields = fields.Skip(1).ToArray();
                    }
                    var kanji = fields[0].Split(' ')[0];

                    var readings = fields[7].Split('、');
                    foreach (var reading in readings)
                    {
                        var outputReading = "";
                        if (this.isKatakana.IsMatch(reading))
                        {
                            outputReading = reading;
                        }
                        else if (this.isHiragana.IsMatch(reading))
                        {
                            outputReading = Shared.WinApiBridge.HiraToKata(reading);
                        }
                        else
                        {
                            continue; // otherwise ignore. mainly, kun-yomi + followings
                        }

                        var outputLine = $"{kanji}{this.dictFields}{kanji},{outputReading},{outputReading}";

                        outputStream.WriteLine(outputLine);
                    }
                }
            }
        }
    }
}
