using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;


namespace Ribbon
{
    class Constants
    {
        // constants
        public const string mecabExe = "c:\\Program Files (x86)\\MeCab\\bin\\mecab.exe";
        public const string workingFolder = "c:\\lmworking\\";
        public const string ftpUploader = "..\\..\\..\\webui\\ftpupload.cmd";

        public const string topicModelFileName = "topicmodel.txt";
        public const string topicModelSummaryFilename = "topicmodel-summary.json";
        public const string mixUnigramlFileName = "mixunigram.txt";
        public const string mixUnigramSummaryFilename = "mixunigram-summary.json";

        public const string phraseList = "phrase-list.txt";
        public const string phraseListSummary = "phrase-list-summary.json";

        public const string old3prefix = "old3-";
        public const string old2prefix = "old2-";
        public const string old1prefix = "old-";

        public const string summaryTopicModel = "topicModel";
        public const string summaryPhraseList = "phraseList";
    }
}
