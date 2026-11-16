// cinegrain2 - realistic film grain for mpv
// https://github.com/mr-berndt/cinegrain2
// Copyright (C) 2026 mr-berndt
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Grain model: white noise, shaped like real grain - a Gaussian blur for
// the grain size minus a wide copy that takes out the coarse structure -
// and applied multiplicatively on the picture, as film density works. A
// five-zone equalizer shapes the grain strength over luma:
//   Black 0.00, Shadow 0.13, Mid 0.30, High 0.60, White 1.00
// with smoothstep blending between neighbouring zones, normalised to 1 at
// mid grey.
//
// Calibrated against ProRes 4K DCI scans of real 35 mm, 16 mm and 8 mm
// grain: LEVEL 1.0 is the grain strength of the scans on a 50 % grey card,
// at every Size. Size is in pixels at 2160p and scales with the output
// height; below 2160p fine grain is attenuated as a 4K scan would be.
//
// All parameters are shader PARAMs and are set live by cinegrain2-control.lua
// through glsl-shader-opts; the values below are only the start-up defaults
// (the 35mm preset).

//!PARAM LEVEL
//!DESC Grain strength: 1.0 = as strong as the film scans on a 50 % grey card, at every Size
//!TYPE float
//!MINIMUM 0.0
//!MAXIMUM 5.0
0.4

//!PARAM GAIN_1
//!DESC EQ zone Black (luma 0.00): -1 = no grain, 0 = flat, +3 = four times
//!TYPE float
//!MINIMUM -3.0
//!MAXIMUM 3.0
0.00

//!PARAM GAIN_2
//!DESC EQ zone Shadow (luma 0.13)
//!TYPE float
//!MINIMUM -3.0
//!MAXIMUM 3.0
0.65

//!PARAM GAIN_3
//!DESC EQ zone Mid (luma 0.30)
//!TYPE float
//!MINIMUM -3.0
//!MAXIMUM 3.0
0.00

//!PARAM GAIN_4
//!DESC EQ zone High (luma 0.60)
//!TYPE float
//!MINIMUM -3.0
//!MAXIMUM 3.0
-0.65

//!PARAM GAIN_5
//!DESC EQ zone White (luma 1.00)
//!TYPE float
//!MINIMUM -3.0
//!MAXIMUM 3.0
-2.00

//!PARAM GRAIN_SIZE
//!DESC Grain size: sigma in px at 2160p (~0.4 = 35mm, ~1.2 = 16mm, ~2.7 = 8mm)
//!TYPE float
//!MINIMUM 0.0
//!MAXIMUM 3.0
0.37

//!PARAM CHROMA
//!DESC Colour grain: 0 = monochrome like the reference scans, 1 = independent per channel
//!TYPE float
//!MINIMUM 0.0
//!MAXIMUM 1.0
0.0

//!PARAM DEMO
//!DESC Test picture: 0 = off, 0.5 = 50 % grey card, 1 = grey staircase
//!TYPE float
//!MINIMUM 0.0
//!MAXIMUM 1.0
0.0

//!PARAM SOFTBLUR
//!DESC Grain-field softening radius (pixels). 0 = off (zero cost).
//!TYPE float
//!MINIMUM 0.0
//!MAXIMUM 10.0
0.0

//!HOOK OUTPUT
//!SAVE GRAIN_N
//!COMPONENTS 1
//!WIDTH HOOKED.w
//!HEIGHT HOOKED.h
//!DESC cinegrain2 noise

// Irwin-Hall with N=4: the sum of 4 uniform hash draws minus 2 has mean 0
// and variance 1/3. Drawn once per pixel; all later passes only read it.
float hash_noise(vec2 p, float seed)
{
    float sum = 0.0;
    for (int i = 0; i < 4; i++) {
        float fi = float(i);
        vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973) + seed + fi * 7.31);
        p3 += dot(p3, p3.yzx + 33.33);
        sum += fract((p3.x + p3.y) * p3.z);
    }
    return sum - 2.0;
}

vec4 hook()
{
    // Range -2..2, stored as 0..1 so it survives a unorm texture as well.
    return vec4(hash_noise(gl_FragCoord.xy, random) * 0.25 + 0.5, 0.0, 0.0, 1.0);
}

// Grain shape
// -----------
// Real grain is Gaussian-blurred noise with its coarse structure taken out:
// grain = narrow(sigma1) - DOG_C * wide(DOG_K * sigma1). Fitted to the
// radial power spectra of ten ProRes 4K DCI grain plates (8 mm to 35 mm);
// all of them show the same shape, DOG_K 6.7..8.6, DOG_C 0.88..1.0. Plain
// blurred noise keeps the coarse structure and shows it as bright and dark
// blotches, three to eight times stronger than in the scans, that change
// with every frame.
//
// The wide part only carries coarse structure, so it is computed on a grid
// of a quarter of the size: 4x4 box average, separable Gaussian, bilinear
// upsampling. Box and upsampling add WIDE_FLOOR px^2 of variance, which is
// taken off the Gaussian.

//!HOOK OUTPUT
//!BIND GRAIN_N
//!SAVE GRAIN_D
//!COMPONENTS 1
//!WIDTH HOOKED.w 4 /
//!HEIGHT HOOKED.h 4 /
//!DESC cinegrain2 grain downsample

vec4 hook()
{
    // four bilinear taps at the corners between 2x2 pixels = 4x4 box mean
    vec2 p = GRAIN_N_pos;   // centre of the 4x4 block
    vec2 d = GRAIN_N_pt;
    float s = GRAIN_N_tex(p + vec2(-d.x, -d.y)).x + GRAIN_N_tex(p + vec2(d.x, -d.y)).x
            + GRAIN_N_tex(p + vec2(-d.x,  d.y)).x + GRAIN_N_tex(p + vec2(d.x,  d.y)).x;
    return vec4(s * 0.25, 0.0, 0.0, 1.0);
}

//!HOOK OUTPUT
//!BIND GRAIN_D
//!SAVE GRAIN_WH
//!COMPONENTS 1
//!WIDTH GRAIN_D.w
//!HEIGHT GRAIN_D.h
//!DESC cinegrain2 grain wide h-blur

#define DOG_K 8.4
#define REF_HEIGHT 2160.0
#define WIDE_FLOOR 3.92
#define MAX_RW 16

vec4 hook()
{
    float s2 = DOG_K * max(GRAIN_SIZE * 4.0 * GRAIN_D_size.y / REF_HEIGHT, 0.05);
    float sq = sqrt(max(s2 * s2 - WIDE_FLOOR, 0.0)) * 0.25;   // in grid cells
    if (sq < 0.3)
        return GRAIN_D_tex(GRAIN_D_pos);
    int R = int(min(ceil(2.5 * sq), float(MAX_RW)));
    float c1 = -0.5 / (sq * sq);
    float sum = GRAIN_D_tex(GRAIN_D_pos).x, wsum = 1.0;
    for (int i = 1; i <= MAX_RW; i++) {
        if (i > R) break;
        float d = float(i);
        float w = exp(d * d * c1);
        sum  += (GRAIN_D_tex(GRAIN_D_pos + vec2(-d, 0.0) * GRAIN_D_pt).x
               + GRAIN_D_tex(GRAIN_D_pos + vec2( d, 0.0) * GRAIN_D_pt).x) * w;
        wsum += 2.0 * w;
    }
    return vec4(sum / wsum, 0.0, 0.0, 1.0);
}

//!HOOK OUTPUT
//!BIND GRAIN_WH
//!SAVE GRAIN_W
//!COMPONENTS 1
//!WIDTH GRAIN_WH.w
//!HEIGHT GRAIN_WH.h
//!DESC cinegrain2 grain wide v-blur

#define DOG_K 8.4
#define REF_HEIGHT 2160.0
#define WIDE_FLOOR 3.92
#define MAX_RW 16

vec4 hook()
{
    float s2 = DOG_K * max(GRAIN_SIZE * 4.0 * GRAIN_WH_size.y / REF_HEIGHT, 0.05);
    float sq = sqrt(max(s2 * s2 - WIDE_FLOOR, 0.0)) * 0.25;
    if (sq < 0.3)
        return GRAIN_WH_tex(GRAIN_WH_pos);
    int R = int(min(ceil(2.5 * sq), float(MAX_RW)));
    float c1 = -0.5 / (sq * sq);
    float sum = GRAIN_WH_tex(GRAIN_WH_pos).x, wsum = 1.0;
    for (int i = 1; i <= MAX_RW; i++) {
        if (i > R) break;
        float d = float(i);
        float w = exp(d * d * c1);
        sum  += (GRAIN_WH_tex(GRAIN_WH_pos + vec2(0.0, -d) * GRAIN_WH_pt).x
               + GRAIN_WH_tex(GRAIN_WH_pos + vec2(0.0,  d) * GRAIN_WH_pt).x) * w;
        wsum += 2.0 * w;
    }
    return vec4(sum / wsum, 0.0, 0.0, 1.0);
}

//!HOOK OUTPUT
//!BIND GRAIN_N
//!SAVE GRAIN_H
//!COMPONENTS 1
//!WIDTH HOOKED.w
//!HEIGHT HOOKED.h
//!DESC cinegrain2 grain h-blur

#define REF_HEIGHT 2160.0
#define MAX_R 8

vec4 hook()
{
    float s1 = max(GRAIN_SIZE * GRAIN_N_size.y / REF_HEIGHT, 0.05);
    int R = int(min(ceil(2.5 * s1), float(MAX_R)));
    float c1 = -0.5 / (s1 * s1);
    float sum = GRAIN_N_tex(GRAIN_N_pos).x, wsum = 1.0;
    for (int i = 1; i <= MAX_R; i++) {
        if (i > R) break;
        float d = float(i);
        float w = exp(d * d * c1);
        sum  += (GRAIN_N_tex(GRAIN_N_pos + vec2(-d, 0.0) * GRAIN_N_pt).x
               + GRAIN_N_tex(GRAIN_N_pos + vec2( d, 0.0) * GRAIN_N_pt).x) * w;
        wsum += 2.0 * w;
    }
    return vec4(sum / wsum, 0.0, 0.0, 1.0);   // still encoded as n/4 + 0.5
}

//!HOOK OUTPUT
//!BIND GRAIN_H
//!BIND GRAIN_W
//!SAVE GRAIN_V
//!COMPONENTS 1
//!WIDTH HOOKED.w
//!HEIGHT HOOKED.h
//!DESC cinegrain2 grain v-blur

#define DOG_K 8.4
#define DOG_C 0.95
#define REF_HEIGHT 2160.0
#define WIDE_FLOOR 3.92
#define MAX_R 8

vec4 hook()
{
    float s1 = max(GRAIN_SIZE * GRAIN_H_size.y / REF_HEIGHT, 0.05);
    int R = int(min(ceil(2.5 * s1), float(MAX_R)));
    float c1 = -0.5 / (s1 * s1);
    float sum = GRAIN_H_tex(GRAIN_H_pos).x, wsum = 1.0, wsq = 1.0;
    for (int i = 1; i <= MAX_R; i++) {
        if (i > R) break;
        float d = float(i);
        float w = exp(d * d * c1);
        sum  += (GRAIN_H_tex(GRAIN_H_pos + vec2(0.0, -d) * GRAIN_H_pt).x
               + GRAIN_H_tex(GRAIN_H_pos + vec2(0.0,  d) * GRAIN_H_pt).x) * w;
        wsum += 2.0 * w;
        wsq  += 2.0 * w * w;
    }
    float narrow = (sum / wsum) * 4.0 - 2.0;
    float wide   = GRAIN_W_tex(GRAIN_H_pos).x * 4.0 - 2.0;
    float grain  = narrow - DOG_C * wide;

    // Variance of the 2-D kernel on noise of variance 1/3: the narrow part
    // exact from its discrete weights, wide part and cross term as
    // continuous Gaussians (wide width incl. box and upsampling).
    float s2 = max(DOG_K * s1, sqrt(WIDE_FLOOR));
    float A  = wsq / (wsum * wsum);
    float m1 = s1 * s1;
    float var = A * A
              - 2.0 * DOG_C / (6.2831853 * (m1 + s2 * s2))
              + DOG_C * DOG_C / (12.566371 * s2 * s2);
    grain *= inversesqrt(max(var / 3.0, 1e-8));

    // Unit variance, stored with room for +-5 sigma.
    return vec4(grain * 0.1 + 0.5, 0.0, 0.0, 1.0);
}

//!HOOK OUTPUT
//!BIND HOOKED
//!SAVE GRAIN_AMP
//!COMPONENTS 1
//!WIDTH 1
//!HEIGHT 1
//!DESC cinegrain2 grain amplitude

// The grain we imitate is the film's own, as a 4K (2160p) scan shows it -
// all scans of one stock look alike there. A smaller display cannot show
// all of it: when a 4K picture is brought down to fewer pixels, fine grain
// averages out and coarse grain survives. This pass computes how much of
// the grain standard deviation is left, for mpv's default downscaler
// (hermite, kernel widened by the scale factor): the variance of one
// downscaled sample of the 2160p grain field, from the autocorrelation of
// the 2160p grain kernel, averaged over 8 sample phases. Separable, so the
// 1-D variance is the std factor of the 2-D field. 1 at 2160p and above.
// Once per frame, 1x1.
#define REF_HEIGHT 2160.0
#define MAX_R 8
#define MAX_T 24

float hermite(float x)
{
    x = abs(x);
    return x < 1.0 ? (2.0 * x - 3.0) * x * x + 1.0 : 0.0;
}

vec4 hook()
{
    float k = REF_HEIGHT / HOOKED_size.y;               // downscale factor
    if (k <= 1.0)
        return vec4(1.0, 0.0, 0.0, 1.0);
    k = min(k, 0.5 * float(MAX_T - 2));
    float s = max(GRAIN_SIZE, 0.05);
    int R = int(min(ceil(2.5 * s), float(MAX_R)));

    // autocorrelation of the 2160p kernel, normalised: rho[0] = 1
    float w[MAX_R + 1];
    for (int d = 0; d <= MAX_R; d++)
        w[d] = d <= R ? exp(-0.5 * float(d * d) / (s * s)) : 0.0;
    float rho[2 * MAX_R + 1];
    for (int m = 0; m <= 2 * MAX_R; m++) {
        float a = 0.0;
        for (int d = -MAX_R; d <= MAX_R; d++) {
            int e = d + m;
            if (abs(d) <= R && abs(e) <= R) a += w[abs(d)] * w[abs(e)];
        }
        rho[m] = a;
    }
    for (int m = 2 * MAX_R; m >= 0; m--) rho[m] /= rho[0];

    float var = 0.0;
    for (int p = 0; p < 8; p++) {
        float c = (float(p) + 0.5) / 8.0;                // sample position
        int i0 = int(floor(c - k)) + 1;
        float h[MAX_T];
        float hs = 0.0;
        for (int t = 0; t < MAX_T; t++) {
            h[t] = hermite((float(i0 + t) - c) / k);
            hs += h[t];
        }
        float v = 0.0;
        for (int t = 0; t < MAX_T; t++) {
            if (h[t] == 0.0) continue;
            for (int u = 0; u < MAX_T; u++) {
                int m = abs(t - u);
                if (h[u] == 0.0 || m > 2 * R) continue;
                v += h[t] * h[u] * rho[m];
            }
        }
        var += v / (hs * hs);
    }
    return vec4(clamp(var / 8.0, 0.0, 1.0), 0.0, 0.0, 1.0);
}

//!HOOK OUTPUT
//!BIND HOOKED
//!BIND GRAIN_V
//!BIND GRAIN_AMP
//!DESC cinegrain2 apply

// Pixel amplitude of the old Gaussian grain (0.577 * 0.702).
#define GRAIN_SCALE 0.405
// LEVEL 1.0 = as strong as the ProRes 4K DCI grain scans on a 50 % grey
// card at 2160p (they all have the same strength there).
#define LEVEL_REF 0.210
float hash_noise(vec2 p, float seed)
{
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973) + seed);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z) * 2.0 - 1.0;
}

// Five-zone equalizer over luma, smoothstep between neighbouring zones.
float luma_weight(float luma)
{
    float l = clamp(luma, 0.0, 1.0);
    float g, t;
    if (l <= 0.13) {
        t = l / 0.13;                    t = t*t*(3.0 - 2.0*t);
        g = mix(GAIN_1, GAIN_2, t);
    } else if (l <= 0.30) {
        t = (l - 0.13) / 0.17;           t = t*t*(3.0 - 2.0*t);
        g = mix(GAIN_2, GAIN_3, t);
    } else if (l <= 0.60) {
        t = (l - 0.30) / 0.30;           t = t*t*(3.0 - 2.0*t);
        g = mix(GAIN_3, GAIN_4, t);
    } else {
        t = (l - 0.60) / 0.40;           t = t*t*(3.0 - 2.0*t);
        g = mix(GAIN_4, GAIN_5, t);
    }
    return max(0.0, 1.0 + g);
}

// The zones shape the grain over luma relative to mid grey: the weight is
// normalised to 1 at luma 0.5, the grey card of the reference scans, so
// LEVEL keeps its meaning whatever the zones do.
float zone_weight(float luma)
{
    return luma_weight(luma) / max(luma_weight(0.5), 0.05);
}

vec4 hook()
{
    vec4 color = HOOKED_tex(HOOKED_pos);

    if (DEMO > 0.99) {
        float step = floor(gl_FragCoord.x / HOOKED_size.x * 16.0);
        color.rgb = vec3(step / 15.0);
    } else if (DEMO > 0.0) {
        color.rgb = vec3(DEMO);
    }

    // Per-channel RGB grain: sample GRAIN_V at 3 different large shifts
    // (≫ grain correlation length → independent draws).
    // TILED shifts (fract-wrap instead of clamp-to-edge). Large shifts give
    // RGB decorrelation beyond grain correlation length; fract() wraps
    // out-of-bounds samples to the opposite texture edge → no edge-clamp
    // artifact. Independent noise field means the wrap produces no visible
    // seam (sampled values at wrapped positions are just more uncorrelated
    // noise).
    vec2 SHIFT_R = vec2(137.0, -73.0) * GRAIN_V_pt;
    vec2 SHIFT_B = vec2(-82.0, 235.0) * GRAIN_V_pt;
    vec2 pos_r = fract(HOOKED_pos + SHIFT_R);
    vec2 pos_b = fract(HOOKED_pos + SHIFT_B);

    vec3 grain_raw = vec3(
        GRAIN_V_tex(pos_r     ).x,
        GRAIN_V_tex(HOOKED_pos).x,
        GRAIN_V_tex(pos_b     ).x
    ) * 10.0 - 5.0;

    if (SOFTBLUR > 0.01) {
        float r = SOFTBLUR;
        float aoff = hash_noise(gl_FragCoord.xy + vec2(13.37, 71.3), random) * 6.2831853;
        vec3 gn = vec3(0.0);
        for (int j = 0; j < 8; j++) {
            float a = float(j) * 0.7853981634 + aoff;
            vec2 noff = vec2(cos(a), sin(a)) * r * GRAIN_V_pt;
            gn.r += GRAIN_V_tex(fract(pos_r      + noff)).x * 10.0 - 5.0;
            gn.g += GRAIN_V_tex(fract(HOOKED_pos + noff)).x * 10.0 - 5.0;
            gn.b += GRAIN_V_tex(fract(pos_b      + noff)).x * 10.0 - 5.0;
        }
        gn *= 0.125;
        float t = clamp(SOFTBLUR / 10.0, 0.0, 1.0);
        grain_raw = mix(grain_raw, gn, t);
    }

    float grain_y = dot(grain_raw, vec3(0.2126, 0.7152, 0.0722));
    grain_raw = mix(vec3(grain_y), grain_raw, CHROMA);

    // GRAIN_V has unit variance at every size; GRAIN_AMP is the share of
    // the 2160p grain the output can show.
    vec3 grain = grain_raw * (GRAIN_SCALE * GRAIN_AMP_tex(vec2(0.5)).x);

    float luma = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    float weight = zone_weight(luma);

    color.rgb *= exp(LEVEL * LEVEL_REF * weight * grain);

    color.rgb = max(color.rgb, vec3(0.0));
    return color;
}
