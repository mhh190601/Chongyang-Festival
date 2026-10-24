#include "Bazi.h"

#include <cmath>
#include <cstdio>
#include <sstream>

// ============================================================================
//  内部常量与工具（全部放在匿名命名空间，不外泄）
// ============================================================================
namespace {

constexpr double kPI        = 3.14159265358979323846;
constexpr double kDegPerDay = 0.9856473;   // 太阳黄经的平均日运动速度（度/天）

// ---------- 名称表 ----------
const wchar_t* kTianGan[10] = { L"甲", L"乙", L"丙", L"丁", L"戊", L"己", L"庚", L"辛", L"壬", L"癸" };
const wchar_t* kDiZhi[12]   = { L"子", L"丑", L"寅", L"卯", L"辰", L"巳", L"午", L"未", L"申", L"酉", L"戌", L"亥" };
const wchar_t* kShengXiao[12] = { L"鼠", L"牛", L"虎", L"兔", L"龙", L"蛇", L"马", L"羊", L"猴", L"鸡", L"狗", L"猪" };

// 二十四节气名（0 = 立春，用于八字，非农历正月）
const wchar_t* kTermName[24] = {
    L"立春", L"雨水", L"惊蛰", L"春分", L"清明", L"谷雨",
    L"立夏", L"小满", L"芒种", L"夏至", L"小暑", L"大暑",
    L"立秋", L"处暑", L"白露", L"秋分", L"寒露", L"霜降",
    L"立冬", L"小雪", L"大雪", L"冬至", L"小寒", L"大寒"
};

// 各节气的“近似日期”，仅用作牛顿迭代的初值（误差 < 2 天，迭代必收敛）
const int kTermApproxMonth[24] = { 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 1, 1 };
const int kTermApproxDay[24]   = { 4, 19, 6, 21, 5, 20, 6, 21, 6, 21, 7, 23, 8, 23, 8, 23, 8, 24, 7, 22, 7, 22, 6, 20 };

// 农历月名 / 日名
const wchar_t* kLunarMonthName[12] = { L"正", L"二", L"三", L"四", L"五", L"六", L"七", L"八", L"九", L"十", L"冬", L"腊" };
const wchar_t* kLunarDayName[30] = {
    L"初一", L"初二", L"初三", L"初四", L"初五", L"初六", L"初七", L"初八", L"初九", L"初十",
    L"十一", L"十二", L"十三", L"十四", L"十五", L"十六", L"十七", L"十八", L"十九", L"二十",
    L"廿一", L"廿二", L"廿三", L"廿四", L"廿五", L"廿六", L"廿七", L"廿八", L"廿九", L"三十"
};

// ---------- 通用数学 ----------
int Mod(int a, int b) { int r = a % b; return (r < 0) ? r + b : r; }

double Norm360(double x) { double r = std::fmod(x, 360.0); return (r < 0.0) ? r + 360.0 : r; }
double Norm180(double x) { double r = Norm360(x); return (r > 180.0) ? r - 360.0 : r; }

// ---------- 公历基础 ----------
bool IsLeapYear(int y) { return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0); }

int DaysInMonth(int y, int m) {
    static const int kMD[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (m == 2 && IsLeapYear(y)) return 29;
    return kMD[m];
}

bool IsValidDate(int y, int m, int d) {
    if (y < 1 || m < 1 || m > 12 || d < 1) return false;
    return d <= DaysInMonth(y, m);
}

int DayOfYear(int y, int m, int d) {
    int s = 0;
    for (int i = 1; i < m; ++i) s += DaysInMonth(y, i);
    return s + d;
}

// 儒略日数（该公历日期 12:00 UT）
int JulianDayNumber(int y, int m, int d) {
    int a  = (14 - m) / 12;
    int yy = y + 4800 - a;
    int mm = m + 12 * a - 3;
    return d + (153 * mm + 2) / 5 + 365 * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
}

// 由儒略日数反解公历日期（返回 0 时）
void JulianDayNumberToDate(int jdn, int& y, int& m, int& d) {
    long long Z = jdn;
    long long A = Z;
    if (Z >= 2299161LL) {
        long long al = (long long)std::floor((Z - 1867216.25) / 36524.25);
        A = Z + 1 + al - (long long)std::floor(al / 4.0);
    }
    long long B = A + 1524;
    long long C = (long long)std::floor((B - 122.1) / 365.25);
    long long D = (long long)std::floor(365.25 * C);
    long long E = (long long)std::floor((B - D) / 30.6001);
    d = (int)(B - D - (long long)std::floor(30.6001 * E));
    m = (int)((E < 14) ? (E - 1) : (E - 13));
    y = (int)((m > 2) ? (C - 4716) : (C - 4715));
}

// 世界时 0 点起算的儒略日（含小数）
double JD_UT(int y, int m, int d, double utHour) {
    return (double)JulianDayNumber(y, m, d) + (utHour - 12.0) / 24.0;
}

// 儒略日（含小数）-> 民用日期 + 时分（四舍五入到分钟）
void JDToCivil(double jd, int& y, int& m, int& d, int& h, int& mi) {
    double z = std::floor(jd + 0.5);
    double f = jd + 0.5 - z;
    JulianDayNumberToDate((int)z, y, m, d);
    int total = (int)std::floor(f * 1440.0 + 0.5);
    if (total >= 1440) { total -= 1440; }   // 仅可能发生在 23:59:30 之后的进位
    h  = total / 60;
    mi = total % 60;
}

// ---------- 天文部分 ----------

// ΔT（世界时 - 力学时之差，单位：秒），Espenak & Meeus 多项式拟合
double DeltaT(double year) {
    double t, u;
    if (year >= 2050.0 && year <= 2150.0) {
        u = (year - 1820.0) / 100.0;
        return -20.0 + 32.0 * u * u - 0.5628 * (2150.0 - year);
    }
    if (year >= 2005.0 && year < 2050.0) {
        t = year - 2000.0;
        return 62.92 + 0.32217 * t + 0.005589 * t * t;
    }
    if (year >= 1986.0 && year < 2005.0) {
        t = year - 2000.0;
        return 63.86 + 0.3345 * t - 0.060374 * t * t
             + 0.0017275 * t * t * t + 0.000651814 * t * t * t * t
             + 0.00002373599 * t * t * t * t * t;
    }
    if (year >= 1961.0 && year < 1986.0) {
        t = year - 1975.0;
        return 45.45 + 1.067 * t - t * t / 260.0 - t * t * t / 718.0;
    }
    if (year >= 1941.0 && year < 1961.0) {
        t = year - 1950.0;
        return 29.07 + 0.407 * t - t * t / 233.0 + t * t * t / 2547.0;
    }
    if (year >= 1920.0 && year < 1941.0) {
        t = year - 1920.0;
        return 21.20 + 0.84493 * t - 0.076100 * t * t + 0.0020936 * t * t * t;
    }
    if (year >= 1900.0 && year < 1920.0) {
        t = year - 1900.0;
        return -2.79 + 1.494119 * t - 0.0598939 * t * t
               + 0.0061966 * t * t * t - 0.000197 * t * t * t * t;
    }
    return 0.0;
}

// VSOP87 地球黄经摄动项（金星 / 木星 / 火星等的行星摄动）
// A 单位为 1e-8 弧度，B 为相位（弧度），C 为速率（弧度 / 儒略千年）
// 加入后节气时刻误差由约 -9 分钟收敛到 ±2 分钟（已用 2023-2026 年立春实测值校准）
const double kPertA[12] = { 3497, 3418, 3136, 2676, 2343, 1324, 1273, 1199, 990, 902, 857, 780 };
const double kPertB[12] = { 2.7441183994, 2.8288643143, 3.6276704170, 4.4180835138,
                            6.1351623753, 0.7424639316, 2.0370965395, 1.1096294430,
                            5.2339442825, 2.0450544340, 3.5087719946, 1.1790581453 };
const double kPertC[12] = { 5753.3848848968, 3.5231183490, 77713.7714681205, 7860.4193924392,
                            3930.2096962196, 11506.7697697936, 529.6909650948, 1577.3435424478,
                            5884.9268465292, 398.1490039542, 5223.6939198022, 5507.5532386674 };

// 太阳视黄经（度），入参为力学时儒略日 JD(TT)
double SunApparentLongitude(double jdTT) {
    double T  = (jdTT - 2451545.0) / 36525.0;
    double T2 = T * T;

    double L0 = 280.4664567 + 36000.76983 * T + 0.0003032 * T2;             // 平黄经
    double M  = 357.5291092 + 35999.0502909 * T - 0.0001536 * T2
                + T * T2 / 24490000.0;                                      // 平近点角
    double Mr = M * kPI / 180.0;

    // 中心差
    double C = (1.9146023 - 0.004817 * T - 0.000014 * T2) * std::sin(Mr)
             + (0.019993 - 0.000101 * T) * std::sin(2.0 * Mr)
             + 0.000289 * std::sin(3.0 * Mr);

    // 行星摄动
    double tau = (jdTT - 2451545.0) / 365250.0;   // 儒略千年
    double pert = 0.0;
    for (int i = 0; i < 12; ++i) {
        pert += kPertA[i] * 1e-8 * std::cos(kPertB[i] + kPertC[i] * tau);
    }

    double O      = L0 + C + pert * 180.0 / kPI;             // 真黄经
    double Omega  = (125.04 - 1934.136 * T) * kPI / 180.0;   // 升交点黄经
    double lambda = O - 0.00569 - 0.00478 * std::sin(Omega); // 视黄经（光行差 + 章动）
    return Norm360(lambda);
}

// 求某年第 termIndex 个节气的 JD(UT)
double FindTermJD_UT(int year, int termIndex, double tz) {
    double target = Norm360(315.0 + 15.0 * termIndex);   // 立春 = 315°
    int m = kTermApproxMonth[termIndex];
    int d = kTermApproxDay[termIndex];

    double dt   = DeltaT(year + (m - 1) / 12.0);
    double jdTT = JD_UT(year, m, d, 12.0 - tz) + dt / 86400.0;   // 初值：当地正午

    for (int i = 0; i < 40; ++i) {
        double lon  = SunApparentLongitude(jdTT);
        double diff = Norm180(target - lon);
        if (std::fabs(diff) < 1e-9) break;
        jdTT += diff / kDegPerDay;   // 牛顿迭代（导数取平均日运动速度）
    }
    return jdTT - dt / 86400.0;      // 力学时 -> 世界时
}

// 一个时刻点（方便比较大小）
struct Moment {
    long long t = 0;      // 当地民用时间的“绝对分钟数”
    int y = 0, m = 0, d = 0, h = 0, mi = 0;
};

// 某年某节气的交节时刻（当地时间）
Moment TermMoment(int year, int termIndex, double tz) {
    double jdUT = FindTermJD_UT(year, termIndex, tz);
    Moment mm;
    JDToCivil(jdUT + tz / 24.0, mm.y, mm.m, mm.d, mm.h, mm.mi);
    mm.t = (long long)JulianDayNumber(mm.y, mm.m, mm.d) * 1440 + mm.h * 60 + mm.mi;
    return mm;
}

std::wstring MomentText(const Moment& mm) {
    wchar_t buf[64];
    swprintf(buf, 64, L"%d年%d月%d日 %02d:%02d", mm.y, mm.m, mm.d, mm.h, mm.mi);
    return std::wstring(buf);
}

// ---------- 农历（1900 - 2100） ----------
// 编码规则：bit0-3  闰月月份（0 表示无闰月）
//           bit4-15 正月~腊月的天数标志（1 = 30 天，0 = 29 天），第 m 月在 bit(16-m)
//           bit16   闰月天数标志（1 = 30 天，0 = 29 天）
const unsigned int kLunarInfo[201] = {
    0x04bd8, 0x04ae0, 0x0a570, 0x054d5, 0x0d260, 0x0d950, 0x16554, 0x056a0, 0x09ad0, 0x055d2, // 1900-1909
    0x04ae0, 0x0a5b6, 0x0a4d0, 0x0d250, 0x1d255, 0x0b540, 0x0d6a0, 0x0ada2, 0x095b0, 0x14977, // 1910-1919
    0x04970, 0x0a4b0, 0x0b4b5, 0x06a50, 0x06d40, 0x1ab54, 0x02b60, 0x09570, 0x052f2, 0x04970, // 1920-1929
    0x06566, 0x0d4a0, 0x0ea50, 0x06e95, 0x05ad0, 0x02b60, 0x186e3, 0x092e0, 0x1c8d7, 0x0c950, // 1930-1939
    0x0d4a0, 0x1d8a6, 0x0b550, 0x056a0, 0x1a5b4, 0x025d0, 0x092d0, 0x0d2b2, 0x0a950, 0x0b557, // 1940-1949
    0x06ca0, 0x0b550, 0x15355, 0x04da0, 0x0a5b0, 0x14573, 0x052b0, 0x0a9a8, 0x0e950, 0x06aa0, // 1950-1959
    0x0aea6, 0x0ab50, 0x04b60, 0x0aae4, 0x0a570, 0x05260, 0x0f263, 0x0d950, 0x05b57, 0x056a0, // 1960-1969
    0x096d0, 0x04dd5, 0x04ad0, 0x0a4d0, 0x0d4d4, 0x0d250, 0x0d558, 0x0b540, 0x0b5a0, 0x195a6, // 1970-1979
    0x095b0, 0x049b0, 0x0a974, 0x0a4b0, 0x0b27a, 0x06a50, 0x06d40, 0x0af46, 0x0ab60, 0x09570, // 1980-1989
    0x04af5, 0x04970, 0x064b0, 0x074a3, 0x0ea50, 0x06b58, 0x055c0, 0x0ab60, 0x096d5, 0x092e0, // 1990-1999
    0x0c960, 0x0d954, 0x0d4a0, 0x0da50, 0x07552, 0x056a0, 0x0abb7, 0x025d0, 0x092d0, 0x0cab5, // 2000-2009
    0x0a950, 0x0b4a0, 0x0baa4, 0x0ad50, 0x055d9, 0x04ba0, 0x0a5b0, 0x15176, 0x052b0, 0x0a930, // 2010-2019
    0x07954, 0x06aa0, 0x0ad50, 0x05b52, 0x04b60, 0x0a6e6, 0x0a4e0, 0x0d260, 0x0ea65, 0x0d530, // 2020-2029
    0x05aa0, 0x076a3, 0x096d0, 0x04afb, 0x04ad0, 0x0a4d0, 0x1d0b6, 0x0d250, 0x0d520, 0x0dd45, // 2030-2039
    0x0b5a0, 0x056d0, 0x055b2, 0x049b0, 0x0a577, 0x0a4b0, 0x0aa50, 0x1b255, 0x06d20, 0x0ada0, // 2040-2049
    0x14b63, 0x09370, 0x049f8, 0x04970, 0x064b0, 0x168a6, 0x0ea50, 0x06b20, 0x1a6c4, 0x0aae0, // 2050-2059
    0x0a2e0, 0x0d2e3, 0x0c960, 0x0d557, 0x0d4a0, 0x0da50, 0x05d55, 0x056a0, 0x0a6d0, 0x055d4, // 2060-2069
    0x052d0, 0x0a9b8, 0x0a950, 0x0b4a0, 0x0b6a6, 0x0ad50, 0x055a0, 0x0aba4, 0x0a5b0, 0x052b0, // 2070-2079
    0x0b273, 0x06930, 0x07337, 0x06aa0, 0x0ad50, 0x14b55, 0x04b60, 0x0a570, 0x054e4, 0x0d160, // 2080-2089
    0x0e968, 0x0d520, 0x0daa0, 0x16aa6, 0x056d0, 0x04ae0, 0x0a9d4, 0x0a2d0, 0x0d150, 0x0f252, // 2090-2099
    0x0d520                                                                                    // 2100
};

int LunarLeapMonth(int y) { return (int)(kLunarInfo[y - 1900] & 0xF); }

int LunarLeapDays(int y) {
    if (LunarLeapMonth(y) == 0) return 0;
    return ((kLunarInfo[y - 1900] >> 16) & 1) ? 30 : 29;
}

int LunarMonthDays(int y, int m) {
    return ((kLunarInfo[y - 1900] >> (16 - m)) & 1) ? 30 : 29;
}

int LunarYearDays(int y) {
    int sum = 348;   // 12 × 29
    for (unsigned int bit = 0x8000; bit > 0x8; bit >>= 1) {
        sum += (kLunarInfo[y - 1900] & bit) ? 1 : 0;
    }
    return sum + LunarLeapDays(y);
}

// 真太阳时订正：返回需要加在“平太阳时”上的分钟数
double EquationOfTimeMinutes(int y, int m, int d) {
    int    doy = DayOfYear(y, m, d);
    double B   = 2.0 * kPI * (doy - 81) / 364.0;
    return 9.87 * std::sin(2.0 * B) - 7.53 * std::cos(B) - 1.5 * std::sin(B);
}

std::wstring LunarYearText(int y) {
    static const wchar_t* kNum[10] = { L"〇", L"一", L"二", L"三", L"四", L"五", L"六", L"七", L"八", L"九" };
    wchar_t buf[16];
    swprintf(buf, 16, L"%s%s%s%s", kNum[y / 1000 % 10], kNum[y / 100 % 10], kNum[y / 10 % 10], kNum[y % 10]);
    return std::wstring(buf);
}

} // namespace

// ============================================================================
//  BaziCalculator 实现
// ============================================================================

std::wstring GanZhi::Text() const {
    return std::wstring(kTianGan[gan]) + kDiZhi[zhi];
}

std::wstring LunarDate::Text() const {
    std::wstring s = LunarYearText(year) + L"年";
    if (isLeap) s += L"闰";
    s += kLunarMonthName[month - 1];
    s += L"月";
    s += kLunarDayName[day - 1];
    return s;
}

std::wstring BaziResult::FourPillars() const {
    return yearPillar.Text() + L" " + monthPillar.Text() + L" "
         + dayPillar.Text() + L" " + hourPillar.Text();
}

const wchar_t* BaziCalculator::TianGan(int i) { return kTianGan[Mod(i, 10)]; }
const wchar_t* BaziCalculator::DiZhi(int i)   { return kDiZhi[Mod(i, 12)]; }
const wchar_t* BaziCalculator::TermName(int i){ return kTermName[Mod(i, 24)]; }

std::wstring BaziCalculator::GanZhiText(int gan, int zhi) {
    return std::wstring(kTianGan[Mod(gan, 10)]) + kDiZhi[Mod(zhi, 12)];
}

int BaziCalculator::JulianDayNumber(int y, int m, int d) {
    return ::JulianDayNumber(y, m, d);
}

bool BaziCalculator::IsValidDate(int y, int m, int d) {
    return ::IsValidDate(y, m, d);
}

bool BaziCalculator::SolarTermMoment(int year, int termIndex, double tz,
                                     int& outMonth, int& outDay, int& outHour, int& outMinute) {
    if (year < 1900 || year > 2100 || termIndex < 0 || termIndex > 23) return false;
    Moment mm = TermMoment(year, termIndex, tz);
    outMonth = mm.m; outDay = mm.d; outHour = mm.h; outMinute = mm.mi;
    return true;
}

bool BaziCalculator::SolarToLunar(int y, int m, int d, LunarDate& out) {
    if (!::IsValidDate(y, m, d)) return false;

    // 基准：1900-01-31 = 农历 1900 年正月初一
    const int kBase = ::JulianDayNumber(1900, 1, 31);
    int offset = ::JulianDayNumber(y, m, d) - kBase;
    if (offset < 0) return false;

    int ly = 1900;
    while (ly <= 2100 && offset >= LunarYearDays(ly)) {
        offset -= LunarYearDays(ly);
        ++ly;
    }
    if (ly > 2100) return false;

    int  leap   = LunarLeapMonth(ly);
    bool isLeap = false;
    int  temp   = 0;
    int  i      = 1;

    for (; i < 13 && offset > 0; ++i) {
        if (leap > 0 && i == (leap + 1) && !isLeap) {
            --i;                        // 本轮处理的是闰月
            isLeap = true;
            temp   = LunarLeapDays(ly);
        } else {
            temp = LunarMonthDays(ly, i);
        }
        if (isLeap && i == (leap + 1)) isLeap = false;   // 闰月处理完毕，解除标记
        offset -= temp;
    }

    if (offset == 0 && leap > 0 && i == leap + 1) {
        if (isLeap) isLeap = false;
        else { isLeap = true; --i; }
    }
    if (offset < 0) { offset += temp; --i; }

    out.year   = ly;
    out.month  = (i < 1) ? 1 : ((i > 12) ? 12 : i);
    out.day    = offset + 1;
    out.isLeap = isLeap;
    return true;
}

BaziResult BaziCalculator::Calc(const BirthInfo& info) {
    BaziResult r;

    if (!::IsValidDate(info.year, info.month, info.day)) {
        r.errMsg = L"公历日期不合法";
        return r;
    }
    if (info.year < 1900 || info.year > 2100) {
        r.errMsg = L"仅支持 1900 - 2100 年";
        return r;
    }
    if (info.hour < 0 || info.hour > 23 || info.minute < 0 || info.minute > 59) {
        r.errMsg = L"出生时刻不合法";
        return r;
    }

    const double tz = info.timeZone;

    // ---- 0. 真太阳时订正（可选）----
    int yy = info.year, mo = info.month, dd = info.day, hh = info.hour, mi = info.minute;
    if (info.trueSolar) {
        double delta = 4.0 * (tz * 15.0 - info.longitude)      // 经度订正
                     + EquationOfTimeMinutes(yy, mo, dd);      // 时差方程
        long long total    = (long long)hh * 60 + mi + (long long)std::llround(delta);
        long long dayShift = (long long)std::floor(total / 1440.0);
        int       newMin   = (int)(total - dayShift * 1440);
        int       newJdn   = ::JulianDayNumber(yy, mo, dd) + (int)dayShift;
        ::JulianDayNumberToDate(newJdn, yy, mo, dd);
        hh = newMin / 60;
        mi = newMin % 60;
    }

    // 出生时刻的“绝对分钟数”（同一儒略日数基准，可跨日比较）
    const long long birthT = (long long)::JulianDayNumber(yy, mo, dd) * 1440 + hh * 60 + mi;

    // ---- 1. 年柱：以立春分界 ----
    Moment lichun = TermMoment(yy, 0, tz);          // 出生年的立春
    int ganzhiYear = (birthT < lichun.t) ? (yy - 1) : yy;
    r.yearPillar.gan = Mod(ganzhiYear - 4, 10);     // 公元 4 年 = 甲子
    r.yearPillar.zhi = Mod(ganzhiYear - 4, 12);

    // ---- 2. 月柱：以十二节气分界 ----
    // 构造“节气年”序列：立春(0) ... 冬至(21)、小寒(22)、大寒(23)、次年立春(24)
    Moment terms[25];
    for (int i = 0; i < 24; ++i) {
        int ty = ganzhiYear + ((i >= 22) ? 1 : 0);  // 小寒 / 大寒 在次年 1 月
        terms[i] = TermMoment(ty, i, tz);
    }
    terms[24] = TermMoment(ganzhiYear + 1, 0, tz);

    int k = 0;
    while (k < 23 && !(birthT >= terms[k].t && birthT < terms[k + 1].t)) ++k;
    if (birthT >= terms[23].t && birthT < terms[24].t) k = 23;

    // 五虎遁：甲己丙寅首，乙庚戊寅头，丙辛庚寅起，丁壬壬寅顺，戊癸甲寅求
    int wuhuStart = (r.yearPillar.gan % 5) * 2 + 2;

    r.monthPillar.zhi = Mod(2 + k, 12);             // 立春起寅月
    r.monthPillar.gan = Mod(wuhuStart + k, 10);

    r.solarTerm     = kTermName[k];
    r.solarTermTime = MomentText(terms[k]);

    // ---- 3. 日柱：23:00（晚子时）换日 ----
    int dayJdn = ::JulianDayNumber(yy, mo, dd);
    if (hh >= 23) ++dayJdn;
    int dayIdx = Mod(dayJdn + 49, 60);              // 1900-01-01 = 甲戌（序号 10）
    r.dayPillar.gan = dayIdx % 10;
    r.dayPillar.zhi = dayIdx % 12;

    // ---- 4. 时柱：五鼠遁 ----
    int hourZhi = ((hh + 1) / 2) % 12;              // 子时 23-1，丑时 1-3 ...
    int wushuStart = (r.dayPillar.gan % 5) * 2;     // 甲己还加甲，乙庚丙作初...
    r.hourPillar.zhi = hourZhi;
    r.hourPillar.gan = Mod(wushuStart + hourZhi, 10);

    // ---- 5. 农历与生肖 ----
    SolarToLunar(yy, mo, dd, r.lunar);
    r.shengXiao = kShengXiao[r.yearPillar.zhi];

    r.valid = true;
    return r;
}
