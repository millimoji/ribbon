using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Ribbon.PostProcessor
{
    public class Phrase
    {
        public string [] w { get; set; } // word array
        public double p { get; set; } // probability
    }

    public class PhraseSummary
    {
        public string summaryType { get; set; }
        public string generatedTime { get; set; }
        public List<Phrase> phraseList { get; set; }
        public List<Phrase> unknownPhrase { get; set; }
        public List<Phrase> unknownKatakana { get; set; }
    }

    class WordTreeItem
    {
        public int wordId;
        public int[] wordArray;
        public Dictionary<int, WordTreeItem> children = new Dictionary<int, WordTreeItem>();
        public WordTreeItem parent;
        public WordTreeItem pairItem;

        public long count = 0;
        public double nGramProb; // P(tail|presequnece)
        public double phraseProb; // P(phrase)
        public double entropy;

        public WordTreeItem(int wordId) { this.wordId = wordId; }
    }

    class PhraseFinder
    {
        private WordTreeItem forward = new WordTreeItem(0);
        private WordTreeItem backward = new WordTreeItem(0);

        private double log2;

        public PhraseFinder()
        {
            this.log2 = Math.Log(2.0);
        }

        public void FindAndSave(string fileName, string summaryFileName, Shared.NGramStore nGramStore)
        {
            var nGramList = nGramStore.nGramList;
            var wordIdMappter = nGramStore.GetWordIdMapper();
            this.StoreWords(nGramStore);

            var scoredPhraseList = new List<Tuple<double, WordTreeItem>>();
            this.EvaluateWord(this.forward, scoredPhraseList, 2, 6,
                (WordTreeItem wi) => (wi.phraseProb * wi.entropy * wi.pairItem.entropy));

            // TODO: result text

            var phraseSummary = new PhraseSummary();
            phraseSummary.generatedTime = DateTime.Now.ToString();
            phraseSummary.summaryType = Constants.summaryPhraseList;

            var sortedAllPhrases = scoredPhraseList
                .OrderByDescending(x => x.Item1)
                .Select(x =>
                {
                    var phrase = new Phrase();
                    phrase.w = x.Item2.wordArray.Select(w => wordIdMappter.Item1(w)).ToArray();
                    phrase.p = x.Item1;
                    return phrase;
                });

            phraseSummary.phraseList = sortedAllPhrases
                .Where(phrase => this.isAvailableAsPrediciton(phrase.w))
                .Take(100)
                .ToList();

            phraseSummary.unknownPhrase = sortedAllPhrases
                .Where(phrase => this.hasUnknownWords(phrase.w))
                .Take(100)
                .ToList();

            phraseSummary.unknownKatakana = sortedAllPhrases
                .Where(phrase => this.isAllKatakana(phrase.w))
                .Take(100)
                .ToList();

            using (var fs = File.Create(summaryFileName))
            {
                using (var writer = new Utf8JsonWriter(fs))
                {
                    JsonSerializer.Serialize(writer, phraseSummary);
                }
            }
        }


        void EvaluateWord(WordTreeItem parent, List<Tuple<double, WordTreeItem>> scoredList, int minNgram, int maxnGram, Func<WordTreeItem, double> calculater)
        {
            foreach (var kv in parent.children)
            {
                if (kv.Value.wordArray.Length >= minNgram)
                {
                    var score = calculater(kv.Value);
                    scoredList.Add(new Tuple<double, WordTreeItem>(score, kv.Value));
                }
                if (kv.Value.wordArray.Length < maxnGram)
                {
                    this.EvaluateWord(kv.Value, scoredList, minNgram, maxnGram, calculater);
                }
            }
        }

        void StoreWords(Shared.NGramStore nGramStore)
        {
            var nGramList = nGramStore.nGramList;
            for (int iGram = 0; iGram < nGramList.Length; ++iGram)
            {
                var nGram = nGramList[iGram];
                long totalCount = 0;
                foreach (var wordList in nGram)
                {
                    var intArray = new int[] { wordList.Key.Item1, wordList.Key.Item2, wordList.Key.Item3, wordList.Key.Item4, wordList.Key.Item5, wordList.Key.Item6, wordList.Key.Item7 };
                    Array.Resize(ref intArray, iGram + 1);
                    var forwardItem = this.StoreWordRecv(this.forward, 0, iGram, intArray, wordList.Value, false);
                    var backwordItem = this.StoreWordRecv(this.backward, iGram, 0, intArray, wordList.Value, true);
                    forwardItem.pairItem = backwordItem;
                    backwordItem.pairItem = forwardItem;
                    totalCount += wordList.Value;
                }
                this.CalcPhraseProbRecv(this.forward, iGram, (double)totalCount);
                this.CalcPhraseProbRecv(this.backward, iGram, (double)totalCount);
            }
            this.CalcChildrenProbsdRecv(this.forward);
            this.CalcChildrenProbsdRecv(this.backward);
        }

        WordTreeItem StoreWordRecv(WordTreeItem parent, int nTh, int lastNth, int[] wordIds, long count, bool isBackword)
        {
            int wordId = wordIds[nTh];
            WordTreeItem child;
            if (!parent.children.TryGetValue(wordId, out child))
            {
                child = new WordTreeItem(wordId);
                child.wordArray = wordIds;
                child.parent = parent;
                parent.children.Add(wordId, child);
            }
            // TODO: 0 check?
            if (nTh == lastNth)
            {
                child.count = count;
                return child;
            }
            else if (!isBackword)
            {
                return this.StoreWordRecv(child, nTh + 1, lastNth, wordIds, count, isBackword);
            }
            else
            {
                return this.StoreWordRecv(child, nTh - 1, lastNth, wordIds, count, isBackword);
            }
        }

        void CalcChildrenProbsdRecv(WordTreeItem parent)
        {
            long sum = 0;

            foreach (var kv in parent.children)
            {
                sum += kv.Value.count;
            }

            double entropyWork = 0.0;
            foreach (var kv in parent.children)
            {
                var prob = (double)kv.Value.count / (double)sum;
                kv.Value.nGramProb = prob;
                entropyWork += prob * Math.Log(prob) / this.log2;
            }

            var maxEntropy = -Math.Log(1.0 / (double)parent.children.Count) / this.log2;
            parent.entropy = -entropyWork / maxEntropy;

            foreach (var kv in parent.children)
            {
                if (kv.Value.children.Count > 0)
                {
                    this.CalcChildrenProbsdRecv(kv.Value);
                }
            }
        }
        void CalcPhraseProbRecv(WordTreeItem parent, int leftTrace, double total)
        {
            foreach (var kv in parent.children)
            {
                if (leftTrace == 0)
                {
                    kv.Value.phraseProb = (double)kv.Value.count / total;
                }
                else
                {
                    this.CalcPhraseProbRecv(kv.Value, leftTrace - 1, total);
                }
            }
        }

        WordTreeItem FindForwardItem(int[] wordIds)
        {
            WordTreeItem current = this.forward;
            for (int i = 0; i < wordIds.Length; ++i)
            {
                WordTreeItem next;
                if (!current.children.TryGetValue(wordIds[i], out next))
                {
                    return null;
                }
                current = next;
            }
            return current;
        }

        WordTreeItem FindBackwardItem(int[] wordIds)
        {
            WordTreeItem current = this.backward;
            for (int i = wordIds.Length; i >= 0; --i)
            {
                WordTreeItem next;
                if (!current.children.TryGetValue(wordIds[i], out next))
                {
                    return null;
                }
                current = next;
            }
            return current;
        }


        private static Regex matchNoReading = new Regex(@",\*,\*$");
        private static Regex matchNoTargeting = new Regex(@"(,名詞,固有名詞,人名,姓,|,名詞,固有名詞,人名,名,|,記号,)");
        private static Regex followingType = new Regex(@"(,助詞,|,助動詞,|,非自立,|,接尾,)");
        private static Regex lastDisallowedType = new Regex(@"(,接頭詞,)");
        private static Regex lastRequiredType = new Regex(@",(\*|基本形),[^,]+,[^,]+,[^,]+$");
        private static Regex allKatakana = new Regex(@"^[ァ-ヶー]+,");
        private static Regex isNumber = new Regex(@",名詞,数,");
        private static Regex isAlphabet = new Regex(@"^[Ａ-Ｚａ-ｚ]+,");
        private static Regex isExcludingSymbols = new Regex(@"^[（）．，＆＃；－／％]+,");

        bool isAvailableAsPrediciton(string[] wordArray)
        {
            if (wordArray[0].Equals("[BOS]") || wordArray.Last().Equals("[EOS]"))
            {
                return false;
            }
            if (followingType.IsMatch(wordArray[0]))
            {
                return false;
            }
            if (lastDisallowedType.IsMatch(wordArray.Last()))
            {
                return false;
            }
            if (!lastRequiredType.IsMatch(wordArray.Last()))
            {
                return false;
            }
            for (int i = 0; i < wordArray.Length; ++i)
            {
                if (matchNoReading.IsMatch(wordArray[i]) || matchNoTargeting.IsMatch(wordArray[i]))
                {
                    return false;
                }
            }
            return true;
        }

        bool hasUnknownWords(string [] wordArray)
        {
            if (wordArray[0].Equals("[BOS]") || wordArray.Last().Equals("[EOS]"))
            {
                return false;
            }
            return wordArray.Any(x => matchNoReading.IsMatch(x) && !isNumber.IsMatch(x) && !isAlphabet.IsMatch(x) && !isExcludingSymbols.IsMatch(x));
        }

        bool isAllKatakana(string [] wordArray)
        {
            if (wordArray[0].Equals("[BOS]") || wordArray.Last().Equals("[EOS]"))
            {
                return false;
            }
            return wordArray.All(x => allKatakana.IsMatch(x))
                && wordArray.Any(x => x.Split(',')[0].Length <= 2);
        }
    }
}