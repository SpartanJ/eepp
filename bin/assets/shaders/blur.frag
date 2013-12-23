uniform sampler2D textureUnit0;
uniform float blurfactor;

void main()
{
	vec4 baseColor = vec4(0.0, 0.0, 0.0, 0.0);
	
	baseColor += 0.015625 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(0.0, blurfactor*-3.0) );
	baseColor += 0.093750 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(0.0, blurfactor*-2.0) );
	baseColor += 0.234375 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(0.0, blurfactor*-1.0) );
	baseColor += 0.312500 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(0.0, 0.0) );
	baseColor += 0.234375 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(0.0, blurfactor*1.0) );
	baseColor += 0.093750 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(0.0, blurfactor*2.0) );
	baseColor += 0.015625 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(0.0, blurfactor*3.0) );
	baseColor += 0.015625 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(blurfactor*-3.0, 0.0) );
	baseColor += 0.093750 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(blurfactor*-2.0, 0.0) );
	baseColor += 0.234375 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(blurfactor*-1.0, 0.0) );
	baseColor += 0.312500 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(0.0, 0.0) );
	baseColor += 0.234375 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(blurfactor*1.0, 0.0) );
	baseColor += 0.093750 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(blurfactor*2.0, 0.0) );
	baseColor += 0.015625 * texture2D( textureUnit0, gl_TexCoord[0].xy + vec2(blurfactor*3.0, 0.0) );
	baseColor *= 0.6;
	
	gl_FragColor = baseColor;
}
