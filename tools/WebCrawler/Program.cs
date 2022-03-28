using Ribbon.Shared;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.Remoting.Messaging;
using System.Text;
using System.Threading.Tasks;

namespace Ribbon.WebCrawler
{
    class Program
    {
        static void Main(string[] _)
        {
            var me = new Program();
            me.RunCrawlerOnly();
        }

        class DownloadTaskResult
        {
            public HashSet<string> contentTexts = new HashSet<string>();
            public HashSet<string> pageUrls = new HashSet<string>();
            public HashSet<string> referenceUrls = new HashSet<string>();
        }

        const int saveInternvalHour = 24 * 6; // disabled for now
        const int exitIntervalHour = 24;
        const int parallelDownload = 10;

        private readonly Random random = new Random();
        private DateTime lastSavedTime = DateTime.Now;
        private readonly DateTime programStartTime = DateTime.Now;
        private bool isEveryHourMode = false; // disabled fo rnow
        private bool exitProgram = false;

        const int maxCotentHistory = 40000;
        readonly Dictionary<string, DateTime> contentHistoryDate = new Dictionary<string, DateTime>();

        private readonly string[] focusedDomains = new string[]
        {
            "https://www.asahi.com/",
            "https://www.yomiuri.co.jp/",
            "https://mainichi.jp/",
            "https://www.nikkei.com/",
            "https://www.sankei.com/",
            "https://www.itmedia.co.jp/",
            "https://ascii.jp/",
            "https://www.atmarkit.co.jp/",
            "https://ameblo.jp/",
            "https://lineblog.me/",
            "https://blog.goo.ne.jp/",
            "https://blog.hatenablog.com/",
            // "https://plaza.rakuten.co.jp/",
        };

        void RunCrawlerOnly()
        {
            int[] exitHour = new int[] { 7, 15, 23 };

            const bool splitAtSentenceBreak = false;
            DbAcccessor dbAcccessor = new DbAcccessor(Constants.workingFolder);

            var now = DateTime.Now;
            var gZipFileName = now.Year.ToString() + now.Month.ToString("D2") + now.Day.ToString("D2") + "-" + now.Hour.ToString("D2") + now.Minute.ToString("D2") + ".txt.gz";

            using (var gZipWriter = new CompressedStreamWriter(Constants.workingFolder + gZipFileName))
            {
                var downloadResult = new DownloadTaskResult();
                var targetUrls = dbAcccessor.PickupUrls(parallelDownload, this.focusedDomains);
                if (targetUrls.Count == 0)
                {
                    targetUrls.Add("https://www.sankei.com/");
                }

                var lastCheckedTime = DateTime.Now;

                while (!exitProgram)
                {
                    if (targetUrls.Count == 0)
                    {
                        targetUrls = this.focusedDomains.ToList();
                    }

                    var dbTask = UpdateDatabase(downloadResult, dbAcccessor);
                    var loadTask = LoadWebAndGzip(targetUrls, gZipWriter, splitAtSentenceBreak);

                    Task.WhenAll(loadTask, dbTask).GetAwaiter().GetResult();

                    downloadResult = loadTask.Result;
                    targetUrls = dbTask.Result;

                    var currentTime = DateTime.Now;
                    if (exitHour.Any(x => (lastCheckedTime.Hour < x && x <= lastCheckedTime.Hour))) // Asssuming 1 loop should less than 1 hour
                    {
                        exitProgram = true;
                    }
                    lastCheckedTime = currentTime;
                }
            }
        }


        void Run()
        {
            Shared.MorphAnalyzer morphAnalyzer = new Shared.MorphAnalyzer();
            Shared.NGramStore nGramStore = new Shared.NGramStore(Constants.workingFolder, true /* with Topic Model */);
            DbAcccessor dbAcccessor = new DbAcccessor(Constants.workingFolder);

            const bool splitAtSentenceBreak = true;
            // Loading the last result
            nGramStore.LoadFromFile();

            var downloadResult = new DownloadTaskResult();
            var targetUrls = dbAcccessor.PickupUrls(parallelDownload, this.focusedDomains);
            if (targetUrls.Count == 0)
            {
                targetUrls.Add("https://www.sankei.com/");
            }

            while (!exitProgram)
            {
                if (targetUrls.Count == 0)
                {
                    targetUrls = this.focusedDomains.ToList();
                }

                var dbTask = UpdateDatabase(downloadResult, dbAcccessor);
                var loadTask = LoadWebAndAnalyze(targetUrls, morphAnalyzer, nGramStore, splitAtSentenceBreak);

                Task.WhenAll(loadTask, dbTask).GetAwaiter().GetResult();

                downloadResult = loadTask.Result;
                targetUrls = dbTask.Result;
            }
        }

        Task<DownloadTaskResult> LoadWebAndGzip(List<string> targetUrls, CompressedStreamWriter writer, bool splitAtSentenceBreak)
        {
            return Task<DownloadTaskResult>.Run(() =>
            {
                var startTime = DateTime.Now;

                var thisResult = this.ParallelDownloadAndUnify(targetUrls, splitAtSentenceBreak);

                foreach (var sentence in thisResult.contentTexts)
                {
                    writer.WriteLine(sentence);
                }

                Console.WriteLine($"End: LoadWebAndAnalyze, elapsed: {(DateTime.Now - startTime).TotalSeconds} sec");
                return thisResult;
            });
        }

        Task<DownloadTaskResult> LoadWebAndAnalyze(List<string> targetUrls, MorphAnalyzer morphAnalyzer, NGramStore nGramStore, bool splitAtSentenceBreak)
        {
            return Task<DownloadTaskResult>.Run(() =>
            {
                var startTime = DateTime.Now;

                var thisResult = this.ParallelDownloadAndUnify(targetUrls, splitAtSentenceBreak);

                var morphListList = morphAnalyzer.Run(thisResult.contentTexts);

                Console.WriteLine("Store Ngram");
                foreach (var morphList in morphListList)
                {
                    nGramStore.AddFromWordArray(morphList);
                }
                nGramStore.PrintCurrentState();

                //
                if (nGramStore.CanSave())
                {
                    if (nGramStore.ShouldFlush())
                    {
                        this.lastSavedTime = DateTime.Now;
                        nGramStore.SaveFile(2);
                        nGramStore.LoadFromFile();
                        Shared.FileOperation.RunPostProcessor();
                    }
                    else
                    {
                        var passedTime = DateTime.Now - this.lastSavedTime;
                        if (passedTime.TotalMinutes >= (new TimeSpan(exitIntervalHour, 0, 0)).TotalMinutes)
                        {
                            nGramStore.SaveFile();
                            Shared.FileOperation.RunPostProcessor();
                            this.exitProgram = true;
                        }
                        else if (passedTime.TotalMinutes >= (new TimeSpan(saveInternvalHour, 0, 0)).TotalMinutes)
                        {
                            this.lastSavedTime = DateTime.Now;
                            nGramStore.SaveFile();
                            Shared.FileOperation.RunPostProcessor();
                        }
                        else if (this.isEveryHourMode && passedTime.TotalMinutes >= (new TimeSpan(1, 0, 0)).TotalMinutes)
                        {
                            this.isEveryHourMode = ((DateTime.Now - this.programStartTime).TotalMinutes < (new TimeSpan(saveInternvalHour, 0, 0)).TotalMinutes);
                            this.lastSavedTime = DateTime.Now;
                            nGramStore.SaveFile();
                            Shared.FileOperation.RunPostProcessor();
                        }
                    }
                }
                var filledRate = nGramStore.ContentFilledRate();
                Console.WriteLine($"End: LoadWebAndAnalyze, elapsed: {(DateTime.Now - startTime).TotalSeconds} sec, filledRate: {filledRate}, lastSaveTime: {this.lastSavedTime.Hour}:{this.lastSavedTime.Minute}");
                return thisResult;
            });
        }

        DownloadTaskResult ParallelDownloadAndUnify(List<string> targetUrls, bool splitAtSentenceBreak)
        {
            List<Task<HtmlGetter>> tasks = targetUrls.Select(url =>
                Task<HtmlGetter>.Run(() =>
                {
                    Console.WriteLine("Loading: " + url);
                    var webContent = new HtmlGetter(url);
                    webContent.DoProcess(splitAtSentenceBreak);
                    return webContent;
                })
            ).ToList();

            Task.WhenAll(tasks).Wait();

            DownloadTaskResult thisResult = new DownloadTaskResult();

            foreach (var task in tasks)
            {
                var result = task.Result;
                thisResult.contentTexts.UnionWith(result.JpnTextSet);
                thisResult.referenceUrls.UnionWith(result.AnchorHrefs);
                thisResult.pageUrls.UnionWith(result.PageUrls);
            }

            var rawContentCount = thisResult.contentTexts.Count;
            foreach (var contentText in thisResult.contentTexts.ToArray())
            {
                if (this.contentHistoryDate.ContainsKey(contentText))
                {
                    this.contentHistoryDate[contentText] = DateTime.Now;
                    thisResult.contentTexts.Remove(contentText);
                }
                else
                {
                    this.contentHistoryDate.Add(contentText, DateTime.Now);
                }
            }
            if (this.contentHistoryDate.Count > maxCotentHistory)
            {
                var removeCount = this.contentHistoryDate.Count - maxCotentHistory;
                var removeList = this.contentHistoryDate.OrderBy(kv => kv.Value).Take(removeCount).Select(kv => kv.Key).ToArray();
                foreach (var key in removeList)
                {
                    this.contentHistoryDate.Remove(key);
                }
            }

            Console.WriteLine($"Gottext, raw:{rawContentCount} => uniq:{thisResult.contentTexts.Count}");

            return thisResult;
        }


        Task<List<string>> UpdateDatabase(DownloadTaskResult prevResult, DbAcccessor dbAcccessor)
        {
            return Task<List<string>>.Run(() =>
            {
                var startTime = DateTime.Now;
                Console.WriteLine($">>> Begin Update DB: Store URLs: {prevResult.referenceUrls.Count}, Mark URLs: {prevResult.pageUrls.Count}");

                HashSet<string> addingUrls = prevResult.referenceUrls;

                if (prevResult.referenceUrls.Count > Constants.maxUrlsToAddOnceTime)
                {
                    // reduce urls upto Constants.maxUrlsToAddOnceTime, because it is too slow if 2000 over urls are added.
                    var arrayedSourceUrls = prevResult.referenceUrls.ToArray();
                    addingUrls = new HashSet<string>();

                    int[] indexArray = new int[prevResult.referenceUrls.Count];
                    for (int i = 0; i < prevResult.referenceUrls.Count; ++i) indexArray[i] = i;
                    for (int i = 0; i < Constants.maxUrlsToAddOnceTime; ++i)
                    {
                        int swapTarget = this.random.Next(i, prevResult.referenceUrls.Count);
                        var swapTuple = new Tuple<int, int>(indexArray[i], indexArray[swapTarget]);
                        indexArray[i] = swapTuple.Item2;
                        indexArray[swapTarget] = swapTuple.Item1;
                        addingUrls.Add(arrayedSourceUrls[swapTuple.Item2]);
                    }
                }

                dbAcccessor.StoreUrlsAndMarkRead(addingUrls, prevResult.pageUrls);

                var updateFinishTime = DateTime.Now;

                var targetUrls = dbAcccessor.PickupUrls(parallelDownload, this.focusedDomains);

                Console.WriteLine($"<<< End Update:{(updateFinishTime - startTime).TotalSeconds}, Pickup:{(DateTime.Now - updateFinishTime).TotalSeconds}, Store URLs: {prevResult.referenceUrls.Count}, Mark URLs: {prevResult.pageUrls.Count}");

                return targetUrls;
            });
        }
    }
}
