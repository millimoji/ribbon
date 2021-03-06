using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Ribbon.WebCrawler
{
    class Logger
    {
        static Logger m_logger = null;

        public static void Log(string content)
        {
            if (m_logger == null)
            {
                m_logger = new Logger();
            }
            lock (m_logger)
            {
                m_logger.LogInternal(content);
            }
        }

        StreamWriter m_fs = null;

        public Logger()
        {
            FileStream fs = File.Open("log.txt", FileMode.Append, FileAccess.Write, FileShare.Read);
            m_fs = new StreamWriter(fs);
        }
        ~Logger()
        {
            m_fs.Close();
        }
        public void LogInternal(string content)
        {
            m_fs.WriteLine(content);
            m_fs.Flush();
        }
    }
}
