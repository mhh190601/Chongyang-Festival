#include "DivinationScene.h"

#include "../core/Input.h"
#include "../core/DrawUtil.h"

#include <cstdio>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <windows.h>

namespace {

constexpr double kPI = 3.14159265358979323846;

// ---- 阵法转速（弧度 / 秒）----
constexpr double kIdleSpin    = 0.35;    // 缓缓自转
constexpr double kRollingSpin = 24.0;    // 推演时暴增
constexpr DWORD  kRollingMs   = 1500;    // 推演持续 1.5 秒

// 宽字符转 UTF-8（写日志文件用，避免 printf / wprintf 混用导致的流定向问题）
std::string ToUtf8(const std::wstring& ws) {
    if (ws.empty()) return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, NULL, 0, NULL, NULL);
    std::string s((size_t)len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], len, NULL, NULL);
    s.pop_back();
    return s;
}

// 先天八卦数：1乾 2兑 3离 4震 5巽 6坎 7艮 8坤
const wchar_t* kTrigramName[9] = { L"", L"乾", L"兑", L"离", L"震", L"巽", L"坎", L"艮", L"坤" };

// 经卦三爻（下标 0..2 = 自下而上；1 为阳爻，0 为阴爻）
const int kTrigramYao[9][3] = {
    { 0, 0, 0 },
    { 1, 1, 1 },  // 1 乾  ☰
    { 1, 1, 0 },  // 2 兑  ☱
    { 1, 0, 1 },  // 3 离  ☲
    { 1, 0, 0 },  // 4 震  ☳
    { 0, 1, 1 },  // 5 巽  ☴
    { 0, 1, 0 },  // 6 坎  ☵
    { 0, 0, 1 },  // 7 艮  ☶
    { 0, 0, 0 }   // 8 坤  ☷
};

struct HexagramInfo {
    int upper;
    int lower;
    const wchar_t* name;    // 卦名（文王卦序）
    const wchar_t* ci;      // 卦辞（《大象传》）
};

// 文王六十四卦（按卦序排列）
const HexagramInfo kHexagram[64] = {
    { 1, 1, L"乾为天",   L"天行健，君子以自强不息" },
    { 8, 8, L"坤为地",   L"地势坤，君子以厚德载物" },
    { 6, 4, L"水雷屯",   L"云雷，屯；君子以经纶" },
    { 7, 6, L"山水蒙",   L"山下出泉，蒙；君子以果行育德" },
    { 6, 1, L"水天需",   L"云上于天，需；君子以饮食宴乐" },
    { 1, 6, L"天水讼",   L"天与水违行，讼；君子以作事谋始" },
    { 8, 6, L"地水师",   L"地中有水，师；君子以容民畜众" },
    { 6, 8, L"水地比",   L"地上有水，比；先王以建万国，亲诸侯" },
    { 5, 1, L"风天小畜", L"风行天上，小畜；君子以懿文德" },
    { 1, 2, L"天泽履",   L"上天下泽，履；君子以辨上下，定民志" },
    { 8, 1, L"地天泰",   L"天地交，泰；后以财成天地之道，辅相天地之宜" },
    { 1, 8, L"天地否",   L"天地不交，否；君子以俭德辟难，不可荣以禄" },
    { 1, 3, L"天火同人", L"天与火，同人；君子以类族辨物" },
    { 3, 1, L"火天大有", L"火在天上，大有；君子以遏恶扬善，顺天休命" },
    { 8, 7, L"地山谦",   L"地中有山，谦；君子以裒多益寡，称物平施" },
    { 4, 8, L"雷地豫",   L"雷出地奋，豫；先王以作乐崇德，殷荐上帝" },
    { 2, 4, L"泽雷随",   L"泽中有雷，随；君子以向晦入宴息" },
    { 7, 5, L"山风蛊",   L"山下有风，蛊；君子以振民育德" },
    { 8, 2, L"地泽临",   L"泽上有地，临；君子以教思无穷，容保民无疆" },
    { 5, 8, L"风地观",   L"风行地上，观；先王以省方观民设教" },
    { 3, 4, L"火雷噬嗑", L"雷电，噬嗑；先王以明罚敕法" },
    { 7, 3, L"山火贲",   L"山下有火，贲；君子以明庶政，无敢折狱" },
    { 7, 8, L"山地剥",   L"山附于地，剥；上以厚下安宅" },
    { 8, 4, L"地雷复",   L"雷在地中，复；先王以至日闭关，商旅不行" },
    { 1, 4, L"天雷无妄", L"天下雷行，物与无妄；先王以茂对时，育万物" },
    { 7, 1, L"山天大畜", L"天在山中，大畜；君子以多识前言往行，以畜其德" },
    { 7, 4, L"山雷颐",   L"山下有雷，颐；君子以慎言语，节饮食" },
    { 2, 5, L"泽风大过", L"泽灭木，大过；君子以独立不惧，遁世无闷" },
    { 6, 6, L"坎为水",   L"水洊至，习坎；君子以常德行，习教事" },
    { 3, 3, L"离为火",   L"明两作，离；大人以继明照于四方" },
    { 2, 7, L"泽山咸",   L"山上有泽，咸；君子以虚受人" },
    { 4, 5, L"雷风恒",   L"雷风，恒；君子以立不易方" },
    { 1, 7, L"天山遁",   L"天下有山，遁；君子以远小人，不恶而严" },
    { 4, 1, L"雷天大壮", L"雷在天上，大壮；君子以非礼弗履" },
    { 3, 8, L"火地晋",   L"明出地上，晋；君子以自昭明德" },
    { 8, 3, L"地火明夷", L"明入地中，明夷；君子以莅众，用晦而明" },
    { 5, 3, L"风火家人", L"风自火出，家人；君子以言有物而行有恒" },
    { 3, 2, L"火泽睽",   L"上火下泽，睽；君子以同而异" },
    { 6, 7, L"水山蹇",   L"山上有水，蹇；君子以反身修德" },
    { 4, 6, L"雷水解",   L"雷雨作，解；君子以赦过宥罪" },
    { 7, 2, L"山泽损",   L"山下有泽，损；君子以惩忿窒欲" },
    { 5, 4, L"风雷益",   L"风雷，益；君子以见善则迁，有过则改" },
    { 2, 1, L"泽天夬",   L"泽上于天，夬；君子以施禄及下，居德则忌" },
    { 1, 5, L"天风姤",   L"天下有风，姤；后以施命诰四方" },
    { 2, 8, L"泽地萃",   L"泽上于地，萃；君子以除戎器，戒不虞" },
    { 8, 5, L"地风升",   L"地中生木，升；君子以顺德，积小以高大" },
    { 2, 6, L"泽水困",   L"泽无水，困；君子以致命遂志" },
    { 6, 5, L"水风井",   L"木上有水，井；君子以劳民劝相" },
    { 2, 3, L"泽火革",   L"泽中有火，革；君子以治历明时" },
    { 3, 5, L"火风鼎",   L"木上有火，鼎；君子以正位凝命" },
    { 4, 4, L"震为雷",   L"洊雷，震；君子以恐惧修省" },
    { 7, 7, L"艮为山",   L"兼山，艮；君子以思不出其位" },
    { 5, 7, L"风山渐",   L"山上有木，渐；君子以居贤德善俗" },
    { 4, 2, L"雷泽归妹", L"泽上有雷，归妹；君子以永终知敝" },
    { 4, 3, L"雷火丰",   L"雷电皆至，丰；君子以折狱致刑" },
    { 3, 7, L"火山旅",   L"山上有火，旅；君子以明慎用刑，而不留狱" },
    { 5, 5, L"巽为风",   L"随风，巽；君子以申命行事" },
    { 2, 2, L"兑为泽",   L"丽泽，兑；君子以朋友讲习" },
    { 5, 6, L"风水涣",   L"风行水上，涣；先王以享于帝立庙" },
    { 6, 2, L"水泽节",   L"泽上有水，节；君子以制数度，议德行" },
    { 5, 2, L"风泽中孚", L"泽上有风，中孚；君子以议狱缓死" },
    { 4, 7, L"雷山小过", L"山上有雷，小过；君子以行过乎恭，丧过乎哀，用过乎俭" },
    { 6, 3, L"水火既济", L"水在火上，既济；君子以思患而豫防之" },
    { 3, 6, L"火水未济", L"火在水上，未济；君子以慎辨物居方" }
};

// 依上下卦查文王卦序
int FindHexagram(int upper, int lower) {
    for (int i = 0; i < 64; ++i) {
        if (kHexagram[i].upper == upper && kHexagram[i].lower == lower) return i;
    }
    return -1;
}

const wchar_t* YaoSymbol(int v) { return v ? L"▅▅▅▅▅" : L"▅▅▅ ▅▅"; }

int Num8(int v) { int r = v % 8; return (r == 0) ? 8 : r; }
int Num6(int v) { int r = v % 6; return (r == 0) ? 6 : r; }

// 吉凶签文（装饰用：八字与卦象均由 BaziCalculator 真实推算，此处只做展示）
// poem / note 中的 \n 表示换行，渲染时逐行居中绘制
struct Fortune {
    const wchar_t* no;     // 签号
    const wchar_t* level;  // 吉凶
    const wchar_t* poem;   // 签诗（两句）
    const wchar_t* note;   // 解签
};

const Fortune kFortunes[13] = {
    { L"007", L"上上签", L"云开月现，前程万里\n春风得意，马蹄轻疾", L"所求之事，水到渠成\n宜乘势而行，不可自满" },
    { L"018", L"上上签", L"龙山佳会，贵人相扶\n高台月明，影形不离", L"得他人之力可成大事\n切记知恩图报，不可独功" },
    { L"026", L"上  吉", L"登山涉水，终见桃源\n柳暗花明，又一村庄", L"眼下虽劳，终有善果\n坚持则吉，半途则废" },
    { L"033", L"上  吉", L"枯木逢春，晚景丰隆\n旧雨再来，故人重逢", L"迟来的福分亦是福分\n宜惜眼下人，莫负今朝" },
    { L"041", L"中  吉", L"舟行顺风，所求可成\n满帆待发，一往无前", L"时机已至，宜果断出手\n迟疑再三，反失良机" },
    { L"047", L"中  吉", L"守静待时，自有机缘\n深藏若虚，明镜非台", L"急则无功，静则有机\n宜养精蓄锐，以待天时" },
    { L"052", L"中  吉", L"退一步行，宽一步路\n让他三分，自有归途", L"争强好胜反受其害\n退让从容，反得其门" },
    { L"058", L"平  签", L"平安是福，无贪无求\n淡饭粗茶，亦是珍馐", L"无波无浪便是福\n妄求反招烦恼，知足常乐" },
    { L"063", L"平  签", L"莫问收获，但问耕耘\n滴水穿石，日久自成", L"但行好事，莫问前程\n功夫不负有心之人" },
    { L"071", L"小  吉", L"小心谨慎，逢凶化吉\n如履薄冰，终登彼岸", L"细节之处见吉凶\n慎始慎终，方保无恙" },
    { L"076", L"小  吉", L"早起三光，迟起三慌\n一日之计，在于寅时", L"勤则无忧，惰则事败\n宜早作打算，勿临时抱佛" },
    { L"083", L"待  时", L"藏器于身，待时而动\n潜龙在渊，不争先得", L"羽翼未丰，不宜强出头\n静候风云，自有腾达日" },
    // ---- 专属签：水山蹇（第 39 卦）----
    { L"059", L"中  吉", L"谦谦君子德，蹇难莫强求\n云开见月明，静待好个秋", L"当前遇“蹇”卦，进则有阻，退则安\n不可急躁，宜守不宜攻，待贵人相助" }
};

// 动爻的汉字数字
const wchar_t* kYaoNum[7] = { L"", L"一", L"二", L"三", L"四", L"五", L"六" };

} // namespace

// ============================================================================

DivinationScene::DivinationScene() {
    lastTick  = GetTickCount();
    phaseTick = lastTick;
    printf("[Log] 进入测卦象：等待起局\n");
}

int DivinationScene::ClampInt(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

double DivinationScene::EaseOutCubic(double t) {
    if (t <= 0.0) return 0.0;
    if (t >= 1.0) return 1.0;
    const double u = 1.0 - t;
    return 1.0 - u * u * u;
}

void DivinationScene::AdjustField(int delta) {
    switch (field) {
        case 0: year  = ClampInt(year  + delta, 1900, 2100); break;
        case 1: month = ClampInt(month + delta, 1, 12);     break;
        case 2: day   = ClampInt(day   + delta, 1, 31);     break;
        case 3: hour  = ClampInt(hour  + delta, 0, 23);     break;
        default: break;
    }
    errorMsg.clear();   // 一调整就消掉错误提示
}

void DivinationScene::TryStartRolling() {
    BirthInfo b;
    b.year = year; b.month = month; b.day = day; b.hour = hour; b.minute = 0;

    BaziResult r = BaziCalculator::Calc(b);
    if (!r.valid) {
        errorMsg  = r.errMsg;
        errorTick = GetTickCount();
        printf("[Log] 输入不合法：%s\n", ToUtf8(r.errMsg).c_str());
        return;   // 停留在输入阶段
    }

    birth = b;
    bazi  = r;
    BuildHexagram();

    phase      = DivinationPhase::ROLLING;
    phaseTick  = GetTickCount();
    spinSpeed  = kRollingSpin;
    printf("[Log] 正在推演天机...\n");
}

void DivinationScene::EnterResult() {
    phase     = DivinationPhase::RESULT;
    phaseTick = GetTickCount();
    spinSpeed = 0.0;      // 阵法骤然停止
    hasResult = true;
    printf("[Log] 卦象已成：%s / %s / %s\n",
           ToUtf8(bazi.FourPillars()).c_str(),
           ToUtf8(guaName).c_str(),
           ToUtf8(kHexagram[changedIndex >= 0 ? changedIndex : 0].name).c_str());
}

void DivinationScene::ResetToInput() {
    phase     = DivinationPhase::INPUT;
    phaseTick = GetTickCount();
    spinSpeed = kIdleSpin;
    hasResult = false;
    errorMsg.clear();
    printf("[Log] 重新起局\n");
}

void DivinationScene::BuildHexagram() {
    // 梅花易数·时间卦：
    //   上卦 = (年 + 月 + 日) % 8
    //   下卦 = (年 + 月 + 日 + 时) % 8
    //   动爻 = (年 + 月 + 日 + 时) % 6
    const int nYear  = bazi.yearPillar.zhi + 1;        // 年支数（子1 ... 亥12）
    const int nMonth = bazi.lunar.month;               // 农历月
    const int nDay   = bazi.lunar.day;                 // 农历日
    const int nHour  = bazi.hourPillar.zhi + 1;        // 时支数

    const int sumYMD = nYear + nMonth + nDay;
    const int sumAll = sumYMD + nHour;

    upperTrigram = Num8(sumYMD);
    lowerTrigram = Num8(sumAll);
    movingYao    = Num6(sumAll);

    hexIndex = FindHexagram(upperTrigram, lowerTrigram);
    if (hexIndex < 0) hexIndex = 0;

    guaName = kHexagram[hexIndex].name;
    guaCi   = kHexagram[hexIndex].ci;

    // 六爻符号
    int yao[6];
    for (int i = 0; i < 3; ++i) yao[i]     = kTrigramYao[lowerTrigram][i];
    for (int i = 0; i < 3; ++i) yao[i + 3] = kTrigramYao[upperTrigram][i];

    yaoLines.clear();
    for (int i = 0; i < 6; ++i) {
        std::wstring s = YaoSymbol(yao[i]);
        if (i == movingYao - 1) s += yao[i] ? L" ○" : L" ×";   // 阳动○，阴动×
        yaoLines.push_back(s);
    }

    // 变卦：动爻阴阳互变
    int changed[6];
    for (int i = 0; i < 6; ++i) changed[i] = yao[i];
    changed[movingYao - 1] = 1 - changed[movingYao - 1];

    int newLower = 0, newUpper = 0;
    for (int g = 1; g <= 8; ++g) {
        if (kTrigramYao[g][0] == changed[0] && kTrigramYao[g][1] == changed[1] && kTrigramYao[g][2] == changed[2]) newLower = g;
        if (kTrigramYao[g][0] == changed[3] && kTrigramYao[g][1] == changed[4] && kTrigramYao[g][2] == changed[5]) newUpper = g;
    }
    changedIndex = FindHexagram(newUpper, newLower);

    // 签文序号（由卦象与动爻决定，同一次起卦结果稳定）
    fortuneIndex = (hexIndex * 7 + movingYao * 3) % 12;

    // 水山蹇卦配专属签文（第 059 签）：按卦名查找下标，避免表格调序后错位
    for (int i = 0; i < 64; ++i) {
        if (std::wcscmp(kHexagram[i].name, L"水山蹇") == 0) {
            if (hexIndex == i) fortuneIndex = 12;   // kFortunes 最后一项
            break;
        }
    }

    const Fortune& f = kFortunes[fortuneIndex];
    fortuneNo    = f.no;
    fortuneLevel = f.level;
    fortunePoem  = f.poem;
    fortuneNote  = f.note;
}

GameState DivinationScene::Update() {
    const DWORD now = GetTickCount();
    const double dt = (now - lastTick) / 1000.0;
    lastTick = now;

    // 阵法旋转（时间驱动，不随帧率漂移）
    spin += spinSpeed * dt;

    switch (phase) {
        case DivinationPhase::INPUT: {
            if (Input::IsKeyPressed(VK_LEFT))  field = (field + 3) % 4;
            if (Input::IsKeyPressed(VK_RIGHT)) field = (field + 1) % 4;
            if (Input::IsKeyPressed(VK_UP))    AdjustField(+1);
            if (Input::IsKeyPressed(VK_DOWN))  AdjustField(-1);
            if (Input::IsKeyPressed(VK_RETURN)) TryStartRolling();
            if (Input::IsKeyPressed(VK_ESCAPE)) return STATE_MENU;
            break;
        }

        case DivinationPhase::ROLLING: {
            if (now - phaseTick >= kRollingMs) EnterResult();
            if (Input::IsKeyPressed(VK_ESCAPE)) return STATE_MENU;
            break;
        }

        case DivinationPhase::RESULT: {
            if (Input::IsKeyPressed(VK_RETURN)) ResetToInput();     // 重新起卦
            if (Input::IsKeyPressed(VK_ESCAPE)) return STATE_MENU;  // 返回主菜单
            break;
        }
    }

    return STATE_DIVINATION;
}

// ---------------------------------------------------------------------------
//  绘制：标准的金色八卦阵（先天八卦方位 + 卦名标注 + 发光爻线）
//  注意：本函数在绝对坐标下调用，居中于物理屏幕中心
// ---------------------------------------------------------------------------
void DivinationScene::DrawBaguaArray() const {
    const COLORREF bg    = NightBg();
    const COLORREF kGold = RGB(255, 215, 0);

    // 居中于物理屏幕（设计区中心恰好就是屏幕中心，两者一致）
    const int W = (SCREEN_W > 0 ? SCREEN_W : WIN_W);
    const int H = (SCREEN_H > 0 ? SCREEN_H : WIN_H);
    const int cx = W / 2;
    const int cy = H / 2;

    // INPUT 阶段用小半径，把视觉焦点让给中心的输入面板；
    // ROLLING / RESULT 阶段恢复大半径，铺满全屏后再落黑纱、砸巨签。
    const double radiusRatio = (phase == DivinationPhase::INPUT) ? 0.22 : 0.30;
    const int    R = (int)((W < H ? W : H) * radiusRatio);

    // ---- 外层光晕：几圈逐渐收紧的暗金圆 ----
    for (int i = 5; i >= 1; --i) {
        setlinecolor(BlendColor(bg, kGold, 0.028 * i));
        setlinestyle(PS_SOLID, i * 3);
        circle(cx, cy, R + i * 7);
    }

    // ---- 内外两道金环 ----
    setlinecolor(BlendColor(bg, kGold, 0.85));
    setlinestyle(PS_SOLID, 2);
    circle(cx, cy, R);
    circle(cx, cy, (int)(R * 0.90));

    // ---- 内圈虚线环（24 段）----
    setlinecolor(BlendColor(bg, kGold, 0.40));
    setlinestyle(PS_SOLID, 1);
    for (int i = 0; i < 24; ++i) {
        const double a0 = i * kPI / 12.0;
        const double a1 = a0 + kPI / 36.0;
        const int r = (int)(R * 0.50);
        line(cx + (int)(std::cos(a0) * r), cy + (int)(std::sin(a0) * r),
             cx + (int)(std::cos(a1) * r), cy + (int)(std::sin(a1) * r));
    }

    // ---- 八根辐条：从内环连到各卦 ----
    setlinecolor(BlendColor(bg, kGold, 0.28));
    setlinestyle(PS_SOLID, 1);
    for (int g = 0; g < 8; ++g) {
        const double a = spin + g * kPI / 4.0;
        line(cx + (int)(std::cos(a) * R * 0.50), cy + (int)(std::sin(a) * R * 0.50),
             cx + (int)(std::cos(a) * R * 0.98), cy + (int)(std::sin(a) * R * 0.98));
    }

    // ---- 八组三爻：阳爻实线、阴爻断线，沿半径排列，随阵法旋转 ----
    auto yaoBar = [&](int x1, int y1, int x2, int y2, double alpha) {
        setlinecolor(BlendColor(bg, kGold, alpha * 0.22));
        setlinestyle(PS_SOLID, 6);
        line(x1, y1, x2, y2);
        setlinecolor(BlendColor(bg, kGold, alpha));
        setlinestyle(PS_SOLID, 2);
        line(x1, y1, x2, y2);
    };

    for (int g = 0; g < 8; ++g) {
        const double a  = spin + g * kPI / 4.0;      // 先天八卦：每 45° 一卦
        const double ca = std::cos(a);
        const double sa = std::sin(a);

        for (int k = 0; k < 3; ++k) {
            const int r  = (int)(R * (0.58 + k * 0.10));
            const int px = cx + (int)(ca * r);
            const int py = cy + (int)(sa * r);

            const double tx = -sa;                    // 切向
            const double ty =  ca;

            const bool yang = (kTrigramYao[g + 1][k] != 0);

            if (yang) {
                yaoBar(px - (int)(tx * 26), py - (int)(ty * 26),
                       px + (int)(tx * 26), py + (int)(ty * 26), 0.95);
            } else {
                yaoBar(px - (int)(tx * 26), py - (int)(ty * 26),
                       px - (int)(tx * 9),  py - (int)(ty * 9),  0.80);
                yaoBar(px + (int)(tx * 9),  py + (int)(ty * 9),
                       px + (int)(tx * 26), py + (int)(ty * 26), 0.80);
            }
        }

        // ---- 卦名小字标注（每个卦象旁）----
        const int labelR = (int)(R * 1.02);
        const int lx = cx + (int)(ca * labelR);
        const int ly = cy + (int)(sa * labelR);
        const int labelSize = (phase == DivinationPhase::INPUT) ? 15 : 19;
        settextstyle(labelSize, 0, L"楷体");
        settextcolor(BlendColor(bg, RGB(255, 232, 160), 0.85));
        outtextxy(lx - labelSize / 2, ly - labelSize / 2, kTrigramName[g + 1]);
    }

    // ---- 中心：未揭晓时是一颗发光核心 ----
    if (phase != DivinationPhase::RESULT) {
        const double pulse = 0.5 + 0.5 * std::sin(GetTickCount() / 320.0);
        for (int i = 4; i >= 1; --i) {
            setfillcolor(BlendColor(bg, kGold, 0.03 * i * (0.5 + pulse)));
            setlinecolor(BlendColor(bg, kGold, 0.03 * i * (0.5 + pulse)));
            solidcircle(cx, cy, (int)(R * 0.16 * i / 2.0) + i * 4);
        }
    }
}

// ---------------------------------------------------------------------------
//  绘制：阶段一 · 生辰输入卷轴
// ---------------------------------------------------------------------------
void DivinationScene::DrawInputPanel() const {
    const COLORREF bg    = NightBg();
    const COLORREF kGold = RGB(255, 215, 0);
    const int W = WIN_W;
    const int H = WIN_H;
    const int cx = W / 2;
    const int cy = H / 2 - 10;

    // 面板改小：480 x 200（原来 660x280 太肥，会盖住背后的八卦阵）
    const int pw = 480, ph = 200;
    const int px1 = cx - pw / 2, py1 = cy - ph / 2;
    const int px2 = cx + pw / 2, py2 = cy + ph / 2;

    // ---- 底：非常淡的深色半透明 ----
    const COLORREF fill = BlendColor(bg, RGB(24, 20, 38), 0.55);
    setfillcolor(fill);
    setlinecolor(fill);
    solidroundrect(px1, py1, px2, py2, 12, 12);

    // ---- 边框：极细的金色发光线条（外层柔光 + 内层细亮线，不用粗大圆角矩形）----
    setlinestyle(PS_SOLID, 4);
    setlinecolor(BlendColor(bg, kGold, 0.16));
    roundrect(px1 - 3, py1 - 3, px2 + 3, py2 + 3, 15, 15);
    setlinestyle(PS_SOLID, 1);
    setlinecolor(BlendColor(bg, kGold, 0.90));
    roundrect(px1, py1, px2, py2, 12, 12);

    // ---- 标题 ----
    settextstyle(24, 0, L"楷体");
    settextcolor(BlendColor(bg, RGB(255, 226, 150), 0.95));
    const wchar_t* cap = L"请调整公历（新历）生辰";
    outtextxy(cx - textwidth(cap) / 2, py1 + 18, cap);

    // 四个字段（2x2）
    const wchar_t* labels[4] = { L"年", L"月", L"日", L"时" };
    const int       values[4] = { year, month, day, hour };

    for (int i = 0; i < 4; ++i) {
        const int col  = i % 2;
        const int row  = i / 2;
        const int fx   = px1 + 42 + col * 208;
        const int fy   = py1 + 66 + row * 62;
        const bool sel = (i == field);

        wchar_t buf[16];
        swprintf(buf, 16, L"%d", values[i]);

        // 文字稍微缩小：选中 40px，未选中 28px
        settextstyle(sel ? 40 : 28, 0, L"微软雅黑");
        const int vw = textwidth(buf);
        const int vx = fx + 50;

        if (sel) {
            // 选中：极细金色发光框（不再用深色粗大圆角矩形）
            const int bx1 = vx - 14, by1 = fy - 10;
            const int bx2 = vx + vw + 50, by2 = fy + 52;
            setlinestyle(PS_SOLID, 4);
            setlinecolor(BlendColor(bg, kGold, 0.18));
            roundrect(bx1 - 3, by1 - 3, bx2 + 3, by2 + 3, 11, 11);
            setlinestyle(PS_SOLID, 1);
            setlinecolor(BlendColor(bg, kGold, 0.95));
            roundrect(bx1, by1, bx2, by2, 8, 8);

            // 数值：最亮的金黄，保持高亮
            settextcolor(RGB(255, 240, 170));
            outtextxy(vx, fy, buf);

            // 上下调节箭头
            settextstyle(16, 0, L"微软雅黑");
            settextcolor(BlendColor(bg, RGB(255, 226, 150), 0.90));
            outtextxy(vx + vw + 10, fy + 6, L"▲");
            outtextxy(vx + vw + 10, fy + 28, L"▼");

            // 标签：亮金
            settextstyle(22, 0, L"楷体");
            settextcolor(BlendColor(bg, RGB(255, 226, 150), 0.95));
            outtextxy(fx, fy + 8, labels[i]);
        }
        else {
            // 未选中：纯白字，无底色
            settextcolor(RGB(255, 255, 255));
            outtextxy(vx, fy, buf);

            settextstyle(22, 0, L"楷体");
            settextcolor(BlendColor(bg, RGB(226, 214, 186), 0.92));
            outtextxy(fx, fy + 8, labels[i]);
        }
    }

    // 错误提示（2 秒后自动消失）
    if (!errorMsg.empty() && GetTickCount() - errorTick < 2000) {
        settextstyle(20, 0, L"微软雅黑");
        settextcolor(RGB(255, 130, 120));
        const std::wstring e = L"※ " + errorMsg;
        outtextxy(cx - textwidth(e.c_str()) / 2, py2 + 30, e.c_str());
    }
}

// ---------------------------------------------------------------------------
//  绘制：阶段三 · 悬挂竹简（黑纱 + 竹简册 + 编绳 + 阻尼摇摆 + 光扫）
//
//  构图：屏幕正上方一个支点垂下一根金线，系住竹简顶边中点；整幅绕支点做
//        阻尼摆动。竹简由多条竖向竹片 + 上下两道编绳（带绳结）组成，
//        四柱四列正好每组落在 3 条竹片上。
//
//  为什么用竹简而不是圆角卷轴：竹简全部由矩形和直线构成，
//  「局部坐标 → 绕支点旋转 → 屏幕坐标」后每个多边形只有 4 个顶点，
//  不存在用圆弧拼凑出来的锯齿边，画面干净、缩放任意分辨率都不抖。
// ---------------------------------------------------------------------------
void DivinationScene::DrawResultSlip() const {
    const int W = (SCREEN_W > 0 ? SCREEN_W : WIN_W);
    const int H = (SCREEN_H > 0 ? SCREEN_H : WIN_H);
    const COLORREF bg = NightBg();

    const DWORD tick  = GetTickCount() - phaseTick;
    const double tSec = tick / 1000.0;

    // ---- 时间轴 ----
    constexpr double kLandSec  = 0.55;    // 降落落定
    constexpr double kOpenSec  = 0.26;    // 落定后左右展开
    constexpr double kSweepSec = 0.70;    // 单次金光扫过时长
    constexpr double kCycleSec = 3.00;    // 之后每 3 秒扫一次

    // 0~1 限幅（windows.h 的 min/max 宏会引发 C2589，禁用 std::min/max）
    auto clamp01 = [](double v) { return (v < 0.0) ? 0.0 : ((v > 1.0) ? 1.0 : v); };

    // ---- 黑纱：80% 黑度铺满全屏 ----
    const double pVeil = EaseOutCubic(clamp01(tSec / 0.20));
    const COLORREF veil = BlendColor(bg, RGB(0, 0, 0), 0.80 * pVeil);
    setfillcolor(veil);
    setlinecolor(veil);
    solidrectangle(-60, -60, W + 60, H + 60);

    // ---- 竹简尺寸：62% 屏宽，66% 屏高 ----
    const int    bw    = (int)(W * 0.62);
    const int    bh    = (int)(H * 0.66);
    const int    nStrp = 12;                  // 竹片数（4 的倍数 → 四柱每组 3 片）
    const double stripW = (double)bw / nStrp;
    const double topFinalY = H * 0.18;

    // ---- 入场：0.55 秒从上方降落（起点底边刚好在屏幕顶端外）----
    const double pDrop = EaseOutCubic(clamp01(tSec / kLandSec));
    const double dropY = (topFinalY + bh) * (1.0 - pDrop);
    const double topY  = topFinalY - dropY;

    // ---- 落定后左右展开 ----
    const double pOpen = EaseOutCubic(clamp01((tSec - kLandSec) / kOpenSec));
    const double ex    = (pOpen < 0.02) ? 0.02 : pOpen;
    const double halfW = bw * 0.5 * ex;       // 当前半宽（展开用）
    const double sW    = stripW * ex;         // 当前单片宽

    // ---- 文字淡入 ----
    const double pText = EaseOutCubic(clamp01((tSec - kLandSec - 0.08) / 0.34));

    // ---- 阻尼摇摆：初始 2.2°，周期 2.6s，越摆越慢，最后残留微动 ----
    const double tSw = (tSec > kLandSec) ? (tSec - kLandSec) : 0.0;
    const double ang = 0.0385 * std::sin(tSw * 2.42) * std::exp(-tSw / 2.0)
                     + 0.0045 * std::sin(tSw * 1.55);
    const double ca  = std::cos(ang);
    const double sa  = std::sin(ang);

    // 支点：屏幕正上方
    const double pivX = W / 2.0;
    const double pivY = -60.0;
    const double L    = topY - pivY;

    // 竹简局部坐标（原点在顶边中点，y 向下）→ 屏幕坐标
    auto RX = [&](double x, double y) { return (int)(pivX + x * ca - (L + y) * sa); };
    auto RY = [&](double x, double y) { return (int)(pivY + x * sa + (L + y) * ca); };

    // ---- 竹简本色（必须在下面的 lambda 之前声明，lambda 里要用）----
    const COLORREF woodBase = RGB(233, 205, 158);
    const COLORREF woodEdge = RGB(120, 84, 42);
    // 宣纸纸面色：纯色米白，把竹简纹理彻底隔断，给文字一块干净的底
    const COLORREF paperColor = RGB(247, 240, 222);

    // ---- 旋转矩形三件套（4 顶点，无圆弧，所以没有锯齿）----
    auto QuadPts = [&](double x1, double y1, double x2, double y2, POINT* q) {
        q[0].x = (LONG)RX(x1, y1); q[0].y = (LONG)RY(x1, y1);
        q[1].x = (LONG)RX(x2, y1); q[1].y = (LONG)RY(x2, y1);
        q[2].x = (LONG)RX(x2, y2); q[2].y = (LONG)RY(x2, y2);
        q[3].x = (LONG)RX(x1, y2); q[3].y = (LONG)RY(x1, y2);
    };
    auto FillQuad = [&](double x1, double y1, double x2, double y2, COLORREF c) {
        POINT q[4];
        QuadPts(x1, y1, x2, y2, q);
        setfillcolor(c);
        setlinecolor(c);
        fillpolygon(q, 4);
    };
    auto StrokeQuad = [&](double x1, double y1, double x2, double y2, COLORREF c, int w) {
        POINT q[5];
        QuadPts(x1, y1, x2, y2, q);
        q[4] = q[0];
        setlinecolor(c);
        setlinestyle(PS_SOLID, w);
        polyline(q, 5);
    };
    auto RLine = [&](double x1, double y1, double x2, double y2) {
        POINT p[2] = { { (LONG)RX(x1, y1), (LONG)RY(x1, y1) },
                       { (LONG)RX(x2, y2), (LONG)RY(x2, y2) } };
        polyline(p, 2);
    };
    // 旋转坐标系里画一行居中文字（逐字摆放；只画到第一个 \n 为止）
    auto RText = [&](const std::wstring& s, double x, double y, int size, COLORREF col) {
        if (s.empty() || pText <= 0.02) return;
        const size_t nl = s.find(L'\n');
        const std::wstring line = (nl == std::wstring::npos) ? s : s.substr(0, nl);
        if (line.empty()) return;
        settextstyle(size, 0, L"楷体");
        settextcolor(BlendColor(paperColor, col, pText));
        const int total = textwidth(line.c_str());
        double px = x - ca * total / 2.0;
        double py = y - sa * total / 2.0;
        wchar_t one[2] = { 0, 0 };
        for (wchar_t ch : line) {
            one[0] = ch;
            outtextxy((int)RX(px, py), (int)RY(px, py), one);
            const int cw = textwidth(one);
            px += ca * cw;
            py += sa * cw;
        }
    };

    // ---- 外层柔光：两层沿轮廓放大的矩形，极低透明度 ----
    for (int i = 2; i >= 1; --i) {
        const double    g = 16.0 * i;
        const COLORREF c = BlendColor(RGB(0, 0, 0), RGB(255, 214, 110),
                                      0.060 - 0.024 * (i - 1));
        FillQuad(-halfW - g, -g, halfW + g, bh + g, c);
    }

    // ---- 竹片：逐条填充，左右留缝隙，带木纹与明暗 ----
    const double gap = sW * 0.055;
    for (int i = 0; i < nStrp; ++i) {
        const double x0 = -halfW + i * sW + gap * 0.5;
        const double x1 = -halfW + (i + 1) * sW - gap * 0.5;
        if (x1 - x0 < 1.0) continue;

        // 每条竹片轻微的明暗起伏，避免死板
        const double shade = 0.07 * std::sin(i * 1.7 + 0.6);
        const COLORREF wc = (shade >= 0.0)
                          ? BlendColor(woodBase, RGB(255, 250, 236), shade)
                          : BlendColor(woodBase, RGB(150, 120, 78), -shade);

        // 简身
        FillQuad(x0, 0, x1, bh, wc);
        // 左侧受光 / 右侧背光（模拟竹筒圆弧面）。
        // 强度刻意压得很低：竖条纹会从字后面穿过，太强会严重干扰阅读。
        FillQuad(x0, 0, x0 + (x1 - x0) * 0.12, bh, BlendColor(wc, RGB(255, 252, 240), 0.13));
        FillQuad(x1 - (x1 - x0) * 0.12, 0, x1, bh, BlendColor(wc, RGB(132, 100, 60), 0.13));
        // 木纹：中线一条细线（同样压低，避免糊住文字）
        setlinecolor(BlendColor(wc, RGB(150, 116, 70), 0.20));
        setlinestyle(PS_SOLID, 1);
        RLine((x0 + x1) * 0.5, bh * 0.05, (x0 + x1) * 0.5, bh * 0.95);
        // 竹片描边
        StrokeQuad(x0, 0, x1, bh, woodEdge, 1);
    }

    // ---- 上下编绳（深褐横绳，带绳面高光 + 中心绳结）----
    const double cT0 = bh * 0.068, cT1 = bh * 0.128;
    const double cB0 = bh * 0.872, cB1 = bh * 0.932;
    const double cordL = -halfW - sW * 0.34;
    const double cordR =  halfW + sW * 0.34;

    auto DrawCord = [&](double y0, double y1) {
        FillQuad(cordL, y0, cordR, y1, RGB(92, 58, 30));
        FillQuad(cordL, y0, cordR, y0 + (y1 - y0) * 0.34, RGB(148, 102, 56));
        StrokeQuad(cordL, y0, cordR, y1, RGB(56, 34, 16), 1);

        // 绳结：中心一个深褐菱形
        const double cy = (y0 + y1) * 0.5;
        const double kw = sW * 0.60;
        const double kh = (y1 - y0) * 1.45;
        POINT k[4];
        k[0].x = (LONG)RX(-kw, cy);     k[0].y = (LONG)RY(-kw, cy);
        k[1].x = (LONG)RX(0, cy - kh);  k[1].y = (LONG)RY(0, cy - kh);
        k[2].x = (LONG)RX(kw, cy);      k[2].y = (LONG)RY(kw, cy);
        k[3].x = (LONG)RX(0, cy + kh);  k[3].y = (LONG)RY(0, cy + kh);
        setfillcolor(RGB(114, 74, 38));
        setlinecolor(RGB(58, 36, 18));
        setlinestyle(PS_SOLID, 1);
        fillpolygon(k, 4);
        setfillcolor(RGB(160, 112, 60));
        setlinecolor(RGB(160, 112, 60));
        solidcircle(RX(0, cy), RY(0, cy), (int)(kh * 0.30));
    };
    DrawCord(cT0, cT1);
    DrawCord(cB0, cB1);

    // ---- 宣纸纸面：嵌在竹简中央，给文字一块干净的底 ----
    // 竹片的竖条纹 + 明暗会从字后面穿过，把笔画切得支离破碎，小字根本糊掉。
    // 这里垫一块纯色米白纸面，竹简退化成装饰边框，文字对比度立刻上去。
    const double paperL = -halfW + sW * 0.6;     // 左右各留不到一片竹简做边框
    const double paperR =  halfW - sW * 0.6;
    const double paperT = bh * 0.145;
    const double paperB = bh * 0.855;
    const double paperW = paperR - paperL;
    const double paperH = paperB - paperT;

    FillQuad(paperL, paperT, paperR, paperB, paperColor);
    // 上缘一道淡影 + 下缘一道淡影，做出"嵌进去"的层次
    FillQuad(paperL, paperT, paperR, paperT + bh * 0.014,
             BlendColor(paperColor, RGB(96, 68, 36), 0.18));
    FillQuad(paperL, paperB - bh * 0.010, paperR, paperB,
             BlendColor(paperColor, RGB(96, 68, 36), 0.12));
    // 纸面边框：暖褐细线，明确圈出书写区
    StrokeQuad(paperL, paperT, paperR, paperB, RGB(152, 114, 64), 1);

    // ---- 金线 + 流苏珠（系在竹简顶边中点）----
    const int beadR = (int)(bw * 0.014);
    const int topX = RX(0, 0), topYc = RY(0, 0);
    setlinecolor(BlendColor(RGB(0, 0, 0), RGB(255, 214, 110), 0.90));
    setlinestyle(PS_SOLID, 1);
    line((int)pivX, (int)pivY, topX, topYc - (int)(beadR * 1.7));
    setfillcolor(RGB(198, 44, 40));
    setlinecolor(RGB(255, 196, 128));
    setlinestyle(PS_SOLID, 1);
    solidcircle(topX, topYc - beadR, beadR);
    circle(topX, topYc - beadR, beadR);

    // ---- 金光扫过：入场一次，之后每 3 秒一次（严格限制在竹简范围内）----
    double sweepT = -1.0;
    if (tSec >= kLandSec && tSec <= kLandSec + kSweepSec) {
        sweepT = (tSec - kLandSec) / kSweepSec;
    } else if (tSec > kLandSec + kSweepSec) {
        const double u = std::fmod(tSec - kLandSec, kCycleSec);
        if (u < kSweepSec) sweepT = u / kSweepSec;
    }
    if (sweepT >= 0.0 && pOpen > 0.99) {
        // ---- 高光横线：一条细亮的横线，从纸面上缘扫到下缘 ----
        // 线本身只有几像素高，外面包一层竖向余弦衰减的光晕，
        // 所以是一道柔和的光斑扫过纸面，而不是一条生硬的实心带。
        const double lineH = bh * 0.0055;        // 亮线身高
        const double glowH = bh * 0.055;         // 光晕总高
        const double y0 = paperT + 2.0;
        const double y1 = paperB - 2.0;
        if (y1 > y0) {
            const double ly = y0 + (y1 - y0) * sweepT;

            // 光晕：竖向 14 片，透明度按余弦钟形连续变化（两端 0，中间最亮）
            const int nG = 14;
            for (int i = 0; i < nG; ++i) {
                const double u   = -1.0 + 2.0 * (i + 0.5) / nG;
                const double a   = 0.20 * 0.5 * (1.0 + std::cos(u * kPI));
                if (a < 0.003) continue;
                const double gy0 = ly - glowH * 0.5 + i * (glowH / nG);
                FillQuad(paperL, gy0, paperR, gy0 + glowH / nG + 0.6,
                         BlendColor(paperColor, RGB(255, 240, 190), a));
            }
            // 亮线本体
            FillQuad(paperL, ly - lineH * 0.5, paperR, ly + lineH * 0.5,
                     BlendColor(paperColor, RGB(255, 250, 226), 0.80));
        }
    }

    if (pText <= 0.02) return;

    // =====================================================================
    //  文字层
    //  竹简是浅木色，所以文字一律用「浓墨 / 朱红」——亮金色在浅木底上
    //  对比度几乎为零（根本看不见），已全部替换成深色。
    // =====================================================================
    const COLORREF ink    = RGB(46, 32, 18);    // 浓墨：四柱、签诗
    const COLORREF inkMid = RGB(84, 62, 36);    // 中墨：生辰、变卦、解签
    const COLORREF inkLt  = RGB(106, 80, 46);   // 淡墨：柱标签、动爻、象曰
    const COLORREF cinn   = RGB(158, 30, 26);   // 朱红：标题、本卦、签号
    const double   LX     = paperW * 0.5;       // 纸面半宽（文字布局基准）

    // 纸面内逐项定位（用纸面高度的百分比，自适应任何分辨率）
    auto PY = [&](double f) { return paperT + paperH * f; };

    // ---- 标题 ----
    RText(L"八 字 排 盘", 0, PY(0.030), 30, cinn);

    // ---- 公历生辰 ----
    wchar_t dateBuf[128];
    swprintf(dateBuf, 128, L"公历：%d 年 %d 月 %d 日 %d 时",
             birth.year, birth.month, birth.day, birth.hour);
    RText(dateBuf, 0, PY(0.105), 24, inkMid);

    // ---- 四柱八字（浓墨大字，四列均分纸面）----
    const wchar_t* labels[4] = { L"年柱", L"月柱", L"日柱", L"时柱" };
    const GanZhi*  pillars[4] = { &bazi.yearPillar, &bazi.monthPillar, &bazi.dayPillar, &bazi.hourPillar };
    for (int i = 0; i < 4; ++i) {
        const double bx = -LX + (i + 0.5) * (paperW / 4.0);
        RText(labels[i], bx, PY(0.175), 17, inkLt);
        RText(pillars[i]->Text(), bx, PY(0.210), 54, ink);
    }

    // ---- 分隔 ----
    setlinecolor(BlendColor(paperColor, RGB(104, 72, 36), 0.50 * pText));
    setlinestyle(PS_SOLID, 2);
    RLine(-LX + 70, PY(0.320), LX - 70, PY(0.320));

    // ---- 本卦 / 变卦 ----
    RText(L"本卦：" + guaName, 0, PY(0.350), 40, cinn);
    if (changedIndex >= 0) {
        RText(L"变卦：" + std::wstring(kHexagram[changedIndex].name), 0, PY(0.450), 32, inkMid);
    }

    // ---- 动爻 ----
    wchar_t yaoBuf[128];
    if (movingYao >= 1 && movingYao <= 6) {
        swprintf(yaoBuf, 128, L"动爻：第%s爻动", kYaoNum[movingYao]);
    } else {
        swprintf(yaoBuf, 128, L"动爻：第 %d 爻", movingYao);
    }
    RText(yaoBuf, 0, PY(0.545), 18, inkLt);

    // ---- 分隔 ----
    setlinecolor(BlendColor(paperColor, RGB(104, 72, 36), 0.34 * pText));
    setlinestyle(PS_SOLID, 1);
    RLine(-LX + 70, PY(0.605), LX - 70, PY(0.605));

    // ---- 签号 · 吉凶 ----
    RText(L"第" + fortuneNo + L"签 · " + fortuneLevel, 0, PY(0.640), 38, cinn);

    // ---- 签诗（两句，浓墨）----
    RText(fortunePoem, 0, PY(0.745), 30, ink);
    {
        const size_t nl = fortunePoem.find(L'\n');
        if (nl != std::wstring::npos) {
            RText(fortunePoem.substr(nl + 1), 0, PY(0.815), 30, ink);
        }
    }

    // ---- 解签（两句，中墨）----
    RText(fortuneNote, 0, PY(0.880), 18, inkMid);
    {
        const size_t nl = fortuneNote.find(L'\n');
        if (nl != std::wstring::npos) {
            RText(fortuneNote.substr(nl + 1), 0, PY(0.915), 18, inkMid);
        }
    }

    // ---- 《象》曰（最小字，淡墨）----
    RText(L"《象》曰：" + guaCi, 0, PY(0.950), 16, inkLt);

    // ---- 朱红印章（纸面右下角，跟着竹简一起旋转）----
    const int    sealW = (int)(paperW * 0.072);
    const int    sealH = (int)(sealW * 1.25);
    const double sxc   =  LX - 34 - sealW * 0.5;
    const double syc   =  paperB - 26 - sealH * 0.5;
    POINT sp[4];
    QuadPts(sxc - sealW * 0.5, syc - sealH * 0.5, sxc + sealW * 0.5, syc + sealH * 0.5, sp);
    setfillcolor(cinn);
    setlinecolor(RGB(122, 24, 22));
    setlinestyle(PS_SOLID, 2);
    fillpolygon(sp, 4);
    StrokeQuad(sxc - sealW * 0.5, syc - sealH * 0.5,
               sxc + sealW * 0.5, syc + sealH * 0.5, RGB(122, 24, 22), 2);
    settextstyle((int)(sealW * 0.40), 0, L"楷体");
    settextcolor(BlendColor(woodBase, RGB(252, 244, 228), pText));
    outtextxy((int)RX(sxc - sealW * 0.17, syc - sealH * 0.30), (int)RY(sxc - sealW * 0.17, syc - sealH * 0.30), L"重");
    outtextxy((int)RX(sxc - sealW * 0.17, syc + sealH * 0.08), (int)RY(sxc - sealW * 0.17, syc + sealH * 0.08), L"阳");
}

// ---------------------------------------------------------------------------
//  绘制：标题与提示
// ---------------------------------------------------------------------------
void DivinationScene::DrawTitleAndHints() const {
    const COLORREF bg = NightBg();
    // 绝对坐标，居中于物理屏幕
    const int W = (SCREEN_W > 0 ? SCREEN_W : WIN_W);
    const int H = (SCREEN_H > 0 ? SCREEN_H : WIN_H);

    // 顶部标题
    settextstyle(34, 0, L"楷体");
    DrawGlowText(W / 2 - textwidth(L"推 演 八 卦") / 2, (int)(H * 0.045),
                 L"推 演 八 卦",
                 RGB(255, 232, 170), RGB(255, 215, 0), 0.55);

    if (phase == DivinationPhase::INPUT) {
        settextstyle(19, 0, L"微软雅黑");
        settextcolor(BlendColor(bg, RGB(206, 192, 160), 0.85));
        const wchar_t* hint = L"← → 切换字段　↑ ↓ 调整数值　Enter 起卦";
        outtextxy(W / 2 - textwidth(hint) / 2, H - 66, hint);
    }
    else if (phase == DivinationPhase::ROLLING) {
        settextstyle(26, 0, L"楷体");
        const double p = 0.5 + 0.5 * std::sin(GetTickCount() / 90.0);
        settextcolor(BlendColor(bg, RGB(255, 215, 0), 0.35 + 0.6 * p));
        const wchar_t* tip = L"天 机 推 演 中 ...";
        outtextxy(W / 2 - textwidth(tip) / 2, H - 80, tip);
    }
    else {
        settextstyle(22, 0, L"微软雅黑");
        settextcolor(BlendColor(bg, RGB(210, 196, 164), 0.9));
        const wchar_t* hint = L"按 [Enter] 重新起卦，按 [ESC] 返回主菜单";
        outtextxy(W / 2 - textwidth(hint) / 2, H - 44, hint);
    }
}

void DivinationScene::Render() {
    // =====================================================================
    //  坐标管理：八卦阵 / 黑纱 / 卷轴 / 标题 全部使用物理屏幕绝对坐标；
    //  只有输入面板仍用 1000x700 设计区坐标（它本身居中，与屏幕中心重合）。
    //  注意：结果阶段已改为"悬挂摇摆"，不再有落地震屏。
    // =====================================================================

    // 1) 八卦阵：绝对坐标，居中于物理屏幕
    setorigin(0, 0);
    DrawBaguaArray();

    // 2) 阶段一：输入面板（设计区坐标，居中于屏幕）
    if (phase == DivinationPhase::INPUT) {
        setorigin(UI_OFFSET_X(), UI_OFFSET_Y());
        DrawInputPanel();
    }
    // 3) 阶段三：黑纱 + 悬浮琉璃卷轴（绝对坐标，绕屏幕上方支点摇摆）
    else if (phase == DivinationPhase::RESULT) {
        setorigin(0, 0);
        DrawResultSlip();
    }

    // 4) 标题与提示：绝对坐标
    setorigin(0, 0);
    DrawTitleAndHints();
}
