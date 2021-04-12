precision lowp float;

uniform vec2 resolution;

uniform float scale;
uniform vec2 offset;
uniform sampler2D texture;
uniform vec2 tileDims;

void main(){
    // Normalized pixel coordinates (from 0 to 1)
    vec2 offsetCoords = gl_FragCoord.xy + offset;
    vec2 uv = offsetCoords.xy/resolution.xy;
    uv.xy *= tileDims;
    //uv.x *= (resolution.x/resolution.y);

    // LUT
    //uv.y = 1.-uv.y;
    // float b = (floor(uv.x*scale)+floor(uv.y*scale)*scale)/256.;
    // vec2 rg = fract(uv*scale);
    // vec3 col = vec3(rg,b);
    vec3 col = texture2D(texture, floor(uv.xy));
    // Output to screen 
    gl_FragColor = vec4(col,1);// / vec4(vec(5),1);
}