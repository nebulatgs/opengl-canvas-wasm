precision lowp float;

uniform vec2 resolution;

uniform float scale;
uniform vec2 offset;
uniform sampler2D texture;
uniform vec2 tileDims;

void main(){
    // Normalized pixel coordinates (from 0 to 1)
    vec2 offsetCoords = gl_FragCoord.xy + offset;
    // gl_FragCoord -= vec4(1);
    // offsetCoords *= 100./scale;// / 1.04;
    // vec2 uv = offsetCoords/resolution;
    // uv.x *= resolution.x/resolution.y;
    //uv.x *= (resolution.x/resolution.y);
    vec2 normCoords = vec2(offsetCoords.x/tileDims.x, offsetCoords.y/tileDims.y) * vec2(tileDims.x/2./scale, tileDims.y/2./scale);
    //normCoords = mod(normCoords , vec2(1)) + normCoords;
    normCoords /= tileDims;
    normCoords = floor(normCoords * vec2(tileDims.x, tileDims.y))/vec2(tileDims.x, tileDims.y);
    // LUT
    //uv.y = 1.-uv.y;
    // float b = (floor(uv.x*scale)+floor(uv.y*scale)*scale)/256.;
    // vec2 rg = fract(uv*scale);
    // vec3 col = vec3(rg,b);
    // vec3 col = texture2D(texture, vec2(floor(uv.x), floor(uv.y))).rgb;
    // vec3 col = vec3(0.5);
    // vec3 col = vec3(uv < vec3(0.5) ? 0.1 : 1);
    // vec3 col = texture2D(texture, vec2(100.,0.)).rgb;
    // vec3 col = vec3(0);
    // if(uv.x < tileDims.x/2.)
        // col = vec3((mod(uv.y,1.) + mod(uv.x,1.))/2.);
    //if(col.r < 0.1)
        //col = vec3(1.,0,0);
    // Output to screen S
    vec4 texCol = texture2D(texture, normCoords * vec2(1.));
    // if(texCol.xyz = vec3(0)){
    //     discard;
    // }
    gl_FragColor = vec4(texCol.rgb,1);// / vec4(vec(5),1);
}