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
        private double[] topicProbs; // log
        public Dictionary<int, double[]> wordProbs; // log
        private System.Random random = new System.Random();
        public bool shouldRnadomInitialize = true;

        private double[] savedTopicsProbs;
        private Dictionary<int, double[]> savedWordProbs;

        public TopicModelState()
        {
            this.topicProbs = this.CreateLoggedRandomArray(TopicModelHandler.TopicCount);
            this.wordProbs = new Dictionary<int, double[]>();
        }

        public int GetWordCount()
        {
            return this.wordProbs.Count;
        }

        public void SaveToFile(string fileName, Func<int, string> id2word)
        {
            using (StreamWriter fileStream = new StreamWriter(fileName, false, Encoding.Unicode))
            {
                {
                    var outputLine = "[Topic]\t" + String.Join("\t", this.topicProbs);
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
                        if (Convert.ToDouble(sringify) >= 0.0)
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
            this.shouldRnadomInitialize = false;
            using (StreamReader fileStream = new StreamReader(fileName))
            {
                string line = fileStream.ReadLine();
                string[] headerLine = line.Split('\t');
                if (headerLine[0] != "[Topic]")
                {
                    return false;
                }

                double sum = 0.0;
                TopicModelHandler.DoubleForEach(this.topicProbs, (double x, int idx) =>
                {
                    var value = (idx < (headerLine.Length - 1)) ? Math.Exp(Convert.ToDouble(headerLine[idx + 1])) : this.GetSmallShuffled(1.0 / (double)this.topicProbs.Length);
                    sum += value;
                    return value;
                });
                TopicModelHandler.DoubleForEach(this.topicProbs, (double x, int idx) => Math.Log(x / sum));

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
                    var singleWordProbs = this.GetProbsForWord(wordId);
                    TopicModelHandler.DoubleForEach(singleWordProbs, (double x, int idx) =>
                    {
                        var value = (idx < (wordLine.Length - 1)) ? Convert.ToDouble(wordLine[idx + 1]) : Math.Log(1.0 / singleWordProbs.Length / this.topicProbs.Length);
                        if (value >= 0.0)
                        {
                            throw new Exception("Invalid value");
                        }
                        return value;
                    });
                }
                this.NormalizeWordProbs();
            }

            return true;
        }

        private double[] CreateLoggedRandomArray(int size)
        {
            var array = new double[size];
            var sum = 0.0;
            TopicModelHandler.DoubleForEach(array, (double x, int idx) =>
            {
                var value = this.GetRandomNumber();
                sum += value;
                return value;
            });
            TopicModelHandler.DoubleForEach(array, (double x, int idx) => Math.Log(x / sum));
            return array;
        }

#if false
        // Topic Model
        public double CalculateQAndApply(List<int> wordIds, TopicModelNext next)
        {
            var work = new double[TopicModelHandler.TopicCount];
            var loggedLikelyhood = 0.0;

            foreach (var wordId in wordIds)
            {
                var probs = this.GetProbsForWord(wordId);
                work[0] = this.topicProbs[0] + probs[0];
                var qLoggedDenomi = work[0];
                for (int i = 1; i < work.Length; ++i)
                {
                    work[i] = this.topicProbs[i] + probs[i];
                    qLoggedDenomi = this.AddLogedProb(qLoggedDenomi, work[i]);
                }
                loggedLikelyhood += qLoggedDenomi;

                TopicModelHandler.DoubleForEach(work, (double x, int idx) => Math.Exp(x - qLoggedDenomi)); // convert to probability

                TopicModelHandler.DoubleForEach(next.topicProbs, (double x, int idx) => (x + work[idx]));
                foreach (var targetWordId in wordIds)
                {
                    var targetProbs = next.GetProbsForWord(targetWordId);
                    TopicModelHandler.DoubleForEach(targetProbs, (double x, int idx) => (x + work[idx]));
                }
            }

            return loggedLikelyhood;
        }
#else
        // Mixture Unigram Model
        public double CalculateQAndApply(List<int> wordIds, TopicModelNext next)
        {
            var work = new double[TopicModelHandler.TopicCount];
            TopicModelHandler.DoubleForEach(work, (double x, int idx) => this.topicProbs[idx]);

            foreach (var wordId in wordIds)
            {
                var probs = this.GetProbsForWord(wordId);
                TopicModelHandler.DoubleForEach(work, (double x, int idx) => (x + probs[idx]));
            }
            var qLoggedDenomi = work[0];
            for (int i = 1; i < work.Length; ++i)
            {
                qLoggedDenomi = this.AddLogedProb(qLoggedDenomi, work[i]);
            }

            TopicModelHandler.DoubleForEach(work, (double x, int idx) => Math.Exp(x - qLoggedDenomi)); // convert to probability

            TopicModelHandler.DoubleForEach(next.topicProbs, (double x, int idx) => (x + wordIds.Count * work[idx])); // Heuristics: multiply word count
            foreach (var targetWordId in wordIds)
            {
                var targetProbs = next.GetProbsForWord(targetWordId);
                TopicModelHandler.DoubleForEach(targetProbs, (double x, int idx) => (x + work[idx]));
            }
            return qLoggedDenomi;
        }
#endif

        private double[] GetProbsForWord(int index)
        {
            double[] probLine;
            if (this.wordProbs.TryGetValue(index, out probLine))
            {
                return probLine;
            }
            // use same value to suppress to contribute to calculation Q at first time.
            probLine = new double[TopicModelHandler.TopicCount];
            var newValue = Math.Log(this.GetSmallShuffled(1.0 / TopicModelHandler.updateWordCount / 10.0));
            TopicModelHandler.DoubleForEach(probLine, (double x, int idx) => newValue);
            this.wordProbs.Add(index, probLine);
            return probLine;
        }

        private double AddLogedProb(double logA, double logB)
        {
            // log(exp(a) + exp(b)) = log(exp(a)(1 + exp(b)/exp(a)) = a + log(1 + exp(b - a))
            if (logA < logB)
            { // swap
                var tmp = logA;
                logA = logB;
                logB = tmp;
            }
            return logA + Math.Log(1.0 + Math.Exp(logB - logA));
        }

        public void MergeNext(TopicModelNext next, double keepRate)
        {
            double newRate = 1.0 - keepRate;
            double sum = 0.0;
            TopicModelHandler.DoubleForEach(this.topicProbs, (double x, int idx) =>
            {
                var value = Math.Exp(x) * keepRate + next.topicProbs[idx] * newRate;
                value = this.GetSmallShuffled(value);
                sum += value;
                return value;
            });
            TopicModelHandler.DoubleForEach(this.topicProbs, (double x, int idx) => Math.Log(x / sum));

            int[] indexArray = new int[this.wordProbs.Count + next.wordProbs.Count];
            {
                int i = 0;
                foreach (var kv in this.wordProbs)
                {
                    indexArray[i++] = kv.Key;
                }
                foreach (var kv in next.wordProbs)
                {
                    indexArray[i++] = kv.Key;
                }
            }
            Array.Sort(indexArray);
            var sums = new double[TopicModelHandler.TopicCount];
            TopicModelHandler.DoubleForEach(sums, (double x, int idx) => 0.0);

            for (int i = 0; i < indexArray.Length; ++i)
            {
                var wordId = indexArray[i];
                if ((i < (indexArray.Length - 1)) && wordId == indexArray[i + 1])
                {
                    var dstArray = this.wordProbs[wordId]; // logged
                    var srcArray = next.wordProbs[wordId]; // prob
                    TopicModelHandler.DoubleForEach(dstArray, (double x, int idx) =>
                    {
                        var value = Math.Exp(x) * keepRate + srcArray[idx] * newRate;
                        value = this.GetSmallShuffled(Math.Max(value, 1.0 / Single.MaxValue));
                        sums[idx] += value;
                        return value;
                    });
                    ++i;
                    continue;
                }
                {
                    double[] dstArray;
                    if (this.wordProbs.TryGetValue(indexArray[i], out dstArray))
                    {
                        TopicModelHandler.DoubleForEach(dstArray, (double x, int idx) =>
                        {
                            var value = Math.Exp(x) * keepRate;
                            value = this.GetSmallShuffled(Math.Max(value, 1.0 / Single.MaxValue));
                            sums[idx] += value;
                            return value;
                        });
                    }
                    else
                    {
                        // logically, this path should not happen
                        dstArray = this.GetProbsForWord(wordId);
                        var srcArray = next.wordProbs[wordId]; // prob
                        TopicModelHandler.DoubleForEach(dstArray, (double x, int idx) =>
                        {
                            var value = srcArray[idx] * newRate;
                            value = this.GetSmallShuffled(Math.Max(value, 1.0 / Single.MaxValue));
                            sums[idx] += value;
                            return value;
                        });
                    }
                }
            }
            // normalize
            foreach (var kv in this.wordProbs)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) =>
                {
                    var value = Math.Log(x / sums[idx]);
                    if (value >= 0.0)
                    {
                        throw new Exception("invalid value range");
                    }
                    return value;
                });
            }
        }

        public void FullRandomInitialize()
        {
            double sum = 0.0;
            TopicModelHandler.DoubleForEach(this.topicProbs, (double x, int idx) =>
            {
                var value = (double)random.Next(100, 10000) / (double)10000.0;
                sum += value;
                return value;
            });
            TopicModelHandler.DoubleForEach(this.topicProbs, (double x, int idx) => Math.Log(x / sum));

            var sums = new double[TopicModelHandler.TopicCount];
            TopicModelHandler.DoubleForEach(sums, (double x, int idx) => 0.0);

            foreach (var kv in this.wordProbs)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) =>
                {
                    var value = (double)random.Next(100, 10000) / (double)10000.0;
                    sums[idx] += value;
                    return value;
                });
            }
            foreach (var kv in this.wordProbs)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) =>
                {
                    var value = Math.Log(x / sums[idx]);
                    if (value >= 0.0)
                    {
                        throw new Exception("invalid value range");
                    }
                    return value;
                });
            }
            this.shouldRnadomInitialize = false;
        }

        public void NormalizeWordProbs()
        {
            var sums = new double[TopicModelHandler.TopicCount];
            TopicModelHandler.DoubleForEach(sums, (double x, int idx) => 0.0);

            foreach (var kv in this.wordProbs)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) =>
                {
                    if (x >= 0.0)
                    {
                        throw new Exception("invalid value range");
                    }
                    var value = Math.Exp(x);
                    sums[idx] += value;
                    return value;
                });
            }
            foreach (var kv in this.wordProbs)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) =>
                {
                    var value = Math.Log(x / sums[idx]);
                    if (value >= 0.0)
                    {
                        throw new Exception("invalid value range");
                    }
                    return value;
                });
            }
        }

        private double GetRandomNumber()
        {
            return (double)random.Next(0x10, 0x10000) / (double)0x10000;
        }

        private double GetSmallShuffled(double x)
        {
            return x * (1.0 - 0.01 * (double)random.Next(0x10, 0x10000) / (double)0x10000);
        }

        public void saveProbs()
        {
            this.savedTopicsProbs = (double[])this.topicProbs.Clone();
            this.savedWordProbs = new Dictionary<int, double[]>();
            foreach (var kv in this.wordProbs)
            {
                this.savedWordProbs.Add(kv.Key, (double[])kv.Value.Clone());
            }
        }

        public void mergeProbs(double keepRate)
        {
            double newRate = 1.0 - keepRate;
            double sum = 0.0;
            TopicModelHandler.DoubleForEach(this.topicProbs, (double x, int idx) =>
            {
                var valeu = Math.Exp(this.savedTopicsProbs[idx]) * keepRate + Math.Exp(x) * newRate;
                sum += valeu;
                return valeu;
            });
            TopicModelHandler.DoubleForEach(this.topicProbs, (double x, int idx) => Math.Log(x / sum));

            foreach (var kv in this.wordProbs)
            {
                double[] oldData;
                if (this.savedWordProbs.TryGetValue(kv.Key, out oldData))
                {
                    TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) =>
                    {
                        return Math.Log(Math.Exp(oldData[idx]) * keepRate + Math.Exp(x) * newRate);
                    });
                }
                else
                {
                    // keep new data??
                }
            }
            this.NormalizeWordProbs();
        }
    }


    class TopicModelNext
    {
        public double[] topicProbs;    // prob (NOTE: not log)
        public Dictionary<int, double[]> wordProbs; // prob (NOTE: not log)

        public TopicModelNext()
        {
            this.Clear();
        }

        public double[] GetProbsForWord(int index)
        {
            double[] probLine;
            if (this.wordProbs.TryGetValue(index, out probLine))
            {
                return probLine;
            }
            probLine = new double[TopicModelHandler.TopicCount];
            TopicModelHandler.DoubleForEach(probLine, (double x, int idx) => 0.0);
            this.wordProbs.Add(index, probLine);
            return probLine;
        }

        public void Clear()
        {
            this.topicProbs = new double[TopicModelHandler.TopicCount];
            TopicModelHandler.DoubleForEach(this.topicProbs, (double x, int idx) => 0.0);
            this.wordProbs = new Dictionary<int, double[]>();
        }

        public void Normalize()
        {
            var topicSum = this.topicProbs.Sum();
            TopicModelHandler.DoubleForEach(this.topicProbs, (double x, int idx) => (x / topicSum));

            var wordSums = new double[TopicModelHandler.TopicCount];
            TopicModelHandler.DoubleForEach(wordSums, (double x, int idx) => 0.0);
            foreach (var kv in this.wordProbs)
            {
                TopicModelHandler.DoubleForEach(wordSums, (double x, int idx) => (x + kv.Value[idx]));
            }
            foreach (var kv in this.wordProbs)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) => (x / wordSums[idx]));
            }
        }
    }

    public class TopicModelHandler
    {
        public const int TopicCount = 63; // 255;
        public const double updateMergeRate = 0.5;
        public const double updateWordCount = 200000;
        public const int perplexHistMax = 100;
        public const int wordCountRequirement = 4;

        private TopicModelState baseState;
        private TopicModelNext nextState;
        private Queue<double> ppHist = new Queue<double>();
        private List<List<int>> documentHistory = new List<List<int>>();

        double currentLikelihood = 0.0;
        double currentWordCount = 0.0;


        public TopicModelHandler()
        {
            this.baseState = new TopicModelState();
            this.nextState = new TopicModelNext();
        }

        public void LoadFromFile(string fileName, Func<string, int> word2id, Func<int, string> id2word)
        {
            this.nextState.Clear();
            this.documentHistory.Clear();
            this.currentLikelihood = 0.0;
            this.currentWordCount = 0.0;

            this.baseState.LoadFromFile(fileName, word2id, id2word);
        }

        public void SaveToFile(string fileName, string summaryFilename, Func<int, string> id2Word)
        {
            LearnLoopUntilPPTarget();

            this.baseState.SaveToFile(fileName, id2Word);

            {
                var summarizer = new JsonSummarizer();
                summarizer.Serialize(summaryFilename, this.GetPerplexyAvarage(), this.baseState.wordProbs, id2Word);
            }
        }

        public void LearnDocument(List<string> document, Func<string, int> word2id)
        {
            var listId = this.WordArrayToIntArray(document, word2id);
            if (listId.Count < wordCountRequirement)
            {
                return; // ignore
            }

            var loggedQDenomi = this.baseState.CalculateQAndApply(listId, this.nextState);

            this.currentLikelihood += loggedQDenomi;
            TopicModelHandler.IsValidNumber(this.currentLikelihood);

            this.documentHistory.Add(listId);
            this.currentWordCount += (double)listId.Count;

            if (this.currentWordCount >= updateWordCount)
            {
                LearnLoopUntilPPTarget();
            }
        }

        public void PrintCurretState()
        {
            var curPp = Math.Exp(-this.currentLikelihood / this.currentWordCount);
            Console.WriteLine($"[TopicModel - pp:{curPp}, word:{this.baseState.GetWordCount()}, words:{this.currentWordCount}, pp-ave:{this.GetPerplexyAvarage()}");
        }

        private void LearnLoopUntilPPTarget()
        {
#if false
            if (this.documentHistory.Count > 0)
            {
                if (this.baseState.shouldRnadomInitialize)
                {
                    this.baseState.FullRandomInitialize();
                    this.nextState.Clear();
                    var likelyHood = 0.0;
                    foreach (var document in this.documentHistory)
                    {
                        var loggedQDenomi = this.baseState.CalculateQAndApply(document, this.nextState);
                        likelyHood += loggedQDenomi;
                    }
                    var initialPp = Math.Exp(-likelyHood / this.currentWordCount);
                    this.ppHist.Enqueue(initialPp);
                }
                else
                {
                    var initialPp = Math.Exp(-this.currentLikelihood / this.currentWordCount);
                    this.ppHist.Enqueue(initialPp);
                }
                this.nextState.Normalize();
                this.baseState.MergeNext(this.nextState, updateMergeRate);
            }
#else
            var initialPp = Math.Exp(-this.currentLikelihood / this.currentWordCount);
            this.ppHist.Enqueue(initialPp);
            Console.WriteLine($"[TopicModel - initial] pp:{initialPp}, word:{this.baseState.GetWordCount()} pp-ave:{this.GetPerplexyAvarage()}");

            List<double> ppLocalHistory = new List<double>();
            ppLocalHistory.Add(initialPp);

            this.baseState.saveProbs();

            for (var loopCount = 1; ; ++loopCount)
            {
                var likelyHood = 0.0;
                if (this.baseState.shouldRnadomInitialize)
                {
                    this.baseState.FullRandomInitialize();
                }
                else
                {
                    this.nextState.Normalize();
                    this.baseState.MergeNext(this.nextState, updateMergeRate);
                }
                this.nextState.Clear();

                foreach (var document in this.documentHistory)
                {
                    var loggedQDenomi = this.baseState.CalculateQAndApply(document, this.nextState);
                    likelyHood += loggedQDenomi;
                }

                var currentPp = Math.Exp(-likelyHood / this.currentWordCount);
                ppLocalHistory.Add(currentPp);

                var goNext = ppLocalHistory.Skip(Math.Max(ppLocalHistory.Count - 5, 0)).All(x => Math.Abs(x - currentPp) < currentPp * 0.1);
                if (goNext)
                {
                    Console.WriteLine($"[TopicModel - Ok:[{loopCount}] pp:{currentPp}");
                    break;
                }
                Console.WriteLine($"[TopicModel - retry:[{loopCount}] pp:{currentPp}");
            }

            this.baseState.mergeProbs(updateMergeRate);
#endif
            this.nextState.Clear();
            this.documentHistory.Clear();
            this.currentLikelihood = 0.0;
            this.currentWordCount = 0.0;
        }

        private List<int> WordArrayToIntArray(List<string> document, Func<string, int> word2id)
        {
            return document.Select(word => (TopicModelHandler.isTargetWord(word) ? word2id(word) : -1)).Where(wordId => wordId >= 0).ToList();
        }

        private static Regex matchNoReading = new Regex(@",\*,\*$");
        private static Regex excludedPos = new Regex(@"(,名詞,数,|,助詞,|,助動詞,|,接続詞,|,接頭詞,|,記号,|,非自立,|,接尾,|,副詞可能,|,名詞,固有名詞,人名,姓,|,名詞,固有名詞,人名,名,)");
        private static Regex excludedWord = new Regex(@"(" +
                @",動詞,自立,\*,\*,サ変・|" +
                @",動詞,自立,\*,\*,カ変・|" +
                @",動詞,自立,\*,\*,五段・ラ行,.*,ある,|" +
                @",動詞,自立,\*,\*,五段・ラ行,.*,なる,|" +
                @",動詞,自立,\*,\*,一段,.*,できる,|" +
                @",動詞,自立,\*,\*,五段・ワ行促音便,.*,行う,|" +
                @"^この,連体詞,|" +
                @"^その,連体詞," +
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
            while (this.ppHist.Count > perplexHistMax)
            {
                this.ppHist.Dequeue();
            }
            return this.ppHist.Sum() / (double)(this.ppHist.Count());
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
