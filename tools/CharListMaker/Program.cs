using System;
using System.IO;
using System.Text.RegularExpressions;
using System.Linq;

namespace Ribbon.CharListMaker
{
    class Program
    {
        static void Main(string[] args)
        {
            var me = new Program();
            me.ConvertEomoji();
            me.ConvertSingleKanji();
        }

        void ConvertEomoji()
        {
            var emojiConv = new EmojiListMaker(
                "..\\..\\..\\..\\wordsrcs\\emoji\\",
                "..\\..\\..\\..\\script\\");

            emojiConv.DoConvert(
                new string[] { "emoji-sequences.txt", "emoji-zwj-sequences.txt" },
                "_emoji.csv");
        }

        void ConvertSingleKanji()
        {
            var kanjiConv = new KanjiListMaker(
                "..\\..\\..\\..\\wordsrcs\\kanji\\",
                "..\\..\\..\\..\\script\\");

            kanjiConv.DoConvert(
                "jouyoukanji.txt",
                "tankanji.csv");
        }

    }
}
