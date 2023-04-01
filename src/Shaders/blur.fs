#version 420 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D brightnessTexture;
uniform int blurMethod;
uniform bool horizontal;
const float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

#define M_PI 3.1415926535897932384626433832795

void main()
{    
    vec2 tex_offset = 1.0 / textureSize(brightnessTexture, 0); // gets size of single texel
    vec3 result = texture(brightnessTexture, TexCoord).rgb;

     if(blurMethod == 0)
     {
        float sumWeight = 0;
        for(int i=1;i<16;i++)
            sumWeight += cos(i*M_PI/16)*0.5 + 0.5;
        for(int i=1;i<16;i++)
        {
            float w = cos(i*M_PI/16)*0.5 + 0.5;
            if(horizontal)
            {
                result += texture(brightnessTexture, TexCoord + vec2(tex_offset.x*i, 0)).rgb*w;
                result += texture(brightnessTexture, TexCoord - vec2(tex_offset.x*i, 0)).rgb*w;
            }
            else
            {
                result += texture(brightnessTexture, TexCoord + vec2(0, tex_offset.y*i)).rgb*w;
                result += texture(brightnessTexture, TexCoord - vec2(0, tex_offset.y*i)).rgb*w;
            }
        }
        result = result / (sumWeight*2 + 1);
     }
     else
     {
        result *= weight[0];
        if(horizontal)
        {
            for(int i = 1; i < 5; ++i)
            {
               result += texture(brightnessTexture, TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
               result += texture(brightnessTexture, TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            }
        }
        else
        {
            for(int i = 1; i < 5; ++i)
            {
                result += texture(brightnessTexture, TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
                result += texture(brightnessTexture, TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            }
        }
     }
     
     FragColor = vec4(result.rgb, 1.0f);
}