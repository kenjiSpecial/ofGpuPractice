#version 120
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect durationData;      // recive the velocity texture

uniform float timestep;

void main(void){
    vec2 st = gl_TexCoord[0].st;    // gets the position of the pixel that it´s dealing with...
    
    vec3 vel = texture2DRect( durationData, st ).xyz;     // Fetch both the pos and vel.
    
    vel.x +=  timestep; // Updates the position
    
    gl_FragColor.rgba = vec4(vel.x,vel.y, vel.z, 1.0);  // And finaly it store it on the position FBO
}
