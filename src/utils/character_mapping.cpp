#include "character_mapping.h"

String cleanString(String input)
{
    // A variants
    input.replace("À", "A");
    input.replace("Á", "A");
    input.replace("Â", "A");
    input.replace("Ã", "A");
    input.replace("Ä", "A");
    input.replace("Å", "A");
    input.replace("Ā", "A");
    input.replace("Ă", "A");
    input.replace("Ą", "A");
    input.replace("Ǎ", "A");
    input.replace("Ǟ", "A");
    input.replace("Ǡ", "A");
    input.replace("à", "a");
    input.replace("á", "a");
    input.replace("â", "a");
    input.replace("ã", "a");
    input.replace("ä", "a");
    input.replace("å", "a");
    input.replace("ā", "a");
    input.replace("ă", "a");
    input.replace("ą", "a");
    input.replace("ǎ", "a");
    input.replace("ǟ", "a");
    input.replace("ǡ", "a");

    // E variants
    input.replace("È", "E");
    input.replace("É", "E");
    input.replace("Ê", "E");
    input.replace("Ë", "E");
    input.replace("Ē", "E");
    input.replace("Ĕ", "E");
    input.replace("Ė", "E");
    input.replace("Ę", "E");
    input.replace("Ě", "E");
    input.replace("Ə", "E");
    input.replace("Ɛ", "E");
    input.replace("è", "e");
    input.replace("é", "e");
    input.replace("ê", "e");
    input.replace("ë", "e");
    input.replace("ē", "e");
    input.replace("ĕ", "e");
    input.replace("ė", "e");
    input.replace("ę", "e");
    input.replace("ě", "e");
    input.replace("ə", "e");
    input.replace("ɛ", "e");

    // I variants
    input.replace("Ì", "I");
    input.replace("Í", "I");
    input.replace("Î", "I");
    input.replace("Ï", "I");
    input.replace("Ĩ", "I");
    input.replace("Ī", "I");
    input.replace("Ĭ", "I");
    input.replace("Į", "I");
    input.replace("İ", "I");
    input.replace("Ɨ", "I");
    input.replace("ì", "i");
    input.replace("í", "i");
    input.replace("î", "i");
    input.replace("ï", "i");
    input.replace("ĩ", "i");
    input.replace("ī", "i");
    input.replace("ĭ", "i");
    input.replace("į", "i");
    input.replace("ı", "i");
    input.replace("ɨ", "i");

    // O variants
    input.replace("Ò", "O");
    input.replace("Ó", "O");
    input.replace("Ô", "O");
    input.replace("Õ", "O");
    input.replace("Ö", "O");
    input.replace("Ø", "O");
    input.replace("Ō", "O");
    input.replace("Ŏ", "O");
    input.replace("Ő", "O");
    input.replace("Œ", "OE");
    input.replace("Ɔ", "O");
    input.replace("ò", "o");
    input.replace("ó", "o");
    input.replace("ô", "o");
    input.replace("õ", "o");
    input.replace("ö", "o");
    input.replace("ø", "o");
    input.replace("ō", "o");
    input.replace("ŏ", "o");
    input.replace("ő", "o");
    input.replace("œ", "oe");
    input.replace("ɔ", "o");

    // U variants
    input.replace("Ù", "U");
    input.replace("Ú", "U");
    input.replace("Û", "U");
    input.replace("Ü", "U");
    input.replace("Ũ", "U");
    input.replace("Ū", "U");
    input.replace("Ŭ", "U");
    input.replace("Ů", "U");
    input.replace("Ű", "U");
    input.replace("Ų", "U");
    input.replace("Ʉ", "U");
    input.replace("ù", "u");
    input.replace("ú", "u");
    input.replace("û", "u");
    input.replace("ü", "u");
    input.replace("ũ", "u");
    input.replace("ū", "u");
    input.replace("ŭ", "u");
    input.replace("ů", "u");
    input.replace("ű", "u");
    input.replace("ų", "u");
    input.replace("ʉ", "u");

    // Y variants
    input.replace("Ý", "Y");
    input.replace("Ÿ", "Y");
    input.replace("Ŷ", "Y");
    input.replace("Ƴ", "Y");
    input.replace("ý", "y");
    input.replace("ÿ", "y");
    input.replace("ŷ", "y");
    input.replace("ƴ", "y");

    // C variants
    input.replace("Ç", "C");
    input.replace("Ć", "C");
    input.replace("Ĉ", "C");
    input.replace("Ċ", "C");
    input.replace("Č", "C");
    input.replace("Ƈ", "C");
    input.replace("ç", "c");
    input.replace("ć", "c");
    input.replace("ĉ", "c");
    input.replace("ċ", "c");
    input.replace("č", "c");
    input.replace("ƈ", "c");

    // D variants
    input.replace("Ď", "D");
    input.replace("Đ", "D");
    input.replace("Ɖ", "D");
    input.replace("Ɗ", "D");
    input.replace("ď", "d");
    input.replace("đ", "d");
    input.replace("ɖ", "d");
    input.replace("ɗ", "d");

    // G variants
    input.replace("Ĝ", "G");
    input.replace("Ğ", "G");
    input.replace("Ġ", "G");
    input.replace("Ģ", "G");
    input.replace("ĝ", "g");
    input.replace("ğ", "g");
    input.replace("ġ", "g");
    input.replace("ģ", "g");

    // H variants
    input.replace("Ĥ", "H");
    input.replace("Ħ", "H");
    input.replace("Ƕ", "H");
    input.replace("ĥ", "h");
    input.replace("ħ", "h");
    input.replace("ƕ", "h");

    // J variants
    input.replace("Ĵ", "J");
    input.replace("ĵ", "j");

    // K variants
    input.replace("Ķ", "K");
    input.replace("Ƙ", "K");
    input.replace("ķ", "k");
    input.replace("ƙ", "k");

    // L variants
    input.replace("Ĺ", "L");
    input.replace("Ļ", "L");
    input.replace("Ľ", "L");
    input.replace("Ŀ", "L");
    input.replace("Ł", "L");
    input.replace("Ƚ", "L");
    input.replace("ĺ", "l");
    input.replace("ļ", "l");
    input.replace("ľ", "l");
    input.replace("ŀ", "l");
    input.replace("ł", "l");
    input.replace("ƚ", "l");

    // N variants
    input.replace("Ñ", "N");
    input.replace("Ń", "N");
    input.replace("Ņ", "N");
    input.replace("Ň", "N");
    input.replace("Ŋ", "N");
    input.replace("Ɲ", "N");
    input.replace("ñ", "n");
    input.replace("ń", "n");
    input.replace("ņ", "n");
    input.replace("ň", "n");
    input.replace("ŋ", "n");
    input.replace("ɲ", "n");

    // R variants
    input.replace("Ŕ", "R");
    input.replace("Ŗ", "R");
    input.replace("Ř", "R");
    input.replace("Ʀ", "R");
    input.replace("ŕ", "r");
    input.replace("ŗ", "r");
    input.replace("ř", "r");
    input.replace("ʀ", "r");

    // S variants
    input.replace("Ś", "S");
    input.replace("Ŝ", "S");
    input.replace("Ş", "S");
    input.replace("Š", "S");
    input.replace("Ƨ", "S");
    input.replace("ß", "ss");
    input.replace("ś", "s");
    input.replace("ŝ", "s");
    input.replace("ş", "s");
    input.replace("š", "s");
    input.replace("ƨ", "s");

    // T variants
    input.replace("Ţ", "T");
    input.replace("Ť", "T");
    input.replace("Ŧ", "T");
    input.replace("Ƭ", "T");
    input.replace("ţ", "t");
    input.replace("ť", "t");
    input.replace("ŧ", "t");
    input.replace("ƭ", "t");

    // W variants
    input.replace("Ŵ", "W");
    input.replace("ŵ", "w");

    // Z variants
    input.replace("Ź", "Z");
    input.replace("Ż", "Z");
    input.replace("Ž", "Z");
    input.replace("Ƶ", "Z");
    input.replace("ź", "z");
    input.replace("ż", "z");
    input.replace("ž", "z");
    input.replace("ƶ", "z");

    // === Nordic/Scandinavian ===
    input.replace("Æ", "AE");
    input.replace("æ", "ae");
    input.replace("Þ", "Th");
    input.replace("þ", "th");
    input.replace("Ð", "D");
    input.replace("ð", "d");

    // === Currency Symbols ===
    input.replace("€", "EUR");
    input.replace("£", "GBP");
    input.replace("¥", "YEN");
    input.replace("¢", "c");
    input.replace("₹", "Rs");
    input.replace("₽", "RUB");
    input.replace("₩", "W");
    input.replace("₪", "NIS");
    input.replace("₫", "d");
    input.replace("₡", "C");
    input.replace("₦", "N");
    input.replace("₨", "Rs");
    input.replace("₱", "P");
    input.replace("₴", "G");
    input.replace("₵", "C");

    // === Mathematical Symbols ===
    input.replace("±", "+/-");
    input.replace("×", "x");
    input.replace("÷", "/");
    input.replace("∞", "inf");
    input.replace("≈", "~");
    input.replace("≠", "!=");
    input.replace("≤", "<=");
    input.replace("≥", ">=");
    input.replace("∑", "Sum");
    input.replace("∏", "Prod");
    input.replace("√", "sqrt");
    input.replace("∫", "int");
    input.replace("∂", "d");
    input.replace("∇", "grad");
    input.replace("∆", "Delta");
    input.replace("π", "pi");
    input.replace("Ω", "Ohm");
    input.replace("μ", "u");
    input.replace("α", "alpha");
    input.replace("β", "beta");
    input.replace("γ", "gamma");
    input.replace("δ", "delta");
    input.replace("λ", "lambda");
    input.replace("σ", "sigma");
    input.replace("φ", "phi");
    input.replace("ψ", "psi");
    input.replace("ω", "omega");

    // === Fractions ===
    input.replace("½", "1/2");
    input.replace("⅓", "1/3");
    input.replace("⅔", "2/3");
    input.replace("¼", "1/4");
    input.replace("¾", "3/4");
    input.replace("⅕", "1/5");
    input.replace("⅖", "2/5");
    input.replace("⅗", "3/5");
    input.replace("⅘", "4/5");
    input.replace("⅙", "1/6");
    input.replace("⅚", "5/6");
    input.replace("⅐", "1/7");
    input.replace("⅛", "1/8");
    input.replace("⅜", "3/8");
    input.replace("⅝", "5/8");
    input.replace("⅞", "7/8");
    input.replace("⅑", "1/9");
    input.replace("⅒", "1/10");

    // === Superscripts & Subscripts ===
    input.replace("¹", "1");
    input.replace("²", "2");
    input.replace("³", "3");
    input.replace("⁴", "4");
    input.replace("⁵", "5");
    input.replace("⁶", "6");
    input.replace("⁷", "7");
    input.replace("⁸", "8");
    input.replace("⁹", "9");
    input.replace("⁰", "0");
    input.replace("ⁿ", "n");

    // === Punctuation & Typography ===
    input.replace("–", "-");
    input.replace("—", "-");
    input.replace("―", "-");
    input.replace("\u201C", "\""); // left double quote
    input.replace("\u201D", "\""); // right double quote
    input.replace("„", "\"");      // double low-9 quote
    input.replace("\u2018", "'");  // left single quote
    input.replace("\u2019", "'");  // right single quote
    input.replace("‚", "'");       // single low-9 quote
    input.replace("'", "'");       // apostrophe variant
    input.replace("`", "'");       // grave accent
    input.replace("´", "'");       // acute accent
    input.replace("…", "...");
    input.replace("•", "*");
    input.replace("‣", ">");
    input.replace("◦", "o");
    input.replace("▪", "*");
    input.replace("▫", "o");
    input.replace("‰", "o/oo");
    input.replace("′", "'");
    input.replace("″", "\"");
    input.replace("‴", "'''");
    input.replace("§", "S");
    input.replace("¶", "P");
    input.replace("†", "+");
    input.replace("‡", "++");
    input.replace("‖", "||");

    // === Arrows ===
    input.replace("←", "<-");
    input.replace("→", "->");
    input.replace("↑", "^");
    input.replace("↓", "v");
    input.replace("↔", "<->");
    input.replace("↕", "^v");
    input.replace("⇐", "<=");
    input.replace("⇒", "=>");
    input.replace("⇔", "<=>");

    // === Common Emojis (most popular ones) ===
    input.replace("😀", ":)");
    input.replace("😁", ":D");
    input.replace("😂", "LOL");
    input.replace("🤣", "ROFL");
    input.replace("😃", ":)");
    input.replace("😄", ":D");
    input.replace("😅", ":')");
    input.replace("😆", "XD");
    input.replace("😉", ";)");
    input.replace("😊", ":)");
    input.replace("😋", ":P");
    input.replace("😎", "B)");
    input.replace("😍", "<3");
    input.replace("🥰", "<3");
    input.replace("😘", ":*");
    input.replace("😗", ":*");
    input.replace("😙", ":*");
    input.replace("😚", ":*");
    input.replace("🙂", ":)");
    input.replace("🤗", "hug");
    input.replace("🤔", "hmm");
    input.replace("🤭", "oops");
    input.replace("🤫", "shh");
    input.replace("🤐", "zip");
    input.replace("😐", ":|");
    input.replace("😑", "-_-");
    input.replace("😶", "...");
    input.replace("😏", ";)");
    input.replace("😒", ":/");
    input.replace("🙄", "roll");
    input.replace("😬", "eek");
    input.replace("🤥", "lie");
    input.replace("😔", ":(");
    input.replace("😕", ":/");
    input.replace("🙁", ":(");
    input.replace("☹️", ":(");
    input.replace("😣", ">:(");
    input.replace("😖", "X(");
    input.replace("😫", "argh");
    input.replace("😩", "ugh");
    input.replace("🥺", ":(");
    input.replace("😢", ":'(");
    input.replace("😭", "T_T");
    input.replace("😤", "hmph");
    input.replace("😠", ">:(");
    input.replace("😡", "RAGE");
    input.replace("🤬", "@#$%");
    input.replace("🤯", "BOOM");
    input.replace("😳", "O_O");
    input.replace("🥵", "hot");
    input.replace("🥶", "cold");
    input.replace("😱", "OMG");
    input.replace("😨", "scared");
    input.replace("😰", "nervous");
    input.replace("😥", "phew");
    input.replace("😓", "sweat");
    input.replace("🤗", "hug");
    input.replace("🤤", "drool");
    input.replace("😴", "zzz");
    input.replace("😪", "tired");

    // Hearts and love
    input.replace("❤️", "<3");
    input.replace("🧡", "<3");
    input.replace("💛", "<3");
    input.replace("💚", "<3");
    input.replace("💙", "<3");
    input.replace("💜", "<3");
    input.replace("🖤", "</3");
    input.replace("🤍", "<3");
    input.replace("🤎", "<3");
    input.replace("💔", "</3");
    input.replace("💕", "<3<3");
    input.replace("💖", "<3!");
    input.replace("💗", "<3");
    input.replace("💘", "<3");
    input.replace("💝", "gift");

    // Hands and gestures
    input.replace("👍", "+1");
    input.replace("👎", "-1");
    input.replace("👌", "OK");
    input.replace("✌️", "peace");
    input.replace("🤞", "fingers crossed");
    input.replace("🤟", "love");
    input.replace("🤘", "rock");
    input.replace("🤙", "call");
    input.replace("👈", "<-");
    input.replace("👉", "->");
    input.replace("👆", "^");
    input.replace("👇", "v");
    input.replace("☝️", "!");
    input.replace("✋", "stop");
    input.replace("🤚", "stop");
    input.replace("🖐️", "5");
    input.replace("🖖", "vulcan");
    input.replace("👋", "wave");
    input.replace("🤝", "shake");
    input.replace("👏", "clap");
    input.replace("🙌", "praise");
    input.replace("👐", "open");
    input.replace("🤲", "pray");
    input.replace("🙏", "pray");
    input.replace("✍️", "write");
    input.replace("💪", "strong");

    // Common symbols
    input.replace("⭐", "*");
    input.replace("🌟", "*");
    input.replace("✨", "sparkle");
    input.replace("🔥", "fire");
    input.replace("💧", "drop");
    input.replace("⚡", "zap");
    input.replace("☀️", "sun");
    input.replace("🌙", "moon");
    input.replace("⭐", "star");
    input.replace("🌈", "rainbow");
    input.replace("☁️", "cloud");
    input.replace("⛅", "cloudy");
    input.replace("🌧️", "rain");
    input.replace("⛈️", "storm");
    input.replace("🌩️", "lightning");
    input.replace("❄️", "snow");
    input.replace("☃️", "snowman");
    input.replace("⛄", "snowman");

    // Food common
    input.replace("🍕", "pizza");
    input.replace("🍔", "burger");
    input.replace("🍟", "fries");
    input.replace("🌭", "hotdog");
    input.replace("🥪", "sandwich");
    input.replace("🌮", "taco");
    input.replace("🌯", "burrito");
    input.replace("🍝", "pasta");
    input.replace("🍜", "ramen");
    input.replace("🍲", "stew");
    input.replace("🍛", "curry");
    input.replace("🍚", "rice");
    input.replace("🍞", "bread");
    input.replace("🥖", "baguette");
    input.replace("🥨", "pretzel");
    input.replace("🧀", "cheese");
    input.replace("🥓", "bacon");
    input.replace("🍖", "meat");
    input.replace("🍗", "chicken");
    input.replace("🥩", "steak");
    input.replace("🍳", "egg");
    input.replace("🥚", "egg");
    input.replace("🧈", "butter");
    input.replace("🥞", "pancakes");
    input.replace("🧇", "waffle");
    input.replace("🥯", "bagel");
    input.replace("🍰", "cake");
    input.replace("🎂", "cake");
    input.replace("🧁", "cupcake");
    input.replace("🥧", "pie");
    input.replace("🍮", "pudding");
    input.replace("🍭", "candy");
    input.replace("🍬", "candy");
    input.replace("🍫", "chocolate");
    input.replace("🍩", "donut");
    input.replace("🍪", "cookie");

    // Drinks
    input.replace("☕", "coffee");
    input.replace("🍵", "tea");
    input.replace("🧃", "juice");
    input.replace("🥤", "soda");
    input.replace("🧋", "boba");
    input.replace("🍺", "beer");
    input.replace("🍻", "cheers");
    input.replace("🍷", "wine");
    input.replace("🥂", "champagne");
    input.replace("🍾", "bottle");
    input.replace("🍸", "cocktail");
    input.replace("🍹", "tropical");
    input.replace("🍼", "bottle");
    input.replace("🥛", "milk");
    input.replace("💧", "water");

    // Activities & Objects
    input.replace("🎵", "music");
    input.replace("🎶", "notes");
    input.replace("🎤", "mic");
    input.replace("🎧", "headphones");
    input.replace("📱", "phone");
    input.replace("💻", "laptop");
    input.replace("🖥️", "computer");
    input.replace("⌨️", "keyboard");
    input.replace("🖱️", "mouse");
    input.replace("🖨️", "printer");
    input.replace("📷", "camera");
    input.replace("📹", "video");
    input.replace("📺", "TV");
    input.replace("📻", "radio");
    input.replace("⏰", "alarm");
    input.replace("⏱️", "timer");
    input.replace("⏲️", "timer");
    input.replace("🕐", "1pm");
    input.replace("📚", "books");
    input.replace("📖", "book");
    input.replace("📝", "note");
    input.replace("✏️", "pencil");
    input.replace("🖊️", "pen");
    input.replace("🖋️", "pen");
    input.replace("🖍️", "crayon");
    input.replace("📐", "ruler");
    input.replace("📏", "ruler");
    input.replace("✂️", "scissors");
    input.replace("📎", "clip");
    input.replace("📌", "pin");
    input.replace("🔗", "link");
    input.replace("🔒", "lock");
    input.replace("🔓", "unlock");
    input.replace("🔑", "key");
    input.replace("🗝️", "key");
    input.replace("🔨", "hammer");
    input.replace("⚒️", "hammer");
    input.replace("🛠️", "tools");
    input.replace("⚙️", "gear");
    input.replace("🔧", "wrench");
    input.replace("🔩", "bolt");
    input.replace("⚡", "power");

    // Transport
    input.replace("🚗", "car");
    input.replace("🚙", "SUV");
    input.replace("🚐", "van");
    input.replace("🚛", "truck");
    input.replace("🚲", "bike");
    input.replace("🛴", "scooter");
    input.replace("🛵", "moped");
    input.replace("🏍️", "motorcycle");
    input.replace("✈️", "plane");
    input.replace("🚁", "helicopter");
    input.replace("🚂", "train");
    input.replace("🚃", "train");
    input.replace("🚄", "bullet train");
    input.replace("🚅", "train");
    input.replace("🚆", "train");
    input.replace("🚇", "metro");
    input.replace("🚈", "monorail");
    input.replace("🚉", "station");
    input.replace("🚊", "tram");
    input.replace("🚝", "monorail");
    input.replace("🚞", "railway");
    input.replace("🚟", "suspension");
    input.replace("🚠", "cable");
    input.replace("🚡", "aerial");
    input.replace("⛵", "sail");
    input.replace("🛶", "canoe");
    input.replace("🚤", "speedboat");
    input.replace("🛥️", "boat");
    input.replace("🚢", "ship");
    input.replace("⚓", "anchor");

    // Final fallback for any remaining non-ASCII characters
    String result = "";
    for (int i = 0; i < input.length(); i++)
    {
        unsigned char c = input.charAt(i);
        if (c >= 32 && c <= 126)
        { // ASCII printable range
            result += (char)c;
        }
        else if (c == 9 || c == 10 || c == 13)
        { // Tab, LF, CR
            result += (char)c;
        }
        else
        {
            // Any unmapped character becomes a space to avoid control characters
            result += " ";
        }
    }

    // Clean up multiple consecutive spaces
    while (result.indexOf("  ") != -1)
    {
        result.replace("  ", " ");
    }

    return result;
}
