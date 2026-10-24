#pragma once
#include <string>

// ============================================================================
//  Bazi.h —— 生辰八字排盘（公历 -> 节气 / 农历 / 四柱干支）
//
//  算法要点：
//   1) 节气：以 Meeus《Astronomical Algorithms》的低精度太阳视黄经公式
//            + 牛顿迭代求解“太阳视黄经 = 315° + 15°×n”的时刻，并做 ΔT 订正。
//            （精度约 ±2 分钟，足够排八字用）
//   2) 年柱：以【立春】为分界
//   3) 月柱：以【十二节气】为分界（立春起寅月），天干用“五虎遁”
//   4) 日柱：以儒略日数推算，23:00（晚子时）换日
//   5) 时柱：地支按时辰（子 23-1 ...），天干用“五鼠遁”
//   6) 农历：使用 1900-2100 年农历编码表 + 差值推算
// ============================================================================

// 出生信息（公历 / 阳历）
struct BirthInfo {
    int    year   = 1990;
    int    month  = 1;
    int    day    = 1;
    int    hour   = 12;     // 24 小时制
    int    minute = 0;
    double timeZone  = 8.0;   // 时区，北京时间 = +8
    bool   trueSolar = false; // 是否换算为真太阳时（需配合 longitude）
    double longitude = 116.4; // 出生地经度（东经为正），仅在 trueSolar 时生效
};

// 干支组合
struct GanZhi {
    int gan = 0;   // 0..9   甲..癸
    int zhi = 0;   // 0..11  子..亥
    std::wstring Text() const;   // 例如 "甲子"
};

// 农历日期
struct LunarDate {
    int  year   = 0;
    int  month  = 1;    // 1..12
    int  day    = 1;    // 1..30
    bool isLeap = false;
    std::wstring Text() const;  // 例如 "二〇二四年九月初九"
};

// 排盘结果
struct BaziResult {
    bool valid = false;
    std::wstring errMsg;

    GanZhi yearPillar;   // 年柱
    GanZhi monthPillar;  // 月柱
    GanZhi dayPillar;    // 日柱
    GanZhi hourPillar;   // 时柱

    std::wstring solarTerm;     // 出生时所属节气，如 "寒露"
    std::wstring solarTermTime; // 该节气的交节时刻（当地），如 "2024年2月4日 16:27"

    LunarDate   lunar;          // 农历日期
    std::wstring shengXiao;     // 生肖

    std::wstring FourPillars() const;  // "甲辰 丙寅 甲子 壬申"
};

class BaziCalculator {
public:
    // 主入口：传入公历出生时间，返回四柱
    static BaziResult Calc(const BirthInfo& info);

    // ---------- 可复用的工具接口 ----------
    static const wchar_t* TianGan(int i);   // 0..9
    static const wchar_t* DiZhi(int i);     // 0..11
    static const wchar_t* TermName(int i);  // 0..23（0 = 立春）

    static std::wstring GanZhiText(int gan, int zhi);

    // 儒略日数（该公历日期 12:00 UT 的日数）
    static int JulianDayNumber(int y, int m, int d);

    // 求某年某节气的交节时刻（当地时间）。termIndex: 0=立春 ... 23=大寒
    static bool SolarTermMoment(int year, int termIndex, double tz,
                                int& outMonth, int& outDay, int& outHour, int& outMinute);

    // 公历 -> 农历（1900-01-31 ~ 2100-12-31）
    static bool SolarToLunar(int y, int m, int d, LunarDate& out);

    // 日期合法性检查
    static bool IsValidDate(int y, int m, int d);
};
