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
#if true
            this.BuildNgram();
#else
            var targetFiles = new string []
            {
                Constants.topicModelSummaryFilename,
                Constants.mixUnigramSummaryFilename,
                Constants.phraseList,
                Constants.phraseListSummary,
                Constants.phraseListSummaryDiff,
                Constants.posBigramSummary,
            };
            Shared.FileOperation.SlideDataFile(targetFiles, Constants.workingFolder);

            var nGramStore = new Shared.NGramStore(Constants.workingFolder);
            var totalCounts = nGramStore.LoadFromFile(10 /* cutout */);

            var posListMaker = new PosListMaker(Constants.workingFolder);
            posListMaker.OutputPosBigram(nGramStore);

            this.FindPhraseAndSave(nGramStore, totalCounts);

            this.SummarizeTopicModel(nGramStore);

            Shared.FileOperation.Upload();
#endif
        }

        void BuildNgram()
        {
            var morphAnalyzer = new Shared.MorphAnalyzer();
            var prSw = morphAnalyzer.PrepareProcess();

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
                        }
                    }
                }
            }
            finally
            {
                morphAnalyzer.CloseProcess(prSw.Item1, prSw.Item2);
            }
        }

        void FindPhraseAndSave(Shared.NGramStore nGramStore, long [] totalCounts)
        {
            var phraseFinder = new PhraseFinder();
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
    }
}
