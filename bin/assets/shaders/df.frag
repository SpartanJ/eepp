uniform sampler2D textureUnit0;
const float smoothing = 1.0/16.0;

void main()
{
    float distance = texture2D( textureUnit0, gl_TexCoord[0].xy ).a;
    float alpha = smoothstep( 0.5 - smoothing, 0.5 + smoothing, distance );
    gl_FragColor = vec4( gl_Color.rgb * texture2D( textureUnit0, gl_TexCoord[0].xy ).rgb, alpha );
}
