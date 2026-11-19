/**
 * @file Graphics/SensorShader.h
 *
 * This file contains the code of the shader function that applies the properties of an image sensor (see
 * \c GraphicsContextBase::SensorModel) to a pixel when an image is converted into a readback buffer. It is the same for
 * all graphics APIs: it is written in the Metal Shading Language, and the header of the GLSL version defines the type
 * names that differ.
 *
 * @author Arne Hasselbring
 */

#pragma once

static const char* sensorShaderSharedCode = R"sensor(
uint sensorHash(uint x)
{
  x ^= x >> 16;
  x *= 0x7feb352du;
  x ^= x >> 15;
  x *= 0x846ca68bu;
  x ^= x >> 16;
  return x;
}

// An approximately normally distributed number (the sum of two bytes of a hash, scaled to a variance of 1).
float sensorNormal(uint h)
{
  return (float((h & 255u) + ((h >> 8) & 255u)) - 255.0) * (1.0 / 104.5);
}

// Four independent approximately normally distributed numbers (with triangular distributions).
float4 sensorGaussians(uint seed)
{
  uint h0 = sensorHash(seed);
  uint h1 = sensorHash(h0 + 0x9e3779b9u);
  return float4(sensorNormal(h0), sensorNormal(h0 >> 16), sensorNormal(h1), sensorNormal(h1 >> 16));
}

// Applies exposure and white balance, saturation, vignetting and noise to the color of the pixel (x, y) of an image of the given size.
float3 applySensorModel(float3 color, uint2 pixel, float2 size, uint seed, float3 gain, float saturation, float vignette, float noise)
{
  color *= gain;
  float luma = dot(color, float3(0.299, 0.587, 0.114));
  color = mix(float3(luma), color, saturation);
  // The squared distance from the center is 2 at the corners.
  float2 d = (float2(pixel) + 0.5) / size * 2.0 - 1.0;
  color *= 1.0 - vignette * 0.5 * dot(d, d);
  if(noise > 0.0)
  {
    // The noise is stronger in brighter pixels. A part of it is the same for all channels.
    float4 g = sensorGaussians(pixel.x * 0x1b873593u + pixel.y * 0xcc9e2d51u + seed);
    color += noise * (0.3 + 0.7 * sqrt(clamp(luma, 0.0, 1.0))) * (0.8 * g.xyz + 0.6 * g.w);
  }
  return color;
}
)sensor";
