using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Ribbon.WebCrawler
{
    class Program
    {
        // constants
        public const string mecabExe = "c:\\Program Files (x86)\\MeCab\\bin\\mecab.exe";
        public const string workingFolder = "c:\\lmworking\\";

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

        MorphAnalyzer m_morphAnalyzer = new MorphAnalyzer(workingFolder);
        NGramStore m_nGraphStore = new NGramStore(workingFolder);
        DbAcccessor m_dbAcccessor = new DbAcccessor(workingFolder);
        public int lastSavedHour = -1;

        void Run()
        {
            // Loading the last result
            m_nGraphStore.LoadFromFile();

            Console.WriteLine("Picking up URLs");
            var targetUrls = m_dbAcccessor.PickupUrls(parallelDownload);
            if (targetUrls.Count == 0)
            {
                targetUrls.Add("http://www.sankei.com/");
            }

            DownloadTaskResult downloadResult = LoadWebAndAnalyze(targetUrls).GetAwaiter().GetResult();

            for (;;)
            {
                Console.WriteLine("Picking up URLs");
                targetUrls = m_dbAcccessor.PickupUrls(parallelDownload);

                if (targetUrls.Count == 0)
                {
                    targetUrls.Add("http://www.asahi.com/");
                    targetUrls.Add("http://www.yomiuri.co.jp/");
                    targetUrls.Add("https://mainichi.jp/");
                    targetUrls.Add("http://www.nikkei.com/");
                    targetUrls.Add("http://www.itmedia.co.jp/");
                    targetUrls.Add("http://ascii.jp/");
                    targetUrls.Add("http://www.atmarkit.co.jp/");
                }

                var loadTask = LoadWebAndAnalyze(targetUrls);
                var dbTask = UpdateDatabase(downloadResult);

                Task.WhenAll(loadTask, dbTask).GetAwaiter().GetResult();

                downloadResult = loadTask.Result;
            }
        }

        Task<DownloadTaskResult> LoadWebAndAnalyze(List<string> targetUrls)
        {
            return Task<DownloadTaskResult>.Run(() =>
            {
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

                if ((lastSavedHour + saveInternvalHour) <= DateTime.Now.Hour)
                {
                    m_nGraphStore.SaveFile();
                    lastSavedHour = DateTime.Now.Hour;
                }

                return thisResult;
            });
        }

        Task UpdateDatabase(DownloadTaskResult prevResult)
        {
            return Task.Run(() =>
            {
                Console.WriteLine("Begin: Store URLs");
                m_dbAcccessor.StoreUrls(prevResult.referenceUrls);
                Console.WriteLine("End: Store URLs");
                Console.WriteLine("Begin: Mark read URL");
                m_dbAcccessor.MarkHaveRead(prevResult.pageUrls);
                Console.WriteLine("End: Mark read URL");
            });
        }
    }
}
