using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace Ribbon.WebCrawler
{
    static class WinApiBridge
    {
        [DllImport("kernel32.dll")]
        static extern private int LCMapStringW(int Locale, uint dwMapFlags,
            [MarshalAs(UnmanagedType.LPWStr)] string lpSrcStr, int cchSrc,
            [MarshalAs(UnmanagedType.LPWStr)] string lpDestStr, int cchDest);

        public enum dwMapFlags : uint
        {
            NORM_IGNORECASE = 0x00000001,
            NORM_IGNORENONSPACE = 0x00000002,
            NORM_IGNORESYMBOLS = 0x00000004,
            LCMAP_LOWERCASE = 0x00000100,
            LCMAP_UPPERCASE = 0x00000200,
            LCMAP_SORTKEY = 0x00000400,
            LCMAP_BYTEREV = 0x00000800,
            SORT_STRINGSORT = 0x00001000,
            NORM_IGNOREKANATYPE = 0x00010000,
            NORM_IGNOREWIDTH = 0x00020000,
            LCMAP_HIRAGANA = 0x00100000,
            LCMAP_KATAKANA = 0x00200000,
            LCMAP_HALFWIDTH = 0x00400000,
            LCMAP_FULLWIDTH = 0x00800000,
            LCMAP_LINGUISTIC_CASING = 0x01000000,
            LCMAP_SIMPLIFIED_CHINESE = 0x02000000,
            LCMAP_TRADITIONAL_CHINESE = 0x04000000,
        }

        public static string StringConvert(this string str, dwMapFlags flags)
        {
            var ci = System.Globalization.CultureInfo.GetCultureInfo("ja-JP");
            string result = new string(' ', str.Length);
            LCMapStringW(ci.LCID, (uint)flags, str, str.Length, result, result.Length);
            return result;
        }

        public static string Han2Zen(this string str)
        {
            return StringConvert(str, dwMapFlags.LCMAP_FULLWIDTH);
        }
    }
}
