#include "dots.h"

#include <string>
#include <iostream>

// braille pattern dots - 12345678
// https://www.compart.com/en/unicode/block/U+2800

static int dots[] = {
    1<<1, 1<<4,
    1<<2, 1<<5,
    1<<3, 1<<6,
    1<<7, 1<<8
};

typedef struct {
    const wchar_t *chars;
    const char *dotCode;
} dots_number_map_t;

static const dots_number_map_t dots_map[] = {
{ L"\u2800", "" },
{ L"\u2801", "1" },
{ L"\u2802", "2" },
{ L"\u2803", "12" },
{ L"\u2804", "3" },
{ L"\u2805", "13" },
{ L"\u2806", "23" },
{ L"\u2807", "123" },
{ L"\u2808", "4" },
{ L"\u2809", "14" },
{ L"\u280A", "24" },
{ L"\u280B", "124" },
{ L"\u280C", "34" },
{ L"\u280D", "134" },
{ L"\u280E", "234" },
{ L"\u280F", "1234" },
{ L"\u2810", "5" },
{ L"\u2811", "15" },
{ L"\u2812", "25" },
{ L"\u2813", "125" },
{ L"\u2814", "35" },
{ L"\u2815", "135" },
{ L"\u2816", "235" },
{ L"\u2817", "1235" },
{ L"\u2818", "45" },
{ L"\u2819", "145" },
{ L"\u281A", "245" },
{ L"\u281B", "1245" },
{ L"\u281C", "345" },
{ L"\u281D", "1345" },
{ L"\u281E", "2345" },
{ L"\u281F", "12345" },
{ L"\u2820", "6" },
{ L"\u2821", "16" },
{ L"\u2822", "26" },
{ L"\u2823", "126" },
{ L"\u2824", "36" },
{ L"\u2825", "136" },
{ L"\u2826", "236" },
{ L"\u2827", "1236" },
{ L"\u2828", "46" },
{ L"\u2829", "146" },
{ L"\u282A", "246" },
{ L"\u282B", "1246" },
{ L"\u282C", "346" },
{ L"\u282D", "1346" },
{ L"\u282E", "2346" },
{ L"\u282F", "12346" },
{ L"\u2830", "56" },
{ L"\u2831", "156" },
{ L"\u2832", "256" },
{ L"\u2833", "1256" },
{ L"\u2834", "356" },
{ L"\u2835", "1356" },
{ L"\u2836", "2356" },
{ L"\u2837", "12356" },
{ L"\u2838", "456" },
{ L"\u2839", "1456" },
{ L"\u283A", "2456" },
{ L"\u283B", "12456" },
{ L"\u283C", "3456" },
{ L"\u283D", "13456" },
{ L"\u283E", "23456" },
{ L"\u283F", "123456" },
{ L"\u2840", "7" },
{ L"\u2841", "17" },
{ L"\u2842", "27" },
{ L"\u2843", "127" },
{ L"\u2844", "37" },
{ L"\u2845", "137" },
{ L"\u2846", "237" },
{ L"\u2847", "1237" },
{ L"\u2848", "47" },
{ L"\u2849", "147" },
{ L"\u284A", "247" },
{ L"\u284B", "1247" },
{ L"\u284C", "347" },
{ L"\u284D", "1347" },
{ L"\u284E", "2347" },
{ L"\u284F", "12347" },
{ L"\u2850", "57" },
{ L"\u2851", "157" },
{ L"\u2852", "257" },
{ L"\u2853", "1257" },
{ L"\u2854", "357" },
{ L"\u2855", "1357" },
{ L"\u2856", "2357" },
{ L"\u2857", "12357" },
{ L"\u2858", "457" },
{ L"\u2859", "1457" },
{ L"\u285A", "2457" },
{ L"\u285B", "12457" },
{ L"\u285C", "3457" },
{ L"\u285D", "13457" },
{ L"\u285E", "23457" },
{ L"\u285F", "123457" },
{ L"\u2860", "67" },
{ L"\u2861", "167" },
{ L"\u2862", "267" },
{ L"\u2863", "1267" },
{ L"\u2864", "367" },
{ L"\u2865", "1367" },
{ L"\u2866", "2367" },
{ L"\u2867", "12367" },
{ L"\u2868", "467" },
{ L"\u2869", "1467" },
{ L"\u286A", "2467" },
{ L"\u286B", "12467" },
{ L"\u286C", "3467" },
{ L"\u286D", "13467" },
{ L"\u286E", "23467" },
{ L"\u286F", "123467" },
{ L"\u2870", "567" },
{ L"\u2871", "1567" },
{ L"\u2872", "2567" },
{ L"\u2873", "12567" },
{ L"\u2874", "3567" },
{ L"\u2875", "13567" },
{ L"\u2876", "23567" },
{ L"\u2877", "123567" },
{ L"\u2878", "4567" },
{ L"\u2879", "14567" },
{ L"\u287A", "24567" },
{ L"\u287B", "124567" },
{ L"\u287C", "34567" },
{ L"\u287D", "134567" },
{ L"\u287E", "234567" },
{ L"\u287F", "1234567" },
{ L"\u2880", "8" },
{ L"\u2881", "18" },
{ L"\u2882", "28" },
{ L"\u2883", "128" },
{ L"\u2884", "38" },
{ L"\u2885", "138" },
{ L"\u2886", "238" },
{ L"\u2887", "1238" },
{ L"\u2888", "48" },
{ L"\u2889", "148" },
{ L"\u288A", "248" },
{ L"\u288B", "1248" },
{ L"\u288C", "348" },
{ L"\u288D", "1348" },
{ L"\u288E", "2348" },
{ L"\u288F", "12348" },
{ L"\u2890", "58" },
{ L"\u2891", "158" },
{ L"\u2892", "258" },
{ L"\u2893", "1258" },
{ L"\u2894", "358" },
{ L"\u2895", "1358" },
{ L"\u2896", "2358" },
{ L"\u2897", "12358" },
{ L"\u2898", "458" },
{ L"\u2899", "1458" },
{ L"\u289A", "2458" },
{ L"\u289B", "12458" },
{ L"\u289C", "3458" },
{ L"\u289D", "13458" },
{ L"\u289E", "23458" },
{ L"\u289F", "123458" },
{ L"\u28A0", "68" },
{ L"\u28A1", "168" },
{ L"\u28A2", "268" },
{ L"\u28A3", "1268" },
{ L"\u28A4", "368" },
{ L"\u28A5", "1368" },
{ L"\u28A6", "2368" },
{ L"\u28A7", "12368" },
{ L"\u28A8", "468" },
{ L"\u28A9", "1468" },
{ L"\u28AA", "2468" },
{ L"\u28AB", "12468" },
{ L"\u28AC", "3468" },
{ L"\u28AD", "13468" },
{ L"\u28AE", "23468" },
{ L"\u28AF", "123468" },
{ L"\u28B0", "568" },
{ L"\u28B1", "1568" },
{ L"\u28B2", "2568" },
{ L"\u28B3", "12568" },
{ L"\u28B4", "3568" },
{ L"\u28B5", "13568" },
{ L"\u28B6", "23568" },
{ L"\u28B7", "123568" },
{ L"\u28B8", "4568" },
{ L"\u28B9", "14568" },
{ L"\u28BA", "24568" },
{ L"\u28BB", "124568" },
{ L"\u28BC", "34568" },
{ L"\u28BD", "134568" },
{ L"\u28BE", "234568" },
{ L"\u28BF", "1234568" },
{ L"\u28C0", "78" },
{ L"\u28C1", "178" },
{ L"\u28C2", "278" },
{ L"\u28C3", "1278" },
{ L"\u28C4", "378" },
{ L"\u28C5", "1378" },
{ L"\u28C6", "2378" },
{ L"\u28C7", "12378" },
{ L"\u28C8", "478" },
{ L"\u28C9", "1478" },
{ L"\u28CA", "2478" },
{ L"\u28CB", "12478" },
{ L"\u28CC", "3478" },
{ L"\u28CD", "13478" },
{ L"\u28CE", "23478" },
{ L"\u28CF", "123478" },
{ L"\u28D0", "578" },
{ L"\u28D1", "1578" },
{ L"\u28D2", "2578" },
{ L"\u28D3", "12578" },
{ L"\u28D4", "3578" },
{ L"\u28D5", "13578" },
{ L"\u28D6", "23578" },
{ L"\u28D7", "123578" },
{ L"\u28D8", "4578" },
{ L"\u28D9", "14578" },
{ L"\u28DA", "24578" },
{ L"\u28DB", "124578" },
{ L"\u28DC", "34578" },
{ L"\u28DD", "134578" },
{ L"\u28DE", "234578" },
{ L"\u28DF", "1234578" },
{ L"\u28E0", "678" },
{ L"\u28E1", "1678" },
{ L"\u28E2", "2678" },
{ L"\u28E3", "12678" },
{ L"\u28E4", "3678" },
{ L"\u28E5", "13678" },
{ L"\u28E6", "23678" },
{ L"\u28E7", "123678" },
{ L"\u28E8", "4678" },
{ L"\u28E9", "14678" },
{ L"\u28EA", "24678" },
{ L"\u28EB", "124678" },
{ L"\u28EC", "34678" },
{ L"\u28ED", "134678" },
{ L"\u28EE", "234678" },
{ L"\u28EF", "1234678" },
{ L"\u28F0", "5678" },
{ L"\u28F1", "15678" },
{ L"\u28F2", "25678" },
{ L"\u28F3", "125678" },
{ L"\u28F4", "35678" },
{ L"\u28F5", "135678" },
{ L"\u28F6", "235678" },
{ L"\u28F7", "1235678" },
{ L"\u28F8", "45678" },
{ L"\u28F9", "145678" },
{ L"\u28FA", "245678" },
{ L"\u28FB", "1245678" },
{ L"\u28FC", "345678" },
{ L"\u28FD", "1345678" },
{ L"\u28FE", "2345678" },
{ L"\u28FF", "12345678" }
};

static int cache[1024] = { -1 };

int buildUpDots(int c, int row, int left, int right)
{
    int idx = (row * 2);
    if (left) {
        c = c | dots[idx];
    }
    if (right) {
        c = c | dots[idx + 1];
    }
    return c;
}

const wchar_t* wcharFromDots(int idx)
{
    if (idx == 0) {
        return dots_map[0].chars;
    }
    if (cache[0] == -1) {
        memset(cache, 0, sizeof(int) * 1024);
    }

    if (cache[idx] != 0) {
        // std::cout << "cached:" << dots_map[cache[idx]].dotCode << std::endl;
        return dots_map[cache[idx]].chars;
    }
    
    std::string str;
    for(int i=1;i<9;i++ ) {
        if (idx & 1<<i) {
            str += (char)('1' + (i-1));
        }
    }

    // std::cout << str << std::endl;
    for(int i=0; i<256; i++) {
        if (str == dots_map[i].dotCode) {
            cache[idx] = i;
            // std::cout << dots_map[cache[idx]].dotCode << std::endl;
            return dots_map[cache[idx]].chars;
        }
    }
    
    return L"";
}

int* buildUpDotsForLines(char *row1, char *row2, char *row3, char *row4, float textCompress, int bufferWidth)
{
    if (bufferWidth > 50) {
        // exceeded maximum buffer
        return 0;
    }
    char compressedLine[50];
    size_t sz = sizeof(int) * bufferWidth + 1;
    int *res = (int*)malloc(sz);
    memset(res, 0, sz);
    for(int r=0; r<4; r++) {
        char *row = 0;
        if (r == 0) row = row1;
        if (r == 1) row = row2;
        if (r == 2) row = row3;
        if (r == 3) row = row4;
        int len = strlen(row);
        memset(compressedLine, 0, 50*sizeof(char));
        for(int i=0;i<len && i<50;i++) {
            int idx = i/textCompress;
            if (row[i] != ' ' && row[i] != '\t') {
                compressedLine[idx]=row[i];
            }
        }

        int cx = 0;
        for(int i=0; i<bufferWidth && i<len-1;i++) {
            int left = compressedLine[cx] != 0;
            int right = compressedLine[cx+1] != 0;
            res[i] = buildUpDots(res[i], r, left, right);
            cx+=2;
        }
    }
    return res;
}
