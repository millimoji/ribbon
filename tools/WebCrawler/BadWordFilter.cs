using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Ribbon.WebCrawler
{
    class BadWordFilter
    {
        public const int OK_TEXT = 0;
        public const int NG_TEXT = 1;
        public const int SUPPRESS_TEXT = 2;

        public static int CheckText(string target)
        {
            if (NgWords.Any(target.Contains))
            {
                return NG_TEXT;
            }
            if (SuppressWords.Any(target.Contains))
            {
                return SUPPRESS_TEXT;
            }
            return OK_TEXT;
        }

        public static int CheckUrl(string url)
        {
            if (NgUrls.Any(url.Contains))
            {
                return NG_TEXT;
            }
            return OK_TEXT;
        }

        public static readonly string[] NgWords =
        {
"おっぱい",
"オッパイ",
//"エロ",
"人妻",
"セックス",
"無修正",
"無料動画",
"無料画像",
"無料エロ",
"熟女",
"オナニー",
"エッチ",
"乳首",
"変態",
"レイプ",
"盗撮",
"射精",
"中出し",
"パンスト",
"女子校生",
"女子高生",
"舐め",
"ナンパ",
"中出",
"潮吹き",
"淫乱",
"騎乗位",
"パイパン",
"フェラチオ",
"全裸",
"姦",
"強姦",
"輪姦",
"相姦",
"痴漢",
"勃起",
"若妻",
"童貞",
"まんこ",
"緊縛",
"処女",
//"慰安婦",
"ザーメン",
"パンティ",
"性欲",
"ヘアヌード",
"素人娘",
"ＡＶ",
"丸出し",
"えっち",
"アクメ",
"性交",
"盗撮",
"中出",
"ピストン",
"女教師",
"放尿",
"乳輪",
"パンティー",
"開脚",
"素股",
"レズビアン",
"精子",
"愛撫",
"手マン",
"アダルトビデオ",
"アダルトサイト",
"アダルト動画",
"アダルト天国",
"アダルト無料",
"アダルトエロ",
"アダルトコンテンツ",
"素人アダルト",
"スクール水着",
"官能",
"Ｈな",
"ヤリマン",
"舐め",
"オナニー",
"ペニス",
"精子",
"ソープ嬢",
"ソープランド",
"高級ソープ",
"濡れ場",
"精液",
"桃尻",
"美尻",
"立ちバック",
"マン汁",
"喘",
"コンドーム",
"潮吹き",
"悶え",
"スカトロ",
"チンポ",
"性処理",
"勃起",
"女体",
"クリトリス",
"セクロス",
"指マン",
"ちんちん",
"性器",
"裸エプロン",
"肉棒",
"シックスナイン",
"顔射",
"性欲",
"性行為",
"幼女",
"卑猥",
"膣",
"犯さ",
"完全無料",
"濡れ",
"淫ら",
"犯す",
"全裸",
"喘ぐ",
"陰部",
"陰茎",
"陰核",
"陰唇"
        };

        public static readonly string[] SuppressWords =
        {
"ヌード",
"素人",
"出会い",
"恋愛",
"不倫",
"露出",
"挿入",
//"動画",
"パンツ",
"セクシー",
"出会い",
"妊娠",
"スカート",
//"ラブ",
"裸",
"ぽっちゃり",
"無料",
"ミニスカ",
"美少女",
"痙攣",
"むっちり",
"ポロリ",
"ランジェリー",
"バスト",
"ムチムチ",
"セーラー服",
"着替え",
"ローション",
"寝取",
//"トイレ",
"いやらしい",
"ムッチリ",
"欲求",
"漏",
"乳房",
"股間",
"ノーブラ",
"愛人",
"メス",
"交尾",
"欲望",
"脱い",
"肛門",
"失禁",
"欲情",
"四つん這い",
"網タイツ",
"生々しい",
"唾液",
"寸止め",
"ブラジャー",
"ガーターベルト",
"卑猥",
"フェロモン",
"揉ま",
"ロリコン",
"ビキニ",
"尻",
"アダルト",
"ヘア",
"陰毛"
};

        public static readonly string[] NgUrls =
        {
            "love", "sex"
        };
    }
}
