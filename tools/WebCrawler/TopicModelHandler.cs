using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Ribbon.WebCrawler
{
    class TopicModelState
    {
        // for mixture model;
        private double[] topicProb = new double[TopicModelHandler.TopicCount];

        // for topic model
        private double[][] topicProbs;

        public Dictionary<int, double[]> wordProbs;
        public Dictionary<int, double[]> nextWordProbs;
        
        private System.Random random = new System.Random();

        public TopicModelState()
        {
            this.wordProbs = new Dictionary<int, double[]>();
        }

        public void SaveToFile(string fileName, Func<int, string> id2word)
        {
            using (StreamWriter fileStream = new StreamWriter(fileName, false, Encoding.Unicode))
            {
                foreach (var item in this.wordProbs)
                {
                    var wordText = id2word(item.Key);
                    var outputLine = wordText;
                    foreach (var v in item.Value)
                    {
                        var sringify = v.ToString();
                        outputLine += "\t" + sringify;
                        if (Convert.ToDouble(sringify) < 0.0)
                        {
                            throw new Exception("invalid format");
                        }
                    }
                    fileStream.WriteLine(outputLine);
                }
            }
        }

        public bool LoadFromFile(string fileName, Func<string, int> word2id, Func<int, string> id2word)
        {
            if (!File.Exists(fileName))
            {
                return false;
            }
            using (StreamReader fileStream = new StreamReader(fileName))
            {
                this.wordProbs = new Dictionary<int, double[]>(); // clear
                string line;
                while ((line = fileStream.ReadLine()) != null)
                {
                    var wordLine = line.Split('\t');
                    if (!TopicModelHandler.isTargetWord(wordLine[0]))
                    {
                        continue;
                    }
                    var wordId = word2id(wordLine[0]);
                    if (wordId < 0)
                    {
                        continue;
                    }
                    var wordProb = new double[TopicModelHandler.TopicCount];
                    this.wordProbs.Add(wordId, wordProb);
                    TopicModelHandler.DoubleForEach(wordProb, (double x, int idx) =>
                    {
                        return (idx < (wordLine.Length - 1)) ? Convert.ToDouble(wordLine[idx + 1]) : this.GetRandomNumber();
                    });
                }
                this.NormalizeWordList(this.wordProbs);
            }

            return true;
        }

        #region MixtureUnigramModel
        public bool PrepareMixtureUnigramModel(List<List<int>> documents)
        {
            Func<bool> MakeInitialTopic = () =>
            {
                bool foundAtLeast = false;
                foreach (var doc in documents)
                {
                    foreach (var wordId in doc)
                    {
                        double[] wordProb;
                        if (this.wordProbs.TryGetValue(wordId, out wordProb))
                        {
                            var loggedSum = Math.Log(wordProb.Sum());
                            if (!foundAtLeast)
                            {
                                TopicModelHandler.DoubleForEach(this.topicProb, (double x, int idx) => Math.Log(wordProb[idx]) - loggedSum);
                                foundAtLeast = true;
                            }
                            else
                            {
                                TopicModelHandler.DoubleForEach(this.topicProb, (double x, int idx) => x + Math.Log(wordProb[idx]) - loggedSum);
                            }
                        }
                    }
                }

                if (foundAtLeast)
                {
                    var loggedTpSum = this.topicProb[0];
                    for (int i = 1; i < this.topicProb.Length; ++i) loggedTpSum = this.AddLogedProb(loggedTpSum, this.topicProb[i]);
                    TopicModelHandler.DoubleForEach(this.topicProb, (double x, int idx) => Math.Exp(x - loggedTpSum));
                    this.NormalizeDoubleList(this.topicProb);
                    //// to suppress touch 0
                    //TopicModelHandler.DoubleForEach(this.topicProb, (double x, int idx) => x + 0.01);
                    //this.NormalizeDoubleList(this.topicProb);
                }
                else
                {
                    TopicModelHandler.DoubleForEach(this.topicProb, (double x, int idx) => this.GetRandomNumber());
                    this.NormalizeDoubleList(this.topicProb);
                }
                return true;
            };

            if (this.wordProbs.Count == 0 || !MakeInitialTopic())
            {
                TopicModelHandler.DoubleForEach(this.topicProb, (double x, int idx) => this.GetRandomNumber());
                this.NormalizeDoubleList(this.topicProb);
            }

            this.PrepareForNewWordSet(documents);
            return true;
        }

        public double CalculateMixtureUnigramModel(List<List<int>> documents, double oldRate)
        {
            var likeliHood = 0.0;
            var wordCount = 0;

            double[] nextTopic = new double[TopicModelHandler.TopicCount];
            TopicModelHandler.DoubleForEach(nextTopic, (double x, int idx) => 0.0);
            foreach (var kv in this.nextWordProbs)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) => 0.0);
            }

            double[] qWork = new double[TopicModelHandler.TopicCount];

            foreach (var doc in documents)
            {
                // 
                TopicModelHandler.DoubleForEach(qWork, (double x, int idx) => 0.0);
                foreach (var wordId in doc)
                {
                    var wordPobs = this.wordProbs[wordId];
                    TopicModelHandler.DoubleForEach(qWork, (double x, int idx) => x + Math.Log(Math.Max(wordPobs[idx], 1.0 / Single.MaxValue)));
                }
                var loggedSum = qWork[0];
                for (int i = 0; i < TopicModelHandler.TopicCount; ++i)
                {
                    loggedSum = this.AddLogedProb(loggedSum, qWork[i]);
                }
                TopicModelHandler.DoubleForEach(qWork, (double x, int idx) => Math.Exp(x - loggedSum));
                likeliHood += loggedSum;
                wordCount += doc.Count;

                // apply to next
                TopicModelHandler.DoubleForEach(nextTopic, (double x, int idx) => x + qWork[idx]);
                foreach (var wordId in doc)
                {
                    var nextWordProb = this.nextWordProbs[wordId];
                    TopicModelHandler.DoubleForEach(nextWordProb, (double x, int idx) => x + qWork[idx]);
                }
            }

            // apply to main
            this.topicProb = nextTopic;

            this.NormalizeWordList(this.nextWordProbs);
            this.MergeWordList(oldRate, this.wordProbs, this.nextWordProbs);
            this.NormalizeWordList(this.wordProbs);

            var perplexity = Math.Exp(-likeliHood / wordCount);
            return perplexity;
        }
        #endregion

        #region TopicModel
        public bool PrepareTopicModel(List<List<int>> documents)
        {
            this.topicProbs = new double[documents.Count][];
            for (var docIdx = 0; docIdx < documents.Count; ++docIdx)
            {
                var tmTopicProb = new double[TopicModelHandler.TopicCount];
                this.topicProbs[docIdx] = tmTopicProb;

                var doc = documents[docIdx];
                bool foundAtLeast = false;

                foreach (var wordId in doc)
                {
                    double[] wordProb;
                    if (this.wordProbs.TryGetValue(wordId, out wordProb))
                    {
                        var loggedSum = Math.Log(wordProb.Sum());
                        if (!foundAtLeast)
                        {
                            foundAtLeast = true;
                            TopicModelHandler.DoubleForEach(tmTopicProb, (double x, int idx) => Math.Log(wordProb[idx]) - loggedSum);
                        }
                        else
                        {
                            TopicModelHandler.DoubleForEach(tmTopicProb, (double x, int idx) => x + Math.Log(wordProb[idx]) - loggedSum);
                        }
                    }
                }

                if (foundAtLeast)
                {
                    var loggedTpSum = tmTopicProb[0];
                    for (int i = 1; i < tmTopicProb.Length; ++i) loggedTpSum = this.AddLogedProb(loggedTpSum, tmTopicProb[i]);
                    TopicModelHandler.DoubleForEach(tmTopicProb, (double x, int idx) => Math.Exp(x - loggedTpSum));
                    this.NormalizeDoubleList(tmTopicProb);
                    //// to suppress touch 0
                    //TopicModelHandler.DoubleForEach(tmTopicProb, (double x, int idx) => x + 0.01);
                    //this.NormalizeDoubleList(tmTopicProb);
                }
                else
                {
                    TopicModelHandler.DoubleForEach(tmTopicProb, (double x, int idx) => this.GetRandomNumber());
                    this.NormalizeDoubleList(tmTopicProb);
                }
            }

            this.PrepareForNewWordSet(documents);
            return true;
        }

        public double CalculateTopicModel(List<List<int>> documents, double oldRate)
        {
            var likeliHood = 0.0;
            var wordCount = 0;

            foreach (var kv in this.nextWordProbs)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) => 0.0);
            }

            var qWork = new double[TopicModelHandler.TopicCount];

            for (int docIdx = 0; docIdx < documents.Count; ++docIdx)
            {
                var doc = documents[docIdx];
                var currentTopic = this.topicProbs[docIdx];

                // 
                TopicModelHandler.DoubleForEach(qWork, (double x, int idx) => 0.0);
                foreach (var wordId in doc)
                {
                    var wordPob = this.wordProbs[wordId];
                    TopicModelHandler.DoubleForEach(qWork, (double x, int idx) =>
                        (x + Math.Log(Math.Max(currentTopic[idx], 1.0 / Single.MaxValue)) + Math.Log(Math.Max(wordPob[idx], 1.0 / Single.MaxValue))));
                }
                var loggedSum = qWork[0];
                for (int i = 0; i < TopicModelHandler.TopicCount; ++i)
                {
                    loggedSum = this.AddLogedProb(loggedSum, qWork[i]);
                }
                TopicModelHandler.DoubleForEach(qWork, (double x, int idx) => Math.Exp(x - loggedSum));
                likeliHood += loggedSum;
                wordCount += doc.Count;

                // apply to next
                TopicModelHandler.DoubleForEach(currentTopic, (double x, int idx) => qWork[idx]);
                this.NormalizeDoubleList(currentTopic);

                foreach (var wordId in doc)
                {
                    var nextWordProb = this.nextWordProbs[wordId];
                    TopicModelHandler.DoubleForEach(nextWordProb, (double x, int idx) => x + qWork[idx]);
                }
            }

            // apply to main
            this.NormalizeWordList(this.nextWordProbs);
            this.MergeWordList(oldRate, this.wordProbs, this.nextWordProbs);
            this.NormalizeWordList(this.wordProbs);

            var perplexity = Math.Exp(-likeliHood / wordCount);
            return perplexity;
        }
        #endregion

        public TopicModelState DeepCopy()
        {
            var cloned = new TopicModelState();
            foreach (var kv in this.wordProbs)
            {
                cloned.wordProbs.Add(kv.Key, (double[])kv.Value.Clone());
            }
            return cloned;
        }

        public void MergeWordList(double oldRate, Dictionary<int, double[]> dstWordProbs, Dictionary<int, double[]> newWordProbs)
        {
            var newRate = 1.0 - oldRate;
            foreach (var kv in dstWordProbs)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) => (x * oldRate));
            }
            foreach (var kv in newWordProbs)
            {
                double[] dstWordProb;
                if (dstWordProbs.TryGetValue(kv.Key, out dstWordProb))
                {
                    TopicModelHandler.DoubleForEach(dstWordProb, (double x, int idx) => (x + kv.Value[idx] * newRate));
                }
                else
                {
                    dstWordProbs.Add(kv.Key, (double[])kv.Value.Clone());
                }
            }
            this.NormalizeWordList(this.wordProbs);
        }

        public void PrepareForNewWordSet(List<List<int>> documents)
        {
            this.nextWordProbs = new Dictionary<int, double[]>(); // reset

            var randomRange = (this.wordProbs.Count == 0) ? 1.0 : (1.0 / TopicModelHandler.estimatedFilledWordCount);
            HashSet<int> idArray = new HashSet<int>();
            foreach (var doc in documents) { foreach (var wordId in doc) { idArray.Add(wordId); } }
            foreach (var wordId in idArray)
            {
                double[] wordProb;
                if (!this.wordProbs.TryGetValue(wordId, out wordProb))
                {
                    wordProb = new double[TopicModelHandler.TopicCount];
                    TopicModelHandler.DoubleForEach(wordProb, (double x, int idx) =>
                        ((double)this.random.Next(1, 999999) / 1000000.0 * randomRange + 1.0 / TopicModelHandler.estimatedFilledWordCount));
                    this.wordProbs.Add(wordId, wordProb);
                }

                // zero clear for next word
                if (!this.nextWordProbs.TryGetValue(wordId, out wordProb))
                {
                    wordProb = new double[TopicModelHandler.TopicCount];
                    this.nextWordProbs.Add(wordId, wordProb);
                }
            }
            this.NormalizeWordList(this.wordProbs);
        }

        public int GetWordCount()
        {
            return this.wordProbs.Count;
        }

        private double AddLogedProb(double logA, double logB)
        {
            if (logA == 0.0)
            {
                return logB;
            }
            // log(exp(a) + exp(b)) = log(exp(a)(1 + exp(b)/exp(a)) = a + log(1 + exp(b - a))
            if (logA < logB)
            { // swap
                var tmp = logA;
                logA = logB;
                logB = tmp;
            }
            return logA + Math.Log(1.0 + Math.Exp(logB - logA));
        }

        private void NormalizeDoubleList(double [] target)
        {
            var sum = Math.Max(target.Sum(), 1.0 / Single.MaxValue);
            TopicModelHandler.DoubleForEach(target, (double x, int idx) => (x / sum));
        }

        private void NormalizeWordList(Dictionary<int, double[]> wordList)
        {
            var sums = new double[TopicModelHandler.TopicCount];
            TopicModelHandler.DoubleForEach(sums, (double x, int idx) => (1.0 / Single.MaxValue));
            foreach (var kv in wordList)
            {
                TopicModelHandler.DoubleForEach(sums, (double x, int idx) => (x + kv.Value[idx]));
            }
            foreach (var kv in wordList)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) => (x / sums[idx]));
            }
        }

        private double GetSmallRandomNumber()
        {
            return (double)random.Next(10, 99999) / (double)100000 * (1.0 / TopicModelHandler.estimatedFilledWordCount) + (1.0 / TopicModelHandler.estimatedFilledWordCount);
        }

        private double GetRandomNumber()
        {
            return (double)random.Next(10, 99999) / (double)100000 + 1.0 / TopicModelHandler.estimatedFilledWordCount;
        }

        private double GetSmallShuffled(double x)
        {
            return x * (1.0 - 0.001 * (double)random.Next(0x10, 0x10000) / (double)0x10000);
        }

    }


    public class TopicModelHandler
    {
        public const int TopicCount = 63; // 255;
        public const double updateMergeRate = 0.5;
        public const double updateUniqueWord = 20000;
        public const int perplexHistMax = 50;
        public const int wordCountRequirement = 4;
        public const double estimatedFilledWordCount = 10.0 * updateUniqueWord;

        private TopicModelState baseState;
        private Queue<double> ppHist = new Queue<double>();
        private List<List<int>> documentHistory = new List<List<int>>();
        private HashSet<int> uniqueWord = new HashSet<int>();
        private bool isMixUniModel = false;
        private string logPrefix;

        public TopicModelHandler(bool isMixUnigram)
        {
            this.isMixUniModel = isMixUnigram;
            this.baseState = new TopicModelState();
            this.logPrefix = this.isMixUniModel ? "MixUnigram" : "TopicModel";

            this.ClearStoredData();
            this.ppHist.Clear();
            this.ppHist.Enqueue(0.0);
        }

        public void LoadFromFile(string fileName, Func<string, int> word2id, Func<int, string> id2word)
        {
            this.ClearStoredData();

            this.baseState.LoadFromFile(fileName, word2id, id2word);
        }

        private void ClearStoredData()
        {
            this.documentHistory.Clear();
            this.uniqueWord.Clear();
        }

        public bool CanSave()
        {
            return this.uniqueWord.Count < (TopicModelHandler.updateUniqueWord * 1 / 5);
        }

        public void SaveToFile(string fileName, string summaryFilename, Func<int, string> id2Word)
        {
            // Stop to flush. This must have bad impact on model.
            // LearnLoopUntilPPTarget();
            this.ClearStoredData();

            this.baseState.SaveToFile(fileName, id2Word);

            {
                var summarizer = new JsonSummarizer();
                summarizer.Serialize(summaryFilename, this.ppHist.Average(), this.baseState.wordProbs, id2Word);
            }
        }

        public void LearnDocument(List<string> document, Func<string, int> word2id)
        {
            var listId = this.WordArrayToIntArray(document, word2id);
            if (listId.Count < wordCountRequirement)
            {
                return; // ignore
            }

            this.documentHistory.Add(listId);
            listId.ForEach(x => this.uniqueWord.Add(x));

            if (this.uniqueWord.Count >= TopicModelHandler.updateUniqueWord)
            {
                LearnLoopUntilPPTarget();
            }
        }

        public void PrintCurretState()
        {
            var lastPp = this.ppHist.Count == 0 ? 0.0 : this.ppHist.Last();
            Console.WriteLine($"[{this.logPrefix}] pp:{lastPp}, word:{this.baseState.GetWordCount()}, dos:{this.documentHistory.Count}, words:{this.uniqueWord.Count}, " +
                    $"pp-ave:{this.ppHist.Average()}, pp-min:{this.ppHist.Min()}");
        }

        private void LearnLoopUntilPPTarget()
        {
            var currentModel = this.baseState.DeepCopy();
            var ppLocalHist = new List<double>();

            var result = this.isMixUniModel ?
                this.baseState.PrepareMixtureUnigramModel(this.documentHistory) :
                this.baseState.PrepareTopicModel(this.documentHistory);

            for (int loopCount = 1; ; ++loopCount)
            {
                var currentPp = this.isMixUniModel ?
                        this.baseState.CalculateMixtureUnigramModel(this.documentHistory, TopicModelHandler.updateMergeRate) :
                        this.baseState.CalculateTopicModel(this.documentHistory, TopicModelHandler.updateMergeRate);

                Console.WriteLine($"[{this.logPrefix}:{loopCount}] pp:{currentPp}, word:{this.baseState.GetWordCount()}");

                if (ppLocalHist.Count > 0)
                {
                    var goNext = ppLocalHist.Skip(Math.Max(ppLocalHist.Count - 5, 0)).All(x => Math.Abs(x - currentPp) < currentPp * 0.1);
                    if (goNext)
                    {
                        break;
                    }
                }
                else
                {
                    if (this.ppHist.Count == 1 && this.ppHist.Last() == 0.0) this.ppHist.Clear();
                    this.ppHist.Enqueue(currentPp);
                    while (this.ppHist.Count > perplexHistMax)
                    {
                        this.ppHist.Dequeue();
                    }
                }
                ppLocalHist.Add(currentPp);
            }

            this.baseState.MergeWordList(TopicModelHandler.updateMergeRate, currentModel.wordProbs, this.baseState.wordProbs);
            this.baseState.wordProbs = currentModel.wordProbs;

            this.ClearStoredData();

        }

        private List<int> WordArrayToIntArray(List<string> document, Func<string, int> word2id)
        {
            return document.Select(word => (TopicModelHandler.isTargetWord(word) ? word2id(word) : -1)).Where(wordId => wordId >= 0).ToList();
        }

        private static Regex matchNoReading = new Regex(@",\*,\*$");
        private static Regex excludedPos = new Regex(@"(,名詞,数,|,名詞,代名詞,一般,|,助詞,|,助動詞,|,接続詞,|,接頭詞,|,記号,|,非自立,|,接尾,|,副詞可能,|,名詞,固有名詞,人名,姓,|,名詞,固有名詞,人名,名,)");
        private static Regex excludedWord = new Regex(@"(" +
                @",動詞,自立,\*,\*,一段,.*,いる,|" +
                @",動詞,自立,\*,\*,サ変・|" +
                @",動詞,自立,\*,\*,カ変・|" +
                @",動詞,自立,\*,\*,五段・ラ行,.*,ある,|" +
                @",動詞,自立,\*,\*,五段・ラ行,.*,なる,|" +
                @",動詞,自立,\*,\*,一段,.*,できる,|" +
                @",動詞,自立,\*,\*,五段・ワ行促音便,.*,行う,|" +
                @",形容詞,自立,\*,\*,形容詞・アウオ段,.*,ない,|" +
                @"^この,連体詞,|" +
                @"^その,連体詞,|" +
                @"^どの,連体詞,|" +
                @"^さらに,副詞,|" +
                @"^もちろん,副詞," +
            ")");

        public static bool isTargetWord(string word)
        {
            return
                TopicModelHandler.matchNoReading.IsMatch(word) ||
                TopicModelHandler.excludedPos.IsMatch(word) ||
                TopicModelHandler.excludedWord.IsMatch(word) ?
                false : true;
        }

        private double GetPerplexyAvarage()
        {
            if (this.ppHist.Count == 0)
            {
                return 0.0;
            }
            while (this.ppHist.Count > perplexHistMax)
            {
                this.ppHist.Dequeue();
            }
            return this.ppHist.Sum() / (double)(this.ppHist.Count);
        }

        public static void DoubleForEach(double [] dst, Func<double, int, double> operation)
        {
            for (int i = 0; i < dst.Length; ++i)
            {
                var x = operation(dst[i], i);
                TopicModelHandler.IsValidNumber(x);
                dst[i] = x;
            }
        }

        public static void IsValidNumber(double db)
        {
            if (Double.IsNaN(db) || Double.IsInfinity(db))
            {
                Console.WriteLine("[Invalid Number]\n");
                throw new Exception("[Invalid Number]");
            }
        }
    }
}
