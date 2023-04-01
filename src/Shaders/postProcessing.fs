#version 420 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform sampler2D blurTexture;
uniform float exposure;
uniform uint toneMapping;
uniform uint bloom;

//FXAA
uniform vec2 texelStep;
uniform bool showEdges;
uniform bool fxaaOn;

uniform float lumaThreshold;
uniform float mulReduce;
uniform float minReduce;
uniform float maxSpan;

vec3 fxaa(vec3 color)
{
    vec3 rgbM = color;

	// Sampling neighbour texels. Offsets are adapted to OpenGL texture coordinates. 
	vec3 rgbNW = textureOffset(screenTexture, TexCoord, ivec2(-1, 1)).rgb;
    vec3 rgbNE = textureOffset(screenTexture, TexCoord, ivec2(1, 1)).rgb;
    vec3 rgbSW = textureOffset(screenTexture, TexCoord, ivec2(-1, -1)).rgb;
    vec3 rgbSE = textureOffset(screenTexture, TexCoord, ivec2(1, -1)).rgb;

	// see http://en.wikipedia.org/wiki/Grayscale
	const vec3 toLuma = vec3(0.299, 0.587, 0.114);
	
	// Convert from RGB to luma.
	float lumaNW = dot(rgbNW, toLuma);
	float lumaNE = dot(rgbNE, toLuma);
	float lumaSW = dot(rgbSW, toLuma);
	float lumaSE = dot(rgbSE, toLuma);
	float lumaM = dot(rgbM, toLuma);

	// Gather minimum and maximum luma.
	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	
	// If contrast is lower than a maximum threshold ...
	if (lumaMax - lumaMin <= lumaMax * lumaThreshold)
		// ... do no AA and return.
		return rgbM;
	
    //Final Antialiased Result.
    vec3 antialiased;

	// Sampling is done along the gradient.
	vec2 samplingDirection;
	samplingDirection.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    samplingDirection.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    // Sampling step distance depends on the luma: The brighter the sampled texels, the smaller the final sampling step direction.
    // This results, that brighter areas are less blurred/more sharper than dark areas.  
    float samplingDirectionReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * mulReduce, minReduce);

	// Factor for norming the sampling direction plus adding the brightness influence. 
	float minSamplingDirectionFactor = 1.0 / (min(abs(samplingDirection.x), abs(samplingDirection.y)) + samplingDirectionReduce);
    
    // Calculate final sampling direction vector by reducing, clamping to a range and finally adapting to the texture size. 
    samplingDirection = clamp(samplingDirection * minSamplingDirectionFactor, vec2(-maxSpan), vec2(maxSpan)) * texelStep;
	
	// Inner samples on the tab.
	vec3 rgbSampleNeg = texture(screenTexture, TexCoord + samplingDirection * (1.0/3.0 - 0.5)).rgb;
	vec3 rgbSamplePos = texture(screenTexture, TexCoord + samplingDirection * (2.0/3.0 - 0.5)).rgb;

	vec3 rgbTwoTab = (rgbSamplePos + rgbSampleNeg) * 0.5;  

	// Outer samples on the tab.
	vec3 rgbSampleNegOuter = texture(screenTexture, TexCoord + samplingDirection * (0.0/3.0 - 0.5)).rgb;
	vec3 rgbSamplePosOuter = texture(screenTexture, TexCoord + samplingDirection * (3.0/3.0 - 0.5)).rgb;
	
	vec3 rgbFourTab = (rgbSamplePosOuter + rgbSampleNegOuter) * 0.25 + rgbTwoTab * 0.5;   
	
	// Calculate luma for checking against the minimum and maximum value.
	float lumaFourTab = dot(rgbFourTab, toLuma);
	
	// Are outer samples of the tab beyond the edge ... 
	if (lumaFourTab < lumaMin || lumaFourTab > lumaMax)
		// ... yes, so use only two samples.
		antialiased = rgbTwoTab;
	else
		// ... no, so use four samples. 
		antialiased = rgbFourTab;

	// Show edges for debug purposes.	
	if (showEdges)
		antialiased.r = 1.0;
    
    return antialiased;
}

//Exposure Tone Mapping
vec3 exposureToneMapping(vec3 x){
    return vec3(1.0) - exp(-x * exposure);
}

//Reinhard
vec3 reinhard(vec3 x) {
  return x / (1.0 + x);
}

//Reinhard 2
vec3 reinhard2(vec3 x) {
  const float L_white = 4.0;
  return (x * (1.0 + x / (L_white * L_white))) / (1.0 + x);
}

// Filmic Tonemapping Operators http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 filmic(vec3 x) {
  vec3 X = max(vec3(0.0), x - 0.004);
  vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
  return pow(result, vec3(2.2));
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 aces(vec3 x) 
{
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Lottes 2016, "Advanced Techniques and Optimization of HDR Color Pipelines"
vec3 lottes(vec3 x) {
  const vec3 a = vec3(1.6);
  const vec3 d = vec3(0.977);
  const vec3 hdrMax = vec3(8.0);
  const vec3 midIn = vec3(0.18);
  const vec3 midOut = vec3(0.267);

  const vec3 b =
      (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
      ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
  const vec3 c =
      (pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
      ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);

  return pow(x, a) / (pow(x, a * d) * b + c);
}

// Uchimura 2017, "HDR theory and practice"
// Math: https://www.desmos.com/calculator/gslcdxvipg
// Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
vec3 uchimura(vec3 x, float P, float a, float m, float l, float c, float b) {
  float l0 = ((P - m) * l) / a;
  float L0 = m - m / a;
  float L1 = m + (1.0 - m) / a;
  float S0 = m + l0;
  float S1 = m + a * l0;
  float C2 = (a * P) / (P - S1);
  float CP = -C2 / P;

  vec3 w0 = vec3(1.0 - smoothstep(0.0, m, x));
  vec3 w2 = vec3(step(m + l0, x));
  vec3 w1 = vec3(1.0 - w0 - w2);

  vec3 T = vec3(m * pow(x / m, vec3(c)) + b);
  vec3 S = vec3(P - (P - S1) * exp(CP * (x - S0)));
  vec3 L = vec3(m + a * (x - m));

  return T * w0 + L * w1 + S * w2;
}
vec3 uchimura(vec3 x) {
  const float P = 1.0;  // max display brightness
  const float a = 1.0;  // contrast
  const float m = 0.22; // linear section start
  const float l = 0.4;  // linear section length
  const float c = 1.33; // black
  const float b = 0.0;  // pedestal

  return uchimura(x, P, a, m, l, c, b);
}

//Uncharted2 ToneMapping
vec3 uncharted2Tonemap(vec3 x) {
  float A = 0.15;
  float B = 0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.02;
  float F = 0.30;
  float W = 11.2;
  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}
vec3 uncharted2(vec3 color) {
  const float W = 11.2;
  float exposureBias = 2.0;
  vec3 curr = uncharted2Tonemap(exposureBias * color);
  vec3 whiteScale = 1.0 / uncharted2Tonemap(vec3(W));
  return curr * whiteScale;
}

// Unreal 3, Documentation: "Color Grading"
// Adapted to be close to Tonemap_ACES, with similar range
// Gamma 2.2 correction is baked in, don't use with sRGB conversion!
vec3 unreal(vec3 x) {
  return x / (x + 0.155) * 1.019;
}

void main()
{
    //Get Final Render.
    vec3 color = texture(screenTexture, TexCoord).rgb;
    
    //Do FXAA if Enabled.
    if(fxaaOn) color = fxaa(color);

    //Add BlurTexture To The Screen Texture if Bloom Effect is Enabled.
    if(bloom > 0)
        color += texture(blurTexture, TexCoord).rgb;

    if(toneMapping == 0)
    {
        // Exposure tone mapping
        color = exposureToneMapping(color);
    }
    else if(toneMapping == 1)
    {
        // Reinhard tone mapping
        color = reinhard(color);
    }
    else if(toneMapping == 2)
    {
        // Reinhard 2 tone mapping
        color = reinhard2(color);
    }
    else if(toneMapping == 3)
    {
        // Filmic Tone Mapping
        color = filmic(color);
    }
    else if(toneMapping == 4)
    {
        // ACES Filmic Tone Mapping
        color = aces(color);
    }
    else if(toneMapping == 5)
    {
        // Lottes Tone Mapping
        color = lottes(color);
    }
    else if(toneMapping == 6)
    {
        // Uchimura Tone Mapping
        color = uchimura(color);
    }
    else if(toneMapping == 7)
    {
        // Uncharted 2 Tone Mapping
        color = uncharted2(color);
    }
    else if(toneMapping == 8)
    {
        // Unreal Tone Mapping
        color = unreal(color);
    }

    FragColor = vec4(color, 1.0f);
}  