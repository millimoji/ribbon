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
        Dictionary<Tuple<string, string>, long> m_posBigram;

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
        const string posBigramFilename = "posbigram.txt";
        readonly string[] fileNames = new string[] { unigramFileName, bigramFileName, trigramFileName, n4gramFileName, n5gramFileName, n6gramFileName, n7gramFileName, posBigramFilename,
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
        public Dictionary<Tuple<string, string>, long> posBigram { get { return m_posBigram; } }

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

            for (int nGram = 1; nGram <= Constants.maxNGram; ++nGram)
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
            using (StreamWriter fileStream = new StreamWriter(m_workDir + posBigramFilename, false, Encoding.Unicode))
            {
                foreach (var item in m_posBigram)
                {
                    var outputLine = $"{item.Key.Item1}\t{item.Key.Item2}\t{item.Value}";
                    fileStream.WriteLine(outputLine);
                }
            }

            m_topicModel.SaveToFile(m_workDir + Constants.topicModelFileName, m_workDir + Constants.topicModelSummaryFilename, (int id) => this.WordList[id]);
            m_mixUnigram.SaveToFile(m_workDir + Constants.mixUnigramlFileName, m_workDir + Constants.mixUnigramSummaryFilename, (int id) => this.WordList[id]);
        }

        public long [] LoadFromFile(int divNum = 1, int cutOut = 0)
        {
            var totalCounts = new long[Constants.maxNGram];
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
            this.m_posBigram = new Dictionary<Tuple<string, string>, long>();
            //

            for (int nGram = 1; nGram <= Constants.maxNGram; ++nGram)
            {
                var nGramHashMap = m_nGrams[nGram - 1];
                int[] hashKeyList = new int[Constants.maxNGram];

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
                                if (divNum > 0)
                                {
                                    hashValue = hashValue / divNum;
                                    if (hashValue <= 0)
                                    {
                                        continue;
                                    }
                                }
                                totalCounts[nGram - 1] += hashValue;
                                if (cutOut > 0)
                                {
                                    if (hashValue < cutOut)
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

            try
            {
                using (StreamReader fileStream = new StreamReader(m_workDir + posBigramFilename))
                {
                    string line;
                    while ((line = fileStream.ReadLine()) != null)
                    {
                        string[] fields = line.Split('\t');
                        long bigramCount = 0;
                        if (!long.TryParse(fields[2], out bigramCount))
                        {
                            continue;
                        }
                        if (divNum > 0)
                        {
                            bigramCount = Math.Max(bigramCount / divNum, 1);
                        }
                        this.m_posBigram.Add(new Tuple<string, string>(fields[0], fields[1]), bigramCount);
                    }
                }
            }
            catch (Exception) { }

            this.m_topicModel.LoadFromFile(this.m_workDir + Constants.topicModelFileName,
                (string word) => this.WordToWordId(word, false),
                (int id) => this.WordList[id]);
            this.m_mixUnigram.LoadFromFile(this.m_workDir + Constants.mixUnigramlFileName,
                (string word) => this.WordToWordId(word, false),
                (int id) => this.WordList[id]);

            return totalCounts;
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
                var shiftKeyList = hashKeyList.Skip(i).Take(Constants.maxNGram).ToArray<int>();
                int remained = arrayOfWord.Count - i + 2;

                if (remained >= 7) { AddNgram(7, shiftKeyList, 1); }
                if (remained >= 6) { AddNgram(6, shiftKeyList, 1); }
                if (remained >= 5) { AddNgram(5, shiftKeyList, 1); }
                if (remained >= 4) { AddNgram(4, shiftKeyList, 1); }
                if (remained >= 3) { AddNgram(3, shiftKeyList, 1); }
                if (remained >= 2) { AddNgram(2, shiftKeyList, 1); }
                if (remained >= 1) { AddNgram(1, shiftKeyList, 1); }
            }

            this.AddPosBigram(arrayOfWord);

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

        Regex needToHaveDisplay = new Regex(@"^[^,]+,助詞,|^[^,]+,助動詞,|^お,接頭詞,|^ご,接頭詞,|^御,接頭詞,");

        void AddPosBigram(List<string> wordArray)
        {
            var lastPos = "[BOS]";
            for (int i = 0; i <= wordArray.Count; ++i)
            {
                string curPos;
                if (i == wordArray.Count)
                {
                    curPos = "[EOS]";
                }
                else if (needToHaveDisplay.IsMatch(wordArray[i]))
                {
                    // 0:Display, 1-6:Pos, 7:BaseDsiplay, 8:Reading, 9:Voice Rading 
                    curPos = String.Join(",", wordArray[i].Split(',').Take(7)); // use display
                }
                else
                {
                    curPos = "*," + String.Join(",", wordArray[i].Split(',').Skip(1).Take(6)); // display is *
                }
                var key = new Tuple<string, string>(lastPos, curPos);
                if (this.m_posBigram.ContainsKey(key))
                {
                    this.m_posBigram[key] += 1;
                }
                else
                {
                    this.m_posBigram.Add(key, 1);
                }
                lastPos = curPos;
            }
        }
    }

    public class MorphAnalyzer
    {
        TextNormalizer normalizer = new TextNormalizer();
        NumberConverter numberConverter = new NumberConverter();

        public MorphAnalyzer(string workingFolder)
        {
            this.SetupNormalizeData();
        }

        public List<List<string>> Run(HashSet<string> srcText)
        {
            string parameters = string.Format("--input-buffer-size={0}", 0x8000);
            System.Diagnostics.ProcessStartInfo processStart = new System.Diagnostics.ProcessStartInfo(Constants.mecabExe, parameters);
            processStart.UseShellExecute = false;
            processStart.RedirectStandardInput = true;
            processStart.RedirectStandardOutput = true;
            processStart.StandardOutputEncoding = System.Text.Encoding.UTF8;
            processStart.CreateNoWindow = true;
            processStart.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;

            var documents = new List<List<string>>();

            using (System.Diagnostics.Process p = System.Diagnostics.Process.Start(processStart))
            {
                using (var sw = new StreamWriter(p.StandardInput.BaseStream, System.Text.Encoding.UTF8))
                {
                    foreach (var text in srcText)
                    {
                        var newText = this.normalizer.NormalizeInput(text);
                        if (newText == null)
                        {
                            continue;
                        }
                        sw.WriteLine(newText);
                        sw.Flush();

                        var statement = ReadOutputStream(p.StandardOutput);
                        this.numberConverter.Convert(statement);

                        documents.Add(statement);
                    }
                    sw.Close();
                    p.StandardOutput.Close();
                }
            }
            return documents;
        }

        private List<string> ReadOutputStream(StreamReader mecabOutput)
        {
            List<string> building = new List<string>();

            string line;
            while ((line = mecabOutput.ReadLine()) != null)
            {
                if (line == "EOS")
                {
                    break;
                }
                string[] outPair = line.Split('\t');
                if (outPair.Length != 2)
                {
                    Logger.Log($"Invalid line: [{line}]");
                    continue;
                }
                if (outPair[0].Length == 0 || outPair[0] == "\uFEFF")
                {
                    continue; // ignore empty, usually, first output for BOM
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
                else
                {
                    oneWord += "," + outPair[1];
                    building.Add(oneWord);
                }
            }
            return building;
        }

        private static string[] normalizeSymbolList = new string[]
        {
            "ー,記号,一般,*,*,*,*,ー,ー,ー", // cho-on
            "～,記号,一般,*,*,*,*,～,～,～",
            "〜,記号,一般,*,*,*,*,〜,〜,〜",
            "・,記号,一般,*,*,*,*,・,・,・",
            "：,記号,一般,*,*,*,*,：,：,：",
            "；,記号,一般,*,*,*,*,；,；,；",
            "－,記号,一般,*,*,*,*,－,－,－", // minus
            "＆,記号,一般,*,*,*,*,＆,＆,＆",
            "＾,記号,一般,*,*,*,*,＾,＾,＾",
            "／,記号,一般,*,*,*,*,／,／,／",
            "＃,記号,一般,*,*,*,*,＃,＃,＃",
            "│,記号,一般,*,*,*,*,│,｜,｜",    // 2502 vertical line
            "｜,記号,一般,*,*,*,*,｜,｜,｜",    // FF5C full width pipe
            "＿,記号,一般,*,*,*,*,＿,＿,＿", // underscore
            "、,記号,句点,*,*,*,*,．,．,．",
            "。,記号,句点,*,*,*,*,．,．,．",
            "．,記号,句点,*,*,*,*,．,．,．",
            "，,記号,読点,*,*,*,*,，,，,，",
            "　,記号,空白,*,*,*,*,　,　,　",
            "！,記号,一般,*,*,*,*,！,！,！",
            "‼,記号,一般,*,*,*,*,‼,！,！",
            "？,記号,一般,*,*,*,*,？,？,？",
            "（,記号,括弧開,*,*,*,*,（,（,（",
            "）,記号,括弧閉,*,*,*,*,）,）,）",
            "「,記号,括弧開,*,*,*,*,「,「,「",
            "」,記号,括弧閉,*,*,*,*,」,」,」",
            "［,記号,括弧開,*,*,*,*,［,［,［",
            "］,記号,括弧閉,*,*,*,*,］,］,］",
            "【,記号,括弧開,*,*,*,*,【,【,【",
            "】,記号,括弧閉,*,*,*,*,】,】,】",
            "＜,記号,括弧開,*,*,*,*,＜,＜,＜",
            "＞,記号,括弧閉,*,*,*,*,＞,＞,＞",
            "≪,記号,括弧開,*,*,*,*,≪,＜,＜",
            "≫,記号,括弧閉,*,*,*,*,≫,＞,＞",
            "＂,記号,一般,*,*,*,*,*,＂,＂,＂",	// FF02
            "＇,記号,一般,*,*,*,*,*,＇,’,’",	// FF07
            "％,名詞,接尾,助数詞,*,*,*,％,％,％",
            "℃,名詞,接尾,助数詞,*,*,*,℃,ドシー,ドシー",
            "＋,記号,一般,*,*,*,*,＋,＋,＋",
            "＊,記号,一般,*,*,*,*,＊,＊,＊",
            "＝,記号,一般,*,*,*,*,＝,＝,＝",
            "＠,記号,一般,*,*,*,*,＠,＠,＠",
            "♪,記号,一般,*,*,*,*,♪,オンプ,オンプ",
            "①,記号,一般,*,*,*,*,①,マル１,マル１",
            "②,記号,一般,*,*,*,*,②,マル２,マル２",
            "③,記号,一般,*,*,*,*,③,マル３,マル３",
            "④,記号,一般,*,*,*,*,④,マル４,マル４",
            "⑤,記号,一般,*,*,*,*,⑤,マル５,マル５",
            "⑥,記号,一般,*,*,*,*,⑥,マル６,マル６",
            "⑦,記号,一般,*,*,*,*,⑦,マル７,マル７",
            "⑧,記号,一般,*,*,*,*,⑧,マル８,マル８",
            "⑨,記号,一般,*,*,*,*,⑨,マル９,マル９",
            "⑩,記号,一般,*,*,*,*,⑩,マル１０,マル１０",
            "⑪,記号,一般,*,*,*,*,⑪,マル１１,マル１１",
            "⑫,記号,一般,*,*,*,*,⑫,マル１２,マル１２",
            "⑬,記号,一般,*,*,*,*,⑬,マル１３,マル１３",
            "⑭,記号,一般,*,*,*,*,⑭,マル１４,マル１４",
            "⑮,記号,一般,*,*,*,*,⑮,マル１５,マル１５",
            "⑯,記号,一般,*,*,*,*,⑯,マル１６,マル１６",
            "⑰,記号,一般,*,*,*,*,⑰,マル１７,マル１７",
            "⑱,記号,一般,*,*,*,*,⑱,マル１８,マル１８",
            "⑲,記号,一般,*,*,*,*,⑲,マル１９,マル１９",
            "⑳,記号,一般,*,*,*,*,⑳,マル２０,マル２０",
            "㈱,名詞,一般,*,*,*,*,㈱,カブシキガイシャ,カブシキガイシャ",
            "®,記号,一般,*,*,*,*,®,トウロク,トーロク",
            "©,記号,一般,*,*,*,*,©,チョサクケン,チョサクケン",
            "™,記号,一般,*,*,*,*,™,トレードマーク,トレードマーク",
            "■,記号,一般,*,*,*,*,■,シカク,シカク",
            "※,記号,一般,*,*,*,*,※,コメジルシ,コメジルシ",
            "⇒,記号,一般,*,*,*,*,⇒,ミギ,ミギ",
            "⇛,記号,一般,*,*,*,*,⇛,ミギ,ミギ",
            "▶,記号,一般,*,*,*,*,▶,ミギ,ミギ",
            "▷,記号,一般,*,*,*,*,▷,ミギ,ミギ",
            "♡,記号,一般,*,*,*,*,♡,ハート,ハート",
            "★,記号,一般,*,*,*,*,★,ホシ,ホシ",
            "☆,記号,一般,*,*,*,*,☆,ホシ,ホシ",
            "✩,記号,一般,*,*,*,*,✩,ホシ,ホシ",
        };
        private Dictionary<string, string> normalizeSymbolHash;
        private Regex normalizeSymbolRegex;

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
                {   // TODO: half space seems like remained in tagged text, here convert it forcibly.
                    var sourceChar = " "; // half space
                    var targetWord = this.normalizeSymbolHash["　"]; // full space

                    regexList += sourceChar;
                    this.normalizeSymbolHash.Add(sourceChar, targetWord);
                }
                this.normalizeSymbolRegex = new Regex("^[" + regexList + "]+$");
            }
        }
    }

    class TextNormalizer
    {
        Regex simpleMapping;

        static Dictionary<string, Tuple<string, string>> inputNormalizeList = new Dictionary<string, Tuple<string, string>>()
        {
            { "charcode10", new Tuple<string, string>(@"&#[0-9]{1,5};", "1") },
            { "charcode16", new Tuple<string, string>(@"&#x[0-9A-Fa-f]{1,4};", "1") },
            { "SVS", new Tuple<string, string>(@"\uFE00-\uFE0F", "") }, // remove variation selector (SVS)
            { "a11", new Tuple<string, string>(@"[\u0020\u00A0\u1680\u2000-\u200C\u202F\u205F\u3000]+", "\u3000") }, // spaces
            { "a12", new Tuple<string, string>(@"ーー+", "ー") },
        };
        Regex inputNormalizeRegex;
        Regex inputIgnoreCharacterRegex;

        public TextNormalizer()
        {
            var simpleRule = String.Join("|", simpleMappingList.Select(kv => kv.Key).ToArray());
            this.simpleMapping = new Regex(simpleRule);

            var regexRule = String.Join("|", inputNormalizeList.Select(kv => $"(?<{kv.Key}>{kv.Value.Item1})").ToArray());
            this.inputNormalizeRegex = new Regex(regexRule);

            var ignoreRule = String.Join("|", ignoreSentenceCharList);
            this.inputIgnoreCharacterRegex = new Regex(ignoreRule);
        }

        public string NormalizeInput(string source)
        {
            var result = source;
            // ignore
            if (this.inputIgnoreCharacterRegex.IsMatch(result))
            {
                return null;
            }
            // complex
            {
                var listMatches = this.inputNormalizeRegex.Matches(result);
                for (int i = listMatches.Count - 1; i >= 0; --i)
                {
                    var matchItem = listMatches[i];
                    var replaceTo = "";
                    foreach (var kv in inputNormalizeList)
                    {
                        if (matchItem.Groups[kv.Key].Success)
                        {
                            if (kv.Key == "charcode10")
                            {
                                var numString = matchItem.Value.Substring(2, matchItem.Value.Length - 1 - 2);
                                replaceTo = ((char)UInt32.Parse(numString, System.Globalization.NumberStyles.Number)).ToString();
                            }
                            else if (kv.Key == "charcode16")
                            {
                                var hexString = matchItem.Value.Substring(3, matchItem.Value.Length - 1 - 3);
                                replaceTo = ((char)UInt32.Parse(hexString, System.Globalization.NumberStyles.HexNumber)).ToString();
                            }
                            else
                            {
                                replaceTo = kv.Value.Item2;
                            }
                            if (replaceTo == "\r" || replaceTo == "\n")
                            {
                                replaceTo = "\u3000";
                            }
                            result = result.Substring(0, matchItem.Index) + replaceTo + result.Substring(matchItem.Index + matchItem.Length);
                            break;
                        }
                    }
                }
            }
            // simple replacement rule
            {
                var listMatches = this.simpleMapping.Matches(result);
                for (int i = listMatches.Count - 1; i >= 0; --i)
                {
                    var matchItem = listMatches[i];
                    var replaceTo = simpleMappingList[matchItem.Value];
                    result = result.Substring(0, matchItem.Index) + replaceTo + result.Substring(matchItem.Index + matchItem.Length);
                }
            }
            return result;
        }

        static string[] ignoreSentenceCharList = new string[] { // consider these are in Chinese context
            "经", "网", "简", "舰",
        };

        static Dictionary<string, string> simpleMappingList = new Dictionary<string, string>()
        {
            // HTML espace
            { "&nbsp;", "\u3000" },
            { "&amp;", "＆" },
            { "&gt;", "＞" },
            { "&lt;", "＜" },
            { "&copy;", "©" },
            { "&reg;", "®" },
            { "&trade;", "™" },
            { "&rarr;", "→" },
            { "&quot;", "\"" },
            { "&hellip;", "…" },
            // see lots of this error
            { "ぺージ", "ページ" }, // Hiragana PE => Katakana PE
            // direction control
            { "\u202A", "" }, // LEFT-TO-RIGHT EMBEDDING
            { "\u202B", "" }, // RIGHT-TO-LEFT EMBEDDING
            { "\u202C", "" }, // POP DIRECTIONAL FORMATTING
            { "\u2061", "" }, // Function Application ??
            { "\u2062", "" }, // Invisible Times ??
            // Wrong?
            { "ゔ", "ゔ" }, // for 3099 or 309a
            { "が", "が" }, { "ぎ", "ぎ" }, { "ぐ", "ぐ" }, { "げ", "げ" }, { "ご", "ご" },
            { "ざ", "ざ" }, { "じ", "じ" }, { "ず", "ず" }, { "ぜ", "ぜ" }, { "ぞ", "ぞ" },
            { "だ", "だ" }, { "ぢ", "ぢ" }, { "づ", "づ" }, { "で", "で" }, { "ど", "ど" },
            { "ば", "ば" }, { "び", "び" }, { "ぶ", "ぶ" }, { "べ", "べ" }, { "ぼ", "ぼ" },
            { "ぱ", "ぱ" }, { "ぴ", "ぴ" }, { "ぷ", "ぷ" }, { "ぺ", "ぺ" }, { "ぽ", "ぽ" },
            { "ヴ", "ヴ" },
            { "ガ", "ガ" }, { "ギ", "ギ" }, { "グ", "グ" }, { "ゲ", "ゲ" }, { "ゴ", "ゴ" },
            { "ザ", "ザ" }, { "ジ", "ジ" }, { "ズ", "ズ" }, { "ゼ", "ゼ" }, { "ゾ", "ゾ" },
            { "ダ", "ダ" }, { "ヂ", "ヂ" }, { "ヅ", "ヅ" }, { "デ", "デ" }, { "ド", "ド" },
            { "ヷ", "ヷ" }, { "ヸ", "ヸ" }, { "ヹ", "ヹ" }, { "ヺ", "ヺ" },
            { "バ", "バ" }, { "ビ", "ビ" }, { "ブ", "ブ" }, { "ベ", "ベ" }, { "ボ", "ボ" },
            { "パ", "パ" }, { "ピ", "ピ" }, { "プ", "プ" }, { "ペ", "ペ" }, { "ポ", "ポ" },
            { "う゛", "ゔ" }, // for 309b or 309c
            { "か゛", "が" }, { "き゛", "ぎ" }, { "く゛", "ぐ" }, { "け゛", "げ" }, { "こ゛", "ご" },
            { "さ゛", "ざ" }, { "し゛", "じ" }, { "す゛", "ず" }, { "せ゛", "ぜ" }, { "そ゛", "ぞ" },
            { "た゛", "だ" }, { "ち゛", "ぢ" }, { "つ゛", "づ" }, { "て゛", "で" }, { "と゛", "ど" },
            { "は゛", "ば" }, { "ひ゛", "び" }, { "ふ゛", "ぶ" }, { "へ゛", "べ" }, { "ほ゛", "ぼ" },
            { "は゜", "ぱ" }, { "ひ゜", "ぴ" }, { "ふ゜", "ぷ" }, { "へ゜", "ぺ" }, { "ほ゜", "ぽ" },
            { "ウ゛", "ヴ" },
            { "カ゛", "ガ" }, { "キ゛", "ギ" }, { "ク゛", "グ" }, { "ケ゛", "ゲ" }, { "コ゛", "ゴ" },
            { "サ゛", "ザ" }, { "シ゛", "ジ" }, { "ス゛", "ズ" }, { "セ゛", "ゼ" }, { "ソ゛", "ゾ" },
            { "タ゛", "ダ" }, { "チ゛", "ヂ" }, { "ツ゛", "ヅ" }, { "テ゛", "デ" }, { "ト゛", "ド" },
            { "ワ゛", "ヷ" }, { "ヰ゛", "ヸ" }, { "ヱ゛", "ヹ" }, { "ヲ゛", "ヺ" },
            { "ハ゛", "バ" }, { "ヒ゛", "ビ" }, { "フ゛", "ブ" }, { "ヘ゛", "ベ" }, { "ホ゛", "ボ" },
            { "ハ゜", "パ" }, { "ヒ゜", "ピ" }, { "フ゜", "プ" }, { "ヘ゜", "ペ" }, { "ホ゜", "ポ" },
            { "│", "｜" }, // u2502 => FF5C
            /*
            { "\u3099", "\u309B" }, // Combine Dakuten => Dakuten (TODO: consider later)
            { "\u309A", "\u309C" }, // Combine Han-dakuten => Han-dakuten (TODO: consider later)
            */
            // Variation
            { "﨑", "崎" },
            { "麵", "麺" },
            { "步", "歩" },
            // normalize symbol
            { " ", "\u3000" },    // 20
            { "\u2028", "\u3000" },
            { "\u2800", "\u3000" },
            { "—", "－" },    // 2014
            { "–", "－" },    // 2013
            { "−", "－" },    // 2212
            { "\u2026", "－" },    // 2026
            { "•", "・" },   // 2022
            // 康熙字典部首 KANGXI RADICAL
            // 【1画】
            { "⼀" /* &#x2F00; */, "一" /* &#x4E00; */ },			// いち	ONE
            { "⼁" /* &#x2F01; */, "丨" /* &#x4E28; */ },			// ぼう、たてぼう	LINE
            { "⼂" /* &#x2F02; */, "丶" /* &#x4E36; */ },			// てん	DOT
            { "⼃" /* &#x2F03; */, "丿" /* &#x4E3F; */ },			// の、はらいぼう	SLASH
            { "⼄" /* &#x2F04; */, "乙" /* &#x4E59; */ },			// おつ	SECOND
            { "乚" /* &#x2E83; */, "乚" /* &#x4E5A; */ },			// つりばり	(CJK) SECOND TWO
            { "⼅" /* &#x2F05; */, "亅" /* &#x4E85; */ },			// はねぼう	HOOK
            //【2画】
            { "⼆" /* &#x2F06; */, "二" /* &#x4E8C; */ },			// に	TWO
            { "⼇" /* &#x2F07; */, "亠" /* &#x4EA0; */ },			// なべぶた	LID
            { "⼈" /* &#x2F08; */, "人" /* &#x4EBA; */ },			// ひと	MAN
            // { "𠆢" /* &#x201A2, */, "人" /* &#x4EBA; */ },			// ひとやね、ひとがしら	Unicode U+201A2
            { "亻" /* &#x2E85; */, "亻" /* &#x4EBB; */ },			// にんべん	(CJK) PERSON
            { "⼉" /* &#x2F09; */, "儿" /* &#x513F; */ },			// ひとあし、にんにょう	LEGS
            { "⼊" /* &#x2F0A; */, "入" /* &#x5165; */ },			// いる、いりがしら	ENTER
            { "⼋" /* &#x2F0B; */, "八" /* &#x516B; */ },			// はち、はちがしら	EIGHT
            { "⼌" /* &#x2F0C; */, "冂" /* &#x5182; */ },			// どうがまえ、けいがまえ	DOWN BOX
            { "⼍" /* &#x2F0D; */, "冖" /* &#x5196; */ },			// わかんむり	COVER
            { "⼎" /* &#x2F0E; */, "冫" /* &#x51AB; */ },			// にすい	ICE
            { "⼏" /* &#x2F0F; */, "几" /* &#x51E0; */ },			// つくえ	TABLE
            { "⺇" /* &#x2E87; */, "几" /* &#x51E0; */ },			// かぜかんむり、かぜがまえ	(CJK) TABLE
            { "⼐" /* &#x2F10; */, "凵" /* &#x51F5; */ },			// かんにょう、うけばこ	OPEN BOX
            { "⼑" /* &#x2F11; */, "刀" /* &#x5200; */ },			// かたな	KNIFE
            { "刂" /* &#x2E89; */, "刂" /* &#x5202; */ },			// りっとう	(CJK) KNIFE TWO
            { "⼒" /* &#x2F12; */, "力" /* &#x529B; */ },			// ちから	POWER
            { "⼓" /* &#x2F13; */, "勹" /* &#x52F9; */ },			// つつみがまえ	WRAP
            { "⼔" /* &#x2F14; */, "匕" /* &#x5315; */ },			// ひ、さじ	SPOON
            { "⼕" /* &#x2F15; */, "匚" /* &#x531A; */ },			// はこがまえ	RIGHT OPEN BOX
            { "⼖" /* &#x2F16; */, "匸" /* &#x5338; */ },			// かくしがまえ	HIDING ENCLOSURE
            { "⼗" /* &#x2F17; */, "十" /* &#x5341; */ },			// じゅう	TEN
            { "⼘" /* &#x2F18; */, "卜" /* &#x535C; */ },			// ぼくのと	DIVINATION
            { "⼙" /* &#x2F19; */, "卩" /* &#x5369; */ },			// ふしづくり	SEAL
            { "⺋" /* &#x2E8B; */, "㔾" /* &#x353E; */ },			// まげわりふ	(CJK) SEAL
            { "⼚" /* &#x2F1A; */, "厂" /* &#x5382; */ },			// がんだれ	CLIFF
            { "⼛" /* &#x2F1B; */, "厶" /* &#x53B6; */ },			// む	PRIVATE
            { "⼜" /* &#x2F1C; */, "又" /* &#x53C8; */ },			// また	AGAIN
            // 【3画】					
            { "⼝" /* &#x2F1D; */, "口" /* &#x53E3; */ },			// くち、くちへん	MOUTH
            { "⼞" /* &#x2F1E; */, "囗" /* &#x56D7; */ },			// くにがまえ	ENCLOSURE
            { "⼟" /* &#x2F1F; */, "土" /* &#x571F; */ },			// つち、つちへん	EARTH
            { "⼠" /* &#x2F20; */, "士" /* &#x58EB; */ },			// さむらい	SCHOLAR
            { "⼡" /* &#x2F21; */, "夂" /* &#x5902; */ },			// ふゆがしら	GO
            { "⼢" /* &#x2F22; */, "夊" /* &#x590A; */ },			// なつあし	GO SLOWLY
            { "⼣" /* &#x2F23; */, "夕" /* &#x5915; */ },			// ゆう、ゆうべ	EVENING
            { "⼤" /* &#x2F24; */, "大" /* &#x5927; */ },			// だい	BIG
            { "⼥" /* &#x2F25; */, "女" /* &#x5973; */ },			// おんな、おんなへん	WOMAN
            { "⼦" /* &#x2F26; */, "子" /* &#x5B50; */ },			// こ、こへん	CHILD
            { "⼧" /* &#x2F27; */, "宀" /* &#x5B80; */ },			// うかんむり	ROOF
            { "⼨" /* &#x2F28; */, "寸" /* &#x5BF8; */ },			// すん	INCH
            { "⼩" /* &#x2F29; */, "小" /* &#x5C0F; */ },			// しょう	SMALL
            { "⺌" /* &#x2E8C; */, "小" /* &#x5C0F; */ },			// しょうがしら	(CJK) SMALL ONE
            { "⺍" /* &#x2E8D; */, "小" /* &#x5C0F; */ },			// つかんむり	(CJK) SMALL TWO
            { "⼪" /* &#x2F2A; */, "尢" /* &#x5C22; */ },			// だいのまげあし	LAME
            { "⼫" /* &#x2F2B; */, "尸" /* &#x5C38; */ },			// しかばね	CORPSE
            { "⼬" /* &#x2F2C; */, "屮" /* &#x5C6E; */ },			// てつ、くさのめ	SPROUT
            { "⼭" /* &#x2F2D; */, "山" /* &#x5C71; */ },			// やま、やまへん	MOUNTAIN
            { "⼮" /* &#x2F2E; */, "巛" /* &#x5DDB; */ },			// かわ	RIVER
            { "⼯" /* &#x2F2F; */, "工" /* &#x5DE5; */ },			// こう、たくみへん	WORK
            { "⼰" /* &#x2F30; */, "己" /* &#x5DF1; */ },			// おのれ	ONESELF
            { "⼱" /* &#x2F31; */, "巾" /* &#x5DFE; */ },			// はば、はばへん	TURBAN
            { "⼲" /* &#x2F32; */, "干" /* &#x5E72; */ },			// かん、いちじゅう	DRY
            { "⼳" /* &#x2F33; */, "幺" /* &#x5E7A; */ },			// いとがしら	SHORT THREAD
            { "⼴" /* &#x2F34; */, "广" /* &#x5E7F; */ },			// まだれ	DOTTED CLIFF
            { "⼵" /* &#x2F35; */, "廴" /* &#x5EF4; */ },			// えんにょう	LONG STRIDE
            { "⼶" /* &#x2F36; */, "廾" /* &#x5EFE; */ },			// にじゅうあし、こまぬき	TWO HANDS
            { "⼷" /* &#x2F37; */, "弋" /* &#x5F0B; */ },			// しきがまえ、よく	SHOOT
            { "⼸" /* &#x2F38; */, "弓" /* &#x5F13; */ },			// ゆみへん	BOW
            { "⼹" /* &#x2F39; */, "彐" /* &#x5F50; */ },			// けいがしら	SNOUT
            { "彑" /* &#x2E94; */, "彑" /* &#x5F51; */ },			// けいがしら	(CJK) SNOUT ONE
            { "⺕" /* &#x2E95; */, "彐" /* &#x5F50; */ },			// けいがしら	(CJK) SNOUT TWO
            { "⼺" /* &#x2F3A; */, "彡" /* &#x5F61; */ },			// さんづくり	BRISTLE
            { "⼻" /* &#x2F3B; */, "彳" /* &#x5F73; */ },			// ぎょうにんべん	STEP
            { "艹" /* &#x2EBE; */, "艹" /* &#x8279; */ },			// くさかんむり	(CJK) GRASS ONE
            { "⻌" /* &#x2ECC; */, "辶" /* &#x8FB6; */ },			// (1点)しんにょう、しんにゅう	(CJK) SIMPLIFIED WALK
            { "⻖" /* &#x2ED6; */, "阝" /* &#x961D; */ },			// おおざと	(CJK) MOUND TWO
            //{ "⻖" /* &#x2ED6; */, "阝" /* &#x961D; */ },			// こざと、こざとへん	(CJK) MOUND TWO
            { "忄" /* &#x2E96; */, "忄" /* &#x5FC4; */ },			// りっしんべん	(CJK) HEART ONE
            { "扌" /* &#x2E98; */, "扌" /* &#x624C; */ },			// てへん	(CJK) HAND
            { "氵" /* &#x2EA1; */, "氵" /* &#x6C35; */ },			// さんずい	(CJK) WATER ONE
            { "犭" /* &#x2EA8; */, "犭" /* &#x72AD; */ },			// けものへん	(CJK) DOG
            { "⺦" /* &#x2EA6; */, "丬" /* &#x4E2C; */ },			// しょうへん	(CJK) SIMPLIFIED HALF TREE TRUNK
            // 【4画】
            { "⼼" /* &#x2F3C; */, "心" /* &#x5FC3; */ },			// こころ	HEART
            { "⺗" /* &#x2E97; */, "心" /* &#x5FC3; */ },			// したごころ	(CJK) HEART TWO
            { "⼽" /* &#x2F3D; */, "戈" /* &#x6208; */ },			// ほこづくり、ほこがまえ	HALBERD
            { "⼾" /* &#x2F3E; */, "戶" /* &#x6236; */ },			// と、とかんむり、とだれ	DOOR
            { "⼿" /* &#x2F3F; */, "手" /* &#x624B; */ },			// て	HAND
            { "⽀" /* &#x2F40; */, "支" /* &#x652F; */ },			// し、しにょう	BRANCH
            { "⽁" /* &#x2F41; */, "攴" /* &#x6534; */ },			// ぼくづくり、とまた	RAP
            { "⺙" /* &#x2E99; */, "攵" /* &#x6535; */ },			// のぶん	(CJK) RAP
            { "⽂" /* &#x2F42; */, "文" /* &#x6587; */ },			// ぶん	SCRIPT
            { "⽃" /* &#x2F43; */, "斗" /* &#x6597; */ },			// とます、と	DIPPER
            { "⽄" /* &#x2F44; */, "斤" /* &#x65A4; */ },			// おの、きん	AXE
            { "⽅" /* &#x2F45; */, "方" /* &#x65B9; */ },			// ほう、かたへん	SQUARE
            { "⽆" /* &#x2F46; */, "无" /* &#x65E0; */ },			// なし、むにょう	NOT
            { "⺛" /* &#x2E9B; */, "旡" /* &#x65E1; */ },			// すでのつくり	(CJK) CHOKE
            { "⽇" /* &#x2F47; */, "日" /* &#x65E5; */ },			// ひ、ひへん	SUN
            { "⽈" /* &#x2F48; */, "曰" /* &#x66F0; */ },			// ひらび、いわく	SAY
            { "⽉" /* &#x2F49; */, "月" /* &#x6708; */ },			// つき、つきへん	MOON
            { "⺼" /* &#x2EBC; */, "肉" /* &#x8089; */ },			// にくづき	(CJK) MEAT
            { "⽊" /* &#x2F4A; */, "木" /* &#x6728; */ },			// き、きへん	TREE
            { "⽋" /* &#x2F4B; */, "欠" /* &#x6B20; */ },			// あくび、けんづくり	LACK
            { "⽌" /* &#x2F4C; */, "止" /* &#x6B62; */ },			// とめる、とめへん	STOP
            { "⽍" /* &#x2F4D; */, "歹" /* &#x6B79; */ },			// がつへん、かばねへん、いちたへん	DEATH
            { "歺" /* &#x2E9E; */, "歺" /* &#x6B7A; */ },			// がつへん、かばねへん、いちたへん	(CJK) DEATH
            { "⽎" /* &#x2F4E; */, "殳" /* &#x6BB3; */ },			// るまた、ほこづくり	WEAPON
            { "⽏" /* &#x2F4F; */, "毋" /* &#x6BCB; */ },			// なかれ	DO NOT
            { "⽐" /* &#x2F50; */, "比" /* &#x6BD4; */ },			// ならびひ、くらべる	COMPARE
            { "⽑" /* &#x2F51; */, "毛" /* &#x6BDB; */ },			// け	FUR
            { "⽒" /* &#x2F52; */, "氏" /* &#x6C0F; */ },			// うじ	CLAN
            { "⽓" /* &#x2F53; */, "气" /* &#x6C14; */ },			// きがまえ	STEAM
            { "⽔" /* &#x2F54; */, "水" /* &#x6C34; */ },			// みず	WATER
            { "⽕" /* &#x2F55; */, "火" /* &#x706B; */ },			// ひ、ひへん	FIRE
            { "⺣" /* &#x2EA3; */, "灬" /* &#x706C; */ },			// れっか	(CJK) FIRE
            { "⽖" /* &#x2F56; */, "爪" /* &#x722A; */ },			// つめ	CLAW
            { "⺤" /* &#x2EA4; */, "爫" /* &#x722B; */ },			// つめかんむり	(CJK) PAW ONE
            { "⺥" /* &#x2EA5; */, "爫" /* &#x722B; */ },			// つめかんむり	(CJK) PAW TWO
            { "⽗" /* &#x2F57; */, "父" /* &#x7236; */ },			// ちち	FATHER
            { "⽘" /* &#x2F58; */, "爻" /* &#x723B; */ },			// こう、めめ	DOUBLE X
            { "⽙" /* &#x2F59; */, "爿" /* &#x723F; */ },			// しょうへん	HALF TREE TRUNK
            { "⽚" /* &#x2F5A; */, "片" /* &#x7247; */ },			// かた、かたへん	SLICE
            { "⽛" /* &#x2F5B; */, "牙" /* &#x7259; */ },			// きば、きばへん	FANG
            { "⽜" /* &#x2F5C; */, "牛" /* &#x725B; */ },			// うし、うしへん	COW
            { "牜" /* &#x725C; */, "牛" /* &#x725B; */ },			// うし、うしへん	OX,COW
            { "⽝" /* &#x2F5D; */, "犬" /* &#x72AC; */ },			// いぬ	DOG
            { "⻀" /* &#x2EC0; */, "艹" /* &#x8279; */ },			// くさかんむり	(CJK) GRASS THREE
            { "⻍" /* &#x2ECD; */, "辶" /* &#x8FB6; */ },			// (2点)しんにょう、しんにゅう	(CJK) WALK ONE
            { "⻎" /* &#x2ECE; */, "辶" /* &#x8FB6; */ },			// しんにょう、しんにゅう	(CJK) WALK TWO
            { "⺩" /* &#x2EA9; */, "王" /* &#x738B; */ },			// おう、おうへん	(CJK) JADE
            { "礻" /* &#x2EAD; */, "礻" /* &#x793B; */ },			// しめすへん、ねへん	(CJK) SPIRIT TWO
            { "耂" /* &#x2EB9; */, "耂" /* &#x8002; */ },			// おいかんむり	(CJK) OLD
            { "⺱" /* &#x2EB1; */, "罓" /* &#x7F53; */ },			// あみがしら	(CJK) NET ONE
            { "⺳" /* &#x2EB3; */, "网" /* &#x7F51; */ },			// あみがしら	(CJK) NET THREE
            //【5画】
            { "⽞" /* &#x2F5E; */, "玄" /* &#x7384; */ },			// げん	PROFOUND
            { "⽟" /* &#x2F5F; */, "玉" /* &#x7389; */ },			// たま、おう、おうへん	JADE
            { "⽠" /* &#x2F60; */, "瓜" /* &#x74DC; */ },			// うり	MELON
            { "⽡" /* &#x2F61; */, "瓦" /* &#x74E6; */ },			// かわら	TILE
            { "⽢" /* &#x2F62; */, "甘" /* &#x7518; */ },			// かん、あまい	SWEET
            { "⽣" /* &#x2F63; */, "生" /* &#x751F; */ },			// うまれる	LIFE
            { "⽤" /* &#x2F64; */, "用" /* &#x7528; */ },			// もちいる	USE
            { "⽥" /* &#x2F65; */, "田" /* &#x7530; */ },			// た、たへん	FIELD
            { "⽦" /* &#x2F66; */, "疋" /* &#x758B; */ },			// ひき、ひきへん	BOLT OF CLOTH
            { "𤴔" /* &#x2EAA; */, "疋" /* &#x758B; */ },			// ひき、ひきへん	(CJK) BOLT OF CLOTH
            { "⽧" /* &#x2F67; */, "疒" /* &#x7592; */ },			// やまいだれ	SICKNESS
            { "⽨" /* &#x2F68; */, "癶" /* &#x7676; */ },			// はつがしら	DOTTED TENT
            { "⽩" /* &#x2F69; */, "白" /* &#x767D; */ },			// しろ	WHITE
            { "⽪" /* &#x2F6A; */, "皮" /* &#x76AE; */ },			// けがわ	SKIN
            { "⽫" /* &#x2F6B; */, "皿" /* &#x76BF; */ },			// さら	DISH
            { "⽬" /* &#x2F6C; */, "目" /* &#x76EE; */ },			// め、めへん	EYE
            { "⽭" /* &#x2F6D; */, "矛" /* &#x77DB; */ },			// ほこ、ほこへん	SPEAR
            { "⽮" /* &#x2F6E; */, "矢" /* &#x77E2; */ },			// や、やへん	ARROW
            { "⽯" /* &#x2F6F; */, "石" /* &#x77F3; */ },			// いし、いしへん	STONE
            { "⽰" /* &#x2F70; */, "示" /* &#x793A; */ },			// しめす	SPIRIT
            { "⽱" /* &#x2F71; */, "禸" /* &#x79B8; */ },			// ぐうのあし	TRACK
            { "⽲" /* &#x2F72; */, "禾" /* &#x79BE; */ },			// のぎ、のぎへん	GRAIN
            { "⽳" /* &#x2F73; */, "穴" /* &#x7A74; */ },			// あな、あなかんむり	CAVE
            { "⽴" /* &#x2F74; */, "立" /* &#x7ACB; */ },			// たつ、たつへん	STAND
            { "⺫" /* &#x2EAB; */, "罒" /* &#x7F52; */ },			// あみがしら、あみめ	(CJK) NET TWO
            { "⺲" /* &#x2EB2; */, "目" /* &#x76EE; */ },			// よこめ、よんがしら	(CJK) EYE
            { "⺟" /* &#x2E9F; */, "母" /* &#x6BCD; */ },			// はは	(CJK) MOTHER
            { "⻂" /* &#x2EC2; */, "衤" /* &#x8864; */ },			// ころもへん	(CJK) CLOTHES
            { "⺢" /* &#x2EA2; */, "氺" /* &#x6C3A; */ },			// したみず	(CJK) WATER TWO
            { "⺠" /* &#x2EA0; */, "民" /* &#x6C11; */ },
            //【6画】					
            { "⽵" /* &#x2F75; */, "竹" /* &#x7AF9; */ },			// たけ、たけかんむり	BAMBOO
            { "⺮" /* &#x2EAE; */, "竹" /* &#x7AF9; */ },			// たけかんむり	(CJK) BAMBOO
            { "⽶" /* &#x2F76; */, "米" /* &#x7C73; */ },			// こめ、こめへん	RICE
            { "⽷" /* &#x2F77; */, "糸" /* &#x7CF8; */ },			// いと、いとへん	SILK
            { "⽸" /* &#x2F78; */, "缶" /* &#x7F36; */ },			// かん、ほとぎ	JAR
            { "⽹" /* &#x2F79; */, "网" /* &#x7F51; */ },			// あみ、あみがしら	NET
            { "⽺" /* &#x2F7A; */, "羊" /* &#x7F8A; */ },			// ひつじ	SHEEP
            { "⺷" /* &#x2EB6; */, "羊" /* &#x7F8A; */ },			// ひつじ	(CJK) SHEEP
            { "⺶" /* &#x2EB7; */, "羊" /* &#x7F8A; */ },			// ひつじ	(CJK) RAM
            { "⽻" /* &#x2F7B; */, "羽" /* &#x7FBD; */ },			// はね	FEATHER
            { "⽼" /* &#x2F7C; */, "老" /* &#x8001; */ },			// おい、おいがしら	OLD
            { "⽽" /* &#x2F7D; */, "而" /* &#x800C; */ },			// しこうして	AND
            { "⽾" /* &#x2F7E; */, "耒" /* &#x8012; */ },			// すきへん	PLOW
            { "⽿" /* &#x2F7F; */, "耳" /* &#x8033; */ },			// みみ、みみへん	EAR
            { "⾀" /* &#x2F80; */, "聿" /* &#x807F; */ },			// ふでづくり	BRUSH
            { "⾁" /* &#x2F81; */, "肉" /* &#x8089; */ },			// にく	MEAT
            { "⾂" /* &#x2F82; */, "臣" /* &#x81E3; */ },			// しん	MINISTER
            { "⾃" /* &#x2F83; */, "自" /* &#x81EA; */ },			// みずから	SELF
            { "⾄" /* &#x2F84; */, "至" /* &#x81F3; */ },			// いたる	ARRIVE
            { "⾅" /* &#x2F85; */, "臼" /* &#x81FC; */ },			// うす	MORTAR
            { "⾆" /* &#x2F86; */, "舌" /* &#x820C; */ },			// した	TONGUE
            { "⾇" /* &#x2F87; */, "舛" /* &#x821B; */ },			// ます	OPPOSE
            { "⾈" /* &#x2F88; */, "舟" /* &#x821F; */ },			// ふね、ふねへん	BOAT
            { "⾉" /* &#x2F89; */, "艮" /* &#x826E; */ },			// こんづくり、うしとら	STOPPING
            { "⾊" /* &#x2F8A; */, "色" /* &#x8272; */ },			// いろ	COLOR
            { "⾋" /* &#x2F8B; */, "艸" /* &#x8278; */ },			// くさ、くさかんむり	GRASS
            { "⾌" /* &#x2F8C; */, "虍" /* &#x864D; */ },			// とらがしら、とらかんむり	TIGER
            { "⾍" /* &#x2F8D; */, "虫" /* &#x866B; */ },			// むし、むしへん	INSECT
            { "⾎" /* &#x2F8E; */, "血" /* &#x8840; */ },			// ち	BLOOD
            { "⾏" /* &#x2F8F; */, "行" /* &#x884C; */ },			// ぎょう、ぎょうがまえ	WALK ENCLOSURE
            { "⾐" /* &#x2F90; */, "衣" /* &#x8863; */ },			// ころも	CLOTHES
            { "⾑" /* &#x2F91; */, "襾" /* &#x897E; */ },			// にし、にしかんむり	WEST
            { "⻃" /* &#x2EC3; */, "覀" /* &#x8980; */ },			// にし、にしかんむり	(CJK) WEST ONE
            { "⻄" /* &#x2EC4; */, "西" /* &#x897F; */ },			// にし、にしかんむり	(CJK) WEST TWO
            //【7画】
            { "⾒" /* &#x2F92; */, "見" /* &#x898B; */ },			// みる	SEE
            { "⾓" /* &#x2F93; */, "角" /* &#x89D2; */ },			// つの、つのへん	HORN
            { "⾔" /* &#x2F94; */, "言" /* &#x8A00; */ },			// ごんべん	SPEECH
            { "訁" /* &#x8A01; */, "言" /* &#x8A00; */ },			// ごんべん	GONBEN
            { "⾕" /* &#x2F95; */, "谷" /* &#x8C37; */ },			// たに、たにへん	VALLEY
            { "⾖" /* &#x2F96; */, "豆" /* &#x8C46; */ },			// まめ、まめへん	BEAN
            { "⾗" /* &#x2F97; */, "豕" /* &#x8C55; */ },			// ぶた、いのこ	PIG
            { "⾘" /* &#x2F98; */, "豸" /* &#x8C78; */ },			// むじな、むじなへん	BADGER
            { "⾙" /* &#x2F99; */, "貝" /* &#x8C9D; */ },			// かい、かいへん	SHELL
            { "⾚" /* &#x2F9A; */, "赤" /* &#x8D64; */ },			// あか	RED
            { "⾛" /* &#x2F9B; */, "走" /* &#x8D70; */ },			// はしる、そうにょう	RUN
            { "⾜" /* &#x2F9C; */, "足" /* &#x8DB3; */ },			// あし	FOOT
            { "⻊" /* &#x2ECA; */, "足" /* &#x8DB3; */ },			// あしへん	(CJK) FOOT
            { "⾝" /* &#x2F9D; */, "身" /* &#x8EAB; */ },			// み	BODY
            { "⾞" /* &#x2F9E; */, "車" /* &#x8ECA; */ },			// くるま、くるまへん	CART
            { "⾟" /* &#x2F9F; */, "辛" /* &#x8F9B; */ },			// からい	BITTER
            { "⾠" /* &#x2FA0; */, "辰" /* &#x8FB0; */ },			// しんのたつ	MORNING
            { "⾡" /* &#x2FA1; */, "辵" /* &#x8FB5; */ },			// しんにょう、しんにゅう	WALK
            { "⾢" /* &#x2FA2; */, "邑" /* &#x9091; */ },			// むら、おおざと	CITY
            { "⾣" /* &#x2FA3; */, "酉" /* &#x9149; */ },			// とり	WINE
            { "⾤" /* &#x2FA4; */, "釆" /* &#x91C6; */ },			// のごめ	DISTINGUISH
            { "⾥" /* &#x2FA5; */, "里" /* &#x91CC; */ },			// さと、さとへん	VILLAGE
            { "⻨" /* &#x2EE8; */, "麦" /* &#x9EA6; */ },			// むぎ、むぎへん	(CJK) SIMPLIFIED WHEAT
            // 【8画】
            { "⾦" /* &#x2FA6; */, "金" /* &#x91D1; */ },			// かね、かねへん	GOLD
            { "釒" /* &#x91D2; */, "金" /* &#x91D1; */ },			// かね、かねへん	GOLD
            { "⾧" /* &#x2FA7; */, "長" /* &#x9577; */ },			// ながい	LONG
            { "⻑" /* &#x2ED1; */, "長" /* &#x9577; */ },
            { "⾨" /* &#x2FA8; */, "門" /* &#x9580; */ },			// もんがまえ、もん	GATE
            { "⾩" /* &#x2FA9; */, "阜" /* &#x961C; */ },			// こざとへん	MOUND
            { "⾪" /* &#x2FAA; */, "隶" /* &#x96B6; */ },			// れいづくり	SLAVE
            { "⾫" /* &#x2FAB; */, "隹" /* &#x96B9; */ },			// ふるとり	SHORT TAILED BIRD
            { "⾬" /* &#x2FAC; */, "雨" /* &#x96E8; */ },			// あめ、あめかんむり	RAIN
            { "⻗" /* &#x2ED7; */, "雨" /* &#x96E8; */ },			// あめ、あめかんむり	(CJK) RAIN
            { "⾭" /* &#x2FAD; */, "靑" /* &#x9751; */ },			// あお	BLUE
            { "⻘" /* &#x2ED8; */, "青" /* &#x9752; */ },			// あお	(CJK) BLUE
            { "⾮" /* &#x2FAE; */, "非" /* &#x975E; */ },			// ひ、あらず	WRONG
            { "⻫" /* &#x2EEB; */, "斉" /* &#x6589; */ },			// せい	(CJK) J-SIMPLIFIED EVEN
            { "⻟" /* &#x2EDF; */, "飠" /* &#x98E0; */ },			// しょくへん	(CJK) EAT THREE
            // 【9画】
            { "⾯" /* &#x2FAF; */, "面" /* &#x9762; */ },			// めん	FACE
            { "⾰" /* &#x2FB0; */, "革" /* &#x9769; */ },			// かわ	LEATHER
            { "⾱" /* &#x2FB1; */, "韋" /* &#x97CB; */ },			// なめしがわ	TANNED LEATHER
            { "⾲" /* &#x2FB2; */, "韭" /* &#x97ED; */ },			// にら	LEEK
            { "⾳" /* &#x2FB3; */, "音" /* &#x97F3; */ },			// おと	SOUND
            { "⾴" /* &#x2FB4; */, "頁" /* &#x9801; */ },			// おおがい、いちのかい	LEAF
            { "⾵" /* &#x2FB5; */, "風" /* &#x98A8; */ },			// かぜ	WIND
            { "⾶" /* &#x2FB6; */, "飛" /* &#x98DB; */ },			// とぶ	FLY
            { "⾷" /* &#x2FB7; */, "食" /* &#x98DF; */ },			// しょく、しょくへん	EAT
            { "⻝" /* &#x2EDD; */, "食" /* &#x98DF; */ },			// しょくへん	(CJK) EAT ONE
            { "⻞" /* &#x2EDE; */, "𩙿" /* &#x2967F; */ },			// しょくへん	(CJK) EAT TWO
            { "⾸" /* &#x2FB8; */, "首" /* &#x9996; */ },			// くび	HEAD
            { "⾹" /* &#x2FB9; */, "香" /* &#x9999; */ },			// かおり	FRAGRANT
            // 【10画】
            { "⾺" /* &#x2FBA; */, "馬" /* &#x99AC; */ },			// うま、うまへん	HORSE
            { "⾻" /* &#x2FBB; */, "骨" /* &#x9AA8; */ },			// ほね、ほねへん	BONE
            { "⾼" /* &#x2FBC; */, "高" /* &#x9AD8; */ },			// たかい	TALL
            { "⾽" /* &#x2FBD; */, "髟" /* &#x9ADF; */ },			// かみがしら、かみかんむり	HAIR
            { "⾾" /* &#x2FBE; */, "鬥" /* &#x9B25; */ },			// とうがまえ、たたかいがまえ	FIGHT
            { "⾿" /* &#x2FBF; */, "鬯" /* &#x9B2F; */ },			// ちょう、においざけ	SACRIFICIAL WINE
            { "⿀" /* &#x2FC0; */, "鬲" /* &#x9B32; */ },			// れき、かなえ	CAULDRON
            { "⿁" /* &#x2FC1; */, "鬼" /* &#x9B3C; */ },			// おに、きにょう	GHOST
            { "⻯" /* &#x2EEF; */, "竜" /* &#x9F8D; */ },			// りゅう、たつ	(CJK) J-SIMPLIFIED DRAGON
            // 【11画】
            { "⿂" /* &#x2FC2; */, "魚" /* &#x9B5A; */ },			// うお、うおへん	FISH
            { "⿃" /* &#x2FC3; */, "鳥" /* &#x9CE5; */ },			// とり	BIRD
            { "⻩" /* &#x2EE9; */, "黄" /* &#x9EC4; */ },			// き	(CJK) SIMPLIFIED YELLOW
            { "⿄" /* &#x2FC4; */, "鹵" /* &#x9E75; */ },			// しお	SALT
            { "⿅" /* &#x2FC5; */, "鹿" /* &#x9E7F; */ },			// しか	DEER
            { "⿆" /* &#x2FC6; */, "麥" /* &#x9EA5; */ },			// むぎ、むぎへん	WHEAT
            { "⿇" /* &#x2FC7; */, "麻" /* &#x9EBB; */ },			// あさ	HEMP
            { "⿊" /* &#x2FCA; */, "黒" /* &#x9ED1; */ },			// くろ	BLACK
            { "⻲" /* &#x2EF2; */, "亀" /* &#x4E80; */ },			// かめ	(CJK) J-SIMPLIFIED TURTLE
            // 【12画】
            { "⿈" /* &#x2FC8; */, "黃" /* &#x9EC3; */ },			// き	YELLOW
            { "⿉" /* &#x2FC9; */, "黍" /* &#x9ECD; */ },			// きび	MILLET
            { "⿋" /* &#x2FCB; */, "黹" /* &#x9EF9; */ },			// ふつへん、ぬいとり	EMBROIDERY
            { "⻭" /* &#x2EED; */, "歯" /* &#x6B6F; */ },			// は	(CJK) J-SIMPLIFIED TOOTH
            // 【13画】
            { "⿌" /* &#x2FCC; */, "黽" /* &#x9EFD; */ },			// べん、かえる	FROG
            { "⿍" /* &#x2FCD; */, "鼎" /* &#x9F0E; */ },			// かなえ、てい	TRIPOD
            { "⿎" /* &#x2FCE; */, "鼓" /* &#x9F13; */ },			// つづみ	DRUM
            { "⿏" /* &#x2FCF; */, "鼠" /* &#x9F20; */ },			// ねずみ、ねずみへん	RAT
            // 【14画】
            { "⿐" /* &#x2FD0; */, "鼻" /* &#x9F3B; */ },			// はな	NOSE
            { "⿑" /* &#x2FD1; */, "齊" /* &#x9F4A; */ },			// せい	EVEN
            // 【15画】
            { "⿒" /* &#x2FD2; */, "齒" /* &#x9F52; */ },			// は	TOOTH
            // 【16画】					
            { "⿓" /* &#x2FD3; */, "龍" /* &#x9F8D; */ },			// りゅう、たつ	DRAGON
            { "⿔" /* &#x2FD4; */, "龜" /* &#x9F9C; */ },			// かめ	TURTLE
            // 【17画】
            { "⿕" /* &#x2FD5; */, "龠" /* &#x9FA0; */ },			// やく、ふえ	FLUTE
        };
    }

    class NumberConverter
    {
        static Regex isNumber = new Regex(@",名詞,数,");
        static Dictionary<char, long> kansujiToLong = new Dictionary<char, long>()
        {
            { '〇', 0 }, { '一', 1 }, { '二', 2 }, { '三', 3 }, { '四', 4 },
            { '五', 5 }, { '六', 6 }, { '七', 7 }, { '八', 8 }, { '九', 9 },
            { '壱', 1 }, { '弐', 2 }, { '参', 3 },
        };
        static Dictionary<char, long> smallKuraiToLong = new Dictionary<char, long>()
        {
            { '十', 10 }, { '百', 100 }, { '千', 1000 }, { '拾', 10 },
        };
        static Dictionary<char, long> bigKuraiToLong = new Dictionary<char, long>()
        {
            { '万', 10000 }, { '萬', 10000 }, { '億', 100000000 }, { '兆', 1000000000000 },
        };

        static string posNum31 = ",名詞,数,３１,*,*,*,";        // month, hour, date or small number, japanese year
        static string posNum999 = ",名詞,数,９９９,*,*,*,";     // min, sec, day or countable
        static string posNumBig = ",名詞,数,大,*,*,*,";        // year, price
        
        // 0:DIsplay, 1:POS1, 2:POS2, 3:POS3, 4:POS4, 5:POS5, 6:POS6, 7:Display, 8:Reading: 9:Speachi

        public void Convert(List<string> document)
        {
            for (int i = document.Count - 1; i >= 0; --i)
            {
                var result = TryConvertToNumber(document, i, "");
                if (result.Item1)
                {
                    var posText = (result.Item2 <= 31) ? posNum31 : (result.Item2 <= 1000) ? posNum999 : posNumBig;
                    if (i == result.Item3)
                    {
                        var orgWord = document[i];
                        var splitParts = orgWord.Split(',');
                        if (splitParts.Length > 9)
                        {
                            var newWord = $"{splitParts[0]}{posText}{splitParts[0]},{splitParts[8]},{splitParts[9]}";
                            document[i] = newWord;
                        }
                        else
                        {
                            var newReading = WinApiBridge.Han2Zen(result.Item2.ToString());
                            var newWord = $"{result.Item4}{posText}{result.Item4},{newReading},{newReading}";
                            document[i] = newWord;
                        }
                    }
                    else
                    {
                        var newReading = WinApiBridge.Han2Zen(result.Item2.ToString());
                        var newWord = $"{result.Item4}{posText}{result.Item4},{newReading},{newReading}";

                        // replace
                        document.RemoveRange(result.Item3, i - result.Item3 + 1);
                        document.Insert(result.Item3, newWord);
                        i = result.Item3;
                    }
                }
            }
        }

        Tuple<bool, long, int, string> TryConvertToNumber(List<string> document, int index, string rightPart)
        {
            var taggedWord = document[index];
            if (!isNumber.IsMatch(taggedWord))
            {
                return new Tuple<bool, long, int, string>(false, -1L, -1, null);
            }
            var numberText = document[index].Split(',')[0] + rightPart;
            if (index > 0)
            {
                var extendResult = this.TryConvertToNumber(document, index - 1, numberText + rightPart);
                if (extendResult.Item1)
                {
                    return extendResult;
                }
            }

            var parseResult = this.TryParseNumber(numberText);

            if (parseResult.Item1)
            {
                return new Tuple<bool, long, int, string>(true, parseResult.Item2, index, parseResult.Item3);
            }
            return new Tuple<bool, long, int, string>(false, -1L, -1, null);
        }

        public Tuple<bool, long, string> TryParseNumber(string source)
        {
            var currentBigNumber = -1L;
            var currentSmallNumber = -1L;
            var currentNumber = -1L;
            var allowedSmallNumber = -1L;
            var allowedBigNumber = -1L;
            foreach (var ch in source)
            {
                if (ch >= '０' && ch <= '９')
                {
                    // TODO: consider mix arabic and Kansuji
                    currentNumber = Math.Max(currentNumber, 0L) * 10 + (ch - '０');
                    if ((allowedSmallNumber >= 0 && currentNumber >= allowedSmallNumber) ||
                        (allowedBigNumber >= 0 && currentNumber >= allowedBigNumber))
                    {
                        return new Tuple<bool, long, string>(false, -1, null);
                    }
                    continue;
                }
                long kansujiNumber = 0;
                if (kansujiToLong.TryGetValue(ch, out kansujiNumber))
                {
                    // TODO: consider mix arabic and Kansuji
                    currentNumber = Math.Max(currentNumber, 0L) * 10 + kansujiNumber;
                    if ((allowedSmallNumber >= 0 && currentNumber >= allowedSmallNumber) ||
                        (allowedBigNumber >= 0 && currentNumber >= allowedBigNumber))
                    {
                        return new Tuple<bool, long, string>(false, -1, null);
                    }
                }
                long smallKuraiNumber = 0;
                if (smallKuraiToLong.TryGetValue(ch, out smallKuraiNumber))
                {
                    long addingNumber = (currentNumber >= 0) ? (currentNumber * smallKuraiNumber) : smallKuraiNumber;
                    if ((allowedSmallNumber >= 0 && addingNumber >= allowedSmallNumber) ||
                        (allowedBigNumber >= 0 && addingNumber >= allowedBigNumber))
                    {
                        return new Tuple<bool, long, string>(false, -1, null);
                    }
                    currentNumber = -1L;
                    allowedSmallNumber = smallKuraiNumber;
                    continue;
                }
                long bigKuraiNumber = 0;
                if (bigKuraiToLong.TryGetValue(ch, out bigKuraiNumber))
                {
                    if (currentSmallNumber >= 0 || currentNumber >= 0)
                    {
                        var addingNumber = (Math.Max(currentSmallNumber, 0) + Math.Max(currentNumber, 0)) * bigKuraiNumber;
                        if (allowedBigNumber >= 0 && addingNumber >= allowedBigNumber)
                        {
                            return new Tuple<bool, long, string>(false, -1, null);
                        }
                        currentBigNumber = Math.Max(currentBigNumber, 0) + addingNumber;
                        currentNumber = -1L;
                        currentSmallNumber = -1L;
                        allowedBigNumber = bigKuraiNumber;
                        allowedSmallNumber = -1;
                        continue;
                    }
                    return new Tuple<bool, long, string>(false, -1, null);
                }
                return new Tuple<bool, long, string>(false, -1, null);
            }
            var resultNumber = Math.Max(currentBigNumber, 0) + Math.Max(currentSmallNumber, 0) + Math.Max(currentNumber, 0);
            return new Tuple<bool, long, string>(true, resultNumber, source);
        }
    }

}
