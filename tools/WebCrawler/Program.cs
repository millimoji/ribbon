using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Ribbon.WebCrawler
{
    class Program
    {
        static void Main(string[] args)
        {
            var me = new Program();
            me.Run();
        }

        class DownloadTaskResult
        {
            public HashSet<string> contentTexts = new HashSet<string>();
            public HashSet<string> pageUrls = new HashSet<string>();
            public HashSet<string> referenceUrls = new HashSet<string>();
        }

        const int saveInternvalHour = 3; // 6;
        const int exitIntervalHour = 24;
        const int parallelDownload = 10;

        Shared.MorphAnalyzer m_morphAnalyzer = new Shared.MorphAnalyzer(Constants.workingFolder);
        Shared.NGramStore m_nGraphStore = new Shared.NGramStore(Constants.workingFolder);
        DbAcccessor m_dbAcccessor = new DbAcccessor(Constants.workingFolder);

        private Random random = new Random();
        private DateTime lastSavedTime = DateTime.Now;
        private DateTime programStartTime = DateTime.Now;
        private bool isEveryHourMode = false; // disabled
        private bool exitProgram = false;

        const int maxCotentHistory = 20000;
        Dictionary<string, DateTime> contentHistoryDate = new Dictionary<string, DateTime>();

        private string[] focusedDomains = new string[]
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

        void Run()
        {
            // Loading the last result
            m_nGraphStore.LoadFromFile();

            Console.WriteLine("Picking up URLs");
            var targetUrls = m_dbAcccessor.PickupUrls(parallelDownload, this.focusedDomains);
            if (targetUrls.Count == 0)
            {
                targetUrls.Add("https://www.sankei.com/");
            }

            DownloadTaskResult downloadResult = LoadWebAndAnalyze(targetUrls).GetAwaiter().GetResult();

            while (!exitProgram)
            {
                Console.WriteLine("Picking up URLs");
                targetUrls = m_dbAcccessor.PickupUrls(parallelDownload, this.focusedDomains);

                if (targetUrls.Count == 0)
                {
                    foreach (var url in this.focusedDomains)
                    {
                        targetUrls.Add(url);
                    }
                }

                var dbTask = UpdateDatabase(downloadResult);
                var loadTask = LoadWebAndAnalyze(targetUrls);

                Task.WhenAll(loadTask, dbTask).GetAwaiter().GetResult();

                downloadResult = loadTask.Result;
            }
        }

        Task<DownloadTaskResult> LoadWebAndAnalyze(List<string> targetUrls)
        {
            return Task<DownloadTaskResult>.Run(() =>
            {
                var startTime = DateTime.Now;

                List<Task<HtmlGetter>> tasks = targetUrls.Select(url =>
                    Task<HtmlGetter>.Run(() =>
                    {
                        Console.WriteLine("Loading: " + url);
                        var webContent = new HtmlGetter(url);
                        webContent.DoProcess();
                        return webContent;
                    })
                ).ToList();

                Task.WhenAll(tasks).Wait();

                Console.WriteLine("Concat all result");

                DownloadTaskResult thisResult = new DownloadTaskResult();

                foreach (var task in tasks)
                {
                    var result = task.Result;
                    thisResult.contentTexts.UnionWith(result.JpnTextSet);
                    thisResult.referenceUrls.UnionWith(result.AnchorHrefs);
                    thisResult.pageUrls.UnionWith(result.PageUrls);
                }

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

                Console.WriteLine("Analyze Japanese Text");

                var morphListList = m_morphAnalyzer.Run(thisResult.contentTexts);

                Console.WriteLine("Store Ngram");
                foreach (var morphList in morphListList)
                {
                    m_nGraphStore.AddFromWordArray(morphList);
                }
                m_nGraphStore.PrintCurrentState();

                //
                if (m_nGraphStore.CanSave())
                {
                    if (m_nGraphStore.ShouldFlush())
                    {
                        this.lastSavedTime = DateTime.Now;
                        m_nGraphStore.SaveFile(2);
                        m_nGraphStore.LoadFromFile();
                        Shared.FileOperation.RunPostProcessor();
                    }
                    else
                    {
                        var passedTime = DateTime.Now - this.lastSavedTime;
                        if (passedTime.TotalMinutes >= (new TimeSpan(exitIntervalHour, 0, 0)).TotalMinutes)
                        {
                            m_nGraphStore.SaveFile();
                            Shared.FileOperation.RunPostProcessor();
                            this.exitProgram = true;
                        }
                        else if (passedTime.TotalMinutes >= (new TimeSpan(saveInternvalHour, 0, 0)).TotalMinutes)
                        {
                            this.lastSavedTime = DateTime.Now;
                            m_nGraphStore.SaveFile();
                            Shared.FileOperation.RunPostProcessor();
                        }
                        else if (this.isEveryHourMode && passedTime.TotalMinutes >= (new TimeSpan(1, 0, 0)).TotalMinutes)
                        {
                            this.isEveryHourMode = ((DateTime.Now - this.programStartTime).TotalMinutes < (new TimeSpan(saveInternvalHour, 0, 0)).TotalMinutes);
                            this.lastSavedTime = DateTime.Now;
                            m_nGraphStore.SaveFile();
                            Shared.FileOperation.RunPostProcessor();
                        }
                    }
                }
                var filledRate = m_nGraphStore.ContentFilledRate();
                Console.WriteLine($"End: LoadWebAndAnalyze, elapsed: {(DateTime.Now - startTime).TotalSeconds} sec, filledRate: {filledRate}, lastSaveTime: {this.lastSavedTime.Hour}:{this.lastSavedTime.Minute}");
                return thisResult;
            });
        }

        Task UpdateDatabase(DownloadTaskResult prevResult)
        {
            return Task.Run(() =>
            {
                var startTime = DateTime.Now;
                Console.WriteLine($">>> Begin Update DB: Store URLs: {prevResult.referenceUrls.Count}, Mark URLs: {prevResult.pageUrls.Count}");

                HashSet<string> addingUrls = prevResult.referenceUrls;

                if (prevResult.referenceUrls.Count > Constants.maxUrlsToAddOnceTime)
                {
                    // reduce urls upto Constants.maxUrlsToAddOnceTime, because it is too slow if 2000 over urls are added.
                    var arrayedSourceUrls = prevResult.referenceUrls.ToArray();
                    addingUrls = new HashSet<string>();

                    int [] indexArray = new int[prevResult.referenceUrls.Count];
                    for (int i = 0; i < prevResult.referenceUrls.Count; ++i) indexArray[i] = i;
                    for (int i = 0; i < Constants.maxUrlsToAddOnceTime; ++i)
                    {
                        int swapTarget = this.random.Next(i, prevResult.referenceUrls.Count);
                        int x = indexArray[swapTarget];
                        indexArray[swapTarget] = indexArray[i];
                        indexArray[i] = x;
                        addingUrls.Add(arrayedSourceUrls[x]);
                    }
                }

                m_dbAcccessor.StoreUrlsAndMarkRead(addingUrls, prevResult.pageUrls);
                Console.WriteLine($"<<< End Update DB: Elapsed: {(DateTime.Now - startTime).TotalSeconds} sec, Store URLs: {prevResult.referenceUrls.Count}, Mark URLs: {prevResult.pageUrls.Count}");
            });
        }
    }
}
