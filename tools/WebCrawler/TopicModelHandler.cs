﻿using System;
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
        private Dictionary<int, double[]> wordProbs; // log
        private System.Random random = new System.Random();

        public TopicModelState()
        {
            this.topicProbs = this.CreateLoggedRandomArray(TopicModelHandler.TopicCount);
            this.wordProbs = new Dictionary<int, double[]>();
        }

        public int GetWordCount()
        {
            return this.wordProbs.Count;
        }

        public void SaveToFile(string fileName, string summaryFilename, Func<int, string> id2word)
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
                    var outputLine = wordText + "\t" + String.Join("\t", item.Value);
                    fileStream.WriteLine(outputLine);
                }
            }


            {
                var summarizer = new JsonSummarizer();
                summarizer.Serialize(summaryFilename, this.wordProbs, id2word);
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
                string line = fileStream.ReadLine();
                string[] headerLine = line.Split('\t');
                if (headerLine[0] != "[Topic]")
                {
                    return false;
                }

                double sum = 0.0;
                TopicModelHandler.DoubleForEach(this.topicProbs, (double x, int idx) =>
                {
                    var value = (idx < (headerLine.Length - 1)) ? Math.Exp(Convert.ToDouble(headerLine[idx + 1])) : this.GetSmallRandomNumber();
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
                        (idx < (wordLine.Length - 1)) ? Convert.ToDouble(wordLine[idx + 1]): Math.Log(this.GetSmallRandomNumber()));
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
            probLine = new double[TopicModelHandler.TopicCount];
            TopicModelHandler.DoubleForEach(probLine, (double x, int idx) => this.GetSmallRandomNumber());
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

        public void MergeNext(TopicModelNext next, double keepRate, bool canDelete = true)
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
            var deleteList = new List<int>();

            for (int i = 0; i < indexArray.Length; ++i)
            {
                var wordId = indexArray[i];
                if ((i < (indexArray.Length - 1)) && wordId == indexArray[i + 1])
                {
                    var dstArray = this.wordProbs[wordId]; // logged
                    var srcArray = next.wordProbs[wordId]; // prob
#if false // heuristics
                    var sumDst = dstArray.Select(x => Math.Exp(x)).Sum();
                    var entDst = - dstArray.Select(x => { var p = Math.Max(Math.Exp(x) / sumDst, 1/Single.MaxValue); return p * Math.Log(p); }).Sum();

                    var sumSrc = srcArray.Sum();
                    var entSrc = -srcArray.Select(x => { var p = Math.Max(x / sumSrc, 1/Single.MaxValue); return p * Math.Log(p); }).Sum();

                    var dstRate = entSrc * (entDst + entSrc);
                    var srcRate = entDst * (entDst + entSrc);

                    TopicModelHandler.DoubleForEach(dstArray, (double x, int idx) =>
                    {
                        var value = Math.Exp(x) * dstRate + srcArray[idx] * srcRate;
                        value = this.GetSmallShuffled(Math.Max(value, 1.0 / Single.MaxValue));
                        sums[idx] += value;
                        return value;
                    });
#else
                    TopicModelHandler.DoubleForEach(dstArray, (double x, int idx) =>
                    {
                        var value = Math.Exp(x) * keepRate + srcArray[idx] * newRate;
                        value = this.GetSmallShuffled(Math.Max(value, 1.0 / Single.MaxValue));
                        sums[idx] += value;
                        return value;
                    });
#endif
                    ++i;
                    continue;
                }
                {
                    double[] dstArray;
                    if (this.wordProbs.TryGetValue(indexArray[i], out dstArray))
                    {
                        // Here is for no word in next. if probs is too small, remove this entry
                        if (canDelete && dstArray.Select(x => Math.Exp(x)).Sum() * keepRate <= (1.0 / Single.MaxValue * dstArray.Length))
                        {
                            deleteList.Add(indexArray[i]);
                        }
                        else
                        {
                            TopicModelHandler.DoubleForEach(dstArray, (double x, int idx) =>
                            {
                                var value = Math.Exp(x) * keepRate;
                                value = this.GetSmallShuffled(Math.Max(value, 1.0 / Single.MaxValue));
                                sums[idx] += value;
                                return value;
                            });
                        }
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
            foreach (var deleteItem in deleteList)
            {
                this.wordProbs.Remove(deleteItem);
            }
            // normalize
            foreach (var kv in this.wordProbs)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) => Math.Log(x / sums[idx]));
            }
        }

        private void NormalizeWordProbs()
        {
            var sums = new double[TopicModelHandler.TopicCount];
            TopicModelHandler.DoubleForEach(sums, (double x, int idx) => 0.0);

            foreach (var kv in this.wordProbs)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) =>
                {
                    var value = Math.Exp(x);
                    sums[idx] += value;
                    return value;
                });
            }
            foreach (var kv in this.wordProbs)
            {
                TopicModelHandler.DoubleForEach(kv.Value, (double x, int idx) => Math.Log(x / sums[idx]));
            }
        }

        private double GetRandomNumber()
        {
            return (double)random.Next(0x10, 0x10000) / (double)0x10000;
        }

        private double GetSmallRandomNumber()
        {
            return (double)random.Next(0x10, 0x10000) / (double)Single.MaxValue;
        }

        private double GetSmallShuffled(double x)
        {
            return x * (1.0 + 0.01 * (double)random.Next(0x10, 0x10000) / (double)0x10000);
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
        public const double updateMergeRate = 0.9; // 0.9;
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
            this.baseState.LoadFromFile(fileName, word2id, id2word);
        }

        public void SaveToFile(string fileName, string summaryFilename, Func<int, string> id2Word)
        {
            this.baseState.SaveToFile(fileName, summaryFilename, id2Word);
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

        private void LearnLoopUntilPPTarget()
        {
            var initialPp = Math.Exp(-this.currentLikelihood / this.currentWordCount);
            this.ppHist.Enqueue(initialPp);
            var targetPP = (initialPp > 10000.0) ? 10000.0 : Math.Max(initialPp / 10.0, 2000.0);
            Console.WriteLine($"[TopicMode - initial] pp:{initialPp}, word:{this.baseState.GetWordCount()} pp-ave:{this.GetPerplexyAvarage()}");

            for (var loopCount = 1; ; ++loopCount)
            {
                var likelyHood = 0.0;
                this.nextState.Normalize();
                this.baseState.MergeNext(this.nextState, updateMergeRate, false);
                this.nextState.Clear();

                foreach (var document in this.documentHistory)
                {
                    var loggedQDenomi = this.baseState.CalculateQAndApply(document, this.nextState);
                    likelyHood += loggedQDenomi;
                }
                var currentPp = Math.Exp(-likelyHood / this.currentWordCount);
                if (currentPp <= targetPP)
                {
                    Console.WriteLine($"[TopicMode - Ok:[{loopCount}] pp:{currentPp}");
                    break;
                }
                else
                {
                    Console.WriteLine($"[TopicMode - retry:[{loopCount}] pp:{currentPp}");
                }
            }

            this.nextState.Clear();
            this.documentHistory.Clear();

            this.currentLikelihood = 0.0;
            this.currentWordCount = 0.0;
        }

        private void ApplyNextState()
        {
            var pp = Math.Exp(-this.currentLikelihood / this.currentWordCount);
            this.ppHist.Enqueue(pp);
            Console.WriteLine($"[TopicMode] pp:{pp}, word:{this.baseState.GetWordCount()} pp-ave:{this.GetPerplexyAvarage()}");

            this.nextState.Normalize();
            this.baseState.MergeNext(this.nextState, updateMergeRate);
            this.nextState.Clear();

            this.currentLikelihood = 0.0;
            this.currentWordCount = 0.0;
        }

        private List<int> WordArrayToIntArray(List<string> document, Func<string, int> word2id)
        {
            return document.Select(word => (TopicModelHandler.isTargetWord(word) ? word2id(word) : -1)).Where(wordId => wordId >= 0).ToList();
        }

        private static Regex matchNoReading = new Regex(@",\*,\*$");
        private static Regex excludedPos = new Regex(@"(,名詞,数,|,助詞,|,助動詞,|,接頭詞,|,記号,|,非自立,|,接尾,|,副詞可能,|,名詞,固有名詞,人名,姓,|,名詞,固有名詞,人名,名,)");
        private static Regex excludedWord = new Regex(@"(,動詞,自立,\*,\*,サ変・|,動詞,自立,\*,\*,カ変・)");

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
