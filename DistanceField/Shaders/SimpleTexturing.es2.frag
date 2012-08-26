static const char* SimulatorFragmentShader = STRINGIFY(

varying mediump vec2 TextureCoordOut;

uniform sampler2D Sampler;
uniform mediump float AlphaTest;
uniform mediump vec3 OutlineColor;
uniform mediump vec3 GlyphColor;
uniform mediump vec3 GlowColor;

uniform bool Smooth;
uniform bool Outline;
uniform bool Glow;
uniform bool Shadow;

const mediump vec2 ShadowOffset = vec2(-0.005, 0.01);
const mediump vec3 ShadowColor = vec3(0.0, 0.0, 0.125);
const mediump float SmoothCenter = 0.5;
const mediump float OutlineCenter = 0.4;
const mediump float GlowCenter = 1.0;

void main(void)
{
    // Look up distance from the distance field:
    mediump vec4 color = texture2D(Sampler, TextureCoordOut);
    mediump float alpha = color.a;
    
    // Kill the fragment if it fails the alpha test.
    if (alpha > AlphaTest)
        discard;

    mediump vec3 rgb = color.xyz + GlyphColor;
    mediump float width = 0.04;

    if (Smooth) {
        alpha = smoothstep(SmoothCenter - width, SmoothCenter + width, alpha);
    }

    if (Outline) {
        mediump float mu = smoothstep(OutlineCenter - width, OutlineCenter + width, alpha);
        alpha = smoothstep(SmoothCenter - width, SmoothCenter + width, alpha);
        rgb = rgb * (1.0 - mu) + OutlineColor * mu;
        //rgb = mix(rgb, OutlineColor, mu);
    }

    if (Glow) {
        mediump float mu = smoothstep(SmoothCenter - width, SmoothCenter + width, alpha);
        rgb = mix(rgb, GlowColor, mu);
        alpha = smoothstep(SmoothCenter, GlowCenter, sqrt(alpha));
    }

    if (Shadow) {
        mediump float alpha2 = texture2D(Sampler, TextureCoordOut + ShadowOffset).a;
    
        mediump float s = smoothstep(SmoothCenter - width, SmoothCenter + width, alpha2);
        mediump float v = smoothstep(SmoothCenter - width, SmoothCenter + width, alpha);
        
        // If s is 0 then it's inside the shadow; if it's 1 then it's outside
        // If v is 0 then it's inside the vector; if it's 1 then it's outside
        
        // Totally inside the vector:
        if (v == 0.0) {
            rgb = GlyphColor;
            alpha = 0.0;
        }
        
        // Non-shadowed vector edge:
        else if (s == 1.0 && v != 1.0) {
            rgb = GlyphColor;
            alpha = v;
        }

        // Totally inside the shadow:
        else if (s == 0.0 && v == 1.0) {
            rgb = ShadowColor;
            alpha = 0.0;
        }

        // Shadowed vector edge:
        else if (s == 0.0) {
            //rgb = mix(VectorColor, ShadowColor, v); (this exposes a compiler bug)
            rgb = GlyphColor * (1.0 - v) + ShadowColor * v;
            alpha = 0.0;
        }

        // Shadow's edge:
        else {
            //rgb = mix(GlyphColor, ShadowColor, v); (this exposes a compiler bug)
            rgb = GlyphColor * (1.0 - v) + ShadowColor * v;
            alpha = s;
        }
    }

    gl_FragColor = vec4(rgb, alpha);
}
);

static const char* DeviceFragmentShader =

"#extension GL_OES_standard_derivatives : enable\n" STRINGIFY(

varying mediump vec2 TextureCoordOut;

uniform sampler2D Sampler;
uniform mediump float AlphaTest;
uniform mediump vec3 OutlineColor;
uniform mediump vec3 GlyphColor;
uniform mediump vec3 GlowColor;

uniform bool Smooth;
uniform bool Outline;
uniform bool Glow;
uniform bool Shadow;

const mediump vec2 ShadowOffset = vec2(-0.005, 0.01);
const mediump vec3 ShadowColor = vec3(0.0, 0.0, 0.125);
const mediump float SmoothCenter = 0.5;
const mediump float OutlineCenter = 0.4;
const mediump float GlowCenter = 1.0;

void main(void)
{
    // Look up distance from the distance field:
    mediump vec4 color = texture2D(Sampler, TextureCoordOut);
    mediump float alpha = color.a;

    // Kill the fragment if it fails the alpha test.
    if (alpha > AlphaTest)
        discard;
        
    mediump vec3 rgb = color.xyz + GlyphColor;
    mediump float width = fwidth(alpha); // ~0.04

    if (Smooth) {
        alpha = smoothstep(SmoothCenter - width, SmoothCenter + width, alpha);
    }

    if (Outline) {
        mediump float mu = smoothstep(OutlineCenter - width, OutlineCenter + width, alpha);
        alpha = smoothstep(SmoothCenter - width, SmoothCenter + width, alpha);
        rgb = rgb * (1.0 - mu) + OutlineColor * mu;
        //rgb = mix(rgb, OutlineColor, mu);
    }

    if (Glow) {
        mediump float mu = smoothstep(SmoothCenter - width, SmoothCenter + width, alpha);
        rgb = mix(rgb, GlowColor, mu); // don't trust it
        alpha = smoothstep(SmoothCenter, GlowCenter, sqrt(alpha));
    }

    if (Shadow) {
        mediump float alpha2 = texture2D(Sampler, TextureCoordOut + ShadowOffset).a;
    
        mediump float s = smoothstep(SmoothCenter - width, SmoothCenter + width, alpha2);
        mediump float v = smoothstep(SmoothCenter - width, SmoothCenter + width, alpha);
        
        // If s is 0 then it's inside the shadow; if it's 1 then it's outside
        // If v is 0 then it's inside the vector; if it's 1 then it's outside
        
        // Totally inside the vector:
        if (v == 0.0) {
            rgb = GlyphColor;
            alpha = 0.0;
        }
        
        // Non-shadowed vector edge:
        else if (s == 1.0 && v != 1.0) {
            rgb = GlyphColor;
            alpha = v;
        }

        // Totally inside the shadow:
        else if (s == 0.0 && v == 1.0) {
            rgb = ShadowColor;
            alpha = 0.0;
        }

        // Shadowed vector edge:
        else if (s == 0.0) {
            //rgb = mix(GlyphColor, ShadowColor, v); (this exposes a compiler bug)
            rgb = GlyphColor * (1.0 - v) + ShadowColor * v;
            alpha = 0.0;
        }

        // Shadow's edge:
        else {
            //rgb = mix(GlyphColor, ShadowColor, v); (this exposes a compiler bug)
            rgb = GlyphColor * (1.0 - v) + ShadowColor * v;
            alpha = s;
        }
    }

    gl_FragColor = vec4(rgb, alpha);
}
);
