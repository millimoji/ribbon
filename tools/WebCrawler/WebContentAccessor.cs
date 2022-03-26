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
        static private readonly object __lockObject = "lockobject";

        readonly string m_sourceUrl;
        readonly HtmlDocument m_htmlDoc = new HtmlDocument();

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
                    m_httpClient = new HttpClient { Timeout = TimeSpan.FromSeconds(10) };

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
        public void DoProcess(bool splitAtSentenceBreak)
        {
            m_hasNgWords = false;
            m_suppressWordCount = 0;

            DownloadAndBuildHtmlDocument(m_sourceUrl);
            RetrieveJapaneseText(splitAtSentenceBreak);
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

                    using (var httpContnt = httpResponse.Content)
                    {
                        var meditaType = httpContnt.Headers.ContentType?.MediaType;
                        if (meditaType == "text/html")
                        {
                            var contentLanguages = httpContnt.Headers.ContentLanguage;
                            if (contentLanguages.Count == 0 || contentLanguages.Any(x => x.StartsWith("ja")))
                            {
                                using (var streamData = httpContnt.ReadAsStreamAsync().Result)
                                {
                                    using (StreamReader reader = new StreamReader(streamData))
                                    {
                                        m_htmlDoc.LoadHtml(reader.ReadToEnd());
                                        reader.Close();
                                    }
                                    streamData.Close();
                                }
                            }
                        }
                    }
                }
                Succeeded = true;
            }
            catch (Exception)
            {
            }
        }

        static readonly Regex removeCr = new Regex("[\x00-\x1f]+");

        private void RetrieveJapaneseText(bool splitAtSentenceBreak)
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
                if (splitAtSentenceBreak)
                {
                    textSet.ToList().ForEach(t =>
                    {
                        var textList = sentenceBreakFinder.Replace(t, "$1" + textBreakMarker)
                            .Split(this.textBreakMarkerArray, StringSplitOptions.RemoveEmptyEntries);

                        textList.ToList().ForEach(x =>
                        {
                            JpnTextSet.Add(x);
                        });
                    });
                }
                else
                {
                    textSet.ToList().ForEach(t => JpnTextSet.Add(t));
                }
            }
        }

        private static readonly string textBreakMarker = "__Break__Break__";
        private readonly String[] textBreakMarkerArray = new String[] { textBreakMarker };
        private readonly Regex sentenceBreakFinder = new Regex(@"([。｡!！?？]+)[　]*", RegexOptions.Singleline);

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
                            var pathAndQuery = targetUri.PathAndQuery;
                            if (pathAndQuery.Length == 0 || pathAndQuery == "/")
                            {
                                continue; // NOTE: ignore the URL that is root.
                                // This logic is supporsed to collect variately pages. 
                                // Usually root page has a tend to 'Welcome', so here is to suppress to collect top page.
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
