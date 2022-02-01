using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;


namespace Ribbon.Shared
{
    class NGramStore
    {
        Dictionary<string, int> WordHash = new Dictionary<string, int>(StringComparer.Ordinal);
        List<string> WordList = new List<string>();
        Object thisLock = new Object();
        Dictionary<Tuple<int, int, int, int, int, int, int>, long>[] m_nGrams = new Dictionary<Tuple<int, int, int, int, int, int, int>, long>[7]
        {
            new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
            new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
            new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
            new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
            new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
            new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
            new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
        };

        const int thresholdToDiv2 = 4000000; // n7gram max
        const string BOS = "[BOS]";
        const string EOS = "[EOS]";
        const string unigramFileName = "unigram.txt";
        const string bigramFileName = "bigram.txt";
        const string trigramFileName = "trigram.txt";
        const string n4gramFileName = "n4gram.txt";
        const string n5gramFileName = "n5gram.txt";
        const string n6gramFileName = "n6gram.txt";
        const string n7gramFileName = "n7gram.txt";
        readonly string[] fileNames = new string[] { unigramFileName, bigramFileName, trigramFileName, n4gramFileName, n5gramFileName, n6gramFileName, n7gramFileName,
            Constants.topicModelFileName, Constants.topicModelSummaryFilename, Constants.mixUnigramlFileName, Constants.mixUnigramSummaryFilename };
        const string doHalfFileName = "dohalf";

        TopicModelHandler m_topicModel;
        TopicModelHandler m_mixUnigram;


        string m_workDir;
        public string DateTimeString()
        {
            return DateTime.Now.ToString().Replace(' ', '-').Replace('/', '-').Replace(':', '-');
        }

        public NGramStore(string workDir)
        {
            m_workDir = workDir;
            m_topicModel = new TopicModelHandler(false /* isMixUnigram */);
            m_mixUnigram = new TopicModelHandler(true /* isMixUnigram */);
        }
        public Dictionary<Tuple<int, int, int, int, int, int, int>, long>[] nGramList { get { return this.m_nGrams;  } }

        public bool ShouldFlush()
        {
            return this.m_nGrams[6].Count >= thresholdToDiv2;
        }

        public bool CanSave()
        {
            return m_topicModel.CanSave(); // m_mixUnigram
        }

        public void SaveFile() // should 
        {
            FileOperation.SlideDataFile(this.fileNames, Constants.workingFolder);

            for (int nGram = 1; nGram <= 7; ++nGram)
            {
                var nGramHashMap = m_nGrams[nGram - 1];

                string fileName = fileNames[nGram - 1];
                using (StreamWriter fileStream = new StreamWriter(m_workDir + fileName, false, Encoding.Unicode))
                {
                    foreach (var item in nGramHashMap.OrderByDescending(item => item.Value))
                    {
                        string outputLine = WordList[item.Key.Item1] + "\t";
                        if (nGram >= 2) { outputLine += WordList[item.Key.Item2] + "\t"; }
                        if (nGram >= 3) { outputLine += WordList[item.Key.Item3] + "\t"; }
                        if (nGram >= 4) { outputLine += WordList[item.Key.Item4] + "\t"; }
                        if (nGram >= 5) { outputLine += WordList[item.Key.Item5] + "\t"; }
                        if (nGram >= 6) { outputLine += WordList[item.Key.Item6] + "\t"; }
                        if (nGram >= 7) { outputLine += WordList[item.Key.Item7] + "\t"; }
                        outputLine += item.Value.ToString();
                        fileStream.WriteLine(outputLine);
                    }
                }
            }

            m_topicModel.SaveToFile(m_workDir + Constants.topicModelFileName, m_workDir + Constants.topicModelSummaryFilename, (int id) => this.WordList[id]);
            m_mixUnigram.SaveToFile(m_workDir + Constants.mixUnigramlFileName, m_workDir + Constants.mixUnigramSummaryFilename, (int id) => this.WordList[id]);
        }

        public void LoadFromFile(int divNum = 1, int cutOut = 0)
        {
            if (File.Exists(m_workDir + doHalfFileName))
            {
                divNum = 2;
                File.Delete(m_workDir + doHalfFileName);
            }

            //
            this.WordHash = new Dictionary<string, int>(StringComparer.Ordinal);
            this.WordList = new List<string>();
            this.m_nGrams = new Dictionary<Tuple<int, int, int, int, int, int, int>, long>[7]
                {
                new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
                new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
                new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
                new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
                new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
                new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
                new Dictionary<Tuple<int, int, int, int, int, int, int>, long>(),
                };
            //

            for (int nGram = 1; nGram <= 7; ++nGram)
            {
                var nGramHashMap = m_nGrams[nGram - 1];
                int[] hashKeyList = new int[7];

                string fileName = fileNames[nGram - 1];
                try
                {
                    using (StreamReader fileStream = new StreamReader(m_workDir + fileName))
                    {
                        string line;
                        while ((line = fileStream.ReadLine()) != null)
                        {
                            string[] nGramLine = line.Split('\t');
                            if (nGramLine.Length == (nGram + 1))
                            {
                                long hashValue = 0;
                                long.TryParse(nGramLine[nGram], out hashValue);
                                if (cutOut > 0)
                                {
                                    if (hashValue < cutOut)
                                    {
                                        continue;
                                    }
                                }
                                if (divNum > 0)
                                {
                                    hashValue = hashValue / divNum;
                                    if (hashValue <= 0)
                                    {
                                        continue;
                                    }
                                }

                                hashKeyList[0] = WordToWordId(nGramLine[0], nGram <= 1);
                                if (nGram >= 2) { hashKeyList[1] = WordToWordId(nGramLine[1], false); }
                                if (nGram >= 3) { hashKeyList[2] = WordToWordId(nGramLine[2], false); }
                                if (nGram >= 4) { hashKeyList[3] = WordToWordId(nGramLine[3], false); }
                                if (nGram >= 5) { hashKeyList[4] = WordToWordId(nGramLine[4], false); }
                                if (nGram >= 6) { hashKeyList[5] = WordToWordId(nGramLine[5], false); }
                                if (nGram >= 7) { hashKeyList[6] = WordToWordId(nGramLine[6], false); }
                                if (hashKeyList.Any(x => x < 0))
                                {
                                    continue;
                                }
                                AddNgram(nGram, hashKeyList, hashValue);
                            }
                        }
                    }
                }
                catch (Exception) { }
            }

            this.m_topicModel.LoadFromFile(this.m_workDir + Constants.topicModelFileName,
                (string word) => this.WordToWordId(word, false),
                (int id) => this.WordList[id]);
            this.m_mixUnigram.LoadFromFile(this.m_workDir + Constants.mixUnigramlFileName,
                (string word) => this.WordToWordId(word, false),
                (int id) => this.WordList[id]);
        }

        public void AddFromWordArray(List<string> arrayOfWord) // sentence
        {
            int[] hashKeyList = new int[arrayOfWord.Count + 2 + 5];

            hashKeyList[0] = WordToWordId(BOS);
            hashKeyList[arrayOfWord.Count + 1] = WordToWordId(EOS);
            hashKeyList[arrayOfWord.Count + 2] = 0;
            hashKeyList[arrayOfWord.Count + 3] = 0;
            hashKeyList[arrayOfWord.Count + 4] = 0;
            hashKeyList[arrayOfWord.Count + 5] = 0;
            hashKeyList[arrayOfWord.Count + 6] = 0;

            for (int i = 0; i < arrayOfWord.Count; ++i)
            {
                hashKeyList[i + 1] = WordToWordId(arrayOfWord[i]);
            }

            for (int i = 0; i < (arrayOfWord.Count + 2); ++i)
            {
                var shiftKeyList = hashKeyList.Skip(i).Take(7).ToArray<int>();
                int remained = arrayOfWord.Count - i + 2;

                if (remained >= 7) { AddNgram(7, shiftKeyList, 1); }
                if (remained >= 6) { AddNgram(6, shiftKeyList, 1); }
                if (remained >= 5) { AddNgram(5, shiftKeyList, 1); }
                if (remained >= 4) { AddNgram(4, shiftKeyList, 1); }
                if (remained >= 3) { AddNgram(3, shiftKeyList, 1); }
                if (remained >= 2) { AddNgram(2, shiftKeyList, 1); }
                if (remained >= 1) { AddNgram(1, shiftKeyList, 1); }
            }

            m_topicModel.LearnDocument(arrayOfWord, (string word) => this.WordToWordId(word, false));
            m_mixUnigram.LearnDocument(arrayOfWord, (string word) => this.WordToWordId(word, false));
        }

        public void PrintCurrentState()
        {
            m_topicModel.PrintCurretState();
            m_mixUnigram.PrintCurretState();
        }

        public Tuple<Func<int, string>, Func<string, int>> GetWordIdMapper()
        {
            return new Tuple<Func<int, string>, Func<string, int>>(
                    (int wordId) => this.WordList[wordId],
                    (string word) => this.WordToWordId(word, false));
        }

        int WordToWordId(string word, bool createNew = true)
        {
            int wordId;
            lock (thisLock)
            {
                if (WordList.Count == 0)
                {
                    WordList.Add("[NULL]");
                }
                if (!WordHash.TryGetValue(word, out wordId))
                {
                    if (createNew)
                    {
                        wordId = WordList.Count;
                        WordList.Add(word);
                        WordHash.Add(word, wordId);

                        if (System.Diagnostics.Debugger.IsAttached)
                        {
                            if (WordList[wordId] != word)
                            {
                                System.Diagnostics.Debugger.Break();
                            }
                        }
                    }
                    else
                    {
                        return -1;
                    }
                }
            }
            return wordId;
        }

        void AddNgram(int nGram, int[] hashKeyList, long hashKeyValue)
        {
            var hashKey = new Tuple<int, int, int, int, int, int, int>(
                hashKeyList[0],
                nGram > 1 ? hashKeyList[1] : 0,
                nGram > 2 ? hashKeyList[2] : 0,
                nGram > 3 ? hashKeyList[3] : 0,
                nGram > 4 ? hashKeyList[4] : 0,
                nGram > 5 ? hashKeyList[5] : 0,
                nGram > 6 ? hashKeyList[6] : 0);

            var targetHash = m_nGrams[nGram - 1];
            if (targetHash.ContainsKey(hashKey))
            {
                targetHash[hashKey] += hashKeyValue;
            }
            else
            {
                targetHash.Add(hashKey, hashKeyValue);
            }
        }
    }


    public class MorphAnalyzer
    {
        static Regex regexNbsp = new Regex("&nbsp;");
        static Regex regexAmp = new Regex("&amp;");
        static Regex regexGt = new Regex("&gt;");
        static Regex regexLt = new Regex("&lt;");
        static Regex regexCharCode = new Regex(@"&#[0-9A-Fa-f]{1,4};");

        string inputFilename;
        string outputFilename;

        public MorphAnalyzer(string workingFolder)
        {
            string threadId = System.Threading.Thread.CurrentThread.ManagedThreadId.ToString();
            string dateText = DateTimeString();
            inputFilename = workingFolder + dateText + "-" + threadId + "-in.txt";
            outputFilename = workingFolder + dateText + "-" + threadId + "-out.txt";

            this.SetupNormalizeData();
        }

        public string DateTimeString()
        {
            return DateTime.Now.ToString().Replace(' ', '-').Replace('/', '-').Replace(':', '-');
        }


        public List<List<string>> Run(HashSet<string> srcText)
        {
            WriteInputFile(srcText);

            LaunchMecab();

            var result = ReadOutputFile();

            File.Delete(inputFilename);
            File.Delete(outputFilename);

            return result;
        }

        private void WriteInputFile(HashSet<string> srcText)
        {
            using (StreamWriter writeStream = new StreamWriter(inputFilename, false, Encoding.UTF8))
            {
                foreach (var text in srcText)
                {
                    var newText = regexNbsp.Replace(text, "　");
                    newText = regexAmp.Replace(newText, "&");
                    newText = regexGt.Replace(newText, ">");
                    newText = regexLt.Replace(newText, "<");

                    var matches = regexCharCode.Matches(newText);
                    for (int i = matches.Count - 1; i >= 0; --i)
                    {
                        var hexString = matches[i].Value.Substring(2, matches[i].Value.Length - 1 - 2);
                        char character = (char)UInt32.Parse(hexString, System.Globalization.NumberStyles.HexNumber);
                        newText = newText.Substring(0, matches[i].Index) + character + newText.Substring(matches[i].Index + matches[i].Length);
                    }

                    writeStream.WriteLine(newText);
                }
            }
        }

        private void LaunchMecab()
        {
            string parameters = string.Format("--input-buffer-size={0} --output={1} {2}", 0x8000, outputFilename, inputFilename);

            System.Diagnostics.ProcessStartInfo processStart = new System.Diagnostics.ProcessStartInfo(Constants.mecabExe, parameters);
            processStart.CreateNoWindow = true;
            processStart.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;

            System.Diagnostics.Process p = System.Diagnostics.Process.Start(processStart);
            p.WaitForExit();
        }

        private List<List<string>> ReadOutputFile()
        {

            var result = new List<List<string>>();

            using (StreamReader mecabOutput = new StreamReader(outputFilename))
            {
                List<string> building = new List<string>();

                string line;
                line = mecabOutput.ReadLine(); // skip 1 line for BOM
                while ((line = mecabOutput.ReadLine()) != null)
                {
                    if (line == "EOS")
                    {
                        if (building.Count > 0)
                        {
                            result.Add(building);
                        }
                        building = new List<string>();
                        continue;
                    }
                    string[] outPair = line.Split('\t');
                    if (outPair.Length != 2)
                    {
                        throw new Exception("unknown output");
                    }
                    string oneWord = WinApiBridge.Han2Zen(outPair[0]);

                    // normalizer
                    if (this.normalizeSymbolRegex.IsMatch(oneWord))
                    {
                        foreach (var ch in oneWord)
                        {
                            building.Add(this.normalizeSymbolHash[ch.ToString()]);
                        }
                    }
                    else if (this.normalizeTextRegex.IsMatch(oneWord))
                    {
                        var match = this.normalizeTextRegex.Match(oneWord);
                        var replaceSource = this.normalizeTextHash[match.Value];
                        foreach (var attrText in replaceSource)
                        {
                            building.Add(attrText);
                        }
                    }
                    else
                    {
                        oneWord += "," + outPair[1];
                        building.Add(oneWord);
                    }
                }
            }

            return result;
        }

        private static string[] normalizeSymbolList = new string[]
        {
            "・,記号,一般,*,*,*,*,・,・,・",
            "：,記号,一般,*,*,*,*,：,：,：",
            "；,記号,一般,*,*,*,*,；,；,；",
            "－,記号,一般,*,*,*,*,－,－,－",
            "＆,記号,一般,*,*,*,*,＆,＆,＆",
            "／,記号,一般,*,*,*,*,／,／,／",
            "〜,記号,一般,*,*,*,*,〜,〜,〜",
            "＃,記号,一般,*,*,*,*,＃,＃,＃",
            "＿,記号,一般,*,*,*,*,＿,＿,＿",
            "．,記号,句点,*,*,*,*,．,．,．",
            "，,記号,読点,*,*,*,*,，,，,，",
            "　,記号,空白,*,*,*,*,　,　,　",
            "！,記号,一般,*,*,*,*,！,！,！",
            "（,記号,括弧開,*,*,*,*,（,（,（",
            "）,記号,括弧閉,*,*,*,*,）,）,）",
            "「,記号,括弧開,*,*,*,*,「,「,「",
            "」,記号,括弧閉,*,*,*,*,」,」,」",
            "［,記号,括弧開,*,*,*,*,［,［,［",
            "］,記号,括弧閉,*,*,*,*,］,］,］",
            "【,記号,括弧開,*,*,*,*,【,【,【",
            "】,記号,括弧閉,*,*,*,*,】,】,】",
        };
        private Dictionary<string, string> normalizeSymbolHash;
        private Regex normalizeSymbolRegex;

        private Dictionary<string, string[]> normalizeTextHash = new Dictionary<string, string[]>()
        {
            { "コロナウイルスワクチン", new string [] { "コロナ,名詞,一般,*,*,*,*,コロナ,コロナ,コロナ", "ウイルス,名詞,一般,*,*,*,*,ウイルス,ウイルス,ウイルス", "ワクチン,名詞,一般,*,*,*,*,ワクチン,ワクチン,ワクチン" } },
            { "キャンプ", new string [] { "キャンプ,名詞,サ変接続,*,*,*,*,キャンプ,キャンプ,キャンプ" } },
        };
        private Regex normalizeTextRegex;

        private void SetupNormalizeData()
        {
            if (this.normalizeSymbolHash == null)
            {
                this.normalizeSymbolHash = new Dictionary<string, string>();
                var regexList = "";
                foreach (var target in normalizeSymbolList)
                {
                    var display = target.Split(new char[] { ',' })[0];
                    regexList += display;
                    this.normalizeSymbolHash.Add(display, target);
                }
                this.normalizeSymbolRegex = new Regex("^[" + regexList + "]+$");
            }
            if (this.normalizeTextRegex == null)
            {
                var regexText = String.Join("|", this.normalizeTextHash.Keys.ToArray());
                this.normalizeTextRegex = new Regex("^(" + regexText + ")$");
            }
        }
    }

}
