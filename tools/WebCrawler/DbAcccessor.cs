using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Data.Linq;
using System.Data.Linq.Mapping;
using System.Data;
using System.Data.SqlClient;

namespace Ribbon.WebCrawler
{
    [Table(Name = "TblHostname")]
    public class EntityHostname
    {
        [Column(IsPrimaryKey = true)]
        public string hostname;
    }

    [Table(Name = "TblUrl")]
    public class EntityUrl
    {
        [Column(IsPrimaryKey = true, IsDbGenerated = true)]
        public int Id;
        [Column]
        public string url;
        [Column]
        public string hostname;
        [Column]
        public string orgUrl;
        [Column]
        public DateTime lastAccess;
    }

    [Table(Name = "TblNGrams")]
    public class EntityNGram
    {
        [Column(IsPrimaryKey = true, IsDbGenerated = true)]
        public long Id;
        [Column]
        public string wordList;
        [Column]
        public int count;
        [Column]
        public DateTime lastUpdate;
    }

    public class CrawlerDataContext : DataContext
    {
        public CrawlerDataContext(string connectionString) : base(connectionString) { }
        public Table<EntityHostname> TblHostname;
        public Table<EntityUrl> TblUrl;
        public Table<EntityNGram> TblNGrams;
    }

    internal class DbAccessorRaw
    {
        const int MAX_HOSTNAME_LENGTH = 256;
        const int MAX_URL_LENGTH = 1700;

        const string connectionPreString = "Data Source=(LocalDB)\\MSSQLLocalDB;AttachDbFilename=";
        const string connectionPostString = ";Integrated Security=True;Connect Timeout=30";
        string connectionFullString = "";
        Random random = new System.Random();

        readonly TimeSpan MinimumUpdateInterval = new TimeSpan(7, 0, 0, 0);

        readonly HashSet<string> exceptedHosts = new HashSet<string>
        {
            "http://a-mooc.com",
        };

        CrawlerDataContext m_db;
        int m_lastHostnameIndex = -1;

        public DbAccessorRaw(string filePath)
        {
            connectionFullString = connectionPreString + filePath + connectionPostString;
            m_db = new CrawlerDataContext(connectionFullString);
        }
        ~DbAccessorRaw()
        {
        }

        void Recoonect()
        {
            m_db.Dispose();
            m_db = null;

            m_db = new CrawlerDataContext(connectionFullString);
        }

        bool IsValidUrl(string srcUrl)
        {
            if (srcUrl.Length > MAX_URL_LENGTH)
            {
                return false;
            }

            var uri = new Uri(srcUrl);
            string hostname = uri.GetLeftPart(UriPartial.Authority);
            if (hostname.Length > MAX_HOSTNAME_LENGTH)
            {
                return false;
            }

            string recoveredUrl = Encoding.ASCII.GetString(Encoding.ASCII.GetBytes(srcUrl));
            if (recoveredUrl != srcUrl)
            {
                return false;
            }
            return true;
        }


        public List<EntityNGram> GetNGram(List<EntityNGram> nGram)
        {
            return nGram.Select(e =>
            {
                var existing = m_db.TblNGrams.FirstOrDefault(r => r.wordList == e.wordList);
                if (existing != null)
                {
                    return existing;
                }
                else
                {
                    e.count = 0;
                    return e;
                }
            }).ToList();
        }

        private string GetPureHostName(string dnsHostname)
        {
            int startHostname = dnsHostname.IndexOf("://");
            if (startHostname < 0)
            {
                return dnsHostname;
            }
            string realHostname = dnsHostname.Substring(startHostname + 3);

            int firstDot = realHostname.IndexOf('.');
            if (firstDot <= 0 || firstDot >= (realHostname.Length - 6))
            {
                return realHostname;
            }
            return realHostname.Substring(firstDot + 1);
        }

        public void StoreUrls(HashSet<string> urls)
        {
            var hostNames = new HashSet<string>();
            var uniqueUrl = new HashSet<string>();

            var insertUrlList = urls
                .Where(url => IsValidUrl(url))
                .Select(srcUrl =>
                {
                    var uri = new Uri(srcUrl);
                    // TODO: exbrog -> consider single host
                    return new EntityUrl
                    {
                        //Id = ++uniqueId,
                        url = uri.GetLeftPart(UriPartial.Path),
                        hostname = GetPureHostName(uri.GetLeftPart(UriPartial.Authority)),
                        orgUrl = srcUrl,
                        lastAccess = new DateTime(2001, 1, 1, 0, 0, 0),
                    };
                }).Where(e =>
                {
                    if (!uniqueUrl.Add(e.url))
                    {
                        return false;
                    }
                    hostNames.Add(e.hostname);
                    return !m_db.TblUrl.Any(r => (r.url == e.url));
                }).ToList();

            var insertHostnameList = hostNames
                .Where(e => (!m_db.TblHostname.Any(r => (r.hostname == e))))
                .Select(e => new EntityHostname { hostname = e, })
                .ToList();

            try
            {
                m_db.TblUrl.InsertAllOnSubmit(insertUrlList);
                m_db.TblHostname.InsertAllOnSubmit(insertHostnameList);
                // m_db.SubmitChanges(ConflictMode.ContinueOnConflict);
            }
            catch (Exception e)
            {
                Shared.Logger.Log(e.ToString());
                string insertingList = "";
                insertUrlList.ForEach(url => { insertingList += "[" + url.orgUrl + "]"; });
                Shared.Logger.Log(insertingList);

                insertHostnameList.ForEach(name => { insertingList += "[" + name + "]"; });
                Shared.Logger.Log(insertingList);
            }
        }

        public void MarkHaveRead(HashSet<string> urls)
        {
            try
            {
                m_db.SubmitChanges(ConflictMode.ContinueOnConflict);
            }
            catch (Exception ex)
            {
                Shared.Logger.Log(ex.ToString());
                Recoonect();
            }

            var insertList = urls
                .Where(url => IsValidUrl(url))
                .Select(urlSrc => {
                    var uriSrc = new Uri(urlSrc);
                    return new EntityUrl {
                        url = uriSrc.GetLeftPart(UriPartial.Path),
                        hostname = GetPureHostName(uriSrc.GetLeftPart(UriPartial.Authority)),
                        orgUrl = urlSrc,
                        lastAccess = DateTime.Now,
                    };
                }).Where(e => {
                    var existing = m_db.TblUrl.FirstOrDefault(r => r.url == e.url);
                    if (existing == null) {
                        //e.Id = ++startId;
                        return true;
                    }
                    existing.lastAccess = DateTime.Now;
                    return false;
                }).ToList();

            try
            {
                if (insertList.Count > 0)
                {
                    m_db.TblUrl.InsertAllOnSubmit(insertList);
                }
                // m_db.SubmitChanges(ConflictMode.ContinueOnConflict);
            }
            catch (Exception ex)
            {
                Shared.Logger.Log(ex.ToString());
                string xlist = "";
                insertList.ForEach(x => { xlist += "[" + x.url + "],"; });
                Shared.Logger.Log(xlist);
                Recoonect();
            }
        }

        public void SubmitChanges()
        {
            try
            {
                m_db.SubmitChanges(ConflictMode.ContinueOnConflict);
            }
            catch (Exception ex)
            {
                Shared.Logger.Log("SubmitChanges() is failed: ex.ToString()");
                Recoonect();
            }
        }

        public List<string> PickupUrls(int desiredCount, string [] focusedDomain)
        {
            List<string> hostNamesInDb = m_db.TblHostname
                .Select(e => e.hostname)
                .OrderBy(e => e)
                .ToList();

            List<string> hostNames = hostNamesInDb.Where(e => !exceptedHosts.Contains(e)).ToList();

            if (hostNames.Count == 0)
            {
                return new List<string>(); // empty
            }

            if (m_lastHostnameIndex < 0)
            {
                var random = new System.Random();
                m_lastHostnameIndex = hostNames.Count <= 1 ? 0 : random.Next(1, hostNames.Count - 1);
            }
            else
            {
                m_lastHostnameIndex = m_lastHostnameIndex % hostNames.Count;
            }

            var hostAndCount = new Dictionary<string, int>();

            Enumerable.Range(
                0, desiredCount
            ).ToList().ForEach(x =>
            {
                var hostName = hostNames[(m_lastHostnameIndex + x) % hostNames.Count];
                int hostCount = 0;
                if (hostAndCount.TryGetValue(hostName, out hostCount))
                {
                    hostAndCount[hostName]++;
                }
                else
                {
                    hostAndCount.Add(hostName, 1);
                }
            });

            // update index
            m_lastHostnameIndex = (m_lastHostnameIndex + desiredCount) % hostNames.Count;

            // append focused host names
            foreach (var rootUrl in focusedDomain)
            {
                var uri = new Uri(rootUrl);
                var pureHostname = this.GetPureHostName(uri.GetLeftPart(UriPartial.Authority));
                if (!hostAndCount.Any(x => x.Key == pureHostname))
                {
                    hostAndCount.Add(pureHostname, 1);
                }
            }


            var urlResult = new List<string>();

            // Get Urls
            hostAndCount.Select(kvp =>
            {
                var targetUrlList = m_db.TblUrl.Where(e =>
                    (e.hostname == kvp.Key) && (e.lastAccess < (DateTime.Now - MinimumUpdateInterval))
                ).OrderBy(e =>
                    e.lastAccess
                );
                var addingUrls = targetUrlList.
                    Skip(this.random.Next(0, Math.Max(targetUrlList.Count() - kvp.Value, 1))).
                    Take(kvp.Value).
                    ToList();

                addingUrls.ForEach(e =>
                {
                    urlResult.Add(e.orgUrl);
                    e.lastAccess = DateTime.Now;
                });
                return false;
            }).ToList();

            try
            {
                m_db.SubmitChanges(ConflictMode.ContinueOnConflict);
            }
            catch (Exception ex)
            {
                Shared.Logger.Log(ex.ToString());
                Recoonect();
            }

            return urlResult;
        }
    }


    public class DbAcccessor
    {
        DbAccessorRaw m_dbUrl;

        public DbAcccessor(string dataBasePath)
        {
            m_dbUrl = new DbAccessorRaw(dataBasePath + "WebUrlDB.mdf");
        }
        ~DbAcccessor()
        {
        }

        public void StoreUrlsAndMarkRead(HashSet<string> storeUrls, HashSet<string> markUrls)
        {
            m_dbUrl.StoreUrls(storeUrls);
            m_dbUrl.MarkHaveRead(markUrls);
            m_dbUrl.SubmitChanges();
        }

        public List<string> PickupUrls(int desiredCount, string [] focusedDomains)
        {
            return m_dbUrl.PickupUrls(desiredCount, focusedDomains);
        }
    }
}
