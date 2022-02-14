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

        const int saveInternvalHour = 6;
        const int parallelDownload = 10;

        Shared.MorphAnalyzer m_morphAnalyzer = new Shared.MorphAnalyzer(Constants.workingFolder);
        Shared.NGramStore m_nGraphStore = new Shared.NGramStore(Constants.workingFolder);
        DbAcccessor m_dbAcccessor = new DbAcccessor(Constants.workingFolder);

        private Random random = new Random();
        private DateTime lastSavedTime = DateTime.Now;
        private DateTime programStartTime = DateTime.Now;
        private bool isEveryHourMode = true;

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

            for (;;)
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

                HashSet<string> allText = new HashSet<string>();
                HashSet<string> allAnchorHrefs = new HashSet<string>();
                HashSet<string> allPageUrls = new HashSet<string>();

                foreach (var task in tasks)
                {
                    var result = task.Result;
                    thisResult.contentTexts.UnionWith(result.JpnTextSet);
                    thisResult.referenceUrls.UnionWith(result.AnchorHrefs);
                    thisResult.pageUrls.UnionWith(result.PageUrls);
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
                        m_nGraphStore.SaveFile();
                        m_nGraphStore.LoadFromFile(2);
                        Shared.FileOperation.RunPostProcessor();
                    }
                    else
                    {
                        var passedTime = DateTime.Now - this.lastSavedTime;
                        if (passedTime.TotalMinutes >= (new TimeSpan(saveInternvalHour, 0, 0)).TotalMinutes)
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
                var nextSaveTime = this.lastSavedTime + (this.isEveryHourMode ? new TimeSpan(1, 0, 0) : new TimeSpan(saveInternvalHour, 0, 0));
                Console.WriteLine($"End: LoadWebAndAnalyze, elapsed: {(DateTime.Now - startTime).TotalSeconds} sec, next save time: {nextSaveTime.Hour}:{nextSaveTime.Minute}");
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

                if (prevResult.referenceUrls.Count >= Constants.maxUrlsToAddOnceTime)
                {
                    // reduce urls upto Constants.maxUrlsToAddOnceTime, because it is too slow if 2000 over urls are added.
                    var arrayedSourceUrls = prevResult.referenceUrls.ToArray();
                    addingUrls = new HashSet<string>();

                    int [] indexArray = new int[prevResult.referenceUrls.Count];
                    for (int i = 0; i < prevResult.referenceUrls.Count; ++i) indexArray[i] = i;
                    for (int i = 0; i < Constants.maxUrlsToAddOnceTime; ++i)
                    {
                        int swapTarget = this.random.Next(i + 1, prevResult.referenceUrls.Count);
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
