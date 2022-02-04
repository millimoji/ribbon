﻿using System;
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
        const int parallelDownload = 20;

        Shared.MorphAnalyzer m_morphAnalyzer = new Shared.MorphAnalyzer(Constants.workingFolder);
        Shared.NGramStore m_nGraphStore = new Shared.NGramStore(Constants.workingFolder);
        DbAcccessor m_dbAcccessor = new DbAcccessor(Constants.workingFolder);

        private DateTime lastSavedTime = DateTime.Now;
        private DateTime programStartTime = DateTime.Now;
        private bool isEveryHourMode = true;

        void Run()
        {
            // Loading the last result
            m_nGraphStore.LoadFromFile();

            Console.WriteLine("Picking up URLs");
            var targetUrls = m_dbAcccessor.PickupUrls(parallelDownload);
            if (targetUrls.Count == 0)
            {
                targetUrls.Add("https://www.sankei.com/");
            }

            DownloadTaskResult downloadResult = LoadWebAndAnalyze(targetUrls).GetAwaiter().GetResult();

            for (;;)
            {
                Console.WriteLine("Picking up URLs");
                targetUrls = m_dbAcccessor.PickupUrls(parallelDownload);

                if (targetUrls.Count == 0)
                {
                    targetUrls.Add("https://www.asahi.com/");
                    targetUrls.Add("https://www.yomiuri.co.jp/");
                    targetUrls.Add("https://mainichi.jp/");
                    targetUrls.Add("https://www.nikkei.com/");
                    targetUrls.Add("https://www.itmedia.co.jp/");
                    targetUrls.Add("https://ascii.jp/");
                    targetUrls.Add("https://www.atmarkit.co.jp/");
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
                Console.WriteLine($"End: LoadWebAndAnalyze, next save time: {nextSaveTime.Hour}:{nextSaveTime.Minute}");
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
