using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace Ribbon.PostProcessor
{
    public class PosBigramList
    {
        public string summaryType { get; set; }
        public string generatedTime { get; set; }
        public List<string> posOrder { get; set; }
        public List<List<int>> posBigram { get; set; }
    }

    class PosComparer : IComparer<string>
    {
        public int Compare(string l, string r)
        {
            var lFields = l.Split(',');
            var rFields = r.Split(',');

            if (lFields.Length == 1 || rFields.Length == 1) // for BOS,EOS
            {
                return String.Compare(l, r);
            }

            for (int i = 1; i < 7; ++i)
            {
                int x = String.Compare(lFields[i], rFields[i]);
                if (x != 0)
                {
                    return x;
                }
            }
            return String.Compare(lFields[0], rFields[0]);
        }
    }

    class PosListMaker
    {
        string workDir;

        public PosListMaker(string workDir)
        {
            this.workDir = workDir;
        }

        public void OutputPosBigram(Shared.NGramStore nGramStore)
        {
            var posListHash = nGramStore.posBigram.Select(x => x.Key.Item1).ToHashSet();
            posListHash.UnionWith(nGramStore.posBigram.Select(x => x.Key.Item2));
            var posListOrder = posListHash.OrderBy(x => x, new PosComparer()).ToList();

            var posBigramList = new PosBigramList();
            posBigramList.summaryType = Constants.summaryTypePosBigram;
            posBigramList.generatedTime = DateTime.Now.ToString();

            posBigramList.posOrder = posListOrder;
            posBigramList.posBigram = new List<List<int>>(posListOrder.Count);

            var posModel = nGramStore.posBigram;

            foreach (var lPosName in posListOrder)
            {
                var column = new List<int>(posListOrder.Count);
                foreach (var rPosName in posListOrder)
                {
                    var key = new Tuple<string, string>(lPosName, rPosName);
                    column.Add(posModel.ContainsKey(key) ?
                            (int)Math.Min(posModel[key], 999) :
                            -1);
                }
                posBigramList.posBigram.Add(column);
            }

            using (var fs = File.Create(this.workDir + Constants.posBigramSummary))
            {
                using (var writer = new Utf8JsonWriter(fs))
                {
                    JsonSerializer.Serialize(writer, posBigramList);
                }
            }
        }
    }
}
