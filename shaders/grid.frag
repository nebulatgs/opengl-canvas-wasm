#extension GL_OES_standard_derivatives:enable
#define PI 3.141592
precision lowp float;

// size of a square in pixel
uniform float scale;

uniform vec2 resolution;
//out vec4 fragColor;

float aastep(float threshold,float value){
    float afwidth=.7*length(vec2(dFdx(value),dFdy(value)));
    return smoothstep(threshold-afwidth,threshold+afwidth,value);
}

void main(){
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv=gl_FragCoord.xy/resolution.xy;
    float dotResolution=resolution.y/scale/2.;
    float dotRadius=.3;
    
    uv.x*=dotResolution*(resolution.x/resolution.y);
    uv.y*=dotResolution;
    uv.y+=.5;
    uv.x+=.5;
    vec2 uv2=2.*fract(uv)-1.;
    //uv2.x *= u_resolution.x / u_resolution.y;
    float distance=length(uv2);
    
    vec4 white=vec4(1.,1.,1.,1.);
    vec4 black=vec4(.282,.282,.282,0);
    
    vec4 Coord=cos(PI/scale*gl_FragCoord);
    vec4 gridColor=vec4(.082)+.2*aastep(.97,max(Coord.x,Coord.y));
    gridColor+=vec4(0,0,0,1);
    
    vec4 color=mix(black,gridColor,aastep(dotRadius,distance));
    
    gl_FragColor=color;}