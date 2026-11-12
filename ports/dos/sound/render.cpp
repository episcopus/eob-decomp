// render.cpp: a chip log to a WAV file, for the Tandy FM study (README.md).
//
//   render IN.txt OUT.wav [--rms DBFS]
//   render --centroid FILE.wav     the spectral centroid in Hz of the sounding part (2048-point frames within
//                                  30 dB of the loudest, each weighted by its power): a measure of brightness
//   render --patches IN.txt        for each line "ID r20 r23 r40 r43 r60 r63 r80 r83 rE0 rE3 rC0 FNUM BLOCK" (an
//                                  OPL2 voice as it was at a key-on), plays it on channel 0 for 300 ms and prints
//                                  "ID FLATNESS": the median spectral flatness (geometric over arithmetic mean of the
//                                  power, 100 Hz to 10 kHz) of its frames within 30 dB of the loudest. A tone is near
//                                  0; white noise is about 0.56; -1 is a voice that makes no sound
//   render --lines IN.txt          the same lines: for each, the voice's spectral lines at modulator levels of 0 to
//                                  45 dB, "ID A RATIO,DB ...", RATIO the line's frequency over the voice's base
//                                  frequency, DB its power against a lone sine at full level (lines(), below)
//
// IN.txt is the text form tandymap.py writes:
//   chip opl | chip sn          the chip the writes go to
//   len SECONDS                 the clip's length
//   init REG VAL                OPL2 register state before the first write (opl only)
//   w T REG VAL                 an OPL2 write at T seconds (opl)
//   w T BYTE                    a byte written to the SN76489's port (sn)
//
// The OPL2 is ymfm's YM3812 at the AdLib's 3.579545 MHz (49,716 samples a second). The SN76489 is emulated
// here, as the Tandy 1000's: 3.579545 MHz, tone counters stepped at clock / 16, a 15-bit noise register with
// taps on bits 0 and 4 (SMS Power's notes for the Tandy 1000; the EX's own chip is unconfirmed), 2 dB a
// volume step, and a period of 0 counted as 1024. Both are resampled to 44,100 Hz and scaled to the loudness given
// (the K-weighted RMS of ITU-R BS.1770, default -20 dB; peaks held under -1 dBFS) so that two renders can be compared
// at the same loudness as heard.
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "ymfm_opl.h"

static const double CLOCK = 3579545.0;
static const int OUT_RATE = 44100;

struct Ev {
    double t;
    int a, b;
};

namespace {
class Host : public ymfm::ymfm_interface {};
}

static std::vector<float> render_opl(const std::vector<std::pair<int, int>> &init, const std::vector<Ev> &ev, double len)
{
    Host host;
    ymfm::ym3812 chip(host);
    chip.reset();
    for (auto &p : init) {
        chip.write_address((uint8_t)p.first);
        chip.write_data((uint8_t)p.second);
    }
    const double rate = CLOCK / 72.0;
    size_t n = (size_t)(len * rate) + 1;
    std::vector<float> native(n);
    size_t k = 0;
    ymfm::ym3812::output_data out;
    for (size_t i = 0; i < n; i++) {
        double t = i / rate;
        while (k < ev.size() && ev[k].t <= t) {
            chip.write_address((uint8_t)ev[k].a);
            chip.write_data((uint8_t)ev[k].b);
            k++;
        }
        chip.generate(&out);
        native[i] = (float)out.data[0];
    }
    size_t m = (size_t)(len * OUT_RATE);
    std::vector<float> res(m);
    for (size_t i = 0; i < m; i++) {
        double x = i * rate / OUT_RATE;
        size_t j = (size_t)x;
        double f = x - j;
        float a = native[std::min(j, n - 1)], b = native[std::min(j + 1, n - 1)];
        res[i] = (float)(a + (b - a) * f);
    }
    // the card's output is AC-coupled: the same 20 Hz high-pass as the SN76489's, so the DC of the half and
    // pulse waveforms does not count in the loudness the clips are matched at
    const double hp_r = exp(-2 * M_PI * 20.0 / OUT_RATE);
    double hx = 0, hy = 0;
    for (auto &x : res) {
        double y = hp_r * (hy + x - hx);
        hx = x;
        hy = y;
        x = (float)y;
    }
    return res;
}

struct SN {
    int period[3] = {0x400, 0x400, 0x400};
    int count[4] = {1, 1, 1, 1};
    int out[4] = {1, 1, 1, 1};
    int vol[4] = {15, 15, 15, 15};
    int noise = 0;
    unsigned lfsr = 0x4000;
    int latch = 0;
    double amp[16];
    SN()
    {
        for (int v = 0; v < 15; v++) amp[v] = pow(10.0, -2.0 * v / 20.0);
        amp[15] = 0;
    }
    void write(int b)
    {
        if (b & 0x80) {
            latch = (b >> 4) & 7;
            int ch = latch >> 1;
            if (latch & 1) vol[ch] = b & 15;
            else if (ch < 3) period[ch] = (period[ch] & 0x3F0) | (b & 15);
            else { noise = b & 7; lfsr = 0x4000; }
        } else {
            int ch = latch >> 1;
            if (latch & 1) vol[ch] = b & 15;
            else if (ch < 3) period[ch] = (period[ch] & 15) | ((b & 0x3F) << 4);
            else { noise = b & 7; lfsr = 0x4000; }
        }
    }
    void shift()
    {
        unsigned bit = (noise & 4) ? ((lfsr ^ (lfsr >> 4)) & 1) : (lfsr & 1);
        lfsr = (lfsr >> 1) | (bit << 14);
    }
    // one step of clock / 16
    double step()
    {
        bool t2_rise = false;
        for (int c = 0; c < 3; c++) {
            if (--count[c] <= 0) {
                count[c] = period[c] ? period[c] : 0x400;
                out[c] = -out[c];
                if (c == 2 && out[c] > 0) t2_rise = true;
            }
        }
        int r = noise & 3;
        if (r == 3) {
            if (t2_rise) shift();
        } else if (--count[3] <= 0) {
            count[3] = 0x10 << r;
            out[3] = -out[3];
            if (out[3] > 0) shift();
        }
        double s = 0;
        for (int c = 0; c < 3; c++) s += (period[c] <= 1 ? 1 : out[c]) * amp[vol[c]];
        s += ((lfsr & 1) ? 1 : -1) * amp[vol[3]];
        return s;
    }
};

static std::vector<float> render_sn(const std::vector<Ev> &ev, double len)
{
    SN sn;
    const double tick = CLOCK / 16.0;
    size_t m = (size_t)(len * OUT_RATE);
    std::vector<float> res(m);
    size_t k = 0;
    double pos = 0;   // in ticks
    double lp = 0, hp_x = 0, hp_y = 0;
    const double lp_a = 1.0 - exp(-2 * M_PI * 12000.0 / OUT_RATE);
    const double hp_r = exp(-2 * M_PI * 20.0 / OUT_RATE);
    for (size_t i = 0; i < m; i++) {
        double end = (i + 1) * tick / OUT_RATE;
        double acc = 0;
        int n = 0;
        while (pos < end) {
            double t = pos / tick;
            while (k < ev.size() && ev[k].t <= t) sn.write(ev[k++].a);
            acc += sn.step();
            n++;
            pos += 1;
        }
        double x = n ? acc / n : 0;
        lp += lp_a * (x - lp);
        double y = lp - hp_x + hp_r * hp_y;
        hp_x = lp;
        hp_y = y;
        res[i] = (float)y;
    }
    return res;
}

struct Biquad {
    double b0, b1, b2, a1, a2, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    double step(double x)
    {
        double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x; y2 = y1; y1 = y;
        return y;
    }
};

// ITU-R BS.1770's K-weighting at 44.1 kHz (RBJ forms of its two stages: a +4 dB shelf from about 1.7 kHz, the ear's
// head effect, and a high-pass at 38 Hz): loudness as heard, so a buzz rich in harmonics and a smooth low tone
// at one RMS no longer sound one louder than the other
static std::vector<float> k_weighted(const std::vector<float> &s)
{
    const double fs = OUT_RATE;
    Biquad shelf, hp;
    {
        double A = pow(10.0, 3.99984 / 40), w = 2 * M_PI * 1681.974 / fs, al = sin(w) / (2 * 0.7071752), c = cos(w);
        double sq = 2 * sqrt(A) * al;
        double a0 = (A + 1) - (A - 1) * c + sq;
        shelf = {A * ((A + 1) + (A - 1) * c + sq) / a0, -2 * A * ((A - 1) + (A + 1) * c) / a0,
                 A * ((A + 1) + (A - 1) * c - sq) / a0, 2 * ((A - 1) - (A + 1) * c) / a0, ((A + 1) - (A - 1) * c - sq) / a0};
    }
    {
        double w = 2 * M_PI * 38.13547 / fs, al = sin(w) / (2 * 0.5003270), c = cos(w), a0 = 1 + al;
        hp = {(1 + c) / 2 / a0, -(1 + c) / a0, (1 + c) / 2 / a0, -2 * c / a0, (1 - al) / a0};
    }
    std::vector<float> k(s.size());
    for (size_t i = 0; i < s.size(); i++) k[i] = (float)hp.step(shelf.step(s[i]));
    return k;
}

static void normalise(std::vector<float> &s, double rms_db)
{
    double sum = 0;
    size_t n = 0;
    // the K-weighted RMS of the sounding part: blocks of 10 ms above -60 dB of the peak block
    const std::vector<float> k = k_weighted(s);
    std::vector<double> blocks;
    double peak_block = 0;
    for (size_t i = 0; i < k.size(); i += 441) {
        double b = 0;
        size_t j = i;
        for (; j < std::min(k.size(), i + 441); j++) b += (double)k[j] * k[j];
        b /= (j - i);
        blocks.push_back(b);
        peak_block = std::max(peak_block, b);
    }
    for (double b : blocks)
        if (b > peak_block * 1e-6) { sum += b; n++; }
    double peak = 0;
    for (float v : s) peak = std::max(peak, (double)fabs(v));
    if (!n || peak == 0) return;
    double rms = sqrt(sum / n);
    double g = pow(10.0, rms_db / 20.0) / rms;
    double limit = pow(10.0, -1.0 / 20.0) / peak;
    if (g > limit) g = limit;
    for (float &v : s) v = (float)(v * g);
}

static void write_wav(const char *path, const std::vector<float> &s)
{
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    uint32_t data = (uint32_t)s.size() * 2;
    auto u32 = [&](uint32_t v) { fwrite(&v, 4, 1, f); };
    auto u16 = [&](uint16_t v) { fwrite(&v, 2, 1, f); };
    fwrite("RIFF", 1, 4, f); u32(36 + data); fwrite("WAVEfmt ", 1, 8, f);
    u32(16); u16(1); u16(1); u32(OUT_RATE); u32(OUT_RATE * 2); u16(2); u16(16);
    fwrite("data", 1, 4, f); u32(data);
    for (float v : s) {
        double x = v * 32767.0;
        if (x > 32767) x = 32767;
        if (x < -32768) x = -32768;
        u16((uint16_t)(int16_t)lrint(x));
    }
    fclose(f);
}

static void fft(std::vector<std::complex<double>> &a)
{
    size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; i++) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        double ang = -2 * M_PI / len;
        std::complex<double> wl(cos(ang), sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w(1);
            for (size_t j = 0; j < len / 2; j++) {
                auto u = a[i + j], v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

static int centroid(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return 1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 44, SEEK_SET);
    std::vector<int16_t> s((size - 44) / 2);
    if (fread(s.data(), 2, s.size(), f) != s.size()) { fclose(f); return 1; }
    fclose(f);
    const size_t N = 2048;
    std::vector<double> win(N);
    for (size_t i = 0; i < N; i++) win[i] = 0.5 - 0.5 * cos(2 * M_PI * i / (N - 1));
    std::vector<std::pair<double, double>> frames;   // (power, centroid)
    double top = 0;
    for (size_t at = 0; at + N <= s.size(); at += N / 2) {
        std::vector<std::complex<double>> a(N);
        for (size_t i = 0; i < N; i++) a[i] = s[at + i] * win[i];
        fft(a);
        double p = 0, pf = 0;
        for (size_t k = 1; k < N / 2; k++) {
            double m = std::norm(a[k]);
            p += m;
            pf += m * k * (double)OUT_RATE / N;
        }
        frames.push_back({p, p > 0 ? pf / p : 0});
        top = std::max(top, p);
    }
    double w = 0, c = 0;
    for (auto &fr : frames)
        if (fr.first > top / 1000) { w += fr.first; c += fr.first * fr.second; }
    printf("%.1f\n", w > 0 ? c / w : 0.0);
    return 0;
}

static double flatness_of(const std::vector<float> &s, double rate)
{
    const size_t N = 2048;
    std::vector<double> win(N);
    for (size_t i = 0; i < N; i++) win[i] = 0.5 - 0.5 * cos(2 * M_PI * i / (N - 1));
    size_t lo = (size_t)(100.0 * N / rate), hi = (size_t)(10000.0 * N / rate);
    std::vector<std::pair<double, double>> frames;
    double top = 0;
    for (size_t at = (size_t)(0.02 * rate); at + N <= s.size(); at += N / 4) {
        std::vector<std::complex<double>> a(N);
        for (size_t i = 0; i < N; i++) a[i] = s[at + i] * win[i];
        fft(a);
        double sum = 0, logs = 0;
        for (size_t k = lo; k < hi; k++) {
            double m = std::norm(a[k]) + 1e-9;
            sum += m;
            logs += log(m);
        }
        double n = hi - lo;
        frames.push_back({sum, exp(logs / n) / (sum / n)});
        top = std::max(top, sum);
    }
    if (top < 1e3) return -1;     // silent: the voice sounds nothing (a silenced envelope)
    std::vector<double> keep;
    for (auto &f : frames)
        if (f.first > top / 1000) keep.push_back(f.second);
    if (keep.empty()) return 0;
    std::sort(keep.begin(), keep.end());
    return keep[keep.size() / 2];
}

static int patches(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return 1; }
    char line[512];
    const double rate = CLOCK / 72.0;
    while (fgets(line, sizeof line, f)) {
        char id[64];
        int r[13];
        if (sscanf(line, "%63s %d %d %d %d %d %d %d %d %d %d %d %d %d", id, &r[0], &r[1], &r[2], &r[3], &r[4], &r[5], &r[6],
                   &r[7], &r[8], &r[9], &r[10], &r[11], &r[12]) != 14) continue;
        Host host;
        ymfm::ym3812 chip(host);
        chip.reset();
        auto w = [&](int reg, int val) { chip.write_address((uint8_t)reg); chip.write_data((uint8_t)val); };
        w(0x01, 0x20);
        const int regs[11] = {0x20, 0x23, 0x40, 0x43, 0x60, 0x63, 0x80, 0x83, 0xE0, 0xE3, 0xC0};
        for (int i = 0; i < 11; i++) w(regs[i], r[i]);
        w(0xA0, r[11] & 0xFF);
        w(0xB0, 0x20 | (r[12] & 7) << 2 | (r[11] >> 8 & 3));
        std::vector<float> s((size_t)(0.3 * rate));
        ymfm::ym3812::output_data out;
        for (auto &x : s) { chip.generate(&out); x = (float)out.data[0]; }
        printf("%s %.4f\n", id, flatness_of(s, rate));
    }
    fclose(f);
    return 0;
}

// The spectral lines of one voice sound at each modulator level: the patch held at full level (instant attack,
// sustain held, no vibrato or tremolo) with the modulator's total level set to A dB, A = 0, 3, ... 45, played
// 0.4 s; power spectrum of 8192-point frames from 0.05 s; the 12 strongest peaks within 40 dB of the strongest,
// each as (frequency over the voice's base frequency, power in dB against a lone sine at full level).
static int lines(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return 1; }
    const double rate = CLOCK / 72.0;
    const size_t N = 8192;
    std::vector<double> win(N);
    for (size_t i = 0; i < N; i++) win[i] = 0.5 - 0.5 * cos(2 * M_PI * i / (N - 1));
    auto play = [&](const int *r, int mod_tl, std::vector<double> &spec) {
        Host host;
        ymfm::ym3812 chip(host);
        chip.reset();
        auto w = [&](int reg, int val) { chip.write_address((uint8_t)reg); chip.write_data((uint8_t)val); };
        w(0x01, 0x20);
        w(0x20, (r[0] & 0x1F) | 0x20);
        w(0x23, (r[1] & 0x1F) | 0x20);
        w(0x40, mod_tl);
        w(0x43, 0);
        w(0x60, 0xF0); w(0x63, 0xF0);
        w(0x80, 0x0F); w(0x83, 0x0F);
        w(0xE0, r[8]); w(0xE3, r[9]);
        w(0xC0, r[10]);
        w(0xA0, r[11] & 0xFF);
        w(0xB0, 0x20 | (r[12] & 7) << 2 | (r[11] >> 8 & 3));
        std::vector<double> s((size_t)(0.4 * rate));
        ymfm::ym3812::output_data out;
        for (auto &x : s) { chip.generate(&out); x = out.data[0]; }
        spec.assign(N / 2, 0.0);
        int frames = 0;
        for (size_t at = (size_t)(0.05 * rate); at + N <= s.size(); at += N / 4, frames++) {
            std::vector<std::complex<double>> a(N);
            for (size_t i = 0; i < N; i++) a[i] = s[at + i] * win[i];
            fft(a);
            for (size_t k = 0; k < N / 2; k++) spec[k] += std::norm(a[k]);
        }
        for (auto &x : spec) x /= std::max(1, frames);
    };
    // the reference: a lone sine at full level (modulator silent, both at multiple 1)
    int ref_r[13] = {0x01, 0x01, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0x200, 4};
    std::vector<double> spec;
    play(ref_r, 63, spec);
    double ref = 0;
    for (size_t k = 2; k + 2 < N / 2; k++) ref = std::max(ref, spec[k - 2] + spec[k - 1] + spec[k] + spec[k + 1] + spec[k + 2]);
    char line[512];
    while (fgets(line, sizeof line, f)) {
        char id[96];
        int r[13];
        if (sscanf(line, "%95s %d %d %d %d %d %d %d %d %d %d %d %d %d", id, &r[0], &r[1], &r[2], &r[3], &r[4], &r[5], &r[6],
                   &r[7], &r[8], &r[9], &r[10], &r[11], &r[12]) != 14) continue;
        double base = (r[11] & 0x3FF) * rate / pow(2.0, 20 - (r[12] & 7));
        for (int A = 0; A <= 45; A += 3) {
            play(r, (int)lrint(A / 0.75) & 63, spec);
            std::vector<std::pair<double, double>> pk;     // (power, Hz)
            for (size_t k = 3; k + 3 < N / 2; k++) {
                bool top = true;
                for (int d = -2; d <= 2; d++) if (d && spec[k + d] > spec[k]) top = false;
                if (!top || spec[k] <= 0) continue;
                double p = spec[k - 2] + spec[k - 1] + spec[k] + spec[k + 1] + spec[k + 2];
                double y0 = log(spec[k - 1] + 1e-30), y1 = log(spec[k]), y2 = log(spec[k + 1] + 1e-30);
                double den = y0 - 2 * y1 + y2;
                double off = den != 0 ? 0.5 * (y0 - y2) / den : 0;
                pk.push_back({p, (k + off) * rate / N});
            }
            std::sort(pk.begin(), pk.end(), [](auto &a, auto &b) { return a.first > b.first; });
            printf("%s %d", id, A);
            for (size_t i = 0; i < pk.size() && i < 12; i++) {
                if (pk[i].first < pk[0].first * 1e-4) break;
                printf(" %.4f,%.1f", pk[i].second / base, 10 * log10(pk[i].first / ref));
            }
            printf("\n");
        }
    }
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "--centroid")) return centroid(argv[2]);
    if (argc == 3 && !strcmp(argv[1], "--patches")) return patches(argv[2]);
    if (argc == 3 && !strcmp(argv[1], "--lines")) return lines(argv[2]);
    if (argc < 3) { fprintf(stderr, "usage: render IN.txt OUT.wav [--rms DBFS]\n"); return 2; }
    double rms_db = -20;
    for (int i = 3; i + 1 < argc; i++)
        if (!strcmp(argv[i], "--rms")) rms_db = atof(argv[i + 1]);
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror(argv[1]); return 1; }
    std::string chip;
    double len = 1;
    std::vector<std::pair<int, int>> init;
    std::vector<Ev> ev;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char w[16];
        if (sscanf(line, "%15s", w) != 1) continue;
        if (!strcmp(w, "chip")) { char c[16]; sscanf(line, "%*s %15s", c); chip = c; }
        else if (!strcmp(w, "len")) sscanf(line, "%*s %lf", &len);
        else if (!strcmp(w, "init")) { int a, b; sscanf(line, "%*s %d %d", &a, &b); init.push_back({a, b}); }
        else if (!strcmp(w, "w")) {
            Ev e{0, 0, 0};
            if (chip == "opl") sscanf(line, "%*s %lf %d %d", &e.t, &e.a, &e.b);
            else sscanf(line, "%*s %lf %d", &e.t, &e.a);
            ev.push_back(e);
        }
    }
    fclose(f);
    std::vector<float> s = chip == "opl" ? render_opl(init, ev, len) : render_sn(ev, len);
    normalise(s, rms_db);
    write_wav(argv[2], s);
    return 0;
}
