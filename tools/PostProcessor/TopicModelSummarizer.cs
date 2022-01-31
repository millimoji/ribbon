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
        }
        public class TopicModelSummary
        {
            public double perplexity { get; set; }
            public double latestPerplexity { get; set; }
            public double entropyAverage { get; set; }
            public double topicEntropy { get; set; }
        }
        public class SummaryStruct
        {
            public string generatedTime { get; set; }
            public string summaryType { get; set; }
            public TopicModelSummary tps { get; set; } = new TopicModelSummary();
            public List<WP[]> topicModel { get; set; }
        }
    }

    class TopicModelSummarizer
    {
        private const int summryItemCount = 100;

        public void MakeSumarize(
            string fileName,
            Shared.TopicModelHandler topicModelHandler,
            Func<int, string> id2word)
        {
            var wordProbMatrix = topicModelHandler.wordProbsMatrix;
            var topicProbs = topicModelHandler.topicProbsArray;
            var averagePerplexity = topicModelHandler.GetPerplexyAvarage();
            var latestPerplexity = topicModelHandler.lastPerplexicity;

            if (wordProbMatrix.Count == 0)
            {
                return;
            }
            var summary = new JsonType.SummaryStruct();
            summary.generatedTime = DateTime.Now.ToString();
            summary.summaryType = Constants.summaryTopicModel;
            summary.tps.perplexity = Double.IsInfinity(averagePerplexity) || Double.IsNaN(averagePerplexity) ? 0.0 : averagePerplexity;
            summary.tps.latestPerplexity = Double.IsInfinity(latestPerplexity) || Double.IsNaN(latestPerplexity) ? 0.0 : latestPerplexity;

            var log2 = Math.Log(2.0);
            var logTC = -Math.Log(1.0 / (double)topicProbs.Length) / log2;
            summary.tps.topicEntropy = -topicProbs.Select(x => (x * Math.Log(x) / log2)).Sum() / logTC;

            double entropyAverage;
            summary.topicModel = this.MakeTopicModelSummary(topicProbs, wordProbMatrix, id2word, out entropyAverage);
            summary.tps.entropyAverage = entropyAverage;

            using (var fs = File.Create(fileName))
            {
                using (var writer = new Utf8JsonWriter(fs))
                {
                    JsonSerializer.Serialize(writer, summary);
                }
            }

            this.Upload();
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
                    return new Tuple<int, string, double, double>(kv.Key, id2word(kv.Key), entropy, sum);
                });
            var entropyDict = entropyList.ToDictionary(x => x.Item1);
            entropyAverage = entropyList.Select(x => x.Item3).Sum() / (double)entropyList.Count();

            // order by low entropy
            topicData.Add(entropyList
                .Select(x => new Tuple<string, double>(x.Item2, (1.0 - x.Item3)))
                .OrderByDescending(x => x.Item2)
                .Take(summryItemCount)
                .Select(x => new JsonType.WP { w = x.Item1, p = x.Item2 })
                .ToArray());

            // order by low entropy * propability
            topicData.Add(entropyList
                .Select(x => new Tuple<string, double>(x.Item2, (1.0 - x.Item3) * x.Item4))
                .OrderByDescending(x => x.Item2)
                .Take(summryItemCount)
                .Select(x => new JsonType.WP { w = x.Item1, p = x.Item2 })
                .ToArray());

            // order by high entropy
            topicData.Add(entropyList
                .OrderByDescending(x => x.Item3)
                .Take(summryItemCount)
                .Select(x => new JsonType.WP { w = x.Item2, p = x.Item4 })
                .ToArray());

            // order by high entropy * probability
            topicData.Add(entropyList
                .Select(x => new Tuple<string, double>(x.Item2, x.Item3 * x.Item4))
                .OrderByDescending(x => x.Item2)
                .Take(summryItemCount)
                .Select(x => new JsonType.WP { w = x.Item1, p = x.Item2 })
                .ToArray());

            var indexList = new int[topicCount];
            for (int topic = 0; topic < topicCount; ++topic)
            {
                indexList[topic] = topic;
            }
            var outputOrder = indexList.Select(x => new Tuple<int, double>(x, topicProbs[x])).OrderByDescending(x => x.Item2).Select(x => x.Item1).ToArray();

            for (int topic = 0; topic < topicCount; ++topic)
            {
                var topicIndex = outputOrder[topic];
                var sortByProb = wordProbMatrix
                    .Select(x => new Tuple<int, double>(x.Key, x.Value[topicIndex] * (1.0 - entropyDict[x.Key].Item3)))
                    .OrderByDescending(x => x.Item2)
                    .Take(summryItemCount)
                    .Select(x => new JsonType.WP { w = id2word(x.Item1), p = x.Item2 });

                topicData.Add(sortByProb.ToArray());
            }

            return topicData;
        }
        private void Upload()
        {
            var process = System.Diagnostics.Process.GetCurrentProcess(); // Or whatever method you are using
            string fullPath = process.MainModule.FileName;
            var folder = Path.GetDirectoryName(fullPath);
            var ftpUploader = Path.Combine(folder, Constants.ftpUploader);

            System.Diagnostics.ProcessStartInfo processStart = new System.Diagnostics.ProcessStartInfo("cmd.exe", "/C " + ftpUploader);
            processStart.CreateNoWindow = true;
            processStart.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;

            System.Diagnostics.Process p = System.Diagnostics.Process.Start(processStart);
            p.WaitForExit();
        }
    }
}
