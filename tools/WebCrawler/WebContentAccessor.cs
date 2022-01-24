using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
//using System.Net;
using System.Net.Http;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using HtmlAgilityPack;

namespace Ribbon.WebCrawler
{
    class HtmlGetter
    {
        static private HttpClient m_httpClient;
        static private object __lockObject = "lockobject";

        string m_sourceUrl;
        HtmlDocument m_htmlDoc = new HtmlDocument();

        public bool Succeeded = false;
        public HashSet<string> JpnTextSet = new HashSet<string>();
        public HashSet<string> AnchorHrefs = new HashSet<string>();
        public HashSet<string> PageUrls = new HashSet<string>();
        private bool m_hasNgWords;
        private int m_suppressWordCount;
        private const int SUPPRESS_WORD_THRESHOLD = 5;

        public HtmlGetter(string url)
        {
            lock (__lockObject)
            {
                if (m_httpClient == null)
                {
                    m_httpClient = new HttpClient();
                    m_httpClient.Timeout = TimeSpan.FromSeconds(10);

                    m_httpClient.DefaultRequestHeaders.Add("User-Agent", "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.2; .NET CLR 1.0.3705;)");
                    if (!m_httpClient.DefaultRequestHeaders.Contains("Accept-Language"))
                    {
                        m_httpClient.DefaultRequestHeaders.Add("Accept-Language", "ja");
                    }
                }
            }

            m_sourceUrl = url;
            PageUrls.Add(url);
        }
        public void DoProcess()
        {
            m_hasNgWords = false;
            m_suppressWordCount = 0;

            DownloadAndBuildHtmlDocument(m_sourceUrl);
            RetrieveJapaneseText();
            RetrieveAnchorUrls();

            if (m_hasNgWords /*|| m_suppressWordCount > SUPPRESS_WORD_THRESHOLD*/)
            {
                // ignore this content
                JpnTextSet.Clear();
                AnchorHrefs.Clear();
                PageUrls.Clear();
                return;
            }
            if (m_suppressWordCount > SUPPRESS_WORD_THRESHOLD)
            {
                // suppress to trace link from this page
                AnchorHrefs.Clear();
                return;
            }
        }

        private void DownloadAndBuildHtmlDocument(string url)
        {
            try
            {
                var getTask = m_httpClient.GetAsync(url);
                getTask.Wait();
                using (var httpResponse = getTask.Result)
                {
                    if (httpResponse.StatusCode != System.Net.HttpStatusCode.OK)
                    {
                        return;
                    }
/*
                    var contentType = httpResponse.Headers.GetValues("Content-Type");
                    if (!contentType.Any(x => x.StartsWith("text/html")))
                    {
                        return;
                    }
*/
                    using (var streamData = httpResponse.Content.ReadAsStreamAsync().Result)
                    {
                        using (StreamReader reader = new StreamReader(streamData))
                        {
                            m_htmlDoc.LoadHtml(reader.ReadToEnd());
                            reader.Close();
                        }
                        streamData.Close();
                    }
                }
                Succeeded = true;
            }
            catch (Exception)
            {
            }
        }

        static Regex removeCr = new Regex("[\r|\n|\t|　| ]+");

        private void RetrieveJapaneseText()
        {
            HashSet<string> textSet = new HashSet<string>();

            try
            {
                var lang = m_htmlDoc.DocumentNode.SelectNodes("//html[@lang]").First().Attributes.AttributesWithName("lang").First().Value;
                if (lang != "ja")
                {
                    return;
                }
            }
            catch (Exception)
            {
                return;
            }

            bool hasHiraOrKata = false;
            var textNodes = m_htmlDoc.DocumentNode.SelectNodes("//text()");
            if (textNodes != null && textNodes.Count > 0)
            {
                foreach (var textNode in textNodes)
                {
                    if (textNode.XPath.IndexOf("/script") >= 0)
                    {
                        continue;
                    }
                    var textContent = textNode.InnerText;
                    int hiraKataCount = textContent.Count(c => (0x3041 <= c && c <= 0x30ff));
                    int kanjiCount = textContent.Count(c => (0x3400 <= c && c <= 0x9fdf));

                    if ((hiraKataCount == 0 && kanjiCount == 0) || (hiraKataCount + kanjiCount) < textContent.Length * 2 / 3)
                    {
                        continue;
                    }

                    hasHiraOrKata = hasHiraOrKata || (hiraKataCount > 0);

                    // normalize
                    textContent = removeCr.Replace(textContent, "　");
                    textSet.Add(textContent);

                    int checkResult = BadWordFilter.CheckText(textContent);
                    if (checkResult == BadWordFilter.NG_TEXT)
                    {
                        m_hasNgWords = true;
                    }
                    else if (checkResult == BadWordFilter.SUPPRESS_TEXT)
                    {
                        m_suppressWordCount++;
                    }

                }
            }
            if (hasHiraOrKata)
            {
                textSet.ToList().ForEach(t =>
                {
                    String[] textBreaker = new String[] { "__Break__Break__" };

                    var textList = Regex.Replace(t, "([。｡!！?？]+)[　]*", "$1__Break__Break__", RegexOptions.Singleline).Split(textBreaker, StringSplitOptions.RemoveEmptyEntries);

                    textList.ToList().ForEach(x =>
                    {
                        JpnTextSet.Add(x);
                    });
                });
            }
        }

        private void RetrieveAnchorUrls()
        {
            Uri currentUri;
            string currentUrl;
            try
            {
                currentUri = new Uri(m_sourceUrl);
                currentUrl = currentUri.GetLeftPart(UriPartial.Query);
            }
            catch (Exception)
            {
                return;
            }
            try
            {
                var mainUrl = m_htmlDoc.DocumentNode.SelectNodes("//head/link[@rel='canonical'][@href]").First().Attributes.AttributesWithName("href").First().Value;
                var targetUri = new Uri(currentUri, mainUrl);
                PageUrls.Add(targetUri.GetLeftPart(UriPartial.Query));
            }
            catch (Exception) { }
            try
            {
                var mainUrl = m_htmlDoc.DocumentNode.SelectNodes("//head/meta/[property='og:url'][@content]").First().Attributes.AttributesWithName("content").First().Value;
                var targetUri = new Uri(currentUri, mainUrl);
                PageUrls.Add(targetUri.GetLeftPart(UriPartial.Query));
            }
            catch (Exception) { }

            if (JpnTextSet.Count > 0)
            {
                var anchorNodes = m_htmlDoc.DocumentNode.SelectNodes("//a[@href]");
                if (anchorNodes != null && anchorNodes.Count > 0)
                {
                    foreach (var anchorNode in anchorNodes)
                    {
                        try
                        {
                            var href = anchorNode.Attributes.AttributesWithName("href").First().Value;
                            var targetUri = new Uri(currentUri, href);
                            if (targetUri.Scheme != "http" && targetUri.Scheme != "https")
                            {
                                continue;
                            }
                            var nextUrl = targetUri.GetLeftPart(UriPartial.Query);

                            if (nextUrl == currentUrl)
                            {
                                continue;
                            }

                            if (BadWordFilter.CheckUrl(nextUrl) == BadWordFilter.OK_TEXT)
                            {
                                AnchorHrefs.Add(nextUrl);
                            }
                        }
                        catch (Exception) { }
                    }
                }
            }
        }
    }
}
