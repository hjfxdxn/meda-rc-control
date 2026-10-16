#ifndef RA0020_CLASSIFIER_H
#define RA0020_CLASSIFIER_H
#include <math.h>
#include <stdint.h>

// Empirical matching of the two supplied recordings, not general whistle identification.
// 10 kHz / 2560 samples = 256 ms. Ten samples form one 1 ms envelope value.
constexpr int AUDIO_SAMPLES = 2560;
constexpr int ENV_SAMPLES = 256;
constexpr float MIN_RMS_COUNTS = 4.0f; // Tune with the actual microphone; 4 counts ~= 4.9 mV RMS.
constexpr float MIN_TONE_RATIO = 0.65f;
constexpr float MIN_MOD_DEPTH = 0.10f;
constexpr float MIN_MOD_PURITY = 0.25f;
struct SoundResult {
  char candidate;
  float mean, peakToPeak, rms, modulationHz, toneRatio, modulationDepth, purity;
  bool clipped;
};
class WhistleClassifier {
  float carrierCos[10], carrierSin[10], window[ENV_SAMPLES];
  float coefficients[86], envelope[ENV_SAMPLES];
public:
  void begin() {
    const float pi = 3.14159265358979323846f;
    for (int i = 0; i < 10; ++i) {
      carrierCos[i] = cosf(2.0f*pi*3200.0f*i/10000.0f);
      carrierSin[i] = sinf(2.0f*pi*3200.0f*i/10000.0f);
    }
    for (int i = 0; i < ENV_SAMPLES; ++i)
      window[i] = 0.5f - 0.5f*cosf(2.0f*pi*i/(ENV_SAMPLES-1));
    for (int f = 15; f <= 100; ++f)
      coefficients[f-15] = 2.0f*cosf(2.0f*pi*f/1000.0f);
  }
  SoundResult analyze(const uint16_t *samples) {
    SoundResult r = {};
    r.candidate = '?';
    uint32_t sum = 0;
    int lo = 4095, hi = 0, clipping = 0;
    for (int i = 0; i < AUDIO_SAMPLES; ++i) {
      int v = samples[i]; sum += v;
      if (v < lo) lo = v;
      if (v > hi) hi = v;
      if (v <= 2 || v >= 4093) ++clipping;
    }
    r.mean = float(sum)/AUDIO_SAMPLES;
    r.peakToPeak = hi-lo;
    r.clipped = clipping > AUDIO_SAMPLES/100;
    float energy = 0, envelopeSum = 0, envelopeEnergy = 0;
    for (int b = 0; b < ENV_SAMPLES; ++b) {
      float re = 0, im = 0;
      for (int j = 0; j < 10; ++j) {
        float v = float(samples[b*10+j])-r.mean;
        energy += v*v; re += v*carrierCos[j]; im += v*carrierSin[j];
      }
      envelope[b] = sqrtf(re*re+im*im);
      envelopeSum += envelope[b]; envelopeEnergy += envelope[b]*envelope[b];
    }
    r.rms = sqrtf(energy/AUDIO_SAMPLES);
    if (energy < 0.001f) return r;
    r.toneRatio = 2.0f*envelopeEnergy/(10.0f*energy);
    float envelopeMean = envelopeSum/ENV_SAMPLES, variance = 0, windowEnergy = 0;
    for (int i = 0; i < ENV_SAMPLES; ++i) {
      float delta = envelope[i]-envelopeMean;
      variance += delta*delta;
      envelope[i] = delta*window[i];
      windowEnergy += envelope[i]*envelope[i];
    }
    r.modulationDepth = sqrtf(variance/ENV_SAMPLES)/(envelopeMean+0.000001f);
    if (windowEnergy < 0.001f) return r;
    float bestPower = 0;
    // Goertzel scan of the 1 kHz envelope: 15..100 Hz in 1 Hz steps.
    for (int f = 15; f <= 100; ++f) {
      float q1 = 0, q2 = 0, coeff = coefficients[f-15];
      for (int i = 0; i < ENV_SAMPLES; ++i) {
        float q0 = envelope[i]+coeff*q1-q2;
        q2 = q1; q1 = q0;
      }
      float power = q1*q1+q2*q2-coeff*q1*q2;
      if (power > bestPower) { bestPower = power; r.modulationHz = f; }
    }
    r.purity = 2.0f*bestPower/(ENV_SAMPLES*windowEnergy);
    if (r.clipped || r.rms < MIN_RMS_COUNTS || r.toneRatio < MIN_TONE_RATIO ||
        r.modulationDepth < MIN_MOD_DEPTH || r.purity < MIN_MOD_PURITY) return r;
    if (r.modulationHz >= 54 && r.modulationHz <= 68) r.candidate = 'A';
    else if (r.modulationHz >= 21 && r.modulationHz <= 32) r.candidate = 'B';
    return r;
  }
};
#endif
