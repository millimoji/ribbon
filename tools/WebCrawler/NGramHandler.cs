using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;


namespace Ribbon.WebCrawler
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

        const string BOS = "[BOS]";
        const string EOS = "[EOS]";
        const string unigramFileName = "unigram.txt";
        const string bigramFileName = "bigram.txt";
        const string trigramFileName = "trigram.txt";
        const string n4gramFileName = "n4gram.txt";
        const string n5gramFileName = "n5gram.txt";
        const string n6gramFileName = "n6gram.txt";
        const string n7gramFileName = "n7gram.txt";
        const string topicModelFileName = "topicmodel.txt";
        readonly string[] fileNames = new string[] { unigramFileName, bigramFileName, trigramFileName, n4gramFileName, n5gramFileName, n6gramFileName, n7gramFileName, topicModelFileName };
        const string doHalfFileName = "dohalf";
        const string old3prefix = "old3-";
        const string old2prefix = "old2-";
        const string old1prefix = "old-";

        TopicModelHandler m_topicModel;


        string m_workDir;
        public string DateTimeString()
        {
            return DateTime.Now.ToString().Replace(' ', '-').Replace('/', '-').Replace(':', '-');
        }

        public NGramStore(string workDir)
        {
            m_workDir = workDir;
            m_topicModel = new TopicModelHandler();
        }

        public void SlideDataFile()
        {
            foreach (var fileName in fileNames)
            {
                try {
                    File.Delete(m_workDir + old3prefix + fileName);
                } catch (Exception) { }
                try {
                    File.Move(m_workDir + old2prefix + fileName, m_workDir + old3prefix + fileName);
                } catch (Exception) { }
                try {
                    File.Move(m_workDir + old1prefix + fileName, m_workDir + old2prefix + fileName);
                } catch (Exception) { }
                try {
                    File.Move(m_workDir + fileName, m_workDir + old1prefix + fileName);
                } catch (Exception) { }
                try {
                    File.Delete(m_workDir + fileName);// just in case
                } catch (Exception) { }
            }
        }

        public void SaveFile()
        {
            SlideDataFile();

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

            m_topicModel.SaveToFile(m_workDir + topicModelFileName, (int id) => this.WordList[id]);
        }

        public void LoadFromFile()
        {
            long divNum = 1;
            if (File.Exists(m_workDir + doHalfFileName))
            {
                divNum = 2;
                File.Delete(m_workDir + doHalfFileName);
            }

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
                                hashValue = hashValue / divNum;
                                if (hashValue <= 0)
                                {
                                    continue;
                                }

                                hashKeyList[0] = WordToWordId(nGramLine[0]);
                                if (nGram >= 2) { hashKeyList[1] = WordToWordId(nGramLine[1]); }
                                if (nGram >= 3) { hashKeyList[2] = WordToWordId(nGramLine[2]); }
                                if (nGram >= 4) { hashKeyList[3] = WordToWordId(nGramLine[3]); }
                                if (nGram >= 5) { hashKeyList[4] = WordToWordId(nGramLine[4]); }
                                if (nGram >= 6) { hashKeyList[5] = WordToWordId(nGramLine[5]); }
                                if (nGram >= 7) { hashKeyList[6] = WordToWordId(nGramLine[6]); }
                                AddNgram(nGram, hashKeyList, hashValue);
                            }
                        }
                    }
                }
                catch (Exception) { }
            }

            this.m_topicModel.LoadFromFile(this.m_workDir + topicModelFileName, (string word) => this.WordToWordId(word, false));
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

        string inputFilename;
        string outputFilename;
        List<List<string>> result = new List<List<string>>();

        public MorphAnalyzer(string workingFolder)
        {
            string threadId = System.Threading.Thread.CurrentThread.ManagedThreadId.ToString();
            string dateText = DateTimeString();
            inputFilename = workingFolder + dateText + "-" + threadId + "-in.txt";
            outputFilename = workingFolder + dateText + "-" + threadId + "-out.txt";
        }

        public string DateTimeString()
        {
            return DateTime.Now.ToString().Replace(' ', '-').Replace('/', '-').Replace(':', '-');
        }


        public List<List<string>> Run(HashSet<string> srcText)
        {
            WriteInputFile(srcText);

            LaunchMecab();

            ReadOutputFile();

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
                    writeStream.WriteLine(newText);
                }
            }
        }

        private void LaunchMecab()
        {
            string parameters = string.Format("--input-buffer-size={0} --output={1} {2}", 0x8000, outputFilename, inputFilename);

            System.Diagnostics.ProcessStartInfo processStart = new System.Diagnostics.ProcessStartInfo(Program.mecabExe, parameters);
            processStart.CreateNoWindow = true;
            processStart.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;

            System.Diagnostics.Process p = System.Diagnostics.Process.Start(processStart);
            p.WaitForExit();
        }

        private void ReadOutputFile()
        {
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

#if false
                    string[] properties = outPair[1].Split(',');
                    foreach (string orgProp in properties)
                    {
                        oneWord += "," + Strings.StrConv(orgProp, VbStrConv.Wide);
                    }
#else
                    oneWord += "," + outPair[1];
#endif
                    building.Add(oneWord);
                }
            }
        }
    }

}
