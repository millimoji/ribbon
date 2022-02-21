using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace Ribbon.PostProcessor
{
    class PhraseDiffMaker
    {
        string workDir;

        public PhraseDiffMaker(string workDir)
        {
            this.workDir = workDir;
        }

        public void MakeDiff(PhraseSummary newSummary)
        {
            var oldSummary = this.ReadOldestPhrase();

            var diffSummary = new PhraseSummary();
            diffSummary.generatedTime = $"{oldSummary.generatedTime} => ${newSummary.generatedTime}";

            diffSummary.phraseList = this.MakeDiff(newSummary.phraseList, oldSummary.phraseList);
            diffSummary.unknownPhrase = this.MakeDiff(newSummary.unknownPhrase, oldSummary.unknownPhrase);
            diffSummary.unknownWords = this.MakeDiff(newSummary.unknownWords, oldSummary.unknownWords);
            diffSummary.katakanaPhrase = this.MakeDiff(newSummary.katakanaPhrase, oldSummary.katakanaPhrase);
            diffSummary.unknownKatakana = this.MakeDiff(newSummary.unknownKatakana, oldSummary.unknownKatakana);
            diffSummary.unknownHiragana = this.MakeDiff(newSummary.unknownHiragana, oldSummary.unknownHiragana);
            diffSummary.singleUnknown = this.MakeDiff(newSummary.singleUnknown, oldSummary.singleUnknown);
            diffSummary.unknownKanji = this.MakeDiff(newSummary.unknownKanji, oldSummary.unknownKanji);
            diffSummary.unknownOthers = this.MakeDiff(newSummary.unknownOthers, oldSummary.unknownOthers);
            diffSummary.numbers = this.MakeDiff(newSummary.numbers, oldSummary.numbers);
            diffSummary.emojis = this.MakeDiff(newSummary.emojis, oldSummary.emojis);
            diffSummary.personNames = this.MakeDiff(newSummary.personNames, oldSummary.personNames);

            using (var fs = File.Create(this.workDir + Constants.phraseListSummaryDiff))
            {
                using (var writer = new Utf8JsonWriter(fs))
                {
                    JsonSerializer.Serialize(writer, diffSummary);
                }
            }
        }

        PhraseSummary ReadOldestPhrase()
        {
            string tryFileName = this.workDir + Constants.old3prefix + Constants.phraseListSummary;
            if (!File.Exists(tryFileName))
            {
                tryFileName = this.workDir + Constants.old2prefix + Constants.phraseListSummary;
                if (!File.Exists(tryFileName))
                {
                    tryFileName = this.workDir + Constants.old1prefix + Constants.phraseListSummary;
                    if (!File.Exists(tryFileName))
                    {
                        return new PhraseSummary();
                    }
                }
            }

            string allLine;
            {
                using (var sr = new StreamReader(tryFileName, Encoding.UTF8))
                {
                    allLine = sr.ReadToEnd();
                }
            }

            var oldSummary = JsonSerializer.Deserialize<PhraseSummary>(allLine);

            return oldSummary;
        }


        List<Phrase> MakeDiff(List<Phrase> newPhrases, List<Phrase> oldPhrases)
        {
            var diffList = new List<Phrase>();

            if (oldPhrases != null && oldPhrases.Count > 0)
            {
                var hashOld = oldPhrases.Select(x => String.Join("", x.w.Select(w => w.Split(',')[0]).ToArray())).ToHashSet();

                foreach (var newPhrase in newPhrases)
                {
                    var newPhraseText = String.Join("", newPhrase.w.Select(w => w.Split(',')[0]).ToArray());
                    if (!hashOld.Contains(newPhraseText))
                    {
                        diffList.Add(newPhrase);
                    }
                }
            }

            return diffList;
        }





    }
}
