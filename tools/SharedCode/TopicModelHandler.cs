using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Ribbon.Shared
{
    class TopicModelState
    {
        // for mixture model, topic model uses this also
        private double[] topicProb = new double[TopicModelHandler.TopicCount];

        // for topic model
        private double[][] topicProbs;

        public double[] averageTopicProbs = new double[TopicModelHandler.TopicCount];
        public Dictionary<int, double[]> wordProbs;
        public Dictionary<int, double[]> nextWordProbs;
        
        private System.Random random = new System.Random();

        public TopicModelState()
        {
            this.wordProbs = new Dictionary<int, double[]>();

            TopicModelHandler.DoubleForEach(this.averageTopicProbs, (double x, int idx) => this.GetRandomNumber());
            this.NormalizeDoubleList(this.averageTopicProbs);
        }

        public void SaveToFile(string fileName, Func<int, string> id2word, double avePp, double minPp, double latestPp)
        {
            using (StreamWriter fileStream = new StreamWriter(fileName, false, Encoding.Unicode))
            {
                {
                    var outputLine = "[Perplexity]\t" + String.Join("\t", new double[] { avePp, minPp, latestPp });
                    fileStream.WriteLine(outputLine);
                }

                {
                    var outputLine = "[Topic]\t" + String.Join("\t", this.averageTopicProbs);
                    fileStream.WriteLine(outputLine);
                }

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

        public bool LoadFromFile(string fileName, Func<string, int> word2id, Func<int, string> id2word, ref double [] pps)
        {
            if (!File.Exists(fileName))
            {
                return false;
            }
            using (StreamReader fileStream = new StreamReader(fileName))
            {
                string line = fileStream.ReadLine();
                string[] headerLine = line.Split('\t');

                if (headerLine[0] == "[Perplexity]")
                {
                    pps[0] = Convert.ToDouble(headerLine[1]); // ave
                    pps[1] = Convert.ToDouble(headerLine[2]); // min
                    pps[2] = Convert.ToDouble(headerLine[3]); // latest

                    // ignore just skip
                    line = fileStream.ReadLine();
                    headerLine = line.Split('\t');
                }

                if (headerLine[0] != "[Topic]")
                {
                    return false;
                }

                TopicModelHandler.DoubleForEach(this.averageTopicProbs, (double x, int idx) =>
                {
                    return (idx < (headerLine.Length - 1)) ? Convert.ToDouble(headerLine[idx + 1]) : this.GetRandomNumber();
                });
                this.NormalizeDoubleList(this.averageTopicProbs);

                this.wordProbs = new Dictionary<int, double[]>(); // clear
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
        public bool PrepareMixtureUnigramModel(HashSet<HashSet<int>> documents)
        {
            Func<bool> MakeInitialTopic = () =>
            {
                TopicModelHandler.DoubleForEach(this.topicProb, (double x, int idx) => Math.Log(this.averageTopicProbs[idx]));

                bool foundAtLeast = false;
                foreach (var doc in documents)
                {
                    foreach (var wordId in doc)
                    {
                        double[] wordProb;
                        if (this.wordProbs.TryGetValue(wordId, out wordProb))
                        {
                            var loggedSum = Math.Log(wordProb.Sum());
                            TopicModelHandler.DoubleForEach(this.topicProb, (double x, int idx) => x + Math.Log(wordProb[idx]) - loggedSum);
                            foundAtLeast = true;
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

        public double CalculateMixtureUnigramModel(HashSet<HashSet<int>> documents, double oldRate)
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
            this.NormalizeDoubleList(this.topicProb);

            this.NormalizeWordList(this.nextWordProbs);
            this.MergeWordList(oldRate, this.wordProbs, this.nextWordProbs);
            this.NormalizeWordList(this.wordProbs);

            var perplexity = Math.Exp(-likeliHood / wordCount);
            return perplexity;
        }

        public bool FinalizeMixUnigram(double keepRate)
        {
            var newRate = 1.0 - keepRate;
            TopicModelHandler.DoubleForEach(this.averageTopicProbs, (double x, int idx) => x * keepRate + this.topicProb[idx] * newRate);
            this.NormalizeDoubleList(this.averageTopicProbs);
            return true;
        }

        #endregion

        #region TopicModel
        public bool PrepareTopicModel(HashSet<HashSet<int>> documents)
        {
            var qWork = new double[TopicModelHandler.TopicCount];
            this.topicProbs = new double[documents.Count][];
            int docIdx = -1;
            foreach (var doc in documents)
            {
                docIdx++;
                bool foundAtLeast = false;

                var tmTopicProb = this.topicProbs[docIdx] = new double[TopicModelHandler.TopicCount];
                TopicModelHandler.DoubleForEach(tmTopicProb, (double x, int idx) => 0.0);

                foreach (var wordId in doc)
                {
                    double[] wordProb;
                    if (this.wordProbs.TryGetValue(wordId, out wordProb))
                    {
                        TopicModelHandler.DoubleForEach(qWork, (double x, int idx) => this.averageTopicProbs[idx] * wordProb[idx]);
                        var sum = qWork.Sum();
                        TopicModelHandler.DoubleForEach(qWork, (double x, int idx) => x / sum);
                        foundAtLeast = true;

                        TopicModelHandler.DoubleForEach(tmTopicProb, (double x, int idx) => x + qWork[idx]);
                    }
                }

                if (foundAtLeast)
                {
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

        public double CalculateTopicModel(HashSet<HashSet<int>> documents, double oldRate)
        {
            var likeliHood = 0.0;
            var wordCount = 0;

            foreach (var kv in this.nextWordProbs)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) => 0.0);
            }

            var qWork = new double[TopicModelHandler.TopicCount];
            var nextTopicProbs = new double[TopicModelHandler.TopicCount];

            var docIdx = -1;
            foreach (var doc in documents)
            {
                docIdx++;
                var currentTopic = this.topicProbs[docIdx];
                wordCount += doc.Count;

                // 
                TopicModelHandler.DoubleForEach(qWork, (double x, int idx) => 0.0);
                TopicModelHandler.DoubleForEach(nextTopicProbs, (double x, int idx) => 0.0);

                foreach (var wordId in doc)
                {
                    var wordPob = this.wordProbs[wordId];
                    TopicModelHandler.DoubleForEach(qWork, (double x, int idx) => currentTopic[idx] * wordPob[idx]);
                    var qSum = qWork.Sum();
                    TopicModelHandler.DoubleForEach(qWork, (double x, int idx) => x / qSum);

                    // update other words
                    TopicModelHandler.DoubleForEach(nextTopicProbs, (double x, int idx) => x + qWork[idx]);
                    foreach (var coOccuredWordId in doc)
                    {
                        var nextCoOccurdWordProbs = this.nextWordProbs[coOccuredWordId];
                        TopicModelHandler.DoubleForEach(nextCoOccurdWordProbs, (double x, int idx) => x + qWork[idx]);
                    }
                    likeliHood += Math.Log(qSum);
                }

                // apply to next topic, because this topic prob is not used at rest of logic. and calcurate likelihood
                TopicModelHandler.DoubleForEach(currentTopic, (double x, int idx) => nextTopicProbs[idx]);
                this.NormalizeDoubleList(currentTopic);
            }

            // apply to main
            this.NormalizeWordList(this.nextWordProbs);
            this.MergeWordList(oldRate, this.wordProbs, this.nextWordProbs);
            this.NormalizeWordList(this.wordProbs);

            var perplexity = Math.Exp(-likeliHood / wordCount);
            return perplexity;
        }

        public bool FinalizeTopicModel(double keepRate)
        {
            TopicModelHandler.DoubleForEach(this.topicProb, (double x, int idx) => 0.0);
            foreach (var prob in this.topicProbs)
            {
                TopicModelHandler.DoubleForEach(this.topicProb, (double x, int idx) => x + prob[idx]);
            }
            this.NormalizeDoubleList(this.topicProb);

            var revDocCount = 1.0 / (double)this.topicProbs.Length;
            var newRate = 1.0 - keepRate;
            TopicModelHandler.DoubleForEach(this.averageTopicProbs, (double x, int idx) => x * keepRate + this.topicProb[idx] * newRate);
            this.NormalizeDoubleList(this.averageTopicProbs);
            return true;
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

        public void PrepareForNewWordSet(HashSet<HashSet<int>> documents)
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
        public const int TopicCount = 63;
        public const double updateMergeRate = 0.5;
        public const double updateUniqueWord = 30000;
        public const int perplexHistMax = 50;
        public const int wordCountRequirement = 4;
        public const double estimatedFilledWordCount = 10.0 * updateUniqueWord;

        private TopicModelState baseState;
        private Queue<double> ppHist = new Queue<double>();
        private HashSet<HashSet<int>> documentHistory = null;
        private HashSet<HashSet<int>> lastDocumentHistory = null;
        private HashSet<int> uniqueWord = new HashSet<int>();
        private bool isMixUniModel = false;
        private string logPrefix;
        private double[] loadedPp = new double[] { 0.0, 0.0, 0.0 };

        class HashSetIntComparer : IEqualityComparer<HashSet<int>>
        {
            public bool Equals(HashSet<int> l, HashSet<int> r)
            {
                return l.SetEquals(r);
            }
            public int GetHashCode(HashSet<int> h)
            {
                return h.Sum().GetHashCode();
            }
        }

        public TopicModelHandler(bool isMixUnigram)
        {
            this.isMixUniModel = isMixUnigram;
            this.baseState = new TopicModelState();
            this.logPrefix = this.isMixUniModel ? "MixUnigram" : "TopicModel";
            this.documentHistory = new HashSet<HashSet<int>>(new HashSetIntComparer());
            this.lastDocumentHistory = null;

            this.ClearStoredData();
            this.ppHist.Clear();
            this.ppHist.Enqueue(0.0);
        }

        public Dictionary<int, double[]> wordProbsMatrix
        {
            get { return this.baseState.wordProbs; }
        }

        public double[] topicProbsArray
        {
            get { return this.baseState.averageTopicProbs; }
        }

        public double lastPerplexicity
        {
            get { return this.ppHist.Last(); }
        }

        public double [] loadedPerplexities
        {
            get {  return this.loadedPp; }
        }

        public void LoadFromFile(string fileName, Func<string, int> word2id, Func<int, string> id2word)
        {
            this.ClearStoredData();
                        
            this.baseState.LoadFromFile(fileName, word2id, id2word, ref this.loadedPp);
        }

        private void ClearStoredData()
        {
            this.documentHistory.Clear();
            this.lastDocumentHistory = null;
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

            this.baseState.SaveToFile(fileName, id2Word, this.GetPerplexyAverage(), this.ppHist.Min(), this.ppHist.Last());
        }

        public void LearnDocument(List<string> document, Func<string, int> word2id)
        {
            var hashId = this.WordArrayToIntHash(document, word2id);
            if (hashId.Count < wordCountRequirement)
            {
                return; // ignore
            }

            this.documentHistory.Add(hashId);
            this.uniqueWord.UnionWith(hashId);

            if (this.uniqueWord.Count >= TopicModelHandler.updateUniqueWord)
            {
                LearnLoopUntilPPTarget();
            }
        }

        public void PrintCurretState()
        {
            var lastPp = this.ppHist.Count == 0 ? 0.0 : this.ppHist.Last();
            Console.WriteLine($"[{this.logPrefix}] pp:{lastPp}, word:{this.baseState.GetWordCount()}, docs:{this.documentHistory.Count}, words:{this.uniqueWord.Count}, " +
                    $"pp-ave:{this.ppHist.Average()}, pp-min:{this.ppHist.Min()}");
        }

        private void LearnLoopUntilPPTarget()
        {
            var currentModel = this.baseState.DeepCopy();
            var ppLocalHist = new List<double>();

            var mergedDocuments = new HashSet<HashSet<int>>(this.documentHistory, new HashSetIntComparer());
            if (this.lastDocumentHistory != null)
            {
                mergedDocuments.UnionWith(this.lastDocumentHistory);
            }

            var result = this.isMixUniModel ?
                this.baseState.PrepareMixtureUnigramModel(mergedDocuments) :
                this.baseState.PrepareTopicModel(mergedDocuments);

            for (int loopCount = 1; ; ++loopCount)
            //for (int loopCount = 1; loopCount < 3; ++loopCount)
            {
                var currentPp = this.isMixUniModel ?
                        this.baseState.CalculateMixtureUnigramModel(mergedDocuments, TopicModelHandler.updateMergeRate) :
                        this.baseState.CalculateTopicModel(mergedDocuments, TopicModelHandler.updateMergeRate);

                Console.WriteLine($"[{this.logPrefix}:{loopCount}] pp:{currentPp}, word:{this.baseState.GetWordCount()}");

                if (ppLocalHist.Count > 0)
                {
#if true
                    var goNext = ppLocalHist.Skip(Math.Max(ppLocalHist.Count - 5, 0)).All(x => Math.Abs(x - currentPp) < currentPp * 0.1);
                    if (goNext)
                    {
                        break;
                    }
#endif
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

            result = this.isMixUniModel ?
                this.baseState.FinalizeMixUnigram(TopicModelHandler.updateMergeRate) :
                this.baseState.FinalizeTopicModel(TopicModelHandler.updateMergeRate);

            // this.ClearStoredData();
            this.lastDocumentHistory = this.documentHistory;
            this.documentHistory = new HashSet<HashSet<int>>(new HashSetIntComparer());
            this.uniqueWord.Clear();
        }

        private HashSet<int> WordArrayToIntHash(List<string> document, Func<string, int> word2id)
        {
            return document.Select(word => (TopicModelHandler.isTargetWord(word) ? word2id(word) : -1)).Where(wordId => wordId >= 0).ToHashSet();
        }

        private static Regex matchNoReading = new Regex(@",\*,\*$");
        private static Regex excludedPos = new Regex(@"(,名詞,数,|,名詞,代名詞,一般,|,助詞,|,助動詞,|,接続詞,|,接頭詞,|,記号,|,非自立,|,接尾,|,副詞可能,|,名詞,固有名詞,人名,姓,|,名詞,固有名詞,人名,名,)");
        private static Regex excludedWord = new Regex(@"(" +
                @",動詞,自立,\*,\*,一段,.*,いる," +
                @"|,動詞,自立,\*,\*,サ変・.*,.*,する," +
                @"|,動詞,自立,\*,\*,カ変・.*,.*,くる," +
                @"|,動詞,自立,\*,\*,カ変・.*,.*,来る," +
                @"|,動詞,自立,\*,\*,五段・ラ行,.*,ある," +
                @"|,動詞,自立,\*,\*,五段・ラ行,.*,なる," +
                @"|,動詞,自立,\*,\*,一段,.*,できる," +
                @"|,動詞,自立,\*,\*,一段,.*,出来る," +
                @"|,動詞,自立,\*,\*,五段・ワ行促音便,.*,行う," +
                @"|,形容詞,自立,\*,\*,形容詞・アウオ段,.*,ない," +
                @"|^この,連体詞," +
                @"|^その,連体詞," +
                @"|^どの,連体詞," +
                @"|^さらに,副詞," +
                @"|^もちろん,副詞," +
                // @"^情報,名詞," +  // too frequent word
            ")");

        public static bool isTargetWord(string word)
        {
            return
                TopicModelHandler.matchNoReading.IsMatch(word) ||
                TopicModelHandler.excludedPos.IsMatch(word) ||
                TopicModelHandler.excludedWord.IsMatch(word) ?
                false : true;
        }

        public double GetPerplexyAverage()
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
