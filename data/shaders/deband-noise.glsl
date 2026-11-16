// deband.glsl — haasn's stochastic debander, adapted to mpv hook syntax
// Requires deband=no in mpv.conf (built-in debander must be off)

//!HOOK LUMA
//!HOOK CHROMA
//!HOOK RGB
//!BIND HOOKED
//!DESC deband (haasn)

//------------ Configuration section ------------
// Higher = more debanding, but too high diminishes image details.
#define THRESHOLD 64
// Range (in source pixels) at which to sample for neighbours.
#define RANGE 8
// More iterations = better debanding, slower. Each iteration uses a
// multiple of RANGE and a lower THRESHOLD.
#define ITERATIONS 4
// Extra noise to cover remaining banding. 0 disables.
//#define GRAIN 48
#define GRAIN 32
//------------ End of configuration ------------

// Wide usage friendly PRNG, from a GLSL tricks forum post
float mod289(float x)  { return x - floor(x / 289.0) * 289.0; }
float permute(float x) { return mod289((34.0*x + 1.0) * x); }
float rand(float x)    { return fract(x / 41.0); }

// Stochastic approximation of the average color around a pixel
vec4 average(vec2 pos, float range, inout float h)
{
    // Random distance and direction
    float dist = rand(h) * range;     h = permute(h);
    float dir  = rand(h) * 6.2831853; h = permute(h);

    vec2 pt = dist / HOOKED_size;
    vec2 o = vec2(cos(dir), sin(dir));

    // Sample at quarter-turn intervals around the source pixel
    vec4 ref[4];
    ref[0] = HOOKED_tex(pos + pt * vec2( o.x,  o.y));
    ref[1] = HOOKED_tex(pos + pt * vec2(-o.y,  o.x));
    ref[2] = HOOKED_tex(pos + pt * vec2(-o.x, -o.y));
    ref[3] = HOOKED_tex(pos + pt * vec2( o.y, -o.x));

    // Normalized average
    return (ref[0] + ref[1] + ref[2] + ref[3]) / 4.0;
}

vec4 hook()
{
    float h;
    // Initialize the PRNG by hashing the position + a random uniform
    vec3 m = vec3(HOOKED_pos, random) + vec3(1.0);
    h = permute(permute(permute(m.x)+m.y)+m.z);

    // Sample the source pixel
    vec4 col = HOOKED_tex(HOOKED_pos);

    for (int i = 1; i <= ITERATIONS; i++) {
        // Use the average instead if the difference is below the threshold
        vec4 avg = average(HOOKED_pos, i*RANGE, h);
        vec4 diff = abs(col - avg);
        col = mix(avg, col, greaterThan(diff, vec4(THRESHOLD/(i*16384.0))));
    }

    // Add some random noise to the output
    vec3 noise;
    noise.x = rand(h); h = permute(h);
    noise.y = rand(h); h = permute(h);
    noise.z = rand(h); h = permute(h);
    col.rgb += (GRAIN/8192.0) * (noise - vec3(0.5));

    return col;
}