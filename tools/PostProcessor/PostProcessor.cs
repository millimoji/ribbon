using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Ribbon.PostProcessor
{
    class PostProcessor
    {
        public void Run()
        {
            var nGramStore = new Shared.NGramStore(Constants.workingFolder, false);
            this.BuildNgram(nGramStore);

            var targetFiles = new string[]
            {
                Constants.phraseList,
                Constants.phraseListSummary,
                Constants.phraseListSummaryDiff,
                Constants.posBigramSummary,
            };
            Shared.FileOperation.SlideDataFile(targetFiles, Constants.workingFolder);

            var nGramSourceCounts = nGramStore.GetNGramSourceCounts();

            var phraseFinder = new PhraseFinder(10 /* cutout */);
            var phraseSummary = phraseFinder.FindAndSave(Constants.workingFolder + Constants.phraseList, Constants.workingFolder + Constants.phraseListSummary, nGramStore, nGramSourceCounts);

            var phraseDiffMaker = new PhraseDiffMaker(Constants.workingFolder);
            phraseDiffMaker.MakeDiff(phraseSummary);

            var posListMaker = new PosListMaker(Constants.workingFolder);
            posListMaker.OutputPosBigram(nGramStore);

            // this.SummarizeTopicModel(nGramStore);

            Shared.FileOperation.Upload();
        }

        void BuildNgram(Shared.NGramStore nGramStore)
        {
            var morphAnalyzer = new Shared.MorphAnalyzer();
            var prSw = morphAnalyzer.PrepareProcess();

            long stateCount = 0;
            try
            {
                var sourceFiles = Shared.CompressedFileSet.getNewerFiles(1);
                foreach (var sourceFile in sourceFiles)
                {
                    using (var decompressor = new Shared.CompressedStreamReader(sourceFile))
                    {
                        for (; ; )
                        {
                            var sourceText = decompressor.ReadLine();
                            if (sourceText.Length == 0)
                            {
                                break;
                            }
                            var morph = morphAnalyzer.AnalyzeSingletext(prSw.Item1, prSw.Item2, sourceText);
                            if (morph != null)
                            {
                                nGramStore.AddFromWordArray(morph);
                            }

                            ++stateCount;
                            if ((stateCount % 20000) == 0)
                            {
                                Console.WriteLine($"Current state count: {stateCount}");
                            }
                        }
                    }
                }
                Console.WriteLine($"Total state count: {stateCount}");
            }
            finally
            {
                morphAnalyzer.CloseProcess(prSw.Item1, prSw.Item2);
            }
        }

#if false
        void FindPhraseAndSave(Shared.NGramStore nGramStore, long [] totalCounts)
        {
            var phraseFinder = new PhraseFinder(0);
            var newSummary = phraseFinder.FindAndSave(Constants.workingFolder + Constants.phraseList, Constants.workingFolder + Constants.phraseListSummary, nGramStore, totalCounts);

            var phraseDiffMaker = new PhraseDiffMaker(Constants.workingFolder);
            phraseDiffMaker.MakeDiff(newSummary);
        }

        void SummarizeTopicModel(Shared.NGramStore nGramStore)
        {
            var wordIdMapper = nGramStore.GetWordIdMapper();

            var topicModelHandler = new Shared.TopicModelHandler(false);
            topicModelHandler.LoadFromFile(Constants.workingFolder + Constants.topicModelFileName, wordIdMapper.Item2, wordIdMapper.Item1);

            var mixedUnigramHandler = new Shared.TopicModelHandler(true);
            mixedUnigramHandler.LoadFromFile(Constants.workingFolder + Constants.mixUnigramlFileName, wordIdMapper.Item2, wordIdMapper.Item1);

            var topicModelSummarizer = new TopicModelSummarizer();
            topicModelSummarizer.MakeSumarize(Constants.workingFolder + Constants.topicModelSummaryFilename, topicModelHandler, wordIdMapper.Item1);
            topicModelSummarizer.MakeSumarize(Constants.workingFolder + Constants.mixUnigramSummaryFilename, mixedUnigramHandler, wordIdMapper.Item1);
        }
#endif
    }
}
