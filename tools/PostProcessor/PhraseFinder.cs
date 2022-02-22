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
        public long c { get; set; } // count
    }

    public class PhraseSummary
    {
        public string summaryType { get; set; }
        public string generatedTime { get; set; }
        public List<Phrase> phraseList { get; set; }
        public List<Phrase> unknownPhrase { get; set; }
        public List<Phrase> unknownWords { get; set; }
        public List<Phrase> katakanaPhrase { get; set; }
        public List<Phrase> unknownKatakana { get; set; }
        public List<Phrase> unknownHiragana { get; set; }
        public List<Phrase> singleUnknown { get; set; }
        public List<Phrase> unknownKanji { get; set; }
        public List<Phrase> unknownOthers { get; set; }
        public List<Phrase> numbers { get; set; }
        public List<Phrase> emojis { get; set; }
        public List<Phrase> personNames { get; set; }
    }

    class WordTreeItem
    {
        public int wordId;
        public string wordText;
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
        private int bosId;
        private int eosId;

        private double log2;

        public PhraseFinder()
        {
            this.log2 = Math.Log(2.0);
        }

        public PhraseSummary FindAndSave(string fileName, string summaryFileName, Shared.NGramStore nGramStore, long [] totalCounts)
        {
            var nGramList = nGramStore.nGramList;
            var wordIdMappter = nGramStore.GetWordIdMapper();
            this.bosId = wordIdMappter.Item2("[BOS]");
            this.eosId = wordIdMappter.Item2("[EOS]");

            this.StoreWords(nGramStore, totalCounts);

            var scoredPhraseList = new List<Tuple<double, WordTreeItem>>();
            this.EvaluateWord(this.forward, scoredPhraseList, 1, 6,
                (WordTreeItem wi) => (wi.phraseProb * wi.entropy * wi.pairItem.entropy));

            // TODO: result text

            var phraseSummary = new PhraseSummary();
            phraseSummary.generatedTime = DateTime.Now.ToString();
            phraseSummary.summaryType = Constants.summaryTypePhraseList;

            var sortedAllPhrases = scoredPhraseList
                .OrderByDescending(x => x.Item1)
                //.Where(x => x.Item2.wordArray.Length > 1)
                .Select(x =>
                {
                    var phrase = new Phrase();
                    phrase.w = x.Item2.wordArray.Select(w => wordIdMappter.Item1(w)).ToArray();
                    phrase.p = x.Item1;
                    phrase.c = x.Item2.count;
                    return phrase;
                });

            phraseSummary.phraseList = sortedAllPhrases
                .Where(phrase => phrase.w.Length > 1)
                .Where(phrase => this.isAvailableAsPrediciton(phrase.w))
                .Take(100).ToList();

            phraseSummary.katakanaPhrase = sortedAllPhrases
                .Where(phrase => phrase.w.Length > 1)
                .Where(phrase => this.isAllKatakana(phrase.w))
                .Take(100).ToList();

            phraseSummary.unknownPhrase = sortedAllPhrases
                .Where(phrase => phrase.w.Length > 1)
                .Select(phrase => this.evaluateAsUnknown(phrase))
                .Where(x => x.Item1)
                .OrderByDescending(x => x.Item2 * x.Item3.p)
                .Take(100).Select(x => x.Item3).ToList();

            phraseSummary.unknownWords = sortedAllPhrases
                .Where(phrase => phrase.w.Length == 1)
                .Select(phrase => this.evaluateAsUnknown(phrase))
                .Where(x => x.Item1)
                .OrderByDescending(x => x.Item2 * x.Item3.p)
                .Take(100).Select(x => x.Item3).ToList();

            phraseSummary.unknownKatakana = sortedAllPhrases
                .Where(phrase => phrase.w.Length > 1)
                .Select(phrase => this.evaluateAsUnknownKatakana(phrase))
                .Where(x => x.Item1)
                .OrderByDescending(x => x.Item2 * x.Item3.p)
                .Take(100).Select(x => x.Item3).ToList();

            phraseSummary.unknownHiragana = sortedAllPhrases
                .Where(phrase => phrase.w.Length > 1)
                .Select(phrase => this.evaluateAsUnknownHiragana(phrase))
                .Where(x => x.Item1)
                .OrderByDescending(x => x.Item2 * x.Item3.p)
                .Take(100).Select(x => x.Item3).ToList();

            phraseSummary.unknownKanji = sortedAllPhrases
                //.Where(phrase => phrase.w.Length > 1)
                .Select(phrase => this.evaluateAsUnknownKanji(phrase))
                .Where(x => x.Item1)
                .OrderByDescending(x => x.Item2 * x.Item3.p)
                .Take(100).Select(x => x.Item3).ToList();

            phraseSummary.singleUnknown = sortedAllPhrases
                .Select(phrase => this.evaluateSingleKanaWord(phrase))
                .Where(x => x.Item1)
                .OrderByDescending(x => x.Item2 * x.Item3.p)
                .Take(100).Select(x => x.Item3).ToList();

            phraseSummary.unknownOthers = sortedAllPhrases
                //.Where(phrase => phrase.w.Length > 1)
                .Where(phrase => this.hasUnknownOthers(phrase.w))
                .Take(100).ToList();

            var id2text = nGramStore.GetWordIdMapper().Item1;
            var unigramList = this.forward.children
                .OrderByDescending(x => x.Value.count)
                .Select(x => new Tuple<string, WordTreeItem>(id2text(x.Value.wordId), x.Value));

            phraseSummary.numbers = unigramList
                .Where(x => isNumberPos.IsMatch(x.Item1))
                .Take(100)
                .Select(x =>
                {
                    var phrase = new Phrase();
                    phrase.w = new string[] { x.Item1 };
                    phrase.p = x.Item2.nGramProb;
                    phrase.c = x.Item2.count;
                    return phrase;
                })
                .ToList();

            phraseSummary.emojis = unigramList
                .Where(x => isEmojiPos.IsMatch(x.Item1))
                .Take(100)
                .Select(x =>
                {
                    var phrase = new Phrase();
                    phrase.w = new string[] { x.Item1 };
                    phrase.p = x.Item2.nGramProb;
                    phrase.c = x.Item2.count;
                    return phrase;
                })
                .ToList();

            phraseSummary.personNames = sortedAllPhrases
                .Where(phrase => phrase.w.Length > 1)
                .Where(phrase => this.isNamesPos(phrase.w))
                .Take(100).ToList();

            using (var fs = File.Create(summaryFileName))
            {
                using (var writer = new Utf8JsonWriter(fs))
                {
                    JsonSerializer.Serialize(writer, phraseSummary);
                }
            }
            return phraseSummary;
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
                if (kv.Value.wordArray.Length <= maxnGram)
                {
                    this.EvaluateWord(kv.Value, scoredList, minNgram, maxnGram, calculater);
                }
            }
        }

        void StoreWords(Shared.NGramStore nGramStore, long [] totalCounts)
        {
            var id2text = nGramStore.GetWordIdMapper().Item1;
            var nGramList = nGramStore.nGramList;
            for (int iGram = 0; iGram < nGramList.Length; ++iGram)
            {
                var nGram = nGramList[iGram];
                var totalCountsInDouble = totalCounts[iGram];
                foreach (var wordList in nGram)
                {
                    var intArray = new int[] { wordList.Key.Item1, wordList.Key.Item2, wordList.Key.Item3, wordList.Key.Item4, wordList.Key.Item5, wordList.Key.Item6, wordList.Key.Item7 };
                    Array.Resize(ref intArray, iGram + 1);
                    var forwardItem = this.StoreWordRecv(this.forward, 0, iGram, intArray, wordList.Value, id2text, false);
                    var backwordItem = this.StoreWordRecv(this.backward, iGram, 0, intArray, wordList.Value, id2text, true);
                    forwardItem.pairItem = backwordItem;
                    backwordItem.pairItem = forwardItem;
                    forwardItem.phraseProb = (double)forwardItem.count / totalCountsInDouble;
                    backwordItem.phraseProb = (double)backwordItem.count / totalCountsInDouble;
                }
            }
            this.CalcChildrenProbsdRecv(this.forward);
            this.CalcChildrenProbsdRecv(this.backward);
        }

        WordTreeItem StoreWordRecv(WordTreeItem parent, int nTh, int lastNth, int[] wordIds, long count, Func<int, string> id2text, bool isBackword)
        {
            int wordId = wordIds[nTh];
            WordTreeItem child;
            if (!parent.children.TryGetValue(wordId, out child))
            {
                child = new WordTreeItem(wordId);
                child.wordText = id2text(wordId);
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
                return this.StoreWordRecv(child, nTh + 1, lastNth, wordIds, count, id2text, isBackword);
            }
            else
            {
                return this.StoreWordRecv(child, nTh - 1, lastNth, wordIds, count, id2text, isBackword);
            }
        }

        void CalcChildrenProbsdRecv(WordTreeItem parent)
        {
            if (parent.children.Count <= 1)
            {
                parent.entropy = 1.0 / Single.MaxValue; // do not touch to 0
            }
            else
            {
                long sum = 0;

                foreach (var kv in parent.children)
                {
                    sum += kv.Value.count;
                }

                if (sum == 0.0)
                {
                    throw new Exception("sum is 0");
                }

                double entropyWork = 0.0;
                foreach (var kv in parent.children)
                {
                    var prob = (double)kv.Value.count / (double)sum;
                    kv.Value.nGramProb = prob;
                    entropyWork += prob * Math.Log(prob) / this.log2;
                }

                var maxEntropy = -Math.Log(1.0 / (double)parent.children.Count) / this.log2;

                if (maxEntropy == 0.0)
                {
                    throw new Exception("maxEntropy is 0");
                }

                parent.entropy = -entropyWork / maxEntropy;
            }

            if (parent.children.Any(x => (x.Value.wordId == this.bosId) || (x.Value.wordId == this.eosId)))
            {
                // boost up for BOS or EOS.
                parent.entropy = Math.Max(parent.entropy, parent.entropy * 0.5 + 0.5);
            }

            foreach (var kv in parent.children)
            {
                if (kv.Value.children.Count > 0)
                {
                    this.CalcChildrenProbsdRecv(kv.Value);
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


        private static Regex matchFiller = new Regex(@",フィラー,");
        private static Regex matchNoReading = new Regex(@",\*,\*$");
        private static Regex matchNoTargeting = new Regex(@"(,名詞,固有名詞,人名,姓,|,名詞,固有名詞,人名,名,|,記号,)");
        private static Regex followingType = new Regex(@"(,助詞,|,助動詞,|,非自立,|,接尾,)");
        private static Regex lastDisallowedType = new Regex(@"(,接頭詞,)");
        private static Regex lastRequiredType = new Regex(@",(\*|基本形),[^,]+,[^,]+,[^,]+$");
        private static Regex allKatakana = new Regex(@"^[ァ-ヶー]+,");
        private static Regex allHiragana = new Regex(@"^[ぁ-ゖー]+,");
        private static Regex isNumberPos = new Regex(@",名詞,数,");
        private static Regex isNumber = new Regex(@"^[０-９]+,");
        private static Regex isAlphabet = new Regex(@"^[Ａ-Ｚａ-ｚ]+,");
        private static Regex hasKanji = new Regex(@"^[\u2E80-\u2FDF々〻\u3400-\u4DBF\u4E00-\u9FFF\uF900-\uFAFF]+,"); // \u20000-\u2FFFF ??
        private static Regex isEmojiPos = new Regex(@",記号,絵文字,");
        private static Regex isAllowedSingleCharPos = new Regex(@",助詞,|,助動詞,|,動詞,|,形容詞,|,接頭詞,|,接続詞,|,名詞,接尾,|,名詞,非自立,");
        private static Regex isOneCharKana= new Regex(@"^[ぁ-ゖァ-ヶ],"); // exclude 'ー', because it is used wrongly frequently.
        private static Regex isNamePos = new Regex(@",名詞,固有名詞,人名,");
        private static Regex isSymbolPos = new Regex(@",記号,");
        // private static Regex isExcludingSymbols = new Regex(@"^[（）．，＆＃；－／％]+,");

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
            if (wordArray.Select(x => x.Split(new char[] { ',' })[8].Length).Sum() < 4)
            {
                return false;
            }

            return true;
        }

        bool isUnknownWord(string x)
        {
            return (matchNoReading.IsMatch(x) || matchFiller.IsMatch(x)) && !isNumberPos.IsMatch(x) && !isAlphabet.IsMatch(x) /* && !isExcludingSymbols.IsMatch(x) */;
        }

        Tuple<bool, double, Phrase> evaluateAsUnknown(Phrase phrase)
        {
            var wordArray = phrase.w;
            if (wordArray[0].Equals("[BOS]") || wordArray.Last().Equals("[EOS]"))
            {
                return new Tuple<bool, double, Phrase>(false, 1.0, phrase);
            }
            if (!wordArray.Any(x => this.isUnknownWord(x)))
            {
                return new Tuple<bool, double, Phrase>(false, 1.0, phrase);
            }
            var isBoost = this.isUnknownWord(wordArray[wordArray.Length / 2]);
            return new Tuple<bool, double, Phrase>(true, isBoost ? 1.0 : 0.5, phrase);
        }

        bool isAllKatakana(string [] wordArray)
        {
            if (wordArray[0].Equals("[BOS]") || wordArray.Last().Equals("[EOS]"))
            {
                return false;
            }
            return wordArray.All(x => allKatakana.IsMatch(x))
                && wordArray.Any(x => (x.Split(',')[0].Length <= 2 || matchNoReading.IsMatch(x) || matchFiller.IsMatch(x)));
        }

        bool isUnknownHiragana(string x)
        {
            return (matchNoReading.IsMatch(x) || matchFiller.IsMatch(x)) && allHiragana.IsMatch(x);
        }

        Tuple<bool, double, Phrase> evaluateAsUnknownHiragana(Phrase phrase)
        {
            var wordArray = phrase.w;
            if (wordArray[0].Equals("[BOS]") || wordArray.Last().Equals("[EOS]"))
            {
                return new Tuple<bool, double, Phrase>(false, 1.0, phrase);
            }
            if (!wordArray.Any(x => isUnknownHiragana(x)))
            {
                return new Tuple<bool, double, Phrase>(false, 1.0, phrase);
            }
            var isBoost = this.isUnknownHiragana(wordArray[wordArray.Length / 2]);
            return new Tuple<bool, double, Phrase>(true, isBoost ? 1.0 : 0.5, phrase);
        }

        bool isUnknownKatakana(string x)
        {
            return (matchNoReading.IsMatch(x) || matchFiller.IsMatch(x)) && allKatakana.IsMatch(x);
        }

        Tuple<bool, double, Phrase> evaluateAsUnknownKatakana(Phrase phrase)
        {
            var wordArray = phrase.w;
            if (wordArray[0].Equals("[BOS]") || wordArray.Last().Equals("[EOS]"))
            {
                return new Tuple<bool, double, Phrase>(false, 1.0, phrase);
            }
            if (!wordArray.Any(x => isUnknownKatakana(x)))
            {
                return new Tuple<bool, double, Phrase>(false, 1.0, phrase);
            }
            var isBoost = this.isUnknownKatakana(wordArray[wordArray.Length / 2]);
            return new Tuple<bool, double, Phrase>(true, isBoost ? 1.0 : 0.5, phrase);
        }

        bool isUnknownKanji(string x)
        {
            return (matchNoReading.IsMatch(x) || matchFiller.IsMatch(x)) && hasKanji.IsMatch(x);
        }

        Tuple<bool, double, Phrase> evaluateAsUnknownKanji(Phrase phrase)
        {
            var wordArray = phrase.w;
            if (wordArray[0].Equals("[BOS]") || wordArray.Last().Equals("[EOS]"))
            {
                return new Tuple<bool, double, Phrase>(false, 1.0, phrase);
            }
            if (!wordArray.Any(x => isUnknownKanji(x)))
            {
                return new Tuple<bool, double, Phrase>(false, 1.0, phrase);
            }
            var isBoost = this.isUnknownKanji(wordArray[wordArray.Length / 2]);
            return new Tuple<bool, double, Phrase>(true, isBoost ? 1.0 : 0.5, phrase);
        }

        bool hasUnknownOthers(string[] wordArray)
        {
            if (wordArray[0].Equals("[BOS]") || wordArray.Last().Equals("[EOS]"))
            {
                return false;
            }
            return wordArray.Any(x => (matchNoReading.IsMatch(x) || matchFiller.IsMatch(x)) && !isNumber.IsMatch(x) && !hasKanji.IsMatch(x) && !allHiragana.IsMatch(x) && !allKatakana.IsMatch(x));
        }

        bool isNamesPos(string[] wordArray)
        {
            if (wordArray[0].Equals("[BOS]") || wordArray.Last().Equals("[EOS]"))
            {
                return false;
            }
            return wordArray.Count(x => isNamePos.IsMatch(x)) >= 2 && wordArray.All(x => (isNamePos.IsMatch(x) || isSymbolPos.IsMatch(x)));
        }

        bool isOneKanaChar(string x)
        {
            return !isAllowedSingleCharPos.IsMatch(x) && isOneCharKana.IsMatch(x);
        }

        Tuple<bool, double, Phrase> evaluateSingleKanaWord(Phrase phrase)
        {
            var wordArray = phrase.w;
            if (wordArray[0].Equals("[BOS]") || wordArray.Last().Equals("[EOS]") || wordArray.Length < 3)
            {
                return new Tuple<bool, double, Phrase>(false, 1.0, phrase);
            }
            if (!(wordArray.Any(x => isOneKanaChar(x)) && wordArray.Any(x => hasKanji.IsMatch(x))))
            {
                return new Tuple<bool, double, Phrase>(false, 1.0, phrase);
            }
            var isBoost = this.isOneKanaChar(wordArray[wordArray.Length / 2]);
            return new Tuple<bool, double, Phrase>(true, isBoost ? 1.0 : 0.5, phrase);
        }

    }
}