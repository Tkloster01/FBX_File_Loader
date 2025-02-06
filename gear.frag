//  Set the fragment color
#version 120

uniform sampler2D tex0;

void main()
{
   vec4 a = texture2D(tex0, gl_TexCoord[0].xy);
   gl_FragColor = gl_Color * a;
}
