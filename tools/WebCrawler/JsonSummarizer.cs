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
        public class SummaryStruct
        {
            public List<WP[]> topicModel { get; set; }
        }
    }

    class JsonSummarizer
    {
        private const int summryItemCount = 100;

        public void Serialize(
            string fileName,
            Dictionary<int, double[]> topicLogWordRate,
            Func<int, string> id2word)
        {
            var summary = new Ribbon.WebCrawler.JsonType.SummaryStruct();
            summary.topicModel = this.MakeTopicModelSummary(topicLogWordRate, id2word);

            using (var fs = File.Create(fileName))
            {
                using (var writer = new Utf8JsonWriter(fs))
                {
                    JsonSerializer.Serialize(writer, summary);
                }
            }
        }

        private List<JsonType.WP[]> MakeTopicModelSummary(
            Dictionary<int, double[]> topicLogWordRate,
            Func<int, string> id2word)
        {
            var topicData = new List<JsonType.WP[]>();

            var sortByEntropy = topicLogWordRate
                .Select(x =>
                {
                    var sum = x.Value.Select(l => Math.Exp(l)).Sum();
                    var h = -x.Value.Select(l =>
                    {
                        var p = Math.Exp(l) / sum;
                        return p * Math.Log(p);
                    }).Sum();
                    return new Tuple<int, double>(x.Key, h);
                })
                .OrderBy(x => x.Item2)
                .Take(summryItemCount)
                .Select(x => new JsonType.WP { w = id2word(x.Item1), p = x.Item2 });

            topicData.Add(sortByEntropy.ToArray());

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
    }
}