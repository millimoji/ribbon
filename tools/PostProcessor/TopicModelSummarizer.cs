using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;

namespace Ribbon.PostProcessor
{
    namespace JsonType
    {
        public class WP // word and probability
        {
            public string w { get; set; }
            public double p { get; set; }
            public bool x { get; set; } // flag isUsed
            public int u { get; set; }  // used Count
        }
        public class TI // topic info
        {
            public double dev { get; set; } // deviation
            public long wc { get; set; }    // word count
        }
        public class TopicModelSummary
        {
            public double perplexity { get; set; }
            public double minPerplexity { get; set; }
            public double latestPerplexity { get; set; }
            public double entropyAverage { get; set; }
            public double topicEntropy { get; set; }
            public int maxTopic { get; set; }
            public double averageTopic { get; set; }
            public int wordCount { get; set; }
        }
        public class SummaryStruct
        {
            public string generatedTime { get; set; }
            public string summaryType { get; set; }
            public TopicModelSummary tps { get; set; } = new TopicModelSummary();
            public List<WP[]> topicModel { get; set; }
            public List<TI> topicInfo { get; set; }
        }
    }

    class TopicModelSummarizer
    {
        private const int summryItemCount = 100;

        // dirty ... calculated in MakeTopicModelSummary()
        int maxTopicCount = 0;
        double averateTopicCount = 0.0;
        int wordCount = 0;
        List<JsonType.TI> topicInfo = new List<JsonType.TI>();

        public void MakeSumarize(
            string fileName,
            Shared.TopicModelHandler topicModelHandler,
            Func<int, string> id2word)
        {
            var wordProbMatrix = topicModelHandler.wordProbsMatrix;
            var topicProbs = topicModelHandler.topicProbsArray;

            if (wordProbMatrix.Count == 0)
            {
                return;
            }
            var summary = new JsonType.SummaryStruct();
            summary.generatedTime = DateTime.Now.ToString();
            summary.summaryType = Constants.summaryTypeTopicModel;

            var perplexities = topicModelHandler.loadedPerplexities;
            summary.tps.perplexity = perplexities[0];
            summary.tps.minPerplexity = perplexities[1];
            summary.tps.latestPerplexity = perplexities[2];

            var log2 = Math.Log(2.0);
            var logTC = -Math.Log(1.0 / (double)topicProbs.Length) / log2;
            summary.tps.topicEntropy = -topicProbs.Select(x => (x * Math.Log(x) / log2)).Sum() / logTC;

            double entropyAverage;
            summary.topicModel = this.MakeTopicModelSummary(topicProbs, wordProbMatrix, id2word, out entropyAverage);
            summary.tps.entropyAverage = entropyAverage;
            summary.tps.maxTopic = this.maxTopicCount;
            summary.tps.averageTopic = this.averateTopicCount;
            summary.tps.wordCount = this.wordCount;
            summary.topicInfo = this.topicInfo;


            using (var fs = File.Create(fileName))
            {
                using (var writer = new Utf8JsonWriter(fs))
                {
                    JsonSerializer.Serialize(writer, summary);
                }
            }
        }

        private List<JsonType.WP[]> MakeTopicModelSummary(
            double[] topicProbs,
            Dictionary<int, double[]> wordProbMatrix,
            Func<int, string> id2word,
            out double entropyAverage)
        {
            var topicData = new List<JsonType.WP[]>();

            var topicCount = wordProbMatrix.First().Value.Length;
            var normalizeWork = new double[topicCount];
            var log2 = Math.Log(2.0);
            var logTC = -Math.Log(1.0 / (double)topicCount) / log2;
            var entropyList = wordProbMatrix
                .Select(kv =>
                {
                    Shared.TopicModelHandler.DoubleForEach(normalizeWork, (double x, int idx) => (topicProbs[idx] * kv.Value[idx]));
                    var sum = normalizeWork.Sum();
                    var pLogPs = normalizeWork.Select(l =>
                    {
                        var p = l / sum;
                        var pLogP = p * Math.Log(p) / log2;
                        return pLogP;
                    });
                    var entropy = -pLogPs.Sum() / logTC;
                    if (entropy > 1.0)
                    {
                        throw new Exception("Invalid range");
                    }
                    return new Tuple<int, string, double, double, double[]>(kv.Key, id2word(kv.Key), entropy, sum, kv.Value);
                });
            var entropyDict = entropyList.ToDictionary(x => x.Item1);
            entropyAverage = entropyList.Select(x => x.Item3).Sum() / (double)entropyList.Count();

            var threshold = this.CalcStandardDeviation(wordProbMatrix);

            // order by low entropy
            topicData.Add(entropyList
                .Select(x => new Tuple<string, double, double[]>(x.Item2, (1.0 - x.Item3), x.Item5))
                .OrderByDescending(x => x.Item2)
                .Take(summryItemCount)
                .Select(x => new JsonType.WP {
                    w = x.Item1,
                    p = x.Item2,
                    x = true,
                    u = x.Item3.Select((v, idx) => (v >= threshold[idx])).Count(f => f),
                })
                .ToArray());

            // order by low entropy * propability
            topicData.Add(entropyList
                .Select(x => new Tuple<string, double, double[]>(x.Item2, (1.0 - x.Item3) * x.Item4, x.Item5))
                .OrderByDescending(x => x.Item2)
                .Take(summryItemCount)
                .Select(x => new JsonType.WP {
                    w = x.Item1,
                    p = x.Item2,
                    x = true,
                    u = x.Item3.Select((v, idx) => (v >= threshold[idx])).Count(f => f),
               })
                .ToArray());

            // order by high entropy
            topicData.Add(entropyList
                .OrderByDescending(x => x.Item3)
                .Take(summryItemCount)
                .Select(x => new JsonType.WP {
                    w = x.Item2,
                    p = x.Item4,
                    x = true,
                    u = x.Item5.Select((v, idx) => (v >= threshold[idx])).Count(f => f),
                })
                .ToArray());

            // order by high entropy * probability
            topicData.Add(entropyList
                .Select(x => new Tuple<string, double, double[]>(x.Item2, x.Item3 * x.Item4, x.Item5))
                .OrderByDescending(x => x.Item2)
                .Take(summryItemCount)
                .Select(x => new JsonType.WP {
                    w = x.Item1,
                    p = x.Item2,
                    x = true,
                    u = x.Item3.Select((v, idx) => (v >= threshold[idx])).Count(f => f),
                })
                .ToArray());

            var indexList = new int[topicCount];
            for (int topic = 0; topic < topicCount; ++topic)
            {
                indexList[topic] = topic;
            }
            var outputOrder = indexList.Select(x => new Tuple<int, double>(x, topicProbs[x])).OrderByDescending(x => x.Item2).Select(x => x.Item1).ToArray();

            var usedFlagCounts = wordProbMatrix.Select(x => x.Value.Where((p, idx) => p >= threshold[idx]).Count()).Where(x => x > 0);
            this.maxTopicCount = usedFlagCounts.Max();
            this.averateTopicCount = usedFlagCounts.Average(x => (double)x);
            this.wordCount = usedFlagCounts.Count();

            for (int topic = 0; topic < topicCount; ++topic)
            {
                var topicIndex = outputOrder[topic];

                this.topicInfo.Add(new JsonType.TI
                {
                    dev = threshold[topicIndex],
                    wc = wordProbMatrix.Select(x => (x.Value[topicIndex] >= threshold[topicIndex])).Where(f => f).Count()
                }); ;

                var sortByProb = wordProbMatrix
                    .Select(x => new Tuple<int, double, double[]>(x.Key, x.Value[topicIndex] * (1.0 - entropyDict[x.Key].Item3), x.Value))
                    .OrderByDescending(x => x.Item2)
                    .Take(summryItemCount)
                    .Select(x => new JsonType.WP
                    {
                        w = id2word(x.Item1),
                        p = x.Item2,
                        x = (x.Item2 >= threshold[topicIndex]),
                        u = x.Item3.Select((v, idx) => (v >= threshold[idx])).Where(f => f).Count(),
                    });

                topicData.Add(sortByProb.ToArray());
            }

            return topicData;
        }

        double[] CalcStandardDeviation( // threshold
            Dictionary<int, double[]> wordProbMatrix)
        {
            var average = 1.0 / (double)wordProbMatrix.Count;

            var deviation = new double[wordProbMatrix.First().Value.Length];

            foreach (var kv in wordProbMatrix)
            {
                Shared.TopicModelHandler.DoubleForEach(deviation, (x, idx) =>
                {
                    var diff = kv.Value[idx] - average;
                return x + (diff * diff);
                });
            }

            // try use single deviation and distribution.
            var singleDeviation = Math.Sqrt(deviation.Sum() / (double)wordProbMatrix.Count / (double)deviation.Length) * 0.8;  // TO BE adjusted
            Shared.TopicModelHandler.DoubleForEach(deviation, (x, idx) => singleDeviation + average);
            return deviation;
        }
    }
}