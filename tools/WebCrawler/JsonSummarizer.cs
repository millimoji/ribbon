using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;

namespace Ribbon.WebCrawler
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
        }
        public class SummaryStruct
        {
            public string generatedTime { get; set; }
            public TopicModelSummary tps { get; set; } = new TopicModelSummary();
            public List<WP[]> topicModel { get; set; }
        }
    }

    class JsonSummarizer
    {
        private const int summryItemCount = 100;

        public void Serialize(
            string fileName,
            double perplexity,
            Dictionary<int, double[]> topicLogWordRate,
            Func<int, string> id2word)
        {
            var summary = new Ribbon.WebCrawler.JsonType.SummaryStruct();
            summary.generatedTime = DateTime.Now.ToString();
            summary.tps.perplexity = Double.IsInfinity(perplexity) || Double.IsNaN(perplexity) ? 0.0 : perplexity;
            summary.topicModel = this.MakeTopicModelSummary(topicLogWordRate, id2word);

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
            Dictionary<int, double[]> topicLogWordRate,
            Func<int, string> id2word)
        {
            var topicData = new List<JsonType.WP[]>();

            var entroyList = topicLogWordRate
                .Select(x =>
                {
                    var sum = x.Value.Select(l => Math.Exp(l)).Sum();
                    var pLogP = x.Value.Select(l =>
                    {
                        var p = Math.Exp(l) / sum;
                        var itempLogP = p * Math.Log(p);
                        return itempLogP;
                    });
                    var entropy = - pLogP.Sum();
                    return new Tuple<int, double>(x.Key, entropy);
                });

            // order by low entropy
            topicData.Add(entroyList
                .OrderBy(x => x.Item2)
                .Take(summryItemCount)
                .Select(x => new JsonType.WP { w = id2word(x.Item1), p = x.Item2 })
                .ToArray());

            // order by high entropy
            topicData.Add(entroyList
                .OrderByDescending(x => x.Item2)
                .Take(summryItemCount)
                .Select(x => new JsonType.WP { w = id2word(x.Item1), p = x.Item2 })
                .ToArray());

            var topicCount = topicLogWordRate.First().Value.Length;
            for (int topic = 0; topic < topicCount; ++topic)
            {
                var sortByProb = topicLogWordRate
                    .Select(x => new Tuple<int, double>(x.Key, x.Value[topic]))
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
            var ftpUploader = Path.Combine(folder, Program.ftpUploader);

            System.Diagnostics.ProcessStartInfo processStart = new System.Diagnostics.ProcessStartInfo("cmd.exe", "/C " + ftpUploader);
            processStart.CreateNoWindow = true;
            processStart.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;

            System.Diagnostics.Process p = System.Diagnostics.Process.Start(processStart);
            p.WaitForExit();
        }
    }
}
